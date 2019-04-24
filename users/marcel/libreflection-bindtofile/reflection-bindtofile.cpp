#include "reflection.h"
#include "reflection-bindtofile.h"
#include "reflection-textio.h"

#include "lineReader.h"
#include "lineWriter.h"

#include "FileStream.h"
#include "framework.h"
#include "Log.h"
#include "TextIO.h"

struct ObjectToFileBinding
{
	const TypeDB * typeDB;
	const Type * type;
	void * object;
	const char * filename;
	
	bool loadFromFile()
	{
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
	
		if (TextIO::load(filename, lines, lineEndings) == false)
		{
			LOG_ERR("failed to load text lines from file %s", filename);
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
	
	// todo : check if the file exists. if so, deserialize it
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
			
			if (TextIO::save(filename, lines, TextIO::kLineEndings_Unix) == false)
			{
				LOG_WRN("failed to save lines to file %s", filename);
				result = false;
			}
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
		if (framework.fileHasChanged(objectToFileBinding.filename))
		{
			objectToFileBinding.loadFromFile();
		}
	}
}
