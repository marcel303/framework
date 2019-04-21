#pragma once

struct ParameterBase;
struct parameterMgr;
struct ParameterComponentMgr;

void doParameterUi(ParameterBase & parameter);
void doParameterUi(ParameterMgr & parameterMgr, const char * filter);
