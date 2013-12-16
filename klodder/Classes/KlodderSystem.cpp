#include "Exception.h"
#include "KlodderSystem.h"

System gSystem;

std::string System::GetResourcePath(const char* fileName)
{
	//return std::string("C:/gg-code/klodder/") + fileName;
	return fileName;
}

std::string System::GetDocumentPath(const char* fileName)
{
	return fileName;
}
	
bool System::PathExists(const char* path)
{
	throw ExceptionVA("not implemented: System::PathExist");
}

void System::CreatePath(const char* path)
{
	throw ExceptionVA("not implemented: System::CreatePath");
}

void System::DeletePath(const char* path)
{
	throw ExceptionVA("not implemented: System::DeletePath");
}

void System::SaveAsPng(const MacImage& image, const char* path)
{
	// todo: use FreeImage loader
}

MacImage* System::LoadImage(const char* path, bool flipY)
{
	throw ExceptionVA("not implemented: System::LoadImage");
}

#include "MacImage.h"

MacImage* System::Transform(MacImage& image, const BlitTransform& transform, int sx, int sy) const
{
	MacImage* result = new MacImage();

	result->Size_set(sx, sy, true);

	return result;
}
