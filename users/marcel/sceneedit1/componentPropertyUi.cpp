#include "componentPropertyUi.h"
#include "componentType.h"
#include "helpers.h"
#include "imgui.h"
#include "StringEx.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

bool doComponentProperty(
	const Member & member,
	ComponentBase * component,
	const bool signalChanges,
	bool & isSet,
	ComponentBase * defaultComponent)
{
	if (member.isVector) // todo : add support for vector types
		return false;
	
	auto & member_scalar = static_cast<const Member_Scalar&>(member);
	
	auto * member_type = g_typeDB.findType(member_scalar.typeIndex);
	auto * member_object = member_scalar.scalar_access(component);
	
	auto * default_member_object =
		defaultComponent == nullptr
		? nullptr
		: member_scalar.scalar_access(defaultComponent);
	
	Assert(member_type != nullptr);
	if (member_type == nullptr)
		return false;
	
	if (member_type->isStructured) // todo : add support for structured types
		return false;
	
	auto & plain_type = static_cast<const PlainType&>(*member_type);
	
	if (doReflection_PlainType(member, plain_type, member_object, isSet, default_member_object))
	{
		if (signalChanges)
			component->propertyChanged(member_object);
		
		return true;
	}
	else
	{
		return false;
	}
}

bool doReflection_PlainType(
	const Member & member,
	const PlainType & plain_type,
	void * member_object,
	bool & isSet,
	void * default_member_object)
{
	bool result = false;
	
	ImGui::PushStyleColor(ImGuiCol_Text, isSet ? (ImU32)ImColor(255, 255, 255, 255) : (ImU32)ImColor(0, 255, 0, 255));
	
	switch (plain_type.dataType)
	{
	case kDataType_Bool:
		{
			auto & value = plain_type.access<bool>(member_object);
			
			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<bool>(default_member_object);
				}
			}
			
			if (ImGui::Checkbox(member.name, &value))
			{
				result = true;
			}
		}
		break;
	case kDataType_Int:
		{
			auto & value = plain_type.access<int>(member_object);
			
			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<int>(default_member_object);
				}
			}
			
			auto * limits = member.findFlag<ComponentMemberFlag_IntLimits>();
			
			if (limits != nullptr)
			{
				if (ImGui::SliderInt(member.name, &value, limits->min, limits->max))
				{
					result = true;
				}
			}
			else
			{
				if (ImGui::InputInt(member.name, &value))
				{
					result = true;
				}
			}
		}
		break;
	case kDataType_Float:
		{
			auto & value = plain_type.access<float>(member_object);
			
			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<float>(default_member_object);
				}
			}
			
			auto * limits = member.findFlag<ComponentMemberFlag_FloatLimits>();
			
			if (limits != nullptr)
			{
				auto * curveExponential = member.findFlag<ComponentMemberFlag_FloatEditorCurveExponential>();
				
				if (ImGui::SliderFloat(member.name, &value, limits->min, limits->max, "%.3f",
					curveExponential == nullptr ? 1.f : curveExponential->exponential))
				{
					result = true;
				}
			}
			else
			{
				if (ImGui::InputFloat(member.name, &value))
				{
					result = true;
				}
			}
		}
		break;
	case kDataType_Vec2:
		{
			auto & value = plain_type.access<Vec2>(member_object);
			
			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<Vec2>(default_member_object);
				}
			}
			
			if (ImGui::InputFloat2(member.name, &value[0]))
			{
				result = true;
			}
		}
		break;
	case kDataType_Vec3:
		{
			auto & value = plain_type.access<Vec3>(member_object);
			
			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<Vec3>(default_member_object);
				}
			}

			if (ImGui::InputFloat3(member.name, &value[0]))
			{
				result = true;
			}
		}
		break;
	case kDataType_Vec4:
		{
			auto & value = plain_type.access<Vec4>(member_object);

			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<Vec4>(default_member_object);
				}
			}
			
			if (ImGui::InputFloat4(member.name, &value[0]))
			{
				result = true;
			}
		}
		break;
	case kDataType_String:
		{
			auto & value = plain_type.access<std::string>(member_object);

			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<std::string>(default_member_object);
				}
			}
			
			char buffer[1024];
			strcpy_s(buffer, sizeof(buffer), value.c_str());
			
			if (ImGui::InputText(member.name, buffer, sizeof(buffer)))
			{
				value = buffer;
				
				result = true;
			}
		}
		break;
	case kDataType_Other:
		if (strcmp(plain_type.typeName, "AngleAxis") == 0) // todo : replace AngleAxis with structured type
		{
			auto & value = plain_type.access<AngleAxis>(member_object);
			
			if (isSet == false)
			{
				if (default_member_object != nullptr)
				{
					value = plain_type.access<AngleAxis>(default_member_object);
				}
			}
			
			if (ImGui::SliderAngle(member.name, &value.angle))
			{
				result = true;
			}
			
			ImGui::PushID(&value.axis);
			{
				if (ImGui::SliderFloat3(member.name, &value.axis[0], -1.f, +1.f))
				{
					result = true;
				}
			}
			ImGui::PopID();
		}
		break;
	}
	
	ImGui::PopStyleColor();
	
	if (result)
	{
		isSet = true;
	}
	
	return result;
}

