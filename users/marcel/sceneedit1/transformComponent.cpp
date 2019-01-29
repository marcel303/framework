#include "Parse.h"
#include "transformComponent.h"

bool TransformComponent::init(const std::vector<KeyValuePair> & params)
{
	for (auto & param : params)
	{
		if (strcmp(param.key, "scale") == 0)
			scale = Parse::Float(param.value);
		else if (strcmp(param.key, "x") == 0)
			position[0] = Parse::Float(param.value);
		else if (strcmp(param.key, "y") == 0)
			position[1] = Parse::Float(param.value);
		else if (strcmp(param.key, "z") == 0)
			position[2] = Parse::Float(param.value);
	}
	
	return true;
}
