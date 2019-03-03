#pragma once

struct ParameterBase;
struct ParameterComponent;
struct ParameterComponentMgr;

void doParameterUi(ParameterBase & parameter);
void doParameterUi(ParameterComponent & component, const char * filter);
void doParameterUi(ParameterComponentMgr & componentMgr, const char * filter);
