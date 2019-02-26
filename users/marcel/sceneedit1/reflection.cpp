#include "test_reflection.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <string>

//

void StructuredType::add(Member * member)
{
	if (members_head == nullptr)
		members_head = member;
	else
	{
		Member ** members_tail = &members_head;
		
		while ((*members_tail)->next != nullptr)
			members_tail = &(*members_tail)->next;
		
		(*members_tail)->next = member;
	}
}

StructuredType & StructuredType::add(const std::type_index & typeIndex, const size_t offset, const char * name)
{
	Member_Scalar * member = new Member_Scalar(name, typeIndex, offset);
	
	add(member);
	
	return *this;
}

//

#include <map>

struct TypeDB_impl
{
	std::map<std::type_index, const Type*> types;

	const Type * findType(const std::type_index & typeIndex) const
	{
		auto i = types.find(typeIndex);

		if (i == types.end())
			return nullptr;
		else
			return i->second;
	}

	void add(std::type_index typeIndex, const Type * type)
	{
		types[typeIndex] = type;
	}
};

TypeDB::TypeDB()
{
	impl = new TypeDB_impl();
}

TypeDB::~TypeDB()
{
	delete impl;
	impl = nullptr;
}

const Type * TypeDB::findType(const std::type_index & typeIndex) const
{
	return impl->findType(typeIndex);
}

void TypeDB::add(const std::type_index & typeIndex, Type * type)
{
	impl->add(typeIndex, type);
}

StructuredType & TypeDB::add(const std::type_index & typeIndex, const char * typeName)
{
	StructuredType * type = new StructuredType(typeName);
	
	add(typeIndex, type);
	
	return *type;
}

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
			// todo : show values
		case kDataType_Bool:
			printf("(bool) %s\n", plain_type->access<bool>(object) ? "true" : "false");
			break;
		case kDataType_Int:
			printf("(int) %d\n", plain_type->access<int>(object));
			break;
		case kDataType_Float:
			printf("(float) %f\n", plain_type->access<float>(object));
			break;
		case kDataType_Vec2:
			printf("(vec2) %f %f\n",
				plain_type->access<Vec2>(object)[0],
				plain_type->access<Vec2>(object)[1]);
			break;
		case kDataType_Vec3:
			printf("(vec3) %f %f %f\n",
				plain_type->access<Vec3>(object)[0],
				plain_type->access<Vec3>(object)[1],
				plain_type->access<Vec3>(object)[2]);
			break;
		case kDataType_Vec4:
			printf("(vec4) %f %f %f %f\n",
				plain_type->access<Vec4>(object)[0],
				plain_type->access<Vec4>(object)[1],
				plain_type->access<Vec4>(object)[2],
				plain_type->access<Vec4>(object)[3]);
			break;
		case kDataType_String:
			printf("(string) %s\n", plain_type->access<std::string>(object).c_str());
			break;
		}
	}
}

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
};

#define TYPEDB_ADD_STRUCT(typeDB, type) \
	typeDB.add(std::type_index(typeid(type)), #type)

void test_reflection_1()
{
	TypeDB typeDB;

	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("Vec2", kDataType_Vec2);
	typeDB.addPlain<Vec3>("Vec3", kDataType_Vec3);
	typeDB.addPlain<Vec4>("Vec4", kDataType_Vec4);
	typeDB.addPlain<std::string>("std::string", kDataType_String);
	
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
			.add("s", &TestStruct_3::s);
	}
	
	int x = 3;
	TestStruct_1 s1;
	TestStruct_2 s2;
	TestStruct_3 s3;
	
	s2.x.resize(4, 42);
	s2.f.resize(4, 3.14f);
	s2.s.resize(4);

	auto * object = &s3;
	auto * type = typeDB.findType(*object);

	dumpReflectionInfo_traverse(typeDB, type, object, 0);
}
