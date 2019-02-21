#include "framework.h"
#include "helpers.h"
#include "template.h"

#include "scene.h"

void test_templates()
{
	if (!framework.init(640, 480))
		return;

	registerComponentTypes();
	
	Template t;
	
	if (!loadTemplateFromFile("textfiles/base-entity-v1.txt", t))
		logError("failed to load template from file");
	
	Template overlay;
	
	if (!loadTemplateFromFile("textfiles/base-entity-v1-overlay.txt", overlay))
		logError("failed to load template from file");
	
	if (!overlayTemplate(t, overlay, false, true))
		logError("failed to overlay template");
	
	SceneNode node;
	
	if (!instantiateComponentsFromTemplate(t, node.components))
		logError("failed to instantiate components from template");
	
	// show what we just parsed
	
	for (auto & component : t.components)
	{
		for (auto & property : component.properties)
		{
			logDebug("%30s : %20s : %20s = %s",
				component.type_name.c_str(),
				component.id.c_str(),
				property.name.c_str(),
				property.value.c_str());
		}
	}
	
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
