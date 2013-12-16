#pragma once

#include "IImageLoader.h"

class ImageLoader_FreeImage : public IImageLoader
{
public:
	virtual ~ImageLoader_FreeImage()
	{
	}
	
	virtual void Load(Image& image, const std::string& fileName);
	virtual void Save(const Image& image, const std::string& fileName);
};
