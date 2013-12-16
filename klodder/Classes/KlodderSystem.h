#pragma once

#include <string>
#include "klodder_forward.h"

class System
{
public:
	std::string GetResourcePath(const char* fileName);
	std::string GetDocumentPath(const char* fileName);
	
	bool PathExists(const char* path);
	void CreatePath(const char* path);
	void DeletePath(const char* path);
	
	void SaveAsPng(const MacImage& image, const char* path);
	void SaveAsJpeg(const MacImage& image, const char* path);
	MacImage* LoadImage(const char* path, bool flipY);
	MacImage* Transform(MacImage& image, const BlitTransform& transform, int sx, int sy) const;
	
	bool IsLiteVersion();
};

extern System gSystem;
