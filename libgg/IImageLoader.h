#pragma once

#include <string>
#include "libgg_forward.h"

class IImageLoader
{
public:
	virtual ~IImageLoader();
	
	virtual void Load(Image& image, const std::string& fileName) = 0;
	virtual void Save(const Image& image, const std::string& fileName) = 0;
};
