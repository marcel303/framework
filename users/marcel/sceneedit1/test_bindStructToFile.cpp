#include "FileStream.h" // FileStream::Exists
#include "framework.h" // fileHasChanged
#include "helpers.h" // object (de)serialization
#include "lineReader.h"
#include "lineWriter.h"
#include "Log.h"
#include "reflection.h"
#include "TextIO.h"
#include <vector>

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

bool bindObjectToFile(const TypeDB * typeDB, const Type * type, void * object, const char * filename)
{
	s_objectToFileBindings.resize(s_objectToFileBindings.size() + 1);
	auto & objectToFileBinding = s_objectToFileBindings.back();
	objectToFileBinding.typeDB = typeDB;
	objectToFileBinding.type = type;
	objectToFileBinding.object = object;
	objectToFileBinding.filename = filename;

	// todo : check if the file exists. if so, deserialize it
	// if not, create a new file and serialize the object to it

	if (FileStream::Exists(filename))
	{
		objectToFileBinding.loadFromFile();
	}
	else
	{
		LineWriter line_writer;
		
		if (object_tolines_recursive(
			*typeDB, type, object,
			line_writer, 0) == false)
		{
			LOG_WRN("failed to serialize object to lines", 0);
		}
		else
		{
			auto lines = line_writer.to_lines();
			if (TextIO::save(filename, lines, TextIO::kLineEndings_Unix) == false)
				LOG_WRN("failed to save lines to file %s", filename);
		}
	}

	return true;
}

template <typename T>
bool bindObjectToFile(const TypeDB * typeDB, T * object, const char * filename)
{
	auto * type = typeDB->findType<T>();

	Assert(type != nullptr);
	if (type == nullptr)
		return false;
	
	return bindObjectToFile(typeDB, type, object, filename);
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

//

#include "Vec2.h"

void test_bindObjectToFile()
{
	TypeDB typeDB;
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("Vec2", kDataType_Float2);
	typeDB.addPlain<Vec3>("Vec3", kDataType_Float3);

	struct Polygon
	{
		std::vector<Vec2> points;
	};
	
	typeDB.addStructured<Polygon>("Polygon")
		.add("points", &Polygon::points);
	
	Polygon polygon;
	
	Vec3 color = Vec3(0.f, 1.f, 0.f);
	
	if (bindObjectToFile(&typeDB, &polygon, "out/polygon.txt") == false)
		logError("failed to bind object to file");
	if (bindObjectToFile(&typeDB, &color, "out/polygon-color.txt") == false)
		logError("failed to bind object to file");
	
	// real-time editing needs to be enabled to let framework detect file changes
	framework.enableRealTimeEditing = true;
	
	framework.init(640, 480);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		tickObjectToFileBinding();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setColorf(
				color[0],
				color[1],
				color[2]);
			
			gxBegin(GX_LINE_LOOP);
			{
				for (auto & point : polygon.points)
					gxVertex2f(point[0], point[1]);
			}
			gxEnd();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
}
