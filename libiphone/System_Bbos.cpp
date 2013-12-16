#include <bps/accelerometer.h>
#include <screen/screen.h>
#include "Log.h"
#include "StringEx.h"
#include "System_Bbos.h"

void System_Bbos::Vibrate()
{
	// nop
}

void System_Bbos::CheckNetworkConnectivity()
{
}

bool System_Bbos::HasNetworkConnectivity_get()
{
	return false;
}

void System_Bbos::HasNetworkConnectivity_set(bool value)
{
}

bool System_Bbos::IsHacked()
{
	return false;
}

std::string System_Bbos::GetDeviceId()
{
	return String::Empty;
}

std::string System_Bbos::GetResourcePath(const char* fileName)
{
	return std::string("./app/native/BBDATA/") + std::string(fileName);
}

std::string System_Bbos::GetDocumentPath(const char* fileName)
{
	return std::string("./data/") + std::string(fileName);
}

std::string System_Bbos::GetCountryCode()
{
	return "xx";
}

void System_Bbos::SaveToAlbum(class Screenshot* ss, const char* name)
{
}

Vec3 System_Bbos::GetTiltVector()
{
	double x, y, z;

	if (accelerometer_read_forces(&x, &y, &z) == BPS_SUCCESS)
	{
	#ifdef BBOS_ALLTOUCH
		return Vec3(float(x), float(y), float(z));
	#else
		return Vec3(float(y), float(x), float(z));
	#endif
	}
	else
		return Vec3(0.0f, 0.0f, 1.0f);
}

Vec2F System_Bbos::GetTiltDirectionXY()
{
	return Vec2F(0.0f, 0.0f);
}

bool System_Bbos::IsIpad()
{
	return false;
}

void System_Bbos::SetScreenDisplay(screen_display_t screenDisplay)
{
	mScreenDisplay = screenDisplay;
}

int System_Bbos::GetScreenRotation()
{
	int rotation = 0;

	if (screen_get_display_property_iv(mScreenDisplay, SCREEN_PROPERTY_ROTATION, &rotation) != 0)
	{
		LOG_ERR("failed: get SCREEN_PROPERTY_ROTATION", 0);
	}

	//printf("rotation: %d\n", rotation);

	return rotation;
}
