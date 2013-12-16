#pragma once

#include "IImageLoader.h"

class ImageLoader_Tiff : IImageLoader
{
public:
	void Load(Image& image, const std::string& fileName);
	void Save(const Image& image, const std::string& fileName);
};
