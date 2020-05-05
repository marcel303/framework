#include "Log.h"

#include "component.h"

#include "helpers.h"
#include "template.h"
#include "templateIo.h"

#include "helpers2.h"

#include "framework.h" // setupPaths

static void dump_template(const Template & t)
{
	for (auto & component : t.components)
	{
		LOG_INF("%30s : %20s *",
			component.typeName.c_str(),
			component.id.c_str());
		
		for (auto & property : component.properties)
		{
			LOG_INF("%30s : %20s : %20s = %s",
				component.typeName.c_str(),
				component.id.c_str(),
				property.name.c_str(),
				property.value_lines.size() == 1 ? property.value_lines[0].c_str() : "");
			
			if (property.value_lines.size() > 1)
			{
				for (auto & value_line : property.value_lines)
					LOG_INF("\t%s", value_line.c_str());
			}
		}
	}
}

static void test_v1()
{
	Template t;
	
	if (!parseTemplateFromFile("textfiles/base-entity-v1.txt", t))
		LOG_ERR("failed to load template from file", 0);
	
	Template overlay;
	
	if (!parseTemplateFromFile("textfiles/base-entity-v1-overlay.txt", overlay))
		LOG_ERR("failed to load template from file", 0);
	
	if (!overlayTemplate(t, overlay, false, true))
		LOG_ERR("failed to overlay template", 0);
	
	ComponentSet componentSet;
	if (!instantiateComponentsFromTemplate(g_typeDB, t, componentSet))
		LOG_ERR("failed to instantiate components from template", 0);
	freeComponentsInComponentSet(componentSet);
	
	// show what we just parsed
	
	dump_template(t);
}

static void test_v2()
{
	Template t;
	
	if (!parseTemplateFromFileAndRecursivelyOverlayBaseTemplates(
		"textfiles/base-entity-v2-overlay.txt",
		true,
		true,
		t))
	{
		LOG_ERR("failed to load template with overlays from file", 0);
	}
	
	ComponentSet componentSet;
	if (!instantiateComponentsFromTemplate(g_typeDB, t, componentSet))
		LOG_ERR("failed to instantiate components from template", 0);
	freeComponentsInComponentSet(componentSet);
	
	dump_template(t);
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	registerBuiltinTypes(g_typeDB);
	registerComponentTypes(g_typeDB);
	
	LOG_INF("[running test-v1]", 0);
	test_v1();
	LOG_INF("[done]", 0);
	
	LOG_INF("[running test-v2]", 0);
	test_v2();
	LOG_INF("[done]", 0);
	
	return 0;
}
