// sceneedit
#include "helpers2.h"

// ecs-sceneEditor
#include "editor/componentPropertyUi.h"

// ecs-scene
#include "helpers.h"
#include "template.h"
#include "templateIo.h"

// ecs-component
#include "componentType.h"

// reflection-textio
#include "lineReader.h"
#include "lineWriter.h"
#include "reflection-textio.h"

// imgui-framework
#include "imgui.h"
#include "imgui-framework.h"

// framework
#include "framework.h"

// libgg
#include "Log.h"
#include "Path.h"
#include "StringEx.h" // strcpy_s

// std
#include <list>
#include <set>
#include <sstream> // ostringstream

// todo : when editing, apply the latest version to the component instance
// todo : allow for live value viewing ?

// note : when saving we will want to distinguish between templates which define (new) components, and those that merely override component properties. component definitions that are merely there because of component property overrides have a minus sign (-transform) to signify it's not a new definition

// todo : avoid creating a component instance just to aid in editing templates

#define VIEW_SX 1200
#define VIEW_SY 480

static bool doComponentTypeMenu(std::string & out_typeName)
{
	bool result = false;
	
	// sort the component types by name by putting them in a set
	
	std::set<std::string> componentTypeNames;
	
	for (auto * componentType : g_componentTypes)
		componentTypeNames.insert(componentType->typeName);
	
	for (auto & typeName : componentTypeNames)
	{
		if (ImGui::MenuItem(typeName.c_str()))
		{
			result = true;
			
			out_typeName = typeName;
		}
	}
	
	return result;
}

static void createFallbackTemplateForComponent(const TypeDB & typeDB, const char * componentTypeName, const char * componentId, TemplateComponent & template_component)
{
	template_component.typeName = componentTypeName;
	template_component.id = componentId;
	
	int componentSetId = allocComponentSetId();
	
	auto * componentType = findComponentType(componentTypeName);
	auto * component = componentType->componentMgr->createComponent(componentSetId);
	
	for (auto * member = componentType->members_head; member != nullptr; member = member->next)
	{
		TemplateComponentProperty template_property;
		
		template_property.name = member->name;
		
		LineWriter line_writer;
		if (!member_tolines_recursive(typeDB, member, component, line_writer, 0))
		{
		// fixme : this may trigger an error. let createFallbackTemplateForComponent return false in this case
			LOG_ERR("failed to serialize component property to text", 0);
			continue;
		}
		
		template_property.value_lines = line_writer.to_lines(); // todo : optimize
		
		template_component.properties.push_back(template_property);
	}
	
	componentType->componentMgr->destroyComponent(componentSetId);
	
	freeComponentSetId(componentSetId);
}

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
	
	bool init(const TypeDB & typeDB, ComponentTypeBase * in_componentType, const char * in_id, const TemplateComponent * templateComponent, const int componentSetId)
	{
		bool result = true;
		
		componentType = in_componentType;
		component = componentType->componentMgr->createComponent(componentSetId);
		id = in_id;
		
		// iterate over each property to see if it's set or not
		
		int numProperties = 0;
		
		for (auto * member = componentType->members_head; member != nullptr; member = member->next)
			numProperties++;
		
		propertyIsSetArray.resize(numProperties);
		
		if (templateComponent != nullptr)
		{
			auto propertyIsSet_itr = propertyIsSetArray.begin();
			
			for (auto * member = componentType->members_head; member != nullptr; member = member->next)
			{
				// see if a value is set for this property. if so, remember this fact so the editor knows to shows the right value
				
				bool propertyIsSet = false;
				
				for (auto & templateProperty : templateComponent->properties)
				{
					if (templateProperty.name == member->name)
					{
						LineReader line_reader(templateProperty.value_lines, 0, 0);
						
						result &= member_fromlines_recursive(typeDB, member, component, line_reader);
						
						propertyIsSet = true;
						
						break;
					}
				}
				
				*propertyIsSet_itr++ = propertyIsSet;
			}
		}
		
		return result;
	}
	
	void shut(const int componentSetId)
	{
		if (component != nullptr)
		{
			componentType->componentMgr->destroyComponent(componentSetId);
			component = nullptr;
		}
		
		componentType = nullptr;
		
		id.clear();
		
		propertyIsSetArray.clear();
	}
};

struct TemplateInstance
{
	std::string name;
	std::string base;
	
	std::list<TemplateComponentInstance> components;
	
