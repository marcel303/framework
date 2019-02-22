#include "parameterComponent.h"

void ParameterComponent::add(ParameterBase * parameter)
{
	parameters.push_back(parameter);
}

ParameterBool * ParameterComponent::addBool(const char * name, const bool defaultValue)
{
	auto * parameter = new ParameterBool(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterInt * ParameterComponent::addInt(const char * name, const int defaultValue)
{
	auto * parameter = new ParameterInt(name, defaultValue);
	add(parameter);
	return parameter;
}

ParameterFloat * ParameterComponent::addFloat(const char * name, const float defaultValue)
{
	auto * parameter = new ParameterFloat(name, defaultValue);
	add(parameter);
	return parameter;
}

void ParameterComponent::tick(const float dt)
{
}
