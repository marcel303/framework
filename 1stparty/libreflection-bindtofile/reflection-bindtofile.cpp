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
	
	bool loadFromFile()
	{
		return loadObjectFromFile(*typeDB, type, object, filename.c_str());
	}
	
	bool saveToFile()
	{
		return saveObjectToFile(*typeDB, type, object, filename.c_str());
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

static bool saveObjectToJsonFile(const TypeDB & typeDB, const Type * type, const void * object, const char * filename)
{
	bool result = true;

	rapidjson::StringBuffer stringBuffer;
	REFLECTIONIO_JSON_WRITER writer(stringBuffer);

	if (object_tojson_recursive(typeDB, type, object, writer) == false)
	{
		LOG_WRN("failed to serialize object to json");
		result = false;
	}
	else
	{
		const char * text = stringBuffer.GetString();
		
		FILE * file = fopen(filename, "wt");
		
		if (file == nullptr || fprintf(file, "%s", text) < 0)
		{
			LOG_WRN("failed to save json text to file %s", filename);
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

static bool saveObjectToTextFile(const TypeDB & typeDB, const Type * type, const void * object, const char * filename)
{
	if (type == nullptr)
	{
		logError("type is NULL");
		return false;
	}
	else
	{
		bool result = true;
		
		LineWriter line_writer;
		
		if (object_tolines_recursive(
			typeDB, type, object,
			line_writer, 0) == false)
		{
			LOG_WRN("failed to serialize object to lines");
			result = false;
		}
		else
		{
			auto lines = line_writer.to_lines();
			
			if (TextIO::save(filename, lines, TextIO::kLineEndings_Unix) == false)
			{
				LOG_WRN("failed to save lines to file %s", filename);
				result = false;
			}
		}
		
		return result;
	}
}

bool saveObjectToFile(const TypeDB & typeDB, const Type * type, const void * object, const char * filename)
{
	if (Path::GetExtension(filename, true) == "json")
		return saveObjectToJsonFile(typeDB, type, object, filename);
	else
		return saveObjectToTextFile(typeDB, type, object, filename);
}

static bool loadObjectFromJsonFile(const TypeDB & typeDB, const Type * type, void * object, const char * filename)
{
	char * text = nullptr;
	size_t textSize = 0;

	if (TextIO::loadFileContents(filename, text, textSize) == false)
	{
		LOG_ERR("failed to load contents from file %s", filename);
		return false;
	}

	rapidjson::Document document;

	document.Parse(text, textSize);
	
	delete [] text;
	text = nullptr;

	if (document.HasParseError())
	{
		LOG_ERR("failed to parse json contents for file %s", filename);
		return false;
	}

	return object_fromjson_recursive(typeDB, type, object, document);
}

static bool loadObjectFromTextFile(const TypeDB & typeDB, const Type * type, void * object, const char * filename)
{
	if (type == nullptr)
	{
		logError("type is NULL");
		return false;
	}
	else
	{
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
	
		if (TextIO::load(filename, lines, lineEndings) == false)
		{
			LOG_ERR("failed to load text lines from file %s", filename);
			return false;
		}
	
		LineReader line_reader(lines, 0, 0);
	
		if (object_fromlines_recursive(typeDB, type, object, line_reader) == false)
		{
			LOG_ERR("failed to read object from lines");
			return false;
		}
		
		return true;
	}
}

bool loadObjectFromFile(const TypeDB & typeDB, const Type * type, void * object, const char * filename)
{
	if (Path::GetExtension(filename, true) == "json")
		return loadObjectFromJsonFile(typeDB, type, object, filename);
	else
		return loadObjectFromTextFile(typeDB, type, object, filename);
}
