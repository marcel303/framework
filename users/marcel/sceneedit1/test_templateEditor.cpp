#include "componentType.h"
#include "framework.h"
#include "helpers.h"
#include "imgui.h"
#include "imgui-framework.h"
#include "Log.h"
#include "StringEx.h"
#include "template.h"

// todo : create a template instance, similar to how we create component and component property instances, to facilitate editing ?
// editing template would be very similar in fact to editing components. just that the properties live somewhere else
// todo : when editing, apply the latest version to the component instance
// todo : allow for live value viewing ?

// todo : avoid creating a component instance just to aid in editing templates

struct TemplateComponentInstance
{
	struct PropertyInfo
	{
		bool isSet = false;
	};
	
	std::vector<PropertyInfo> propertyInfos;
	
	ComponentBase * component = nullptr;
	
	ComponentTypeBase * componentType;
	
	bool init(ComponentTypeBase * in_componentType, const TemplateComponent & templateComponent)
	{
		bool result = true;
		
		componentType = in_componentType;
		
		propertyInfos.resize(componentType->properties.size());
		
		component = componentType->componentMgr->createComponent();
		
		//
		
		auto propertyInfo_itr = propertyInfos.begin();
		
		for (auto * componentProperty : componentType->properties)
		{
			auto & propertyInfo = *propertyInfo_itr;
			
			// see if there's a value for this property
			
			for (auto & templateProperty : templateComponent.properties)
			{
				if (templateProperty.name == componentProperty->name)
				{
					componentProperty->from_text(component, templateProperty.value.c_str());
					
					propertyInfo.isSet = true;
				}
			}
			
			propertyInfo_itr++;
		}
		
		return result;
	}
};

static void doComponentProperty(ComponentPropertyBase * propertyBase, ComponentBase * component)
{
	switch (propertyBase->type)
	{
	case kComponentPropertyType_Bool:
		{
			auto property = static_cast<ComponentPropertyBool*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (ImGui::Checkbox(property->name.c_str(), &value))
				component->propertyChanged(&value);
		}
		break;
	case kComponentPropertyType_Int32:
		{
			auto property = static_cast<ComponentPropertyInt*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (property->hasLimits)
			{
				if (ImGui::SliderInt(property->name.c_str(), &value, property->min, property->max))
					component->propertyChanged(&value);
			}
			else
			{
				if (ImGui::InputInt(property->name.c_str(), &value))
					component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_Float:
		{
			auto property = static_cast<ComponentPropertyFloat*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (property->hasLimits)
			{
				if (ImGui::SliderFloat(property->name.c_str(), &value, property->min, property->max, "%.3f", property->editingCurveExponential))
					component->propertyChanged(&value);
			}
			else
			{
				if (ImGui::InputFloat(property->name.c_str(), &value))
					component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_Vec2:
		{
			auto property = static_cast<ComponentPropertyVec2*>(propertyBase);
			
			auto & value = property->getter(component);

			if (ImGui::InputFloat2(property->name.c_str(), &value[0]))
				component->propertyChanged(&value);
		}
		break;
	case kComponentPropertyType_Vec3:
		{
			auto property = static_cast<ComponentPropertyVec3*>(propertyBase);
			
			auto & value = property->getter(component);

			if (ImGui::InputFloat3(property->name.c_str(), &value[0]))
				component->propertyChanged(&value);
		}
		break;
	case kComponentPropertyType_Vec4:
		{
			auto property = static_cast<ComponentPropertyVec4*>(propertyBase);
			
			auto & value = property->getter(component);

			if (ImGui::InputFloat4(property->name.c_str(), &value[0]))
				component->propertyChanged(&value);
		}
		break;
	case kComponentPropertyType_String:
		{
			auto property = static_cast<ComponentPropertyString*>(propertyBase);
			
			auto & value = property->getter(component);

			char buffer[1024];
			strcpy_s(buffer, sizeof(buffer), value.c_str());
			
			if (ImGui::InputText(property->name.c_str(), buffer, sizeof(buffer)))
			{
				property->setter(component, buffer);
				
				component->propertyChanged(&value);
			}
		}
		break;
	case kComponentPropertyType_AngleAxis:
		{
			auto property = static_cast<ComponentPropertyAngleAxis*>(propertyBase);
			
			auto & value = property->getter(component);
			
			if (ImGui::SliderAngle(property->name.c_str(), &value.angle))
				component->propertyChanged(&value);
			ImGui::PushID(&value.axis);
			if (ImGui::SliderFloat3(property->name.c_str(), &value.axis[0], -1.f, +1.f))
				component->propertyChanged(&value);
			ImGui::PopID();
		}
		break;
	}
}

void test_templateEditor()
{
	registerComponentTypes();
	
	// load template from file
	
// todo : load all of the overlays

	Template t;
	
	if (!loadTemplateFromFile("textfiles/base-entity-v1.txt", t))
	{
		LOG_ERR("failed to load template from file", 0);
		return;
	}
	
	// create template component instances for each component in the template
	
	std::vector<TemplateComponentInstance> template_component_instances;
	
	for (auto & template_component : t.components)
	{
		TemplateComponentInstance template_component_instance;
		
		ComponentTypeBase * component_type = findComponentType(template_component.type_name.c_str());
		
		if (component_type == nullptr)
		{
			LOG_ERR("failed to find component type: %s", template_component.type_name.c_str());
		}
		else if (!template_component_instance.init(component_type, template_component))
		{
			LOG_ERR("failed to initialize template component instance", 0);
		}
		else
		{
			template_component_instances.emplace_back(std::move(template_component_instance));
		}
	}
	
	framework.init(640, 480);
	
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
			if (ImGui::Begin("Components"))
			{
				for (auto & template_component_instance : template_component_instances)
				{
					ImGui::PushID(&template_component_instance);
					{
						ImGui::Text("%s", template_component_instance.componentType->typeName.c_str());
						
						for (auto & component_property : template_component_instance.componentType->properties)
						{
						// todo : show the default value when a property is set to default
						// todo : use a different color when a property is set to default
						
							doComponentProperty(component_property, template_component_instance.component);
							
							if (ImGui::BeginPopupContextItem(component_property->name.c_str()))
							{
								if (ImGui::MenuItem("Set to default"))
								{
									component_property->setToDefault(template_component_instance.component);
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
}
