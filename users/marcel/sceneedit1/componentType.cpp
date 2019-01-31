#include "componentType.h"
#include "json.hpp"
#include "Parse.h"

void to_json(nlohmann::json & j, const Vec2 & v)
{
	j = { v[0], v[1] };
}

void to_json(nlohmann::json & j, const Vec3 & v)
{
	j = { v[0], v[1], v[2] };
}

void to_json(nlohmann::json & j, const Vec4 & v)
{
	j = { v[0], v[1], v[2], v[3] };
}

void to_json(nlohmann::json & j, const AngleAxis & v)
{
	j["angle"] = v.angle;
	j["axis"] = { v.axis[0], v.axis[1], v.axis[2] };
}

void from_json(const nlohmann::json & j, Vec2 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
}

void from_json(const nlohmann::json & j, Vec3 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
	v[2] = j[2].get<float>();
}

void from_json(const nlohmann::json & j, Vec4 & v)
{
	v[0] = j[0].get<float>();
	v[1] = j[1].get<float>();
	v[2] = j[2].get<float>();
	v[3] = j[3].get<float>();
}

void from_json(const nlohmann::json & j, AngleAxis & v)
{
	AngleAxis defaultValue;
	
	v.angle = j.value("angle", defaultValue.angle);
	v.axis = j.value("axis", defaultValue.axis);
}

//

void ComponentPropertyInt::to_json(ComponentBase * component, nlohmann::json & j)
{
	j = getter(component);
}

void ComponentPropertyInt::from_json(ComponentBase * component, const nlohmann::json & j)
{
	setter(component, j.value(name, 0));
}

//

void ComponentPropertyFloat::to_json(ComponentBase * component, nlohmann::json & j)
{
	j = getter(component);
}

void ComponentPropertyFloat::from_json(ComponentBase * component, const nlohmann::json & j)
{
	setter(component, j.value(name, 0.f));
}

//

void ComponentPropertyVec2::to_json(ComponentBase * component, nlohmann::json & j)
{
	j = getter(component);
}

void ComponentPropertyVec2::from_json(ComponentBase * component, const nlohmann::json & j)
{
	setter(component, j.value(name, Vec2()));
}

//

void ComponentPropertyVec3::to_json(ComponentBase * component, nlohmann::json & j)
{
	j = getter(component);
}

void ComponentPropertyVec3::from_json(ComponentBase * component, const nlohmann::json & j)
{
	setter(component, j.value(name, Vec3()));
}

//

void ComponentPropertyVec4::to_json(ComponentBase * component, nlohmann::json & j)
{
	j = getter(component);
}

void ComponentPropertyVec4::from_json(ComponentBase * component, const nlohmann::json & j)
{
	setter(component, j.value(name, Vec4()));
}

//

void ComponentPropertyString::to_json(ComponentBase * component, nlohmann::json & j)
{
	j = getter(component);
}

void ComponentPropertyString::from_json(ComponentBase * component, const nlohmann::json & j)
{
	setter(component, j.value(name, std::string()));
}

//

void ComponentPropertyAngleAxis::to_json(ComponentBase * component, nlohmann::json & j)
{
	j = getter(component);
}

void ComponentPropertyAngleAxis::from_json(ComponentBase * component, const nlohmann::json & j)
{
	setter(component, j.value(name, AngleAxis()));
}
