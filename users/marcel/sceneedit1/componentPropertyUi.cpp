#include "componentPropertyUi.h"
#include "componentType.h"
#include "helpers.h"
#include "imgui.h"
#include "StringEx.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

bool doComponentProperty(
	const TypeDB & typeDB,
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
	
	bool result = false;
	
	bool treeNodeIsOpen = false;
	
	if (member_type->isStructured)
	{
		auto * structured_type = static_cast<const StructuredType*>(member_type);
		
		if (ImGui::TreeNodeEx(member.name, ImGuiTreeNodeFlags_DefaultOpen))
		{
			treeNodeIsOpen = true;
			
			// note : process these as a group, so the context menu below works when right-clicking on any item inside the structure
			ImGui::BeginGroup();
			{
				if (doReflection_StructuredType(typeDB, *structured_type, member_object, isSet))
				{
					if (signalChanges)
						component->propertyChanged(member_object);
					
					result = true;
				}
			}
			ImGui::EndGroup();
		}
	}
	else
	{
		auto & plain_type = static_cast<const PlainType&>(*member_type);
		
		if (doReflection_PlainType(member, plain_type, member_object, isSet, default_member_object))
		{
			if (signalChanges)
				component->propertyChanged(member_object);
			
			result = true;
		}
	}
	
	if (ImGui::BeginPopupContextItem(member.name))
	{
		if (ImGui::MenuItem("Set to default", nullptr, false, isSet == true))
		{
			isSet = false;
		}
		
		if (ImGui::MenuItem("Set override", nullptr, false, isSet == false))
		{
			isSet = true;
			
			if (defaultComponent != nullptr)
			{
				std::string text;
				member_totext(typeDB, &member, defaultComponent, text);
				member_fromtext(typeDB, &member, component, text.c_str());
			}
		}
		
		ImGui::EndPopup();
	}
	
	if (treeNodeIsOpen)
		ImGui::TreePop();
	
	return result;
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
			
		// todo : add hasFlag method ?
			const bool isAngle = member.findFlag<ComponentMemberFlag_EditorType_Angle>() != nullptr;
			
			if (isAngle)
			{
				if (ImGui::SliderAngle(member.name, &value))
				{
					result = true;
				}
			}
			else
			{
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
			
			const bool isAxis = member.findFlag<ComponentMemberFlag_EditorType_Axis>() != nullptr;

			if (isAxis)
			{
				if (ImGui::SliderFloat3(member.name, &value[0], -1.f, +1.f))
				{
					result = true;
				}
			}
			else
			{
				if (ImGui::InputFloat3(member.name, &value[0]))
				{
					result = true;
				}
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
		Assert(false);
		break;
	}
	
	ImGui::PopStyleColor();
	
	if (result)
	{
		isSet = true;
	}
	
	return result;
}

static bool doReflectionMember_traverse(const TypeDB & typeDB, const Type & type, void * object, const Member * in_member, bool & isSet)
{
	bool result = false;
	
	if (type.isStructured)
	{
		const bool treeNodeIsOpen = in_member != nullptr && false && ImGui::TreeNodeEx(in_member->name, ImGuiTreeNodeFlags_DefaultOpen);
		
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
						int new_size = vector_size;
						bool do_resize = false;
						
						const bool vector_traverse = ImGui::TreeNodeEx(member->name, ImGuiTreeNodeFlags_DefaultOpen);
						
						if (vector_traverse)
						{
							//ImGui::Text("%s", member->name);
							
							if (ImGui::BeginPopupContextItem("Vector"))
							{
								if (ImGui::MenuItem("Add item"))
								{
									insert_index = vector_size;
								}
								
								ImGui::EndPopup();
							}
							
							for (size_t i = 0; i < vector_size; ++i)
							{
								ImGui::PushID(i);
								{
									auto * vector_object = member_interface->vector_access(object, i);
									
									result |= doReflectionMember_traverse(typeDB, *vector_type, vector_object, member, isSet);
									
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
										
										if (ImGui::MenuItem("Insert before"))
										{
											insert_index = i;
										}
										
										if (ImGui::MenuItem("Insert after"))
										{
											insert_index = i + 1;
										}
										
										do_resize = ImGui::InputInt("Resize", &new_size);
										
										ImGui::EndPopup();
									}
								}
								ImGui::PopID();
							}
							
							ImGui::TreePop();
						}
						
						if (insert_index != (size_t)-1)
						{
							member_interface->vector_resize(object, vector_size + 1);
							member_interface->vector_swap(object, vector_size, insert_index);
						}
						
						if (do_resize)
						{
							member_interface->vector_resize(object, new_size);
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
						result |= doReflectionMember_traverse(typeDB, *member_type, member_object, member, isSet);
					}
				}
			}
			ImGui::PopID();
		}
		
		if (treeNodeIsOpen)
		{
			ImGui::TreePop();
		}
	}
	else
	{
		Assert(in_member != nullptr);
		
		auto & plain_type = static_cast<const PlainType&>(type);
		
		result |= doReflection_PlainType(*in_member, plain_type, object, isSet, nullptr);
	}
	
	return result;
}

bool doReflection_StructuredType(
	const TypeDB & typeDB,
	const StructuredType & type,
	void * object,
	bool & isSet)
{
	return doReflectionMember_traverse(typeDB, type, object, nullptr, isSet);
}
