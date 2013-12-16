#include "Settings.h"

class Settings_Macos : public ISettings
{
public:
	Settings_Macos();
	~Settings_Macos();
	
	std::string GetString(std::string name, std::string _default);
	void SetString(std::string name, std::string value);
	
private:
	void* mConfig;
};

extern Settings_Macos gSettings;
