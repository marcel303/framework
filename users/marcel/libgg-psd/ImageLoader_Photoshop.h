#pragma once

#include "IImageLoader.h"

class ImageLoader_Photoshop : IImageLoader
{
public:
	void Load(Image& image, const std::string& fileName);
	void Save(const Image& image, const std::string& fileName);
	void SaveMultiLayer(Image** images, int imageCount, const std::string& fileName);
};
