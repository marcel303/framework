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

// todo : create a template instance, similar to how we create component and component property instances, to facilitate editing ?
// editing template would be very similar in fact to editing components. just that the properties live somewhere else
// todo : when editing, apply the latest version to the component instance
// todo : allow for live value viewing ?

// todo : avoid creating a component instance just to aid in editing templates

struct ComponentTypeWithId
{
	std::string type_name;
	std::string id;
	
	bool operator<(const ComponentTypeWithId & other) const
	{
		if (type_name != other.type_name)
			return type_name < other.type_name;
		else if (id != other.id)
			return id < other.id;
		else
			return false;
	}
};

struct TemplateComponentInstance
{
	struct PropertyInfo
	{
		bool isSet = false;
	};
	
	bool isSet = false;
	
	std::vector<PropertyInfo> property_infos;
	
	ComponentBase * component = nullptr;
	
	ComponentTypeBase * componentType = nullptr;
	
	std::string id;
	
	bool init(ComponentTypeBase * in_componentType, const char * in_id, const TemplateComponent * templateComponent)
	{
		bool result = true;
		
		componentType = in_componentType;
		
		property_infos.resize(componentType->properties.size());
		
		component = componentType->componentMgr->createComponent();
		
		id = in_id;
		
		// iterate over each property to see if it's set or not
		
		if (templateComponent != nullptr)
		{
			auto propertyInfo_itr = property_infos.begin();
			
			for (auto * componentProperty : componentType->properties)
			{
				auto & propertyInfo = *propertyInfo_itr;
				
				// see if there's a value for this property
				
				for (auto & templateProperty : templateComponent->properties)
				{
					if (templateProperty.name == componentProperty->name)
					{
						componentProperty->from_text(component, templateProperty.value.c_str());
						
						propertyInfo.isSet = true;
					}
				}
				
				propertyInfo_itr++;
			}
		}
		
		return result;
	}
};

struct TemplateInstance
{
	Template * t = nullptr;
	
	std::vector<TemplateComponentInstance> component_instances;
	
	bool init(Template & in_t, const std::set<ComponentTypeWithId> & componentTypesWithId)
	{
		t = &in_t;
		
		// create template component instances for each component
		
		for (auto & componentTypeWithId : componentTypesWithId)
		{
			// see if there's a template component for this component + id
			
			TemplateComponent * template_component = nullptr;
			
			for (auto & template_component_itr : t->components)
			{
				if (template_component_itr.type_name == componentTypeWithId.type_name &&
					template_component_itr.id == componentTypeWithId.id)
				{
					template_component = &template_component_itr;
				}
			}
			
			// create the instance
			
			TemplateComponentInstance component_instance;
			
			if (template_component != nullptr)
			{
				component_instance.isSet = true;
			}
			
			ComponentTypeBase * component_type = findComponentType(componentTypeWithId.type_name.c_str());
			
			if (component_type == nullptr)
			{
				LOG_ERR("failed to find component type: %s", template_component->type_name.c_str());
				return false;
			}
			else if (!component_instance.init(component_type, componentTypeWithId.id.c_str(), template_component))
			{
				LOG_ERR("failed to initialize template component instance", 0);
				return false;
			}
			else
			{
				component_instances.emplace_back(std::move(component_instance));
			}
		}
		
		return true;
	}
};