	int componentSetId = kComponentSetIdInvalid;
	
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
	
	bool init(const TypeDB & typeDB, Template & t, const std::set<ComponentTypeWithId> & componentTypesWithId)
	{
		name = t.name;
		base = t.base;
		
		Assert(componentSetId == kComponentSetIdInvalid);
		componentSetId = allocComponentSetId();
		
		// create template component instances for each component
		
		components.resize(componentTypesWithId.size());
		
		auto component_itr = components.begin();
		
		for (auto & componentTypeWithId : componentTypesWithId)
		{
			// see if there's a template component for this component + id
			
			TemplateComponent * templateComponent = nullptr;
			
			for (auto & templateComponent_itr : t.components)
			{
				if (templateComponent_itr.typeName == componentTypeWithId.typeName &&
					templateComponent_itr.id == componentTypeWithId.id)
				{
					templateComponent = &templateComponent_itr;
				}
			}
			
			// initialize the component instance
			
			TemplateComponentInstance & component = *component_itr++;
			
			if (templateComponent == nullptr)
			{
				component.isOverride = true;
			}
			
			ComponentTypeBase * componentType = findComponentType(componentTypeWithId.typeName.c_str());
			
			if (componentType == nullptr)
			{
				LOG_ERR("failed to find component type: %s", templateComponent->typeName.c_str());
				return false;
			}
			else if (!component.init(typeDB, componentType, componentTypeWithId.id.c_str(), templateComponent, componentSetId))
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
			component.shut(componentSetId);
		
		freeComponentSetId(componentSetId);
		Assert(componentSetId == kComponentSetIdInvalid);
	}
	
	bool addComponentByTypeName(const TypeDB & typeDB, const char * typeName, const bool isOverride, const bool isFallback)
	{
		// add component instance
		
		components.resize(components.size() + 1);
		
		// initialize the component instance
		
		TemplateComponentInstance & component = components.back();
	
		component.isOverride = isOverride;
	
		ComponentTypeBase * componentType = findComponentType(typeName);
	
		TemplateComponent template_component;
		
		if (isFallback)
		{
			createFallbackTemplateForComponent(typeDB, typeName, "", template_component);
		}
		
		if (componentType == nullptr)
		{
			LOG_ERR("failed to find component type: %s", typeName);
			components.pop_back();
			return false;
		}
		else if (!component.init(typeDB, componentType, "", &template_component, componentSetId))
		{
			LOG_ERR("failed to initialize template component instance", 0);
			components.pop_back();
			return false;
		}
		
		return true;
	}
};

bool saveTemplateInstanceToString(const TypeDB & typeDB, const std::vector<TemplateInstance> & instances, const size_t instanceIndex, std::string & out_text)
{
	bool result = true;
	
	auto & instance = instances[instanceIndex];
	
	std::ostringstream out;
	
	if (instance.base.empty() == false)
	{
		out << "base " << instance.base << "\n";
	}
	
	for (auto & component : instance.components)
	{
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
			
			Member * member = component.componentType->members_head;
			
			for (size_t property_itr = 0; property_itr < component.propertyIsSetArray.size(); ++property_itr, member = member->next)
			{
				if (component.propertyIsSetArray[property_itr])
				{
					LineWriter line_writer;
					result &= member_tolines_recursive(typeDB, member, component.component, line_writer, 0);
					
					std::vector<std::string> lines = line_writer.to_lines();
					
					out << "\t" << member->name << "\n";
					for (auto & line : lines)
						out << "\t\t" << line << "\n";
				}
			}
		}
	}
	
	out_text = out.str();
	
	return result;
}

