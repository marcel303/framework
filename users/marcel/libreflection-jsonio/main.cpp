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
	
	return 0;
}
