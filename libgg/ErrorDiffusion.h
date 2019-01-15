#pragma once

#include <stdint.h>

// error diffusion helper classes

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
	SampleGetter(const void* data, int stride, int size)
	{
		mData = (const uint8_t*)data;
		mStride = stride;
		mSize = size;
	}

	virtual int Get(int index)
	{
		switch (mSize)
		{
		case 1:
			return *(const uint8_t*)(mData + index * mStride);
		case 2:
			return *(const uint16_t*)(mData + index * mStride);
		default:
			//throw ExceptionNA();
			return 0;
		}
	}

	const uint8_t* mData;
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
