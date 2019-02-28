#pragma once

struct ComponentBase;
struct Member;
struct PlainType;
struct StructuredType;
struct TypeDB;

bool doComponentProperty(
	const TypeDB & typeDB,
	const Member & member,
	ComponentBase * component,
	const bool signalChanges,
	bool & isSet,
	ComponentBase * defaultComponent);

bool doReflection_PlainType(
	const Member & member,
	const PlainType & plain_type,
	void * member_object,
	bool & isSet,
	void * default_member_object);
	
bool doReflection_StructuredType(
	const TypeDB & typeDB,
	const StructuredType & type,
	void * object,
	bool & isSet,
	void ** changedMemberObject); // note : since we can store only one pointer, changedMemberObject is the address of the first member that changed
