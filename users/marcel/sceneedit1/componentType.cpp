#include "componentType.h"
#include "Parse.h"
#include "resource.h"
#include "StringEx.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

//

void splitString(const std::string & str, std::vector<std::string> & result);

//

#if ENABLE_COMPONENT_JSON

#include "componentJson.h"

static void to_json(nlohmann::json & j, const Vec2 & v)
{
	j = { v[0], v[1] };
}

static void to_json(nlohmann::json & j, const Vec3 & v)
{
	j = { v[0], v[1], v[2] };
}

static void to_json(nlohmann::json & j, const Vec4 & v)
{
	j = { v[0], v[1], v[2], v[3] };
}

static void to_json(nlohmann::json & j, const AngleAxis & v)
{
	j["angle"] = v.angle;
	j["axis"] = { v.axis[0], v.axis[1], v.axis[2] };
}

static void from_json(const nlohmann::json & j, Vec2 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
}

static void from_json(const nlohmann::json & j, Vec3 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
	v[2] = j[2].get<float>();
}

static void from_json(const nlohmann::json & j, Vec4 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
	v[2] = j[2].get<float>();
	v[3] = j[3].get<float>();
}

static void from_json(const nlohmann::json & j, AngleAxis & v)
{
	AngleAxis defaultValue;
	
	v.angle = j.value("angle", defaultValue.angle);
	v.axis = j.value("axis", defaultValue.axis);
}

#endif

//

void ComponentPropertyBool::setToDefault(ComponentBase * component)
{
	setter(component, false);
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyBool::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyBool::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, false));
}

#endif

void ComponentPropertyBool::to_text(ComponentBase * component, std::string & text)
{
	text = getter(component) ? "true" : "false";
}

bool ComponentPropertyBool::from_text(ComponentBase * component, const char * text)
{
	setter(component, Parse::Bool(text));
	
	return true;
}

//

void ComponentPropertyInt::setToDefault(ComponentBase * component)
{
	setter(component, 0);
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyInt::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyInt::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, 0));
}

#endif

void ComponentPropertyInt::to_text(ComponentBase * component, std::string & text)
{
	text = String::FormatC("%d", getter(component));
}

bool ComponentPropertyInt::from_text(ComponentBase * component, const char * text)
{
	setter(component, Parse::Int32(text));
	
	return true;
}

//

void ComponentPropertyFloat::setToDefault(ComponentBase * component)
{
	setter(component, 0.f);
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyFloat::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyFloat::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, 0.f));
}

#endif

void ComponentPropertyFloat::to_text(ComponentBase * component, std::string & text)
{
	text = String::FormatC("%f", getter(component));
}

bool ComponentPropertyFloat::from_text(ComponentBase * component, const char * text)
{
	setter(component, Parse::Float(text));
	
	return true;
}

//

void ComponentPropertyVec2::setToDefault(ComponentBase * component)
{
	setter(component, Vec2());
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyVec2::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyVec2::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, Vec2()));
}

#endif

void ComponentPropertyVec2::to_text(ComponentBase * component, std::string & text)
{
	const auto & value = getter(component);
	
	text = String::FormatC("%f %f", value[0], value[1]);
}

bool ComponentPropertyVec2::from_text(ComponentBase * component, const char * text)
{
	std::vector<std::string> parts;
	splitString(text, parts);
	
	if (parts.size() != 2)
		return false;
	
	const Vec2 value(
		Parse::Float(parts[0]),
		Parse::Float(parts[1]));
	
	setter(component, value);
	
	return true;
}

//

void ComponentPropertyVec3::setToDefault(ComponentBase * component)
{
	setter(component, Vec3());
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyVec3::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyVec3::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, Vec3()));
}

#endif

void ComponentPropertyVec3::to_text(ComponentBase * component, std::string & text)
{
	const auto & value = getter(component);
	
	text = String::FormatC("%f %f %f", value[0], value[1], value[2]);
}

bool ComponentPropertyVec3::from_text(ComponentBase * component, const char * text)
{
	std::vector<std::string> parts;
	splitString(text, parts);
	
	if (parts.size() != 3)
		return false;
	
	const Vec3 value(
		Parse::Float(parts[0]),
		Parse::Float(parts[1]),
		Parse::Float(parts[2]));
	
	setter(component, value);
	
	return true;
}

//

void ComponentPropertyVec4::setToDefault(ComponentBase * component)
{
	setter(component, Vec4());
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyVec4::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyVec4::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, Vec4()));
}

#endif

void ComponentPropertyVec4::to_text(ComponentBase * component, std::string & text)
{
	const auto & value = getter(component);
	
	text = String::FormatC("%f %f %f %f", value[0], value[1], value[2], value[3]);
}

bool ComponentPropertyVec4::from_text(ComponentBase * component, const char * text)
{
	std::vector<std::string> parts;
	splitString(text, parts);
	
	if (parts.size() != 4)
		return false;
	
	const Vec4 value(
		Parse::Float(parts[0]),
		Parse::Float(parts[1]),
		Parse::Float(parts[2]),
		Parse::Float(parts[3]));
	
	setter(component, value);
	
	return true;
}

//

void ComponentPropertyString::setToDefault(ComponentBase * component)
{
	setter(component, std::string());
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyString::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyString::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, std::string()));
}

#endif

void ComponentPropertyString::to_text(ComponentBase * component, std::string & text)
{
	text = getter(component);
}

bool ComponentPropertyString::from_text(ComponentBase * component, const char * text)
{
	setter(component, text);
	
	return true;
}

//

void ComponentPropertyAngleAxis::setToDefault(ComponentBase * component)
{
	setter(component, AngleAxis());
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyAngleAxis::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component);
}

void ComponentPropertyAngleAxis::from_json(ComponentBase * component, const ComponentJson & j)
{
	setter(component, j.j.value(name, AngleAxis()));
}

#endif

void ComponentPropertyAngleAxis::to_text(ComponentBase * component, std::string & text)
{
	const auto & value = getter(component);
	
	text = String::FormatC("%f %f %f %f", value.angle, value.axis[0], value.axis[1], value.axis[2]);
}

bool ComponentPropertyAngleAxis::from_text(ComponentBase * component, const char * text)
{
	std::vector<std::string> parts;
	splitString(text, parts);
	
	if (parts.size() != 4)
		return false;
	
	AngleAxis value;
	
	value.angle = Parse::Float(parts[0]);
	value.axis.Set(
		Parse::Float(parts[1]),
		Parse::Float(parts[2]),
		Parse::Float(parts[3]));
	
	setter(component, value);
	
	return true;
}

//

void ComponentPropertyResourcePtr::setToDefault(ComponentBase * component)
{
	setter(component, ResourcePtr());
}

#if ENABLE_COMPONENT_JSON

void ComponentPropertyResourcePtr::to_json(ComponentBase * component, ComponentJson & j)
{
	j.j = getter(component).path;
}

void ComponentPropertyResourcePtr::from_json(ComponentBase * component, const ComponentJson & j)
{
	ResourcePtr value;
	
	value.path = j.j.value(name, "");
	
	setter(component, value);
}

#endif

void ComponentPropertyResourcePtr::to_text(ComponentBase * component, std::string & text)
{
	text = getter(component).path;
}

bool ComponentPropertyResourcePtr::from_text(ComponentBase * component, const char * text)
{
	ResourcePtr value;
	
	value.path = text;
	
	setter(component, value);
	
	return true;
}
