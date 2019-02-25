#pragma once

struct ParameterBase;
struct ParameterComponent;

void doParameterUi(ParameterBase & parameter);
void doParameterUi(ParameterComponent & component);
void doParameterUi(ParameterComponent * components, const int numComponents);
