#include "componentPropertyUi.h"
#include "componentType.h"
#include "imgui.h"
#include "StringEx.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

// todo : replace property editor in main.cpp with this one. this one is more advanced ..
// also see doComponentProperty_v2 below, which uses type info

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

//

#include "helpers.h"

void doComponentProperty_v2( // todo : restore support for limits
	const Member * member,
	ComponentBase * component,
	const bool signalChanges,
	bool & isSet,
	ComponentBase * defaultComponent)
{
	if (member->isVector) // todo : add support for vector types
		return;
	
	auto * member_scalar = static_cast<const Member_Scalar*>(member);
	
	auto * member_type = g_typeDB.findType(member_scalar->typeIndex);
	auto * member_object = member_scalar->scalar_access(component);
	
	if (member_type->isStructured) // todo : add support for structured types
		return;
	
	auto * plain_type = static_cast<const PlainType*>(member_type);
	
	ImGui::PushStyleColor(ImGuiCol_Text, isSet ? (ImU32)ImColor(255, 255, 255, 255) : (ImU32)ImColor(0, 255, 0, 255));
	
	switch (plain_type->dataType)
	{
	case kDataType_Bool:
		{
			auto & value = plain_type->access<bool>(member_object);
			
			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<bool>(default_member_object);
				}
			}
			
			if (ImGui::Checkbox(member->name, &value))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kDataType_Int:
		{
			auto & value = plain_type->access<int>(member_object);
			
			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<int>(default_member_object);
				}
			}
			
			/*
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
			*/
			{
				if (ImGui::InputInt(member->name, &value))
				{
					isSet = true;
					
					if (signalChanges)
						component->propertyChanged(&value);
				}
			}
		}
		break;
	case kDataType_Float:
		{
			auto & value = plain_type->access<float>(member_object);
			
			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<float>(default_member_object);
				}
			}
			
			/*
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
			*/
			{
				if (ImGui::InputFloat(member->name, &value))
				{
					isSet = true;
					
					if (signalChanges)
						component->propertyChanged(&value);
				}
			}
		}
		break;
	case kDataType_Vec2:
		{
			auto & value = plain_type->access<Vec2>(member_object);
			
			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<Vec2>(default_member_object);
				}
			}
			
			if (ImGui::InputFloat2(member->name, &value[0]))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kDataType_Vec3:
		{
			auto & value = plain_type->access<Vec3>(member_object);
			
			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<Vec3>(default_member_object);
				}
			}

			if (ImGui::InputFloat3(member->name, &value[0]))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kDataType_Vec4:
		{
			auto & value = plain_type->access<Vec4>(member_object);

			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<Vec4>(default_member_object);
				}
			}
			
			if (ImGui::InputFloat4(member->name, &value[0]))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kDataType_String:
		{
			auto & value = plain_type->access<std::string>(member_object);

			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<std::string>(default_member_object);
				}
			}
			
			char buffer[1024];
			strcpy_s(buffer, sizeof(buffer), value.c_str());
			
			if (ImGui::InputText(member->name, buffer, sizeof(buffer)))
			{
				value = buffer;
				
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
		}
		break;
	case kDataType_Other:
		if (strcmp(plain_type->typeName, "AngleAxis") == 0) // todo : replace AngleAxis with structured type
		{
			auto & value = plain_type->access<AngleAxis>(member_object);
			
			if (isSet == false)
			{
				if (defaultComponent != nullptr)
				{
					auto * default_member_object = member_scalar->scalar_access(defaultComponent);
					
					value = plain_type->access<AngleAxis>(default_member_object);
				}
			}
			
			if (ImGui::SliderAngle(member->name, &value.angle))
			{
				isSet = true;
				
				if (signalChanges)
					component->propertyChanged(&value);
			}
			
			ImGui::PushID(&value.axis);
			{
				if (ImGui::SliderFloat3(member->name, &value.axis[0], -1.f, +1.f))
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
