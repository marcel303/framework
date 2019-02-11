#pragma once

#include "libgg_forward.h"
#include "MemoryStream.h"
#include "PsdTypes.h"

class PsdChannel
{
public:
	PsdChannel();
	~PsdChannel();
	
	void ReadHeader(StreamReader& reader);
	void ReadChannelHeader(Stream* stream, uint16_t compression, int sy);
	void ReadChannelData(Stream* stream, uint16_t compression, PsdImageMode mode, int sx, int sy);
	
	void WritePrepare(uint16_t compression, uint16_t mode, bool merged);
//private:
public:
	void WriteChannelHeader(Stream* stream, uint16_t compression);
	void WriteChannelData(Stream* stream);
public:
	void WriteHeader(Stream* stream);
	void Write(Stream* stream);
private:
	void EncodeBytes(uint16_t compression, uint16_t mode);
public:
	void Setup(int sx, int sy, int bpp, uint16_t usage);
	void Setup(const Image& image, uint16_t usage);

	uint8_t* mBytes;
	int mSx;
	int mSy;
	uint16_t mUsage;
	uint32_t mLength;

	std::vector<int> mRleHeader;
	MemoryStream mRleData;
	MemoryStream mChannelData;
};
