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
	/**
	 * Presents an editor for a plain type member.
	 * The editor supports editing most common plain types, including integers, floats, strings and (2-, 3- and 4d) vector types.
	 * The editor supports the concept of overrides. When no specific value is set, the editor will show the member's default value. When the user edits the member's value, the value is marked as being overriden. See @isSet.
	 * @param member Member information describing the plain type member. Used to present its name, but also to look for flags describing its semantics and show the approriate editor.
	 * @param plain_type Type information describing the plain member's object (value).
	 * @param member_object The memory address of the plain type member's object (value).
	 * @param isSet Set to true when the value is set as an override. False otherwise.
	 * @param default_member_object The memory address of the member's default object (value). When @isSet is set to false, the editor will show the default value and ignore the value of the member itself (as it's unset or undefined). When the member is edited, isSet is set to true, to mark the user has explicitly set a value, and the value is stored inside the member's object.
	 */
	bool Reflection_PlainTypeMember(
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
