#pragma once

#include <string>
#include <vector>

struct ComponentSet;

struct TemplateComponentProperty
{
	std::string name;
	std::string value; // todo : make this a list of values. with corrected indentation. leave it the the property type to deserialize itself
};

struct TemplateComponent
{
	std::string type_name;
	std::string id;
	
	std::vector<TemplateComponentProperty> properties;
};

struct Template
{
	std::string base;
	
	std::vector<TemplateComponent> components;
};

bool parseTemplateFromLines(const std::vector<std::string> & lines, Template & out_template);
bool loadTemplateFromFile(const char * filename, Template & t);

bool overlayTemplate(Template & target, const Template & overlay, const bool allowAddingComponents, const bool allowAddingProperties);

bool loadTemplateWithOverlaysFromFile(const char * filename, Template & out_template, const bool allowAddingComponentsFromBase);

bool instantiateComponentsFromTemplate(const Template & t, ComponentSet & componentSet);
