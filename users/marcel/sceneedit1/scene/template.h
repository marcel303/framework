#pragma once

#include <string>
#include <vector>

// forward declarations

struct ComponentSet;
struct TypeDB;

//

struct TemplateComponentProperty
{
	std::string name;
	std::vector<std::string> value_lines;
};

struct TemplateComponent
{
	std::string typeName;
	std::string id;
	
	std::vector<TemplateComponentProperty> properties;
};

struct Template
{
	std::string name;
	std::string base;
	
	std::vector<TemplateComponent> components;
};

void dumpTemplateToLog(const Template & t);
