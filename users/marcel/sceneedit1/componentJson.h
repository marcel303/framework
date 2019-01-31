#pragma once

#include "json.hpp" // sadly cannot forward declare json class ..

struct ComponentJson
{
	nlohmann::json & j;
	
	ComponentJson(nlohmann::json & in_j)
		: j(in_j)
	{
	}
	
	ComponentJson(const nlohmann::json & in_j)
		: j(const_cast<nlohmann::json&>(in_j))
	{
	}
};
