#include "test_reflection.h"

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

static void dumpReflectionInfo_traverse(const TypeDB & typeDB, const Type * type, void * object)
{
	if (type->isStructured)
	{
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			if (member->isVector)
			{
				printf("got vector\n");
				
				auto * member_interface = static_cast<Member_VectorInterface*>(member);

				const auto vector_size = member_interface->vector_size(object);
				auto * vector_type = typeDB.findType(member_interface->vector_type());

				for (auto i = 0; i < vector_size; ++i)
				{
					auto * vector_object = member_interface->vector_access(object, i);

					dumpReflectionInfo_traverse(typeDB, vector_type, vector_object);
				}
			}
			else
			{
				auto * member_scalar = static_cast<Member_Scalar*>(member);

				auto * member_type = typeDB.findType(member_scalar->typeIndex);
				void * member_object = member_scalar->scalar_access(object);

				dumpReflectionInfo_traverse(typeDB, member_type, member_object);
			}
		}
	}
	else
	{
		auto * plainType = static_cast<const PlainType*>(type);

		switch (plainType->dataType)
		{
			// todo : show values
		case kDataType_Bool:
			printf("got bool %s\n", plainType->access<bool>(object) ? "true" : "false");
			break;
		case kDataType_Int:
			printf("got int %d\n", plainType->access<int>(object));
			break;
		case kDataType_Float:
			printf("got float %f\n", plainType->access<float>(object));
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
};

void test_reflection_1()
{
	TypeDB typeDB;

	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	
	{
		typeDB.add(typeid(TestStruct_1), "TestStruct_1")
			.add("b", &TestStruct_1::b)
			.add("x", &TestStruct_1::x)
			.add("f", &TestStruct_1::f);
	}
	
	{
		typeDB.add(typeid(TestStruct_2), "TestStruct_2")
			.add("x", &TestStruct_2::x)
			.add("f", &TestStruct_2::f)
			.add("s", &TestStruct_2::s);
	}
	
	int x = 3;
	TestStruct_1 s1;
	TestStruct_2 s2;
	
	s2.x.resize(4, 42);
	s2.f.resize(4, 3.14f);
	s2.s.resize(4);

	auto * object = &s2;
	auto * type = typeDB.findType(*object);

	dumpReflectionInfo_traverse(typeDB, type, object);
}
