#include "componentType.h"
#include "imgui.h"
#include "StringEx.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

void doComponentProperty(
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
