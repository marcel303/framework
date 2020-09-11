#pragma once

struct ComponentBase;
struct Member;
struct PlainType;
struct StructuredType;
struct TypeDB;

namespace ImGui
{
	bool ComponentProperty(
		const TypeDB & typeDB,
		const Member & member,
		ComponentBase * component,
		const bool signalChanges,
		bool & isSet,
		ComponentBase * defaultComponent);
}

namespace ImGui
{
// todo : extend documentation
	/**
	 * Presents an editor for a plain type member.
	 * @param member Member information describing the plain type member.
	 * @param plain_type Plain type information describing the plain member's object (value).
	 * @param member_object The memory address of the plain type member's object (value).
	 * @param isSet Set to true when the value is set as an 'override' (set explicitly). False otherwise.
	 * @param default_member_object The memory address for the member's default object (value). When set to false, the editor will show the default value and ignore the value inside the member. When the member is edited, isSet is set to true, to mark the user has explicitly set a value.
	 */
	bool Reflection_PlainType(
		const Member & member,
		const PlainType & plain_type,
		void * member_object,
		bool & isSet,
		void * default_member_object);

	bool Reflection_StructuredType(
		const TypeDB & typeDB,
		const StructuredType & type,
		void * object,
		bool & isSet,
		void * defaultObject,
		void ** changedMemberObject); // note : since we can store only one pointer, changedMemberObject is the address of the first member that changed
}
