#pragma once

#include <functional>
#include <string>

struct ComponentBase;
struct Member;
struct PlainType;
struct StructuredType;
struct TypeDB;

namespace ImGui
{
	struct Reflection_Callbacks
	{
		std::function<void(std::string & path)> makePathRelative;
		std::function<void()> propertyWillChange;
		std::function<void()> propertyDidChange;
	};
	
	bool ComponentProperty(
		const TypeDB & typeDB,
		const Member & member,
		ComponentBase * component,
		const bool signalChanges,
		bool & isSet,
		ComponentBase * defaultComponent,
		Reflection_Callbacks * callbacks);
}

namespace ImGui
{
	/**
	 * Presents an editor for editing a plain type member object.
	 * The editor supports editing common plain types including integers, floats, strings and (2-, 3- and 4d) vectors.
	 * The editor supports the concept of overrides. When no specific value is set, the editor will show @default_member_object's value. When the user edits @member_object's value, the value is marked as overriden. See @isSet.
	 * @param member Member information. Used to present its name, but also to look for flags describing its semantics and select the approriate editor.
	 * @param plain_type Type information describing @member_object's value.
	 * @param member_object The memory address of the member's value.
	 * @param isSet Set to true when the value is set as an override. False otherwise. In the case this is false, @member_object's value is ignored, and @default_member_object's value is used instead for showing the current value.
	 * @param default_member_object The memory address of the member's default value. When @isSet is set to false, the editor will show the default value and ignore the value of the member itself (as it's unset or undefined). When @member_object's value is edited, isSet is set to true, to mark the user has explicitly set a value.
	 * @return True when the user made changes to @member_object's value. False otherwise.
	 */
	bool Reflection_PlainTypeMember(
		const Member & member,
		const PlainType & plain_type,
		void * member_object,
		bool & isSet,
		void * default_member_object,
		Reflection_Callbacks * callbacks);

	/**
	 * Presents an editor for editing a structured type object.
	 * The editor supports the concept of overrides. When the object doesn't contain any changes, the editor will show the member values of @default_object. When the user edits a member's value, the object is marked as overriden. See @isSet.
	 * @param typeDB The type database used to look up type information while iterating over the structured type's members.
	 * @param type Type information describing the structured type object's members.
	 * @param object The memory address of the structured type object's member values.
	 * @param isSet Set to true when one or more of the object's members is set as an override. False otherwise. In the case this is false, the object's member values are ignored, and the @default_object's member values are used instead for showing the current values.
	 * @param default_object The memory address of the structured type's default object's member values. When @isSet is set to false, the editor will show the default object's member values and ignore the values of the object itself (as it's unset or undefined). When the object is edited, isSet is set to true, to mark the user has explicitly changed the object, and @changedMemberObject is set to the first member that is changed.
	 * @param changedMemberObject Set to the memory address of the first member that's changed, when the object has been modified. nullptr otherwise.
	 * @return True when the user made changes to the structured type's object. False otherwise.
	 */
	bool Reflection_StructuredType(
		const TypeDB & typeDB,
		const StructuredType & type,
		void * object,
		bool & isSet,
		void * default_object,
		void ** changedMemberObject,
		Reflection_Callbacks * callbacks); // note : since we can store only one pointer, changedMemberObject is the address of the first member that's changed
}
