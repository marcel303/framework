#pragma once

struct ParameterBase;
struct parameterMgr;
struct ParameterComponentMgr;

void doParameterUi(ParameterBase & parameter);
void doParameterUi(ParameterMgr & parameterMgr, const char * filter);
void doParameterUi_recursive(ParameterMgr & parameterMgr, const char * filter);

void copyParametersToClipboard(ParameterBase * const * const parameters, const int numParameters);
void copyParametersToClipboard(ParameterMgr * const * const parameterMgrs, const int numParameterMgrs, const char * filter);

void pasteParametersFromClipboard(ParameterBase * const * const parameters, const int numParameters);
void pasteParametersFromClipboard(ParameterMgr * const * const parameterMgrs, const int numParameterMgrs, const char * filter);
