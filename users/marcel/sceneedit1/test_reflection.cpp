#include "test_reflection.h"

static void dumpReflectionInfo_traverse(const TypeDB & typeDB, const Type * type, void * object)
{
	if (type->isStructured)
	{
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		for (auto * member : structured_type->members)
		{
			if (member->isVector)
			{
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
				auto * member_interface = static_cast<Member_ScalarInterface*>(member);

				auto * member_type = typeDB.findType(member_interface->scalar_type());
				void * member_object = member_interface->scalar_access(object);

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

void test_reflection_1()
{
	TypeDB typeDB;

	typeDB.addPlain<bool>(kDataType_Bool);
	typeDB.addPlain<int>(kDataType_Int);
	typeDB.addPlain<float>(kDataType_Float);
	
	StructuredType * stype = new StructuredType("TestStruct_1");
	stype->add(typeid(TestStruct_1::b), offsetof(TestStruct_1, b), "b");
	stype->add(typeid(TestStruct_1::x), offsetof(TestStruct_1, x), "x");
	stype->add(typeid(TestStruct_1::f), offsetof(TestStruct_1, f), "f");
	
	typeDB.add(typeid(TestStruct_1), stype);
	
	int x = 3;
	TestStruct_1 s;

	auto * object = &x;
	auto * type = typeDB.findType(*object);

	dumpReflectionInfo_traverse(typeDB, type, object);
}
