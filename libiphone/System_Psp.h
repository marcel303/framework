#pragma once

#include "ISystem.h"

class System_Psp : public ISystem
{
public:
	~System_Psp();
	
	void Vibrate();
	void CheckNetworkConnectivity();
	bool HasNetworkConnectivity_get();
	void HasNetworkConnectivity_set(bool value);
	bool IsHacked();
	std::string GetDeviceId();
	std::string GetResourcePath(const char* fileName);
	std::string GetDocumentPath(const char* fileName);
	std::string GetCountryCode();
	void SaveToAlbum(class Screenshot* ss, const char* name);
	Vec3 GetTiltVector();
	Vec2F GetTiltDirectionXY();
	bool IsIpad();
};
