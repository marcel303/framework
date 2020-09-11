#include "componentPropertyUi.h"
#include "componentType.h"
#include "helpers2.h" // g_typeDB
#include "imgui.h"
#include "lineReader.h"
#include "lineWriter.h"
#include "nfd.h"
#include "reflection-textio.h"
#include "StringEx.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

#if defined(ANDROID) || defined(IPHONEOS)
	#define HAS_KEYBOARD 0
#else
	#define HAS_KEYBOARD 1
#endif

namespace ImGui
{
	bool DragDouble(const char* label, double* v, double v_speed = 1.0, double v_min = 0.0, double v_max = 0.0, const char* format = "%.3f", float power = 1.0f)     // If v_min >= v_max we have no bound
	{
		return DragScalar(label, ImGuiDataType_Double, v, v_speed, &v_min, &v_max, format, power);
	}
}

namespace ImGui
{
	bool ComponentProperty(
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
					if (Reflection_StructuredType(typeDB, *structured_type, member_object, isSet, default_member_object, nullptr))
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
			
			if (Reflection_PlainTypeMember(member, plain_type, member_object, isSet, default_member_object))
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
					LineWriter line_writer;
					const bool result = member_tolines_recursive(typeDB, &member, defaultComponent, line_writer, 0);
					Assert(result);
					(void)result;
					
					if (result)
					{
						auto lines = line_writer.to_lines();
						LineReader line_reader(lines, 0, 0);
						const bool result = member_fromlines_recursive(typeDB, &member, component, line_reader);
						Assert(result);
						(void)result;
					}
				}
			}
			
			ImGui::EndPopup();
		}
		
		if (treeNodeIsOpen)
			ImGui::TreePop();
		
		return result;
	}

	bool Reflection_PlainTypeMember(
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
					if (HAS_KEYBOARD)
					{
						if (ImGui::InputInt(member.name, &value))
						{
							result = true;
						}
					}
					else
					{
						if (ImGui::DragInt(member.name, &value))
						{
							result = true;
						}
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
				
				const bool isAngle = member.hasFlag<ComponentMemberFlag_EditorType_AngleDegrees>();
				
				if (isAngle)
				{
					if (ImGui::SliderFloat(member.name, &value, -360.f, +360.f, "%.0f deg", 1.0f))
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
						if (HAS_KEYBOARD)
						{
							if (ImGui::InputFloat(member.name, &value))
							{
								result = true;
							}
						}
						else
						{
							if (ImGui::DragFloat(member.name, &value, .01f))
							{
								result = true;
							}
						}
					}
				}
			}
			break;
		case kDataType_Float2:
			{
				auto & value = plain_type.access<Vec2>(member_object);
				
				if (isSet == false)
				{
					if (default_member_object != nullptr)
					{
						value = plain_type.access<Vec2>(default_member_object);
					}
				}
				
				if (HAS_KEYBOARD)
				{
					if (ImGui::InputFloat2(member.name, &value[0]))
					{
						result = true;
					}
				}
				else
				{
					if (ImGui::DragFloat2(member.name, &value[0], .01f))
					{
						result = true;
					}
				}
			}
			break;
		case kDataType_Float3:
			{
				auto & value = plain_type.access<Vec3>(member_object);
				
				if (isSet == false)
				{
					if (default_member_object != nullptr)
					{
						value = plain_type.access<Vec3>(default_member_object);
					}
				}
				
				const bool isColor = member.hasFlag<ComponentMemberFlag_EditorType_ColorSrgb>();
				const bool isOrientation = member.hasFlag<ComponentMemberFlag_EditorType_OrientationVector>();

				if (isColor)
				{
					if (ImGui::ColorEdit3(member.name, &value[0]))
					{
						result = true;
					}
				}
				else if (isOrientation)
				{
					if (ImGui::SliderFloat3(member.name, &value[0], -1.f, +1.f))
					{
						result = true;
					}
				}
				else
				{
					if (HAS_KEYBOARD)
					{
						if (ImGui::InputFloat3(member.name, &value[0]))
						{
							result = true;
						}
					}
					else
					{
						if (ImGui::DragFloat3(member.name, &value[0], .01f))
						{
							result = true;
						}
					}
				}
			}
			break;
		case kDataType_Float4:
			{
				auto & value = plain_type.access<Vec4>(member_object);

				if (isSet == false)
				{
					if (default_member_object != nullptr)
					{
						value = plain_type.access<Vec4>(default_member_object);
					}
				}
				
				const bool isColor = member.hasFlag<ComponentMemberFlag_EditorType_ColorSrgb>();
				
				if (isColor)
				{
					if (ImGui::ColorEdit4(member.name, &value[0]))
					{
						result = true;
					}
				}
				else
				{
					if (HAS_KEYBOARD)
					{
						if (ImGui::InputFloat4(member.name, &value[0]))
						{
							result = true;
						}
					}
					else
					{
						if (ImGui::DragFloat4(member.name, &value[0], .01f))
						{
							result = true;
						}
					}
				}
			}
			break;
		case kDataType_Double:
			{
				auto & value = plain_type.access<double>(member_object);
				
				if (isSet == false)
				{
					if (default_member_object != nullptr)
					{
						value = plain_type.access<double>(default_member_object);
					}
				}

				auto * limits = member.findFlag<ComponentMemberFlag_FloatLimits>();
				
				if (limits != nullptr)
				{
					auto * curveExponential = member.findFlag<ComponentMemberFlag_FloatEditorCurveExponential>();
					
					float value_as_float = float(value);
					
					if (ImGui::SliderFloat(member.name, &value_as_float, limits->min, limits->max, "%.3f",
						curveExponential == nullptr ? 1.f : curveExponential->exponential))
					{
						value = value_as_float;
						
						result = true;
					}
				}
				else
				{
					if (HAS_KEYBOARD)
					{
						if (ImGui::InputDouble(member.name, &value))
						{
							result = true;
						}
					}
					else
					{
						if (ImGui::DragDouble(member.name, &value, .01))
						{
							result = true;
						}
					}
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
				
				const bool isFilePath = member.hasFlag<ComponentMemberFlag_EditorType_FilePath>();
				
				if (isFilePath)
				{
					ImGui::PushID(member.name);
					{
						char buffer[1024];
						strcpy_s(buffer, sizeof(buffer), value.c_str());
						
						if (ImGui::InputText("", buffer, sizeof(buffer)))
						{
							value = buffer;
							
							result = true;
						}
						
						ImGui::SameLine();
						if (ImGui::Button(".."))
						{
							nfdchar_t * filename = nullptr;

							if (NFD_OpenDialog(nullptr, nullptr, &filename) == NFD_OKAY)
							{
							// todo : compute relative path
								value = filename;
								
								result = true;
							}
						
							if (filename != nullptr)
							{
								free(filename);
								filename = nullptr;
							}
						}
					}
					ImGui::PopID();
				}
				else
				{
					char buffer[1024];
					strcpy_s(buffer, sizeof(buffer), value.c_str());
					
					if (ImGui::InputText(member.name, buffer, sizeof(buffer)))
					{
						value = buffer;
						
						result = true;
					}
				}
			}
			break;
		case kDataType_Enum:
			{
				const EnumType & enum_type = static_cast<const EnumType&>(plain_type);
				
				int value;
				
				if (enum_type.get_value(member_object, value) == false)
					isSet = false;

				if (isSet == false)
				{
					if (default_member_object != nullptr)
					{
						enum_type.get_value(default_member_object, value);
					}
				}
				
				std::vector<const char*> items;
				int selectedItem = -1;
				
				for (auto * elem = enum_type.firstElem; elem != nullptr; elem = elem->next)
				{
					if (elem->value == value)
						selectedItem = items.size();
					items.push_back(elem->key);
				}
				
				if (!items.empty())
				{
					if (ImGui::Combo(member.name, &selectedItem, items.data(), items.size()))
					{
						enum_type.set(member_object, items[selectedItem]);
						
						result = true;
					}
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

	static bool ReflectionMember_traverse(
		const TypeDB & typeDB,
		const Type & type,
		void * object,
		const Member * in_member,
		bool & isSet,
		void * default_object,
		void ** changedMemberObject)
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
										
										result |= ReflectionMember_traverse(
											typeDB,
											*vector_type,
											vector_object,
											member,
											isSet,
											nullptr,
											changedMemberObject);
										
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
								for (size_t i = vector_size; i > insert_index; --i)
								{
									member_interface->vector_swap(object, i, i - 1);
								}
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
							auto * default_member_object =
								default_object == nullptr
								? nullptr
								: member_scalar->scalar_access(default_object);
					
							result |= ReflectionMember_traverse(
								typeDB,
								*member_type,
								member_object,
								member,
								isSet,
								default_member_object,
								changedMemberObject);
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
			
			if (Reflection_PlainTypeMember(*in_member, plain_type, object, isSet, default_object))
			{
				result = true;
				
				if (changedMemberObject != nullptr)
					*changedMemberObject = object;
			}
		}
		
		return result;
	}

	bool Reflection_StructuredType(
		const TypeDB & typeDB,
		const StructuredType & type,
		void * object,
		bool & isSet,
		void * defaultObject,
		void ** changedMemberObject)
	{
		Assert(changedMemberObject == nullptr || *changedMemberObject == nullptr);
		
		return ReflectionMember_traverse(
			typeDB,
			type,
			object,
			nullptr,
			isSet,
			defaultObject,
			changedMemberObject);
	}
}
