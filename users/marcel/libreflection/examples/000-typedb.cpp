#include "reflection.h"
#include <vector>

struct ChromaticSomething // aka rainbow
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
	
	const Type * type = typeDB.findType<ChromaticSomething>();
	
	if (type->isStructured)
	{
		const StructuredType * structured_type = static_cast<const StructuredType*>(type);
		
		printf("type: %s\n", structured_type->typeName);
		
		for (const Member * member = structured_type->members_head; member != nullptr; member = member->next)
		{
			printf("\tmember: %s\n", member->name);
		}
	}

	return 0;
}
