#pragma once

#include <stdint.h>

struct OpenglTexture
{
	uint32_t id;
	int sx;
	int sy;
	int internalFormat;

	OpenglTexture();
	~OpenglTexture();

	void allocate(const int sx, const int sy, const int internalFormat);
	void free();
	
	bool isChanged(const int sx, const int sy, const int internalFormat) const;

	void setSwizzle(const int r, const int g, const int b, const int a);

	void upload(const void * src, const int srcAlignment, const int srcPitch, const int uploadFormat, const int uploadElementType);
};
