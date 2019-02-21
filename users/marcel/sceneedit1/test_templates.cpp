#include "framework.h"
#include "helpers.h"
#include "template.h"

#include "scene.h"

static void dump_template(const Template & t)
{
	for (auto & component : t.components)
	{
		logDebug("%30s : %20s *",
			component.type_name.c_str(),
			component.id.c_str());
		
		for (auto & property : component.properties)
		{
			logDebug("%30s : %20s : %20s = %s",
				component.type_name.c_str(),
				component.id.c_str(),
				property.name.c_str(),
				property.value.c_str());
		}
	}
}

static void test_templates_v1()
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
	
	if (!instantiateComponentsFromTemplate(t, node.components))
		logError("failed to instantiate components from template");
	
	// show what we just parsed
	
	dump_template(t);
}

#include "Path.h"
#include <set>

static bool loadTemplateWithOverlaysFromFile(const char * filename, Template & out_template, const bool allowAddingComponentsFromBase)
{
	auto directory = Path::GetDirectory(filename);
	
	std::set<std::string> processed;
	
	std::vector<Template> templates;
	
	std::string current_filename = filename;
	
	for (;;)
	{
		Template t;
		
		if (!loadTemplateFromFile(current_filename.c_str(), t))
			return false;
		
		processed.insert(current_filename);
		
		templates.emplace_back(std::move(t));
		
		auto & base = templates.back().base;
		
		if (base.empty())
			break;
		
		auto new_filename = directory + "/" + base;
		
		if (processed.count(new_filename) != 0)
		{
			logError("cyclic dependency found. %s references %s which is already processed",
				current_filename.c_str(),
				new_filename.c_str());
			return false;
		}
		
		current_filename = new_filename;
	}
	
	Assert(!templates.empty());
	
	for (auto template_itr = templates.rbegin(); template_itr != templates.rend(); ++template_itr)
	{
		if (template_itr == templates.rbegin())
		{
			out_template = *template_itr;
		}
		else
		{
			if (!overlayTemplate(out_template, *template_itr, allowAddingComponentsFromBase, true))
				return false;
		}
	}
	
	return true;
}

void test_templates()
{
	if (!framework.init(640, 480))
		return;

	registerComponentTypes();
	
	Template t;
	
	if (!loadTemplateWithOverlaysFromFile("textfiles/base-entity-v2-overlay.txt", t, false))
		logError("failed to load template with overlays from file");
	
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
