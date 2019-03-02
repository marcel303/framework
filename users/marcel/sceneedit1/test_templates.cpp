#include "framework.h"
#include "helpers.h"
#include "template.h"

#include "scene.h"

static void dump_template(const Template & t)
{
#if ENABLE_LOGGING
	for (auto & component : t.components)
	{
		logInfo("%30s : %20s *",
			component.type_name.c_str(),
			component.id.c_str());
		
		for (auto & property : component.properties)
		{
			logInfo("%30s : %20s : %20s = %s",
				component.type_name.c_str(),
				component.id.c_str(),
				property.name.c_str(),
				property.value_lines.size() == 1 ? property.value_lines[0].c_str() : "");
			
			if (property.value_lines.size() > 1)
			{
				for (auto & value_line : property.value_lines)
					logInfo("\t%s", value_line.c_str());
			}
		}
	}
#endif
}

void test_templates_v1()
{
	Template t;
	
	if (!loadTemplateFromFile("textfiles/base-entity-v1.txt", t))
		logError("failed to load template from file");
	
	Template overlay;
	
	if (!loadTemplateFromFile("textfiles/base-entity-v1-overlay.txt", overlay))
		logError("failed to load template from file");
	
	if (!overlayTemplate(t, overlay, false, true))
		logError("failed to overlay template");
	
	SceneNode node;
	
	if (!instantiateComponentsFromTemplate(g_typeDB, t, node.components))
	{
		logError("failed to instantiate components from template");
		
		node.freeComponents();
	}
	
	// show what we just parsed
	
	dump_template(t);
}

void test_templates()
{
	if (!framework.init(640, 480))
		return;

	registerBuiltinTypes();
	registerComponentTypes();
	
	Template t;
	
	if (!loadTemplateWithOverlaysFromFile("textfiles/base-entity-v2-overlay.txt", t, false))
		logError("failed to load template with overlays from file");
	
	SceneNode node;
	
	if (!instantiateComponentsFromTemplate(g_typeDB, t, node.components))
	{
		logError("failed to instantiate components from template");
		
		node.freeComponents();
	}
	
	dump_template(t);
	
	exit(0);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		framework.beginDraw(0, 0, 0, 0);
		{

		}
		framework.endDraw();
	}

	framework.shutdown();
}
