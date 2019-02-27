#pragma once

struct ComponentBase;
struct Member;

void doComponentProperty(
	const Member * member,
	ComponentBase * component,
	const bool signalChanges,
	bool & isSet,
	ComponentBase * defaultComponent);
