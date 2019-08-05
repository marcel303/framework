#pragma once

struct ParameterBase;
struct parameterMgr;
struct ParameterComponentMgr;

void doParameterUi(ParameterBase & parameter);
void doParameterUi(ParameterMgr & parameterMgr, const char * filter, const bool showCollapsingHeader);
void doParameterUi_recursive(ParameterMgr & parameterMgr, const char * filter);

void copyParametersToText_recursive(ParameterMgr * const parameterMgr, const char * filter, std::string & out_text);
void copyParametersToClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter);

void pasteParametersFromText_recursive(ParameterMgr * const parameterMgr, const char * text, const char * filter);
void pasteParametersFromClipboard_recursive(ParameterMgr * const parameterMgr, const char * filter);
