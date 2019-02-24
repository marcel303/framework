#include "componentPropertyUi.h"
#include "componentType.h"
#include "framework.h"
#include "helpers.h"
#include "imgui.h"
#include "imgui-framework.h"
#include "Log.h"
#include "Path.h"
#include "StringEx.h"
#include "template.h"
#include <set>
#include <sstream>

// todo : when editing, apply the latest version to the component instance
// todo : allow for live value viewing ?

// todo : when saving we will want to distinguish between templates which define (new) components, and those that merely override component properties. perhaps have new component definitions have a plus sign (+transform) to signify it's a new definition ? or maybe just skip saving if a component with the same type name + id exists in a base template and the list of property override is empty ?

// todo : avoid creating a component instance just to aid in editing templates

#define VIEW_SX 1200
#define VIEW_SY 480

struct ComponentTypeWithId
{
	std::string typeName;
	std::string id;
	
	bool operator<(const ComponentTypeWithId & other) const
	{
		if (typeName != other.typeName)
			return typeName < other.typeName;
		else if (id != other.id)
			return id < other.id;
		else
			return false;
	}
};

struct TemplateComponentInstance
{
	bool isOverride = false;
	
	ComponentTypeBase * componentType = nullptr;
	ComponentBase * component = nullptr;
	std::string id;
	
	std::vector<bool> propertyIsSetArray;
	
	~TemplateComponentInstance()
	{
		Assert(component == nullptr);
	}
	
	bool init(ComponentTypeBase * in_componentType, const char * in_id, const TemplateComponent * templateComponent)
	{
		bool result = true;
		
		componentType = in_componentType;
		component = componentType->componentMgr->createComponent();
		id = in_id;
		
		// iterate over each property to see if it's set or not
		
		propertyIsSetArray.resize(componentType->properties.size());
		
		if (templateComponent != nullptr)
		{
			auto propertyIsSet_itr = propertyIsSetArray.begin();
			
			for (auto * componentProperty : componentType->properties)
			{
				// see if a value is set for this property. if so, remember this fact so the editor knows to shows the right value
				
				bool propertyIsSet = false;
				
				for (auto & templateProperty : templateComponent->properties)
				{
					if (templateProperty.name == componentProperty->name)
					{
						componentProperty->from_text(component, templateProperty.value.c_str());
						
						propertyIsSet = true;
						
						break;
					}
				}
				
				*propertyIsSet_itr++ = propertyIsSet;
			}
		}
		
		return result;
	}
	
	void shut()
	{
		if (component != nullptr)
		{
			componentType->componentMgr->removeComponent(component);
			component = nullptr;
		}
	}
};

struct TemplateInstance
{
	std::vector<TemplateComponentInstance> components;
	
	TemplateComponentInstance * findComponentInstance(const char * typeName, const char * id)
	{
		for (auto & component : components)
		{
			if (component.componentType->typeName == typeName &&
				component.id == id)
			{
				return &component;
			}
		}
		
		return nullptr;
	}
	
	bool init(Template & t, const std::set<ComponentTypeWithId> & componentTypesWithId)
	{
		// create template component instances for each component
		
		components.resize(componentTypesWithId.size());
		
		int componentIndex = 0;
		
		for (auto & componentTypeWithId : componentTypesWithId)
		{
			// see if there's a template component for this component + id
			
			TemplateComponent * templateComponent = nullptr;
			
			for (auto & templateComponent_itr : t.components)
			{
				if (templateComponent_itr.type_name == componentTypeWithId.typeName &&
					templateComponent_itr.id == componentTypeWithId.id)
				{
					templateComponent = &templateComponent_itr;
				}
			}
			
			// initialize the component instance
			
			TemplateComponentInstance & component = components[componentIndex++];
			
			if (templateComponent == nullptr)
			{
				component.isOverride = true;
			}
			
			ComponentTypeBase * componentType = findComponentType(componentTypeWithId.typeName.c_str());
			
			if (componentType == nullptr)
			{
				LOG_ERR("failed to find component type: %s", templateComponent->type_name.c_str());
				return false;
			}
			else if (!component.init(componentType, componentTypeWithId.id.c_str(), templateComponent))
			{
				LOG_ERR("failed to initialize template component instance", 0);
				return false;
			}
		}
		
		return true;
	}
	
	void shut()
	{
		for (auto & component : components)
			component.shut();
	}
};

