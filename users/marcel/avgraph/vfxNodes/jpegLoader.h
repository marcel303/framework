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

#if ENABLE_TURBOJPEG

struct JpegLoadData
{
	// optional parameters when loading images
	
	bool flipY;
	
	// loaded image data
	
	unsigned char * buffer;
	int bufferSize;
	
	int sx;
	int sy;
	
	JpegLoadData();
	~JpegLoadData();
	
	void disownBuffer();
	void free();
};

// decompresses the JPEG data contained in buffer of bufferSize bytes, and stores the decoded image in dstBuffer
// + doesn't require any allocations when data.buffer is already allocated
// + doesn't require any IO operations to occur
bool loadImage_turbojpeg(const void * buffer, const int bufferSize, const void * dstBuffer, const int dstBufferSize, const bool decodeGrayscale = false);

// decompresses the JPEG data contained in buffer of bufferSize bytes, and stores the decoded image in data. to speed up allocations, it will try to re-use data.buffer, when allocated
// + doesn't require any allocations when data.buffer is already allocated
// + doesn't require any IO operations to occur
bool loadImage_turbojpeg(const void * buffer, const int bufferSize, JpegLoadData & data, const bool decodeGrayscale = false);

// loads JPEG data from filename and decompresses it into data.buffer. fileBuffer and fileBufferSize specify the buffer to read the file contents into
// + doesn't require any allocations when data.buffer is already allocated
bool loadImage_turbojpeg(const char * filename, JpegLoadData & data, void * fileBuffer, int fileBufferSize, const bool decodeGrayscale = false);

// loads JPEG data from filename and decompresses it into data.buffer
bool loadImage_turbojpeg(const char * filename, JpegLoadData & data, const bool decodeGrayscale = false);

// compresses the image data contained in srcBuffer of srcBufferSize bytes, and stores the encoded image in dstBuffer
// to speed up allocations, it ASSUMES dstBuffer is large enough to fit the encoded JPEG data
// + doesn't require any allocations
// + doesn't require any IO operations to occur
bool saveImage_turbojpeg(const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, const bool srcIsColor, const int dstQualityLevel, void *& dstBuffer, int & dstBufferSize);

// compresses the image data contained in srcBuffer of srcBufferSize bytes, and stores the encoded image in dstBuffer
// to speed up allocations, it ASSUMES dstBuffer is large enough to fit the encoded JPEG data
// + doesn't require any allocations
bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, const bool srcIsColor, const int dstQualityLevel, void * saveBuffer, int saveBufferSize);

// compresses the image data contained in srcBuffer of srcBufferSize bytes, and stores the encoded image in dstBuffer
bool saveImage_turbojpeg(const char * filename, const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, const bool srcIsColor, const int dstQualityLevel);

#endif
