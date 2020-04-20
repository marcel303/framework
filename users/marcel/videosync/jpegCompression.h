#pragma once

struct JpegLoadData
{
	unsigned char * buffer;
	int bufferSize;
	bool flipY;
	
	int sx;
	int sy;
	
	JpegLoadData()
		: buffer(nullptr)
		, bufferSize(0)
		, flipY(false)
		, sx(0)
		, sy(0)
	{
	}
	
	~JpegLoadData()
	{
		free();
	}
	
	void disown()
	{
		buffer = 0;
		bufferSize = 0;
		sx = 0;
		sy = 0;
	}
	
	void free()
	{
		delete [] buffer;
		buffer = nullptr;
		bufferSize = 0;
	}
};

bool saveImage_turbojpeg(const void * srcBuffer, const int srcBufferSize, const int srcSx, const int srcSy, void *& dstBuffer, int & dstBufferSize);
bool loadImage_turbojpeg(const void * buffer, const int bufferSize, JpegLoadData & data);
