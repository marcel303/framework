#pragma once

#include <string>
#include <vector>

struct ComponentSet;
class LineReader;
struct TypeDB;

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

typedef bool (*FetchTemplateCallback)(
	const char * name,
	const void * userData,
	Template & out_template);

/**
 * Parse a template object from lines.
 * This will read the template name, the base name, and components and component properties.
 * @param lineReader The source of the lines.
 * @param name (Optional, may be nullptr) The name of the template.
 * @param out_template The parsed template.
 * @return True upon success.
 */
bool parseTemplateFromLines(
	LineReader & lineReader,
	const char * name,
	Template & out_template);

/**
 * Loads the file contents at the given path and parses the resulting lines to template.
 * @param path The path of the text file to parse.
 * @param out_template The parsed template.
 * @return True upon success.
 */
bool parseTemplateFromFile(
	const char * path,
	Template & out_template);

/**
 * Applies the components and component properties to the target template.
 * When a component already exists, its properties are supplemented.
 * When a property already exists, its value is overridden.
 * @param allowAddingComponents When true, components in the target template will be added when missing.
 * @param allowAddingProperties When true, properties in a target component will be added when missing.
 * @return True upon success.
 */
bool overlayTemplate(
	Template & target,
	const Template & overlay,
	const bool allowAddingComponents,
	const bool allowAddingProperties);

/**
 * Recursively overlay base templates. Base templates are fetched given the
 * name of the base template, and the fetch callback. See overlayTemplate for
 * the meaning of allowAddingComponents and allowAddingProperties.
 * @param t The template to apply the base template overlays to.
 * @param fetchTemplate Callback function used to fetch templates.
 * @param userData User data passed to fetchTemplate.
 * @return True upon success.
 */
bool recursivelyOverlayBaseTemplates(
	Template & t,
	const bool allowAddingComponents,
	const bool allowAddingProperties,
	const FetchTemplateCallback fetchTemplate,
	const void * userData);

bool parseTemplateFromCallbackAndRecursivelyOverlayBaseTemplates(
	const char * name,
	const bool allowAddingComponents,
	const bool allowAddingProperties,
	const FetchTemplateCallback fetchTemplate,
	const void * userData,
	Template & out_template);

bool parseTemplateFromFileAndRecursivelyOverlayBaseTemplates(
	const char * path,
	const bool allowAddingComponents,
	const bool allowAddingProperties,
	Template & out_template);

bool instantiateComponentsFromTemplate(
	const TypeDB & typeDB,
	const Template & t,
	ComponentSet & componentSet);

void dumpTemplateToLog(const Template & t);