bool saveTemplateInstanceToFile(const TypeDB & typeDB, const std::vector<TemplateInstance> & instances, const size_t instanceIndex, const char * filename)
{
	std::string content;
	
	if (!saveTemplateInstanceToString(typeDB, instances, instanceIndex, content))
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

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	auto & typeDB = g_typeDB;
	
	registerBuiltinTypes(typeDB);
	registerComponentTypes(typeDB);
	
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
		
		if (!parseTemplateFromFile(current_filename.c_str(), t))
		{
			LOG_ERR("failed to load template from file", 0);
			return -1;
		}
		
		processed.insert(current_filename);
		templates.emplace_back(t);
		
		if (t.base.empty())
			break;
		
		std::string new_filename = directory + "/" + t.base;
		
		if (processed.count(new_filename) != 0)
		{
			LOG_ERR("cyclic dependency detected", 0);
			return -1;
		}
		
		current_filename = new_filename;
	}

	// generate a set of all referenced component types + their ids. this set will be used to populate the fallback template

	std::set<ComponentTypeWithId> allComponentTypesWithId;

	for (auto & t : templates)
	{
		for (auto & component : t.components)
		{
			ComponentTypeWithId elem;
			elem.typeName = component.typeName;
			elem.id = component.id;
			
			if (allComponentTypesWithId.count(elem) == 0)
				allComponentTypesWithId.insert(elem);
		}
	}
	
	// create a fallback template, with default values for all component properties
	// note : default values are determined by instantiating compontent types and inspecting their initial property values
	
	{
		// create the fallback template
		
		Template fallback_template;
		
		for (auto & componentTypeWithId : allComponentTypesWithId)
		{
			TemplateComponent template_component;
			
			createFallbackTemplateForComponent(
				typeDB,
				componentTypeWithId.typeName.c_str(),
				componentTypeWithId.id.c_str(),
				template_component);
			
			fallback_template.components.emplace_back(std::move(template_component));
		}
		
		templates.emplace_back(std::move(fallback_template));
	}
	
	// create template instances
	// note : this operation will reverse the array. !! the fallback template will be at index zero !!
	
	std::vector<TemplateInstance> template_instances;
	template_instances.resize(templates.size());
	
	std::set<ComponentTypeWithId> componentTypesWithId;
	
	auto template_instance_itr = template_instances.begin();
	
	for (auto template_itr = templates.rbegin(); template_itr != templates.rend(); ++template_itr)
	{
		auto & t = *template_itr;
		
		const bool isFallbackTemplate = (template_itr == templates.rbegin());
		
		if (isFallbackTemplate)
		{
			auto & template_instance = *template_instance_itr++;
			
			if (!template_instance.init(typeDB, t, allComponentTypesWithId))
			{
				LOG_ERR("failed to initialize (fallback) template instance", 0);
				return -1;
			}
		}
		else
		{
			// add the components from this template to the set of all components encountered so far
			
			for (auto & component : t.components)
			{
				ComponentTypeWithId elem;
				elem.typeName = component.typeName;
				elem.id = component.id;
				
				if (componentTypesWithId.count(elem) == 0)
					componentTypesWithId.insert(elem);
			}
			
			auto & template_instance = *template_instance_itr++;
			
			if (!template_instance.init(typeDB, t, componentTypesWithId))
			{
				LOG_ERR("failed to initialize template instance", 0);
				return -1;
			}
		}
	}
	
	//
	
	framework.windowIsResizable = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	int selectedTemplateIndex = (int)template_instances.size() - 1;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		
		int viewSx;
		int viewSy;
		framework.getCurrentViewportSize(viewSx, viewSy);
		
		guiContext.processBegin(framework.timeStep, viewSx, viewSy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(10, 10));
			ImGui::SetNextWindowSize(ImVec2(400, 400));
			
			if (ImGui::Begin("Components"))
			{
				ImGui::SliderInt("Template level", &selectedTemplateIndex, 1, (int)template_instances.size() - 1);
				
				if (selectedTemplateIndex >= 1 && selectedTemplateIndex < template_instances.size())
				{
					// determine and fetch the template we want to edit
					
					auto & template_instance = template_instances[selectedTemplateIndex];
					
					// show its name
					
					ImGui::Text("\t%s", template_instance.name.c_str());
					ImGui::Separator();
					
					// iterate over all of its components
					
					for (auto & component_instance : template_instance.components)
					{
						ImGui::PushID(&component_instance);
						{
							ImGui::Text("%s", component_instance.componentType->typeName);
							
							if (ImGui::BeginPopupContextItem("Component"))
							{
								ImGui::MenuItem("Is override", nullptr, &component_instance.isOverride);
								
								if (ImGui::BeginMenu("Add component.."))
								{
									std::string typeName;
									
									if (doComponentTypeMenu(typeName))
									{
										// code below will break if selectedTemplateIndex is zero
										Assert(selectedTemplateIndex != 0);
										
										for (int i = selectedTemplateIndex; i < template_instances.size(); ++i)
										{
											auto & template_instance = template_instances[i];
											
											if (!template_instance.addComponentByTypeName(typeDB, typeName.c_str(), i != selectedTemplateIndex, false))
											{
												LOG_ERR("failed to add component to template instance", 0);
											}
										}
										
										// add fallback template instance
										
										auto & template_instance = template_instances[0];
										
										if (!template_instance.addComponentByTypeName(typeDB, typeName.c_str(), true, true))
										{
											LOG_ERR("failed to add component to template instance", 0);
										}
									}
									
									ImGui::EndMenu();
								}
								
								char text[64];
								sprintf_s(text, sizeof(text), "Remove %s", component_instance.componentType->typeName);
								if (ImGui::MenuItem(text))
								{
									// todo
								}
								
								ImGui::EndPopup();
							}
							
							char id[64];
							strcpy_s(id, sizeof(id), component_instance.id.c_str());
							if (ImGui::InputText("Id", id, sizeof(id)))
							{
								// patch id for all template instances
								// note : make a copy of the id since it will possibly be modified in the loop below
								
								auto old_id = component_instance.id;
								
								component_instance.id = id;
								
								//for (size_t template_itr = selectedTemplateIndex + 1; template_itr < template_instances.size(); ++template_itr)
								for (size_t template_itr = 0; template_itr < template_instances.size(); ++template_itr)
								{
									auto & instance = template_instances[template_itr];
									
									// make sure we skip ourselves. it may be possible we added a few components of
									// type X, and now we start renaming them. in this case, we don't want to rename
									// all of them at the same time; it would be impossible to give them unique names!
									//Assert(&instance != &template_instance);
									if (&instance == &template_instance)
										continue;
									
									auto * component = instance.findComponentInstance(component_instance.componentType->typeName, old_id.c_str());
									
									if (component != nullptr)
									{
										component->id = id;
									}
								}
							}
							
						#if 0
						// todo : remove Reflection_StructuredType test code
							{
								bool isSet = true;
								auto * base_component = template_instances[0].findComponentInstance(component_instance.componentType->typeName, component_instance.id.c_str());
								
								ImGui::Reflection_StructuredType(
									typeDB,
									*component_instance.componentType,
									component_instance.component,
									isSet,
									base_component,
									nullptr);
							}
						#else
							// iterate over all of the components' properties
							
							size_t property_itr = 0;
							
							for (auto * member = component_instance.componentType->members_head; member != nullptr; member = member->next, ++property_itr)
							{
								ComponentBase * component_with_value = nullptr;
								
								for (int i = selectedTemplateIndex; i >= 0; --i)
								{
									auto * base_component = template_instances[i].findComponentInstance(component_instance.componentType->typeName, component_instance.id.c_str());
									
									if (base_component != nullptr && base_component->propertyIsSetArray[property_itr])
									{
										component_with_value = base_component->component;
										break;
									}
								}
								
								// there should always be a component with a value, as we created a default template instance before
								Assert(component_with_value != nullptr);
								
								bool propertyIsSet = component_instance.propertyIsSetArray[property_itr]; // argh frck c++ with its bit array..
								
								ImGui::ComponentProperty(typeDB, *member, component_instance.component, false, propertyIsSet, component_with_value);
								
								component_instance.propertyIsSetArray[property_itr] = propertyIsSet;
							}
						#endif
						}
						ImGui::PopID();
					}
				}
			}
			ImGui::End();
			
			ImGui::SetNextWindowPos(ImVec2(440, 10));
			ImGui::SetNextWindowSize(ImVec2(700, 400));
			if (ImGui::Begin("Out"))
			{
				for (int instance_itr = (int)template_instances.size() - 1; instance_itr > 0; --instance_itr)
				{
					ImGui::PushID(instance_itr);
					{
						std::string text;
						
						if (!saveTemplateInstanceToString(typeDB, template_instances, instance_itr, text))
						{
							logError("failed to save template instance");
							return -1;
						}
						
						ImGui::InputTextMultiline(
							template_instances[instance_itr].name.c_str(),
							(char*)text.c_str(), text.size());
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
	
	for (size_t instance_itr = 1; instance_itr < template_instances.size(); ++instance_itr)
	{
		char filename[64];
		sprintf_s(filename, sizeof(filename), "out/%03d.txt", out_index);
		out_index++;
		
		if (!saveTemplateInstanceToFile(typeDB, template_instances, instance_itr, filename))
		{
			logError("failed to save template instance");
			return -1;
		}
	}
	
	for (auto & instance : template_instances)
	{
		instance.shut();
	}
	
	template_instances.clear();
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
