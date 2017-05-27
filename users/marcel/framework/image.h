#pragma once

// Copyright (C) 2013 Grannies Games - All rights reserved

class ImageData
{
public:
	ImageData();
	ImageData(int sx, int sy);
	~ImageData();
	
	int sx;
	int sy;

	struct Pixel
	{
		unsigned char r, g, b, a;
	};

	Pixel * imageData; // R8G8B8A8

	//

	Pixel * getLine(int y)
	{
		return &imageData[y * sx];
	}
	
	const Pixel * getLine(int y) const
	{
		return &imageData[y * sx];
	}
};

ImageData * loadImage(const char * filename);
ImageData * imagePremultiplyAlpha(const ImageData * image);
ImageData * imageFixAlphaFilter(const ImageData * image);
