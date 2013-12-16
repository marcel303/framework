#include "StringEx.h"
#include "System_Win32.h"

void System_Win32::Vibrate()
{
	// nop
}

void System_Win32::CheckNetworkConnectivity()
{
}

bool System_Win32::HasNetworkConnectivity_get()
{
	return true;
}

void System_Win32::HasNetworkConnectivity_set(bool value)
{
}

bool System_Win32::IsHacked()
{
	return false;
}

std::string System_Win32::GetDeviceId()
{
	return String::Empty;
}

std::string System_Win32::GetResourcePath(const char* fileName)
{
	return std::string("GAMEDATA/") + std::string(fileName);
}

std::string System_Win32::GetDocumentPath(const char* fileName)
{
	return std::string(fileName);
}

std::string System_Win32::GetCountryCode()
{
	return "xx";
}

void System_Win32::SaveToAlbum(class Screenshot* ss, const char* name)
{
}

Vec3 System_Win32::GetTiltVector()
{
	return Vec3(0.0f, 0.0f, 1.0f);
}

Vec2F System_Win32::GetTiltDirectionXY()
{
	return Vec2F(0.0f, 0.0f);
}

bool System_Win32::IsIpad()
{
	return false;
}
