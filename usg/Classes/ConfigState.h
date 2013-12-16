#pragma once

#include "FixedSizeString.h"

#define MAX_CONFIG_STRING_SIZE 64

void ConfigSetInt(const char* name, int value);
void ConfigSetString(const char* name, const char* value);
int ConfigGetInt(const char* name);
int ConfigGetIntEx(const char* name, int _default);
FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigGetString(const char* name);
FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigGetStringEx(const char* name, const char* _default);
void ConfigLoad();
void ConfigSave(bool force);
