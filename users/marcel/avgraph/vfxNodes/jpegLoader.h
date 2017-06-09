#pragma once

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

// decompresses the JPEG data contained in buffer of bufferSize bytes, and stores the decoded image in data. to speed up allocations, it will try to re-use data.buffer, when allocated
// + doesn't require any allocations when data.buffer is already allocated
// + doesn't require any IO operations to occur
bool loadImage_turbojpeg(const void * buffer, const int bufferSize, JpegLoadData & data);

// loads JPEG data from filename and decompresses it into data.buffer. fileBuffer and fileBufferSize specify the buffer to read the file contents into
// + doesn't require any allocations when data.buffer is already allocated
bool loadImage_turbojpeg(const char * filename, JpegLoadData & data, void * fileBuffer, int fileBufferSize);

// loads JPEG data from filename and decompresses it into data.buffer
bool loadImage_turbojpeg(const char * filename, JpegLoadData & data);

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
