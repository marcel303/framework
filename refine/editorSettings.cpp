#include "editorSettings.h"
#include "fileEditor.h"
#include "lineReader.h"
#include "lineWriter.h"
#include "reflection.h"
#include "reflection-textio.h"
#include "TextIO.h"
#include <typeinfo>

#if defined(__GNUC__)
#include <cxxabi.h>
static std::string demangle(const char * name)
{
	int status;
	return abi::__cxa_demangle(name, 0, 0, &status);
}
#else
static std::string demangle(const char * name)
{
	return name;
}
#endif

static void addTypes(TypeDB & typeDB)
{
	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("vec2", kDataType_Float2);
	typeDB.addPlain<Vec3>("vec3", kDataType_Float3);
	typeDB.addPlain<Vec4>("vec4", kDataType_Float4);
	typeDB.addPlain<std::string>("string", kDataType_String);
}

void saveEditorSettings(FileEditor * editor)
{
	if (editor == nullptr)
		return;
	
	const std::string typeName = demangle(typeid(*editor).name());
	
	TypeDB typeDB;
	auto & type = typeDB.add(typeid(*editor), typeName.c_str());

	if (editor->reflect(typeDB, type))
	{
		addTypes(typeDB);
		
		const std::string filename = std::string("editor-settings/") + typeName + ".txt";
		
		LineWriter lineWriter;
		if (object_tolines_recursive(typeDB, &type, editor, lineWriter, 0) == false)
			logError("failed to save object to lines");
		else
		{
			auto lines = lineWriter.to_lines();
			
			if (TextIO::save(filename.c_str(), lines, TextIO::kLineEndings_Unix) == false)
				logError("failed to save editor settings");
		}
	}
}

void loadEditorSettings(FileEditor * editor)
{
	const std::string typeName = demangle(typeid(*editor).name());
	
	TypeDB typeDB;
	auto & type = typeDB.add(typeid(*editor), typeName.c_str());
	
	if (editor->reflect(typeDB, type))
	{
		addTypes(typeDB);
		
		const std::string filename = std::string("editor-settings/") + typeName + ".txt";
		
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
		
		if (TextIO::load(filename.c_str(), lines, lineEndings))
		{
			LineReader lineReader(lines, 0, 0);
			
			if (object_fromlines_recursive(typeDB, &type, editor, lineReader) == false)
				logError("failed to load object from lines");
		}
	}
}
