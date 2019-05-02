#include "rapidjson/stringbuffer.h"
#include "reflection.h"
#include "reflection-jsonio.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <string>

int main(int argc, char * argv[])
{
	TypeDB typeDB;
	auto & bool_type = typeDB.addPlain<bool>("bool", kDataType_Bool);
	auto & int_type = typeDB.addPlain<int>("int", kDataType_Int);
	auto & float_type = typeDB.addPlain<float>("float", kDataType_Float);
	auto & vec2_type = typeDB.addPlain<Vec2>("vec2", kDataType_Float2);
	auto & vec3_type = typeDB.addPlain<Vec3>("vec3", kDataType_Float3);
	auto & vec4_type = typeDB.addPlain<Vec4>("vec4", kDataType_Float4);
	auto & double_type = typeDB.addPlain<double>("double", kDataType_Double);
	auto & string_type = typeDB.addPlain<std::string>("string", kDataType_String);
	
	printf("-- plain_type_fromjson tests --\n");
	printf("\n");
	
	{
		rapidjson::Document document;
		document.Parse("true");
		bool value = false;
		plain_type_fromjson(&bool_type, &value, document);
		
		printf("bool from json: %s\n", value ? "true" : "false");
	}
	
	{
		rapidjson::Document document;
		document.Parse("42");
		int value = 0;
		plain_type_fromjson(&int_type, &value, document);
		
		printf("int from json: %d\n", value);
	}
	
	{
		rapidjson::Document document;
		document.Parse("42.1");
		float value = 0.f;
		plain_type_fromjson(&float_type, &value, document);
		
		printf("float from json: %f\n", value);
	}
	
	{
		rapidjson::Document document;
		document.Parse("{ \"x\": 1.0, \"y\": 2.0 }");
		Vec2 value;
		plain_type_fromjson(&vec2_type, &value, document);
		
		printf("vec2 from json: (%f, %f)\n", value[0], value[1]);
	}
	
	{
		rapidjson::Document document;
		document.Parse("{ \"x\": 1.0, \"y\": 2.0, \"z\": 3.0  }");
		Vec3 value;
		plain_type_fromjson(&vec3_type, &value, document);
		
		printf("vec3 from json: (%f, %f, %f)\n", value[0], value[1], value[2]);
	}
	
	{
		rapidjson::Document document;
		document.Parse("{ \"x\": 1.0, \"y\": 2.0, \"z\": 3.0, \"w\": 4.0  }");
		Vec4 value;
		plain_type_fromjson(&vec4_type, &value, document);
		
		printf("vec4 from json: (%f, %f, %f, %f)\n", value[0], value[1], value[2], value[3]);
	}
	
	{
		rapidjson::Document document;
		document.Parse("42.1");
		double value = 0.0;
		plain_type_fromjson(&double_type, &value, document);
		
		printf("double from json: %f\n", value);
	}
	
	{
		rapidjson::Document document;
		document.Parse("\"hello json\"");
		std::string value;
		plain_type_fromjson(&string_type, &value, document);
		
		printf("string from json: %s\n", value.c_str());
	}
	
	printf("\n");
	
	printf("-- plain_type_tojson tests --\n");
	printf("\n");
	
	{
		rapidjson::StringBuffer json;
		bool value = true;
		plain_type_tojson(&bool_type, &value, json);
		
		printf("bool json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		int value = 42;
		plain_type_tojson(&int_type, &value, json);
		
		printf("int json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		float value = .42f;
		plain_type_tojson(&float_type, &value, json);
		
		printf("float json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		Vec2 value(.12f, .34f);
		plain_type_tojson(&vec2_type, &value, json);
		
		printf("vec2 json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		Vec3 value(.12f, .34f, .56f);
		plain_type_tojson(&vec3_type, &value, json);
		
		printf("vec3 json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		Vec4 value(.12f, .34f, .56f, .67f);
		plain_type_tojson(&vec4_type, &value, json);
		
		printf("vec4 json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		double value = 0.42;
		plain_type_tojson(&double_type, &value, json);
		
		printf("doubld json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		std::string value = "hello json";
		plain_type_tojson(&string_type, &value, json);
		
		printf("string json: %s\n", json.GetString());
	}
	
	printf("\n");
	
	return 0;
}
