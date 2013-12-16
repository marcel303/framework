#pragma once

#include "libgg_forward.h"
#include "Types.h"

#define DDSD_CAPS        0x1 // required
#define DDSD_HEIGHT      0x2 //required
#define DDSD_WIDTH       0x4 // required
#define DDSD_PITCH       0x8
#define DDSD_PIXELFORMAT 0x1000 // required
#define DDSD_MIPMAPCOUNT 0x20000
#define DDSD_LINEARSIZE  0x80000
#define DDSD_DEPTH       0x800000

#define DDPF_ALPHAPIXELS 0x1
#define DDPF_ALPHA       0x2
#define DDPF_FOURCC      0x4 // set when DDS file contains compressed data
#define DDPF_RGB         0x40
#define DDPF_YUV         0x200
#define DDPF_LUMINANCE   0x20000

#define DDSCAPS_COMPLEX 0x8
#define DDSCAPS_MIPMAP  0x400000
#define DDSCAPS_TEXTURE 0x1000 // required

class DDS_PIXELFORMAT 
{
public:
	uint32_t dwSize; // must be 32
	uint32_t dwFlags;
	char dwFourCC[4];
	uint32_t dwRGBBitCount;
	uint32_t dwRBitMask;
	uint32_t dwGBitMask;
	uint32_t dwBBitMask;
	uint32_t dwABitMask;

	void Load(StreamReader& reader);
	void Save(StreamWriter& writer);
};

class DDS_HEADER
{
public:
	char magic[4]; // "DDS ";

	uint32_t           dwSize; // must be 124
	uint32_t           dwFlags;
	uint32_t           dwHeight;
	uint32_t           dwWidth;
	uint32_t           dwPitchOrLinearSize;
	uint32_t           dwDepth;
	uint32_t           dwMipMapCount;
	uint32_t           dwReserved1[11];
	DDS_PIXELFORMAT    ddspf;
	uint32_t           dwCaps;
	uint32_t           dwCaps2;
	uint32_t           dwCaps3;
	uint32_t           dwCaps4;
	uint32_t           dwReserved2;

	void Load(StreamReader& reader);
	void Save(StreamWriter& writer);
};

class DdsLoader
{
public:
	void LoadHeader(StreamReader& reader);
	void SeekToData(int fileStartPosition, StreamReader& reader);
	bool IsFourCC(const char* name);

	DDS_HEADER mHeader;
};

class DdsWriter
{
public:
	void WriteHeader(StreamWriter& writer);

	DDS_HEADER mHeader;
};
