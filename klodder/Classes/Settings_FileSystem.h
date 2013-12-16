#pragma once
#include "Settings.h"

class Settings_FileSystem : public ISettings
{
	virtual std::string GetString(std::string name, std::string _default);
	virtual void SetString(std::string name, std::string value);
};

//#ifdef WIN32
extern Settings_FileSystem gSettings;
//#endif
