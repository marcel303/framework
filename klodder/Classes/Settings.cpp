#include "Parse.h"
#include "Settings.h"
#include "StringEx.h"

//

ISettings::~ISettings()
{
}

int32_t ISettings::GetInt32(std::string name, int32_t _default)
{
	std::string value = GetString(name, String::Format("%d", _default));

	return Parse::Int32(value);
}

uint32_t ISettings::GetUInt32(std::string name, uint32_t _default)
{
	std::string value = GetString(name, String::Format("%lu", _default));
	
	return Parse::UInt32(value);
}

float ISettings::GetFloat(std::string name, float _default)
{
	std::string value = GetString(name, String::Format("%f", _default));

	return Parse::Float(value);
}

void ISettings::SetInt32(std::string name, int32_t value)
{
	SetString(name, String::Format("%d", value));
}

void ISettings::SetUInt32(std::string name, uint32_t value)
{
	SetString(name, String::Format("%lu", value));
}

void ISettings::SetFloat(std::string name, float value)
{
	SetString(name, String::Format("%f", value));
}
