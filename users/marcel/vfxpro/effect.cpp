#include "effect.h"
#include "tinyxml2.h"
#include "xml.h"

//

using namespace tinyxml2;

//

EffectInfosByName g_effectInfosByName;

bool EffectInfosByName::load(const char * filename)
{
	clear();

	bool result = true;

	tinyxml2::XMLDocument xmlDoc;

	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load %s", filename);

		result = false;
	}
	else
	{
		const XMLElement * xmlEffects = xmlDoc.FirstChildElement("effects");

		if (xmlEffects == nullptr)
		{
			logError("effects element not found");

			result = false;
		}
		else
		{
			for (const XMLElement * xmlEffect = xmlEffects->FirstChildElement("effect"); xmlEffect; xmlEffect = xmlEffect->NextSiblingElement())
			{
				const std::string name = stringAttrib(xmlEffect, "name", "");
				const std::string image = stringAttrib(xmlEffect, "image", "");
				const std::string param1 = stringAttrib(xmlEffect, "param1", "");
				const std::string param2 = stringAttrib(xmlEffect, "param2", "");
				const std::string param3 = stringAttrib(xmlEffect, "param3", "");
				const std::string param4 = stringAttrib(xmlEffect, "param4", "");

				if (name.empty())
				{
					logError("effect name not set");

					result = false;
				}
				else
				{
					EffectInfo effectInfo;

					effectInfo.imageName = image;
					effectInfo.paramName[0] = param1;
					effectInfo.paramName[1] = param2;
					effectInfo.paramName[2] = param3;
					effectInfo.paramName[3] = param4;

					// todo : check not already set

					(*this)[name] = effectInfo;
				}
			}
		}
	}

	if (!result)
	{
		clear();
	}

	return result;
}

std::string effectParamToName(const std::string & effectName, const std::string & param)
{
	const auto effectInfoItr = g_effectInfosByName.find(effectName);

	if (effectInfoItr == g_effectInfosByName.end())
		return param;

	const EffectInfo & info = effectInfoItr->second;

	if (param == "param1" && !info.paramName[0].empty())
		return info.paramName[0];
	else if (param == "param2" && !info.paramName[1].empty())
		return info.paramName[1];
	else if (param == "param3" && !info.paramName[2].empty())
		return info.paramName[2];
	else if (param == "param4" && !info.paramName[3].empty())
		return info.paramName[3];
	else
		return param;
}

std::string nameToEffectParam(const std::string & effectName, const std::string & name)
{
	const auto effectInfoItr = g_effectInfosByName.find(effectName);

	if (effectInfoItr == g_effectInfosByName.end())
		return name;

	const EffectInfo & info = effectInfoItr->second;

	for (int i = 0; i < 4; ++i)
	{
		if (info.paramName[i] == name)
		{
			char temp[32];
			sprintf_s(temp, sizeof(temp), "param%d", i + 1);
			return temp;
		}
	}

	return name;
}

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