static void doComponentProperty(
	ComponentPropertyBase * propertyBase,
	ComponentBase * component,
	const bool signalChanges,
	bool & isSet,
	ComponentPropertyBase * defaultPropertyBase,
	ComponentBase * defaultComponent)
{
	ImGui::PushStyleColor(ImGuiCol_Text, isSet ? (ImU32)ImColor(255, 255, 255, 255) : (ImU32)ImColor(0, 255, 0, 255));
	
	switch (propertyBase->type)
	{
	case kComponentPropertyType_Bool:
		{
			auto property = static_cast<ComponentPropertyBool*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyBool*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}
			
			if (ImGui::Checkbox(property->name.c_str(), &value))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_Int32:
		{
			auto property = static_cast<ComponentPropertyInt*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyInt*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}
			
			if (property->hasLimits)
			{
				if (ImGui::SliderInt(property->name.c_str(), &value, property->min, property->max))
				{
					isSet = true;
					
					if (signalChanges)
						component->propertyChanged(&value);
				}
			}
			else
			{
				if (ImGui::InputInt(property->name.c_str(), &value))
				{
					isSet = true;
					
					if (signalChanges)
						component->propertyChanged(&value);
				}
			}
		}
		break;
	case kComponentPropertyType_Float:
		{
			auto property = static_cast<ComponentPropertyFloat*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyFloat*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}
			
			if (property->hasLimits)
			{
				if (ImGui::SliderFloat(property->name.c_str(), &value, property->min, property->max, "%.3f", property->editingCurveExponential))
				{
					isSet = true;
					
					if (signalChanges)
						component->propertyChanged(&value);
				}
			}
			else
			{
				if (ImGui::InputFloat(property->name.c_str(), &value))
				{
					isSet = true;
					
					if (signalChanges)
						component->propertyChanged(&value);
				}
			}
		}
		break;
	case kComponentPropertyType_Vec2:
		{
			auto property = static_cast<ComponentPropertyVec2*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyVec2*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}
			
			if (ImGui::InputFloat2(property->name.c_str(), &value[0]))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_Vec3:
		{
			auto property = static_cast<ComponentPropertyVec3*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyVec3*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}

			if (ImGui::InputFloat3(property->name.c_str(), &value[0]))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_Vec4:
		{
			auto property = static_cast<ComponentPropertyVec4*>(propertyBase);
			
			auto & value = property->getter(component);

			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyVec4*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}
			
			if (ImGui::InputFloat4(property->name.c_str(), &value[0]))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_String:
		{
			auto property = static_cast<ComponentPropertyString*>(propertyBase);
			
			auto & value = property->getter(component);

			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyString*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}
			
			char buffer[1024];
			strcpy_s(buffer, sizeof(buffer), value.c_str());
			
			if (ImGui::InputText(property->name.c_str(), buffer, sizeof(buffer)))
			{
				property->setter(component, buffer);
				
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_AngleAxis:
		{
			auto property = static_cast<ComponentPropertyAngleAxis*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (isSet == false)
			{
				if (defaultPropertyBase != nullptr)
				{
					auto defaultProperty = static_cast<ComponentPropertyAngleAxis*>(defaultPropertyBase);
					value = defaultProperty->getter(defaultComponent);
				}
			}
			
			if (ImGui::SliderAngle(property->name.c_str(), &value.angle))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
			
			ImGui::PushID(&value.axis);
			{
				if (ImGui::SliderFloat3(property->name.c_str(), &value.axis[0], -1.f, +1.f))
				{
					isSet = true;
					
					if (signalChanges)
						component->propertyChanged(&value);
				}
			}
			ImGui::PopID();
		}
		break;
	}
	
	ImGui::PopStyleColor();
}

bool test_templateEditor()
{
	registerComponentTypes();
	
	// load all of the template overlays from file

	std::set<std::string> processed;
	std::vector<Template> templates;
	
	std::string path = "textfiles/base-entity-v1-overlay.txt";
	
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
			elem.type_name = component.type_name;
			elem.id = component.id;
			
			if (componentTypesWithId.count(elem) == 0)
				componentTypesWithId.insert(elem);
		}
	}
	
	// create template instances
	
	std::vector<TemplateInstance> template_instances;
	
	for (auto & t : templates)
	{
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
			
			template_component.type_name = componentTypeWithId.type_name;
			template_component.id = componentTypeWithId.id;
			
			auto * componentType = findComponentType(componentTypeWithId.type_name.c_str());
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
		for (auto & component_instance : template_instance.component_instances)
		{
			for (auto & property_info : component_instance.property_infos)
				Assert(property_info.isSet);
		}
	#endif
		
		template_instances.emplace_back(std::move(template_instance));
	}
	
	if (!framework.init(640, 480))
		return false;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		
		guiContext.processBegin(framework.timeStep, 640, 480, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(10, 10));
			ImGui::SetNextWindowSize(ImVec2(400, 400));
			
			if (!template_instances.empty() && ImGui::Begin("Components"))
			{
				// determine and fetch the template we want to edit
				
				size_t template_itr = 0;
				
				auto & template_instance = template_instances[template_itr];
				
				// iterate over all of its components
				
				for (size_t component_itr = 0; component_itr < template_instance.component_instances.size(); ++component_itr)
				{
					auto & component_instance = template_instance.component_instances[component_itr];
					
					ImGui::PushID(&component_instance);
					{
						ImGui::Text("%s", component_instance.componentType->typeName.c_str());
						
						// iterate over all of the components' properties
						
						for (size_t property_itr = 0; property_itr < component_instance.componentType->properties.size(); ++property_itr)
						{
						// todo : show the default value when a property is set to default
						// todo : use a different color when a property is set to default
						
							auto & property = component_instance.componentType->properties[property_itr];
							
							auto & property_info = component_instance.property_infos[property_itr];
							
							ComponentBase * component_with_value = nullptr;
							ComponentPropertyBase * property_with_value = nullptr;
							
							for (size_t i = template_itr; i < template_instances.size(); ++i)
							{
								if (template_instances[i].component_instances[component_itr].property_infos[property_itr].isSet)
								{
									component_with_value = template_instances[i].component_instances[component_itr].component;
									property_with_value = template_instances[i].component_instances[component_itr].componentType->properties[property_itr];
								}
							}
							
							// there should always with a property with value, as we create a default template instance before
							Assert(property_with_value != nullptr);
							
							doComponentProperty(property, component_instance.component, false, property_info.isSet, property_with_value, component_with_value);
							
							if (ImGui::BeginPopupContextItem(property->name.c_str()))
							{
								if (ImGui::MenuItem("Set to default"))
								{
									property_info.isSet = false;
								}
								
								ImGui::EndPopup();
							}
						}
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
	
	guiContext.shut();
	
	framework.shutdown();
	
	return true;
}
