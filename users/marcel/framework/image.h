#pragma once

// Copyright (C) 2013 Grannies Games - All rights reserved

#include <stdlib.h>

class ImageData
{
public:
	ImageData()
	{
		memset(this, 0, sizeof(ImageData));
	}
	
	~ImageData()
	{
		if (imageData)
		{
			free(imageData);
			imageData = 0;
			
			sx = sy = 0;
		}
	}
	
	int sx;
	int sy;

	struct Pixel
	{
		unsigned char r, g, b, a;
	};

	Pixel * imageData; // R8G8B8A8
};

ImageData * loadImage(const char * filename);
