#include "template.h"

#include "Log.h"

void dumpTemplateToLog(const Template & t)
{
	for (auto & component : t.components)
	{
		LOG_DBG("%30s : %20s *",
			component.typeName.c_str(),
			component.id.c_str());
		
		for (auto & property : component.properties)
		{
			LOG_DBG("%30s : %20s : %20s = %s",
				component.typeName.c_str(),
				component.id.c_str(),
				property.name.c_str(),
				property.value_lines.size() == 1 ? property.value_lines[0].c_str() : "");
			
			if (property.value_lines.size() > 1)
			{
				for (auto & line : property.value_lines)
					LOG_DBG("\t%s", line.c_str());
			}
		}
	}
}