bool saveTemplateInstanceToString(const std::vector<TemplateInstance> & instances, const size_t instanceIndex, std::string & out_text)
{
	auto & instance = instances[instanceIndex];
	
	std::ostringstream out;
	
	for (size_t component_itr = 0; component_itr < instance.components.size(); ++component_itr)
	{
		auto & component = instance.components[component_itr];
		
		bool anyPropertyIsSet = false;
		
		for (auto propertyIsSet : component.propertyIsSetArray)
			anyPropertyIsSet |= propertyIsSet;
		
		if (component.isOverride == false || anyPropertyIsSet)
		{
			if (component.isOverride)
				out << "-";
			out << component.componentType->typeName;
			if (component.id.empty() == false)
				out << " " << component.id;
			out << "\n";
			
			for (size_t property_itr = 0; property_itr < component.propertyIsSetArray.size(); ++property_itr)
			{
				if (component.propertyIsSetArray[property_itr])
				{
					std::string value;
					
					auto & property = component.componentType->properties[property_itr];
					
					property->to_text(component.component, value);
					
					out << "\t" << property->name << "\n";
					out << "\t\t" << value << "\n";
				}
			}
		}
	}
	
	out_text = out.str();
	
	return true;
}

bool saveTemplateInstanceToFile(const std::vector<TemplateInstance> & instances, const size_t instanceIndex, const char * filename)
{
	std::string content;
	
	if (!saveTemplateInstanceToString(instances, instanceIndex, content))
		return false;
	
	if (!content.empty())
	{
		FILE * file = fopen(filename, "wt");
		
		if (file == nullptr)
		{
			logError("failed to open output file. filename=%s", filename);
			return false;
		}
		else
		{
			fprintf(file, "%s", content.c_str());
			
			fclose(file);
			file = nullptr;
		}
	}
	
	return true;
}

