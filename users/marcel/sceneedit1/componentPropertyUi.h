#pragma once

struct ComponentBase;
struct ComponentPropertyBase;
struct Member;

void doComponentProperty(
	ComponentPropertyBase * propertyBase,
	ComponentBase * component,
	const bool signalChanges,
	bool & isSet,
	ComponentPropertyBase * defaultPropertyBase,
	ComponentBase * defaultComponent);

void doComponentProperty_v2(
	const Member * member,
	ComponentBase * component,
	const bool signalChanges,
	bool & isSet,
	ComponentBase * defaultComponent);
