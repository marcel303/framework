#pragma once

#include <string>
#include "Types.h"

class ISettings
{
public:
	virtual ~ISettings();
	
	virtual std::string GetString(std::string name, std::string _default) = 0;
	virtual void SetString(std::string name, std::string value) = 0;

	int32_t GetInt32(std::string name, int32_t _default);
	uint32_t GetUInt32(std::string name, uint32_t _default);
	float GetFloat(std::string name, float _default);

	void SetInt32(std::string name, int32_t value);
	void SetUInt32(std::string name, uint32_t value);
	void SetFloat(std::string name, float value);
};

#ifdef SETTINGS_MACOS
#include "Settings_Macos.h"
#endif

#ifdef SETTINGS_FILESYSTEM
#include "Settings_FileSystem.h"
#endif
