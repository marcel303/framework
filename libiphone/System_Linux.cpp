#include "StringEx.h"
#include "System_Linux.h"

void System_Linux::Vibrate()
{
	// nop
}

void System_Linux::CheckNetworkConnectivity()
{
}

bool System_Linux::HasNetworkConnectivity_get()
{
	return false;
}

void System_Linux::HasNetworkConnectivity_set(bool value)
{
}

bool System_Linux::IsHacked()
{
	return false;
}

std::string System_Linux::GetDeviceId()
{
	return String::Empty;
}

std::string System_Linux::GetResourcePath(const char* fileName)
{
#if defined(MACOS)
	return std::string("MACOS/") + std::string(fileName);
#elif defined(LINUX)
	return std::string("Resources/") + std::string(fileName);
#else
#error
#endif
}

std::string System_Linux::GetDocumentPath(const char* fileName)
{
	return std::string(fileName);
}

std::string System_Linux::GetCountryCode()
{
	return "xx";
}

void System_Linux::SaveToAlbum(class Screenshot* ss, const char* name)
{
}

Vec3 System_Linux::GetTiltVector()
{
	return Vec3(0.0f, 0.0f, 1.0f);
}

Vec2F System_Linux::GetTiltDirectionXY()
{
	return Vec2F(0.0f, 0.0f);
}

bool System_Linux::IsIpad()
{
	return false;
}
