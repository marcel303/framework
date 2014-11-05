#include "ResLoaderVS.h"
#include "ResVS.h"

Res* ResLoaderVS::Load(const std::string& name)
{
	ResVS* vs = new ResVS();

	vs->Load(name);

	return vs;
}
