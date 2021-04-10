#pragma once

// forward declarations

struct ComponentSet;
struct Template;
struct TypeDB;

// -- template helpers

bool instantiateComponentsFromTemplate(
	const TypeDB & typeDB,
	const Template & t,
	ComponentSet & componentSet);
