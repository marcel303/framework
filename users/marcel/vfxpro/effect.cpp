#include "effect.h"

//

std::map<std::string, Effect*> g_effectsByName;

void registerEffect(const char * name, Effect * effect)
{
	Assert(g_effectsByName.count(name) == 0);

	g_effectsByName[name] = effect;
}

void unregisterEffect(Effect * effect)
{
	for (auto i = g_effectsByName.begin(); i != g_effectsByName.end(); ++i)
	{
		if (i->second == effect)
		{
			g_effectsByName.erase(i);
			break;
		}
	}
}
