#include "Debugging.h"
#include "reflection.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <string>

static void print_indent(const int indent)
{
	for (int i = 0; i < indent; ++i)
		printf("\t");
}

static void dumpReflectionInfo_traverse(const TypeDB & typeDB, const Type * type, void * object, const int indent)
{
	if (type->isStructured)
	{
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		print_indent(indent); printf("structured type '%s'\n", structured_type->typeName);
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			if (member->isVector)
			{
				print_indent(indent); printf("got vector\n");
				
				auto * member_interface = static_cast<Member_VectorInterface*>(member);

				const auto vector_size = member_interface->vector_size(object);
				auto * vector_type = typeDB.findType(member_interface->vector_type());

				for (auto i = 0; i < vector_size; ++i)
				{
					auto * vector_object = member_interface->vector_access(object, i);

					dumpReflectionInfo_traverse(typeDB, vector_type, vector_object, indent + 1);
				}
			}
			else
			{
				print_indent(indent); printf("got scalar\n");
				
				auto * member_scalar = static_cast<Member_Scalar*>(member);

				auto * member_type = typeDB.findType(member_scalar->typeIndex);
				void * member_object = member_scalar->scalar_access(object);

				dumpReflectionInfo_traverse(typeDB, member_type, member_object, indent + 1);
			}
		}
	}
	else
	{
		auto * plain_type = static_cast<const PlainType*>(type);

		print_indent(indent); printf("%s ", plain_type->typeName);
		
		switch (plain_type->dataType)
		{
		case kDataType_Bool:
			printf("(bool) %s\n", plain_type->access<bool>(object) ? "true" : "false");
			break;
		case kDataType_Int:
			printf("(int) %d\n", plain_type->access<int>(object));
			break;
		case kDataType_Float:
			printf("(float) %f\n", plain_type->access<float>(object));
			break;
		case kDataType_Float2:
			printf("(vec2) %f %f\n",
				plain_type->access<Vec2>(object)[0],
				plain_type->access<Vec2>(object)[1]);
			break;
		case kDataType_Float3:
			printf("(vec3) %f %f %f\n",
				plain_type->access<Vec3>(object)[0],
				plain_type->access<Vec3>(object)[1],
				plain_type->access<Vec3>(object)[2]);
			break;
		case kDataType_Float4:
			printf("(vec4) %f %f %f %f\n",
				plain_type->access<Vec4>(object)[0],
				plain_type->access<Vec4>(object)[1],
				plain_type->access<Vec4>(object)[2],
				plain_type->access<Vec4>(object)[3]);
			break;
		case kDataType_Double:
			printf("(double) %f\n", plain_type->access<double>(object));
			break;
		case kDataType_String:
			printf("(string) %s\n", plain_type->access<std::string>(object).c_str());
			break;
		case kDataType_Enum:
			{
				const EnumType * enum_type = static_cast<const EnumType*>(plain_type);
				const char * key;
				if (enum_type->get_key(object, key))
					printf("(enum) %s\n", key);
			}
			break;
			
		case kDataType_Other:
			Assert(false);
			break;
		}
	}
}

enum TestEnum
{
	OptionA,
	OptionB,
	OptionC
};

struct TestStruct_1
{
	bool b = false;
	int x = 42;
	float f = 1.f;
};

struct TestStruct_2
{
	//std::vector<bool> b; // boolean not supported due to STL using a bit vector in the background for this type
	std::vector<int> x;
	std::vector<float> f;
	std::vector<TestStruct_1> s;
	
	float g = 10.f;
};

struct TestStruct_3
{
	Vec2 v2 = Vec2(2.1f, 2.2f);
	Vec3 v3 = Vec3(3.1f, 3.2f, 3.3f);
	Vec4 v4 = Vec4(4.1f, 4.2f, 4.3f, 4.4f);
	std::string s = "hey!";
	TestEnum e = TestEnum::OptionB;
};

#define TYPEDB_ADD_STRUCT(typeDB, type) \
	typeDB.add(std::type_index(typeid(type)), #type)

int main(int argc, char * argv[])
{
	TypeDB typeDB;

	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("Vec2", kDataType_Float2);
	typeDB.addPlain<Vec3>("Vec3", kDataType_Float3);
	typeDB.addPlain<Vec4>("Vec4", kDataType_Float4);
	typeDB.addPlain<std::string>("std::string", kDataType_String);
	
	typeDB.addEnum<TestEnum>("TestEnum")
		.add("OptionA", TestEnum::OptionA)
		.add("OptionB", TestEnum::OptionB)
		.add("OptionC", TestEnum::OptionC);
	
	{
		TYPEDB_ADD_STRUCT(typeDB, TestStruct_1)
			.add("b", &TestStruct_1::b)
			.add("x", &TestStruct_1::x)
			.add("f", &TestStruct_1::f);
	}
	
	{
		TYPEDB_ADD_STRUCT(typeDB, TestStruct_2)
			.add("x", &TestStruct_2::x)
			.add("f", &TestStruct_2::f)
			.add("s", &TestStruct_2::s)
			.add("g", &TestStruct_2::g);
	}
	
	{
		TYPEDB_ADD_STRUCT(typeDB, TestStruct_3)
			.add("v2", &TestStruct_3::v2)
			.add("v3", &TestStruct_3::v3)
			.add("v4", &TestStruct_3::v4)
			.add("s", &TestStruct_3::s)
			.add("e", &TestStruct_3::e);
	}
	
	int x = 3;
	TestStruct_1 s1;
	TestStruct_2 s2;
	TestStruct_3 s3;
	(void)x;
	(void)s1;
	(void)s2;
	(void)s3;
	
	s2.x.resize(4, 42);
	s2.f.resize(4, 3.14f);
	s2.s.resize(4);

	auto * object = &s3;
	auto * type = typeDB.findType(*object);

	dumpReflectionInfo_traverse(typeDB, type, object, 0);
	
	return 0;
}