bool test_templateEditor()
{
	registerComponentTypes();
	
	// load all of the template overlays from file

	std::set<std::string> processed;
	std::vector<Template> templates;
	
	std::string path = "textfiles/base-entity-v3-d.txt";
	
	std::string directory = Path::GetDirectory(path);
	std::string filename = Path::GetFileName(path);
	
	std::string current_filename = directory + "/" + filename;
	
	for (;;)
	{
		Template t;
		
		if (!loadTemplateFromFile(current_filename.c_str(), t))
		{
			LOG_ERR("failed to load template from file", 0);
			return false;
		}
		
		processed.insert(current_filename);
		templates.emplace_back(t);
		
		if (t.base.empty())
			break;
		
		std::string new_filename = directory + "/" + t.base;
		
		if (processed.count(new_filename) != 0)
		{
			LOG_ERR("cyclic dependency detected", 0);
			return false;
		}
		
		current_filename = new_filename;
	}
	
	// generate a set of all referenced component types + their ids. this set will be used to populate template instances later on
	
	std::set<ComponentTypeWithId> componentTypesWithId;
	
	for (auto & t : templates)
	{
		for (auto & component : t.components)
		{
			ComponentTypeWithId elem;
			elem.typeName = component.type_name;
			elem.id = component.id;
			
			if (componentTypesWithId.count(elem) == 0)
				componentTypesWithId.insert(elem);
		}
	}
	
	// create template instances
	
	std::vector<TemplateInstance> template_instances;
	
	for (size_t i = 0; i < templates.size(); ++i)
	{
		auto & t = templates[i];
		
		TemplateInstance template_instance;
		
		if (!template_instance.init(t, componentTypesWithId))
		{
			LOG_ERR("failed to initialize template instance", 0);
			return false;
		}
		
		template_instances.emplace_back(std::move(template_instance));
	}
	
	// create a fallback template instance, with default values for all component properties
	// note : default values are determined by instantiating compontent types and inspecting their initial property values
	
	{
		Template fallback_template;
		
		for (auto & componentTypeWithId : componentTypesWithId)
		{
			TemplateComponent template_component;
			
			template_component.type_name = componentTypeWithId.typeName;
			template_component.id = componentTypeWithId.id;
			
			auto * componentType = findComponentType(componentTypeWithId.typeName.c_str());
			auto * component = componentType->componentMgr->createComponent();
			
			for (auto & property : componentType->properties)
			{
				TemplateComponentProperty template_property;
				
				template_property.name = property->name;
				property->to_text(component, template_property.value);
				
				template_component.properties.push_back(template_property);
			}
			
			componentType->componentMgr->removeComponent(component);
			component = nullptr;
			
			fallback_template.components.emplace_back(std::move(template_component));
		}
		
		TemplateInstance template_instance;
		
		if (!template_instance.init(fallback_template, componentTypesWithId))
		{
			LOG_ERR("failed to initialize (fallback) template instance", 0);
			return false;
		}
		
	#if defined(DEBUG)
		for (auto & component : template_instance.components)
		{
			for (bool propertyIsSet : component.propertyIsSetArray)
				Assert(propertyIsSet);
		}
	#endif
		
		template_instances.emplace_back(std::move(template_instance));
	}
	
#if defined(DEBUG)
	for (size_t i = 1; i < template_instances.size(); ++i)
	{
		auto & a = template_instances[0];
		auto & b = template_instances[i];
		
		Assert(a.components.size() == b.components.size());
		
		for (size_t j = 0; j < a.components.size(); ++j)
		{
			auto & a_comp = a.components[j];
			auto & b_comp = b.components[j];
			
			Assert(a_comp.componentType->typeName == b_comp.componentType->typeName);
			Assert(a_comp.id == b_comp.id);
			
			Assert(a_comp.propertyIsSetArray.size() == b_comp.propertyIsSetArray.size());
		}
	}
#endif
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return false;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	int selectedTemplateIndex = 0;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		
		guiContext.processBegin(framework.timeStep, VIEW_SX, VIEW_SY, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(10, 10));
			ImGui::SetNextWindowSize(ImVec2(400, 400));
			
			if (ImGui::Begin("Components"))
			{
				ImGui::SliderInt("Template level", &selectedTemplateIndex, 0, (int)template_instances.size() - 2);
				
				if (selectedTemplateIndex >= 0 && selectedTemplateIndex < template_instances.size())
				{
					const bool isFallbackTemplate = (selectedTemplateIndex == template_instances.size() - 1);
					
					// determine and fetch the template we want to edit
					
					auto & template_instance = template_instances[selectedTemplateIndex];
					
					// iterate over all of its components
					
					for (auto & component_instance : template_instance.components)
					{
						ImGui::PushID(&component_instance);
						{
							ImGui::Text("%s", component_instance.componentType->typeName.c_str());
							
							char id[64];
							strcpy_s(id, sizeof(id), component_instance.id.c_str());
							if (ImGui::InputText("Id", id, sizeof(id)))
							{
								// patch id for all template instances
								for (auto & instance : template_instances)
								{
									auto * component = instance.findComponentInstance(component_instance.componentType->typeName.c_str(), component_instance.id.c_str());
									
									component->id = id;
								}
							}
							
							// iterate over all of the components' properties
							
							for (size_t property_itr = 0; property_itr < component_instance.componentType->properties.size(); ++property_itr)
							{
								auto & property = component_instance.componentType->properties[property_itr];
								
								ComponentBase * component_with_value = nullptr;
								ComponentPropertyBase * property_with_value = nullptr;
								
								for (size_t i = selectedTemplateIndex; i < template_instances.size(); ++i)
								{
									auto * base_component = template_instances[i].findComponentInstance(component_instance.componentType->typeName.c_str(), component_instance.id.c_str());
									Assert(base_component != nullptr);
									
									if (base_component->propertyIsSetArray[property_itr])
									{
										component_with_value = base_component->component;
										property_with_value = base_component->componentType->properties[property_itr];
										break;
									}
								}
								
								// there should always with a property with value, as we create a default template instance before
								Assert(property_with_value != nullptr);
								
								bool propertyIsSet = component_instance.propertyIsSetArray[property_itr]; // argh frck c++ with its bit array..
								
								doComponentProperty(property, component_instance.component, false, propertyIsSet, property_with_value, component_with_value);
								
								if (ImGui::BeginPopupContextItem(property->name.c_str()))
								{
									if (isFallbackTemplate == false)
									{
										if (ImGui::MenuItem("Set to default"))
										{
											propertyIsSet = false;
										}
									}
									
									ImGui::EndPopup();
								}
								
								component_instance.propertyIsSetArray[property_itr] = propertyIsSet;
							}
						}
						ImGui::PopID();
					}
				}
			}
			ImGui::End();
			
			ImGui::SetNextWindowPos(ImVec2(440, 10));
			ImGui::SetNextWindowSize(ImVec2(600, 400));
			if (ImGui::Begin("Out"))
			{
				for (size_t instance_itr = 0; instance_itr < template_instances.size() - 1; ++instance_itr)
				{
					ImGui::PushID(instance_itr);
					{
						std::string text;
						
						if (!saveTemplateInstanceToString(template_instances, instance_itr, text))
						{
							logError("failed to save template instance");
							return false;
						}
						
						ImGui::InputTextMultiline("Text", (char*)text.c_str(), text.size());
					}
					ImGui::PopID();
				}
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	
	//
	
	int out_index = 0;
	
	for (size_t instance_itr = 0; instance_itr < template_instances.size() - 1; ++instance_itr)
	{
		char filename[64];
		sprintf_s(filename, sizeof(filename), "out/%03d.txt", out_index);
		out_index++;
		
		if (!saveTemplateInstanceToFile(template_instances, instance_itr, filename))
		{
			logError("failed to save template instance");
			return false;
		}
	}
	
	for (auto & instance : template_instances)
	{
		instance.shut();
	}
	
	template_instances.clear();
	
	guiContext.shut();
	
	framework.shutdown();
	
	return true;
}
