#include "reflection.h"
#include <stdio.h>
#include <vector>

/*

This example shows how to perform basic reflection on
a structured type.

*/

struct ChromaticSomething // aka a rainbow
{
	bool chromatic = true;
	std::vector<int> hues;
	float density = 1.f;
};

int main(int argc, char * argv[])
{
	TypeDB typeDB;

	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
		
	typeDB.addStructured<ChromaticSomething>("ChromaticSomething")
		.add("chromatic", &ChromaticSomething::chromatic)
		.add("hues", &ChromaticSomething::hues)
		.add("density", &ChromaticSomething::density);

	// we now have a type database. whoohoo!
	// let's create and instance and do something
	// do something with it through reflection

	ChromaticSomething something;
	something.chromatic = false;
	something.density = .76f;
	
	auto * type = typeDB.findType<ChromaticSomething>();
	
	if (type->isStructured)
	{
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		printf("type: %s\n", structured_type->typeName);
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			printf("\tmember: %s\n", member->name);

			if (member->isVector)
				continue; // ignore arrays for now

			auto * member_scalar = static_cast<const Member_Scalar*>(member);
			void * member_object = member_scalar->scalar_access(&something);
			
			auto * member_type = typeDB.findType(member_scalar->typeIndex);
			
			if (member_type->isStructured)
				continue; // ignore structured types within structured types for now
			
			auto * plain_type = static_cast<const PlainType*>(member_type);
			
			switch (plain_type->dataType)
			{
			case kDataType_Bool:
				{
					const bool value = plain_type->access<bool>(member_object);
					printf("\t\tboolean %s\n", value ? "true" : "false");
				}
				break;
				
			case kDataType_Int:
				{
					const int value = plain_type->access<int>(member_object);
					printf("\t\tint %d\n", value);
				}
				break;
				
			case kDataType_Float:
				{
					const float value = plain_type->access<float>(member_object);
					printf("\t\tfloat %g\n", value);
				}
				break;
				
			default:
				break;
			}
		}
	}

	return 0;
}
