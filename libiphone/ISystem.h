#pragma once

#include <string>
#include "Types.h"
#include "Vec3.h"

class ISystem
{
public:
	virtual ~ISystem()
	{
	}
	
	virtual void Vibrate() = 0;
	virtual void CheckNetworkConnectivity() = 0;
	virtual bool HasNetworkConnectivity_get() = 0;
	virtual void HasNetworkConnectivity_set(bool value) = 0;
	virtual bool IsHacked() = 0;
	//virtual std::string GetDeviceId() = 0;
	virtual std::string GetResourcePath(const char* fileName) = 0;
	virtual std::string GetDocumentPath(const char* fileName) = 0;
	virtual std::string GetCountryCode() = 0;
	virtual void SaveToAlbum(class Screenshot* ss, const char* name) = 0;
	virtual Vec3 GetTiltVector() = 0;
	virtual Vec2F GetTiltDirectionXY() = 0;
	virtual bool IsIpad() = 0;
};