static bool doReflectionMember_traverse(const TypeDB & typeDB, const Type & type, void * object, const Member * in_member)
{
	bool result = false;
	
	if (type.isStructured)
	{
		auto & structured_type = static_cast<const StructuredType&>(type);
		
		for (auto * member = structured_type.members_head; member != nullptr; member = member->next)
		{
			ImGui::PushID(member->name);
			{
				if (member->isVector)
				{
					auto * member_interface = static_cast<const Member_VectorInterface*>(member);
					
					const auto vector_size = member_interface->vector_size(object);
					auto * vector_type = typeDB.findType(member_interface->vector_type());
					
					Assert(vector_type != nullptr);
					if (vector_type != nullptr)
					{
						size_t insert_index = (size_t)-1;
						
						for (size_t i = 0; i < vector_size; ++i)
						{
							ImGui::PushID(i);
							{
								auto * vector_object = member_interface->vector_access(object, i);
								
								result |= doReflectionMember_traverse(typeDB, *vector_type, vector_object, member);
								
								if (ImGui::BeginPopupContextItem("Vector"))
								{
									if (i > 0 && ImGui::MenuItem("Move up"))
									{
										member_interface->vector_swap(object, i, i - 1);
									}
									
									if (i + 1 < vector_size && ImGui::MenuItem("Move down"))
									{
										member_interface->vector_swap(object, i, i + 1);
									}
										
									if (vector_size > 0 && ImGui::MenuItem("Insert before"))
									{
										insert_index = i;
									}
									
									if (ImGui::MenuItem(vector_size > 0 ? "Insert after" : "Insert"))
									{
										insert_index = i + 1;
									}
									
									ImGui::EndPopup();
								}
							}
							ImGui::PopID();
						}
						
						if (insert_index != (size_t)-1)
						{
							member_interface->vector_resize(object, vector_size + 1);
							member_interface->vector_swap(object, vector_size, insert_index);
						}
					}
				}
				else
				{
					auto * member_scalar = static_cast<const Member_Scalar*>(member);
					
					auto * member_type = g_typeDB.findType(member_scalar->typeIndex);
					auto * member_object = member_scalar->scalar_access(object);
					
					Assert(member_type != nullptr);
					if (member_type != nullptr)
					{
						result |= doReflectionMember_traverse(typeDB, *member_type, member_object, member);
					}
				}
			}
			ImGui::PopID();
		}
	}
	else
	{
		Assert(in_member != nullptr);
		
		auto & plain_type = static_cast<const PlainType&>(type);
		
		bool isSet = true;
		
		result |= doReflection_PlainType(*in_member, plain_type, object, isSet, nullptr);
	}
	
	return result;
}

bool doReflection_StructuredType(
	const TypeDB & typeDB,
	const StructuredType & type,
	void * object)
{
	return doReflectionMember_traverse(typeDB, type, object, nullptr);
}
