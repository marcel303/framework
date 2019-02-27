#pragma once

#include <string>
#include <vector>

struct ComponentSet;
struct TypeDB;

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
	std::string name;
	std::string base;
	
	std::vector<TemplateComponent> components;
};

typedef bool (*FetchTemplateCallback)(const char * name, void * user_data, Template & out_template);

bool parseTemplateFromLines(const std::vector<std::string> & lines, const char * name, Template & out_template);
bool loadTemplateFromFile(const char * filename, Template & t);

bool overlayTemplate(Template & target, const Template & overlay, const bool allowAddingComponents, const bool allowAddingProperties);

bool applyTemplateOverlaysWithCallback(const char * name, const Template & t, Template & out_template, const bool allowAddingComponentsFromBase, FetchTemplateCallback fetchTemplate, void * user_data);
bool parseTemplateWithOverlaysWithCallback(const char * name, Template & out_template, const bool allowAddingComponentsFromBase, FetchTemplateCallback fetchTemplate, void * user_data);
bool loadTemplateWithOverlaysFromFile(const char * filename, Template & out_template, const bool allowAddingComponentsFromBase);

bool instantiateComponentsFromTemplate(const TypeDB & typeDB, const Template & t, ComponentSet & componentSet);

void dumpTemplateToLog(const Template & t);
