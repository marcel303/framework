#include "reflection.h"

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
		.add("chromatic", &InterstellarRainbow::chromatic)
		.add("hues", &InterstellarRainbow::hues)
		.add("density", &InterstellarRainbow::density);

	// we now have a type database. whoohoo!

	return 0;
}