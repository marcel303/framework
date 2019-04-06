#include "reflection.h"
#include <assert.h>
#include <vector>

/*

This example shows how to perform reflection recursively on structured types.

*/

struct Location
{
	float elevation = 0.f;
	float azimuth = 0.f;
};

struct Dataset
{
	std::vector<Dataset> subsets;
	Location location;
};

static void indent(const int indentation_level)
{
	for (int i = 0; i < indentation_level; ++i)
		printf("\t");
}

static void printStructuredObject_recursively(const TypeDB & typeDB, const Type * type, const void * object, const int indentation_level)
{
	if (type->isStructured)
	{
		auto * structured_type = static_cast<const StructuredType*>(type);
		
		for (auto * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			if (member->isVector)
			{
				auto * member_vector = static_cast<const Member_VectorInterface*>(member);
				
				auto * vector_type = typeDB.findType(member_vector->vector_type());
				
				assert(vector_type != nullptr);
				if (vector_type == nullptr) // this could happen if the type database is incomplete
					continue;
				
				const size_t vector_size = member_vector->vector_size(object);
				
				indent(indentation_level); printf("member: %s [%zu]\n", member->name, vector_size);
				
				for (size_t i = 0; i < vector_size; ++i)
				{
					auto * vector_object = member_vector->vector_access(object, i);
					
					indent(indentation_level + 1); printf("[%zu]\n", i);
					printStructuredObject_recursively(typeDB, vector_type, vector_object, indentation_level + 2);
				}
			}
			else
			{
				indent(indentation_level); printf("member: %s\n", member->name);
				
				auto * member_scalar = static_cast<const Member_Scalar*>(member);
				auto * member_object = member_scalar->scalar_access(object);
				
				auto * member_type = typeDB.findType(member_scalar->typeIndex);
				
				assert(member_type != nullptr);
				if (member_type == nullptr) // this could happen if the type database is incomplete
					continue;
				
				if (member_type->isStructured)
				{
					printStructuredObject_recursively(typeDB, member_type, member_object, indentation_level + 1);
				}
				else
				{
					auto * plain_type = static_cast<const PlainType*>(member_type);
					
					assert(plain_type != nullptr);
					if (plain_type == nullptr) // this could happen if the type database is incomplete
						continue;
					
					switch (plain_type->dataType)
					{
					case kDataType_Bool:
						{
							const bool value = plain_type->access<bool>(member_object);
							indent(indentation_level + 1); printf("boolean %s\n", value ? "true" : "false");
						}
						break;
						
					case kDataType_Int:
						{
							const int value = plain_type->access<int>(member_object);
							indent(indentation_level + 1); printf("int %d\n", value);
						}
						break;
						
					case kDataType_Float:
						{
							const float value = plain_type->access<float>(member_object);
							indent(indentation_level + 1); printf("float %g\n", value);
						}
						break;
						
					default:
						assert(false);
						break;
					}
				}
			}
		}
	}
}

int main(int argc, char * argv[])
{
	TypeDB typeDB;

	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	
	typeDB.addStructured<Location>("Location")
		.add("elevation", &Location::elevation)
		.add("azimuth", &Location::azimuth);
	
	typeDB.addStructured<Dataset>("Dataset")
		.add("subsets", &Dataset::subsets)
		.add("location", &Dataset::location);

	// we now have a type database. whoohoo!
	// let's set up a dataset and print it

	Dataset dataset;
	dataset.subsets.resize(2);
	dataset.subsets[0].location.azimuth = 10.f;
	dataset.subsets[0].location.elevation = 12.f;
	dataset.subsets[1].location.azimuth = 30.f;
	dataset.subsets[1].location.elevation = 32.f;
	dataset.subsets[1].subsets.resize(2);
	dataset.location.azimuth = 40.f;
	dataset.location.elevation = 42.f;
	
	auto * type = typeDB.findType(dataset);
	
	if (type != nullptr)
	{
		printStructuredObject_recursively(typeDB, type, &dataset, 0);
	}
	
	return 0;
}
