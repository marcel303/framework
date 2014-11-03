#include "ResLoaderPS.h"
#include "ResPS.h"

Res* ResLoaderPS::Load(const std::string& name)
{
	ResPS* ps = new ResPS();

	ps->Load(name);

	return ps;
}
