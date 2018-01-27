#pragma once

typedef short ImageComponent;

class ImagePixel
{
public:
	inline ImagePixel()
	{
		r = g = b = a = 0;
	}

	union
	{
		struct
		{
			ImageComponent r;
			ImageComponent g;
			ImageComponent b;
			ImageComponent a;
		};
		ImageComponent rgba[4];
	};
};

class Image
{
public:
	Image();
	Image(const Image& image);
	~Image();

	void SetSize(int sx, int sy);

	ImagePixel* GetLine(int y);
	const ImagePixel* GetLine(int y) const;
	ImagePixel GetPixel(int x, int y) const;
	void SetPixel(int x, int y, ImagePixel c);
	void Blit(Image* dst, int dstX, int dstY) const;
	void Blit(Image* dst, int srcX, int srcY, int srcSx, int srcSy, int dstX, int dstY) const;
	void RectFill(int x1, int y1, int x2, int y2, ImagePixel c);
	void CopyFrom(const Image& image);
	void DownscaleTo(Image& image, int scale) const;
	void DemultiplyAlpha();

	Image& operator=(const Image& image);

	int m_Sx;
	int m_Sy;
	ImagePixel* m_Pixels;
};

// error diffusion code
// todo: move to CPP

class IErrorDiffusionGetter
{
public:
	virtual ~IErrorDiffusionGetter()
	{
	}
	
	virtual int Get(int index) = 0;
};

class IErrorDiffusionSetter
{
public:
	virtual ~IErrorDiffusionSetter()
	{
	}
	
	virtual void Set(int index, int value) = 0;
};

typedef int (*ErrorDiffusionConverter)(int value);

class ErrorDiffusion
{
public:
	// note: src may equal dst
	void Apply(
		IErrorDiffusionGetter* src,
		IErrorDiffusionSetter* dst,
		int sampleCount,
		ErrorDiffusionConverter convertForward,
		ErrorDiffusionConverter convertBackward) const
	{
		int error = 0;

		for (int i = 0; i < sampleCount; ++i)
		{
			const int v1 = src->Get(i) + error;
			const int v2 = convertForward(v1);
			const int v3 = convertBackward(v2);

			error = v3 - v1;
			
			dst->Set(i, v2);
		}
	}
};

#include "Types.h"

class SampleGetter : public IErrorDiffusionGetter
{
public:
	SampleGetter(void* data, int stride, int size)
	{
		mData = (uint8_t*)data;
		mStride = stride;
		mSize = size;
	}

	virtual int Get(int index)
	{
		switch (mSize)
		{
		case 1:
			return *(uint8_t*)(mData + index * mStride);
		case 2:
			return *(uint16_t*)(mData + index * mStride);
		default:
			//throw ExceptionNA();
			return 0;
		}
	}

	uint8_t* mData;
	int mStride;
	int mSize;
};

class SampleSetter : public IErrorDiffusionSetter
{
public:
	SampleSetter(void* data, int stride, int size)
	{
		mData = (uint8_t*)data;
		mStride = stride;
		mSize = size;
	}

	virtual void Set(int index, int value)
	{
		switch (mSize)
		{
		case 1:
			if (value < 0)
				value = 0;
			else if (value > 255)
				value = 255;
			*(uint8_t*)(mData + index * mStride) = (uint8_t)value;
			break;
		case 2:
			if (value < 0)
				value = 0;
			else if (value > 65535)
				value = 65535;
			*(uint16_t*)(mData + index * mStride) = (uint16_t)value;
			break;
		//default:
			//throw ExceptionNA();
		}
	}

	uint8_t* mData;
	int mStride;
	int mSize;
};
