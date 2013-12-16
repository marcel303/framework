#include "System_Psp.h"

System_Psp::~System_Psp()
{
}

void System_Psp::Vibrate()
{
}

void System_Psp::CheckNetworkConnectivity()
{
}

bool System_Psp::HasNetworkConnectivity_get()
{
	return false;
}

void System_Psp::HasNetworkConnectivity_set(bool value)
{
}

bool System_Psp::IsHacked()
{
	return false;
}

std::string System_Psp::GetDeviceId()
{
	return "";
}

std::string System_Psp::GetResourcePath(const char* fileName)
{
	//return std::string("/app_home/") + fileName;
	return std::string("host0:") + fileName;
}

std::string System_Psp::GetDocumentPath(const char* fileName)
{
	//return fileName;
	return std::string("host0:") + fileName;
}

std::string System_Psp::GetCountryCode()
{
	return "";
}

void System_Psp::SaveToAlbum(class Screenshot* ss, const char* name)
{
}

Vec3 System_Psp::GetTiltVector()
{
	return Vec3(0.0f, 0.0f, 1.0f);
}

Vec2F System_Psp::GetTiltDirectionXY()
{
	return Vec2F(0.0f, 0.0f);
}

bool System_Psp::IsIpad()
{
	return false;
}
