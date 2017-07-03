/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

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
