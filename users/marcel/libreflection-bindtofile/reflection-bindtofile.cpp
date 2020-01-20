#include "reflection.h"
#include "reflection-bindtofile.h"
#include "reflection-jsonio.h"
#include "reflection-textio.h"

#include "lineReader.h"
#include "lineWriter.h"

#include "FileStream.h"
#include "framework.h"
#include "Log.h"
#include "Path.h"
#include "TextIO.h"

#include <string>

struct ObjectToFileBinding
{
	const TypeDB * typeDB;
	const Type * type;
	void * object;
	std::string filename;
	
	bool loadFromTextFile()
	{
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
	
		if (TextIO::load(filename.c_str(), lines, lineEndings) == false)
		{
			LOG_ERR("failed to load text lines from file %s", filename.c_str());
			return false;
		}
	
		LineReader line_reader(lines, 0, 0);
	
		if (object_fromlines_recursive(*typeDB, type, object, line_reader) == false)
		{
			LOG_ERR("failed to read object from lines", 0);
			return false;
		}
		
		return true;
	}
	
	bool loadFromJsonFile()
	{
		char * text = nullptr;
		size_t textSize = 0;
		
		if (TextIO::loadFileContents(filename.c_str(), text, textSize) == false)
		{
			LOG_ERR("failed to load contents from file %s", filename.c_str());
			return false;
		}
	
		rapidjson::Document document;
		
		document.Parse(text, textSize);
		
		if (document.HasParseError())
		{
			LOG_ERR("failed to parse json contents for file %s", filename.c_str());
			return false;
		}
		
		return object_fromjson_recursive(*typeDB, type, object, document);
	}
	
	bool loadFromFile()
	{
		if (Path::GetExtension(filename.c_str(), true) == "json")
			return loadFromJsonFile();
		else
			return loadFromTextFile();
	}
	
	//
	
	bool saveToTextFile()
	{
		bool result = true;
		
		LineWriter line_writer;
		
		if (object_tolines_recursive(
			*typeDB, type, object,
			line_writer, 0) == false)
		{
			LOG_WRN("failed to serialize object to lines", 0);
			result = false;
		}
		else
		{
			auto lines = line_writer.to_lines();
			
			if (TextIO::save(filename.c_str(), lines, TextIO::kLineEndings_Unix) == false)
			{
				LOG_WRN("failed to save lines to file %s", filename.c_str());
				result = false;
			}
		}
		
		return result;
	}
	
	bool saveToJsonFile()
	{
		bool result = true;
		
		rapidjson::StringBuffer stringBuffer;
		REFLECTIONIO_JSON_WRITER writer(stringBuffer);
		
		if (object_tojson_recursive(*typeDB, type, object, writer) == false)
		{
			LOG_WRN("failed to serialize object to json", 0);
			result = false;
		}
		else
		{
			const char * text = stringBuffer.GetString();
			
			FILE * file = fopen(filename.c_str(), "wt");
			
			if (file == nullptr || fprintf(file, "%s", text) < 0)
			{
				LOG_WRN("failed to save json text to file %s", filename.c_str());
				result = false;
			}
			
			if (file != nullptr)
			{
				fclose(file);
				file = nullptr;
			}
		}
		
		return result;
	}
	
	bool saveToFile()
	{
		if (Path::GetExtension(filename.c_str(), true) == "json")
			return saveToJsonFile();
		else
			return saveToTextFile();
	}
};

static std::vector<ObjectToFileBinding> s_objectToFileBindings;

//

bool bindObjectToFile(const TypeDB * typeDB, const Type * type, void * object, const char * filename)
{
	Assert(typeDB != nullptr);
	Assert(type != nullptr);
	Assert(object != nullptr);
	
	if (typeDB == nullptr ||
		type == nullptr ||
		object == nullptr)
	{
		return false;
	}
	
	s_objectToFileBindings.resize(s_objectToFileBindings.size() + 1);
	auto & objectToFileBinding = s_objectToFileBindings.back();
	objectToFileBinding.typeDB = typeDB;
	objectToFileBinding.type = type;
	objectToFileBinding.object = object;
	objectToFileBinding.filename = filename;

	bool result = true;
	
	// check if the file exists. if so, deserialize it
	// if not, create a new file and serialize the object to it

	if (FileStream::Exists(filename))
	{
		if (objectToFileBinding.loadFromFile() == false)
		{
			result = false;
		}
	}
	else
	{
		if (objectToFileBinding.saveToFile() == false)
		{
			result = false;
		}
	}

	if (result == false)
	{
		s_objectToFileBindings.pop_back();
	}
	
	return result;
}

void tickObjectToFileBinding()
{
	for (auto & objectToFileBinding : s_objectToFileBindings)
	{
		if (framework.fileHasChanged(objectToFileBinding.filename.c_str()))
		{
			objectToFileBinding.loadFromFile();
		}
	}
}

bool flushObjectToFile(const void * object)
{
	bool result = true;
	
	for (auto & objectToFileBinding : s_objectToFileBindings)
		if (objectToFileBinding.object == object)
			result &= objectToFileBinding.saveToFile();
	
	return result;
}

// --- helper functions ---

bool saveObjectToFile(const TypeDB & typeDB, const Type * type, const void * object, const char * filename)
{
	if (type == nullptr)
	{
		logError("type is NULL");
		return false;
	}
	else
	{
	// todo : refactor. let ObjectToFileBinding use these functions, instead of the other way around

		ObjectToFileBinding binding;
		binding.typeDB = &typeDB;
		binding.type = type;
		binding.object = (void*)object;
		binding.filename = filename;
		
		return binding.saveToFile();
	}
}

bool loadObjectFromFile(const TypeDB & typeDB, const Type * type, void * object, const char * filename)
{
	if (type == nullptr)
	{
		logError("type is NULL");
		return false;
	}
	else
	{
		ObjectToFileBinding binding;
		binding.typeDB = &typeDB;
		binding.type = type;
		binding.object = object;
		binding.filename = filename;
		
		return binding.loadFromFile();
	}
}
