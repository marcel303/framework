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
		REFLECTIONIO_JSON_WRITER writer(json);
		bool value = true;
		plain_type_tojson(&bool_type, &value, writer);
		
		printf("bool json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		int value = 42;
		plain_type_tojson(&int_type, &value, writer);
		
		printf("int json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		float value = .42f;
		plain_type_tojson(&float_type, &value, writer);
		
		printf("float json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		Vec2 value(.12f, .34f);
		plain_type_tojson(&vec2_type, &value, writer);
		
		printf("vec2 json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		Vec3 value(.12f, .34f, .56f);
		plain_type_tojson(&vec3_type, &value, writer);
		
		printf("vec3 json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		Vec4 value(.12f, .34f, .56f, .67f);
		plain_type_tojson(&vec4_type, &value, writer);
		
		printf("vec4 json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		double value = 0.42;
		plain_type_tojson(&double_type, &value, writer);
		
		printf("doubld json: %s\n", json.GetString());
	}
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		std::string value = "hello json";
		plain_type_tojson(&string_type, &value, writer);
		
		printf("string json: %s\n", json.GetString());
	}
	
	printf("\n");
	
	struct TestStruct
	{
		int i = 1;
		float f = 2.f;
		double d = 3.0;
		Vec2 v2 = Vec2(4.f, 5.f);
		Vec3 v3 = Vec3(6.f, 7.f, 8.f);
		Vec4 v4 = Vec4(9.f, 10.f, 11.f, 12.f);
		std::string s = "hello json";
		std::vector<TestStruct> children;
		std::vector<int> int_children;
		std::vector<Vec2> vec2_children;
		
		void dump(const int indent)
		{
			auto print_indent = [](const int indent)
			{
				for (int i = 0; i < indent; ++i)
					printf("\t");
			};
			
			print_indent(indent); printf("%d, %f, %f\n", i, f, d);
			print_indent(indent); printf("(%f, %f)\n", v2[0], v2[1]);
			print_indent(indent); printf("(%f, %f, %f)\n", v3[0], v3[1], v3[2]);
			print_indent(indent); printf("(%f, %f, %f, %f)\n", v4[0], v4[1], v4[2], v4[3]);
			print_indent(indent); printf("%s\n", s.c_str());
			
			for (size_t i = 0; i < children.size(); ++i)
			{
				print_indent(indent); printf("child [%d]\n", int(i));
				children[i].dump(indent + 1);
			}
			
			print_indent(indent); printf("int_children:\n");
			for (auto & i : int_children)
			{
				print_indent(indent + 1); printf("%d\n", i);
			}
			
			print_indent(indent); printf("vec2_children:\n");
			for (auto & v : vec2_children)
			{
				print_indent(indent + 1); printf("%(%f, %f)\n", v[0], v[1]);
			}
		}
	};
	
	typeDB.addStructured<TestStruct>("TestStruct")
		.add("i", &TestStruct::i)
		.add("f", &TestStruct::f)
		.add("d", &TestStruct::d)
		.add("v2", &TestStruct::v2)
		.add("v3", &TestStruct::v3)
		.add("v4", &TestStruct::v4)
		.add("s", &TestStruct::s)
		.add("children", &TestStruct::children)
		.add("int_children", &TestStruct::int_children)
		.add("vec2_children", &TestStruct::vec2_children);
	
	{
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		
		TestStruct object;
		const Type * object_type = typeDB.findType(object);
		
		if (object_tojson_recursive(typeDB, object_type, &object, writer) == false)
			printf("object_tojson_recursive failed\n");
		else
		{
			printf("object_tojson_recursive succeeded\n");
			printf("%s\n", json.GetString());
		}
	}
	
	{
		TestStruct object;
		object.children.resize(2);
		object.children[0].i = 123456;
		object.children[1].i = 234567;
		object.children[0].s = "ok! 1";
		object.children[1].s = "ok! 2";
		object.int_children.resize(4, 42);
		object.vec2_children.resize(4, Vec2(4.f, 2.f));
		
		const Type * object_type = typeDB.findType(object);
		
		rapidjson::StringBuffer json;
		REFLECTIONIO_JSON_WRITER writer(json);
		object_tojson_recursive(typeDB, object_type, &object, writer);
		
		printf("object at time of object_tojson_recursive:\n");
		object.dump(1);
		
		rapidjson::Document document;
		document.Parse(json.GetString());
		
		object.i = 0;
		object.f = 0.f;
		object.d = 0.0;
		object.v2.SetZero();
		object.v3.SetZero();
		object.v4.SetZero();
		object.s.clear();
		object.children.clear();
		object.int_children.clear();
		object.vec2_children.clear();
		
		printf("object before object_fromjson_recursive:\n");
		object.dump(1);
		
		if (object_fromjson_recursive(typeDB, object_type, &object, document) == false)
			printf("object_fromjson_recursive failed\n");
		else
		{
			printf("object_fromjson_recursive succeeded\n");
			printf("object after object_fromjson_recursive:\n");
			object.dump(1);
		}
	}
	
	return 0;
}
