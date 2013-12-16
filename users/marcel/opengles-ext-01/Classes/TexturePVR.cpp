#import <QuartzCore/QuartzCore.h> // todo: use correct header file.
#include "TexturePVR.h"

#define PVR_TEXTURE_TYPE_MASK 0xff

const static char* g_PVR_Identifier = "PVR!";

typedef struct
{
	uint32_t headerLength;
	uint32_t height;
	uint32_t width;
	uint32_t numMipmaps;
	uint32_t flags;
	uint32_t dataLength;
	uint32_t bpp;
	uint32_t bitmaskRed;
	uint32_t bitmaskGreen;
	uint32_t bitmaskBlue;
	uint32_t bitmaskAlpha;
	uint32_t pvrTag;
	uint32_t numSurfs;
} PVR_Header;

TexturePVRLevel::TexturePVRLevel()
{
	Initialize(0, 0, 0, false, 0, 0);
}
		
TexturePVRLevel::TexturePVRLevel(
				int sx,
				int sy,
				int bpp,
				bool hasAlpha,
				uint8_t* data,
				int dataSize)
{
	Initialize(sx, sy, bpp, hasAlpha, data, dataSize);
}
		
void TexturePVRLevel::Initialize(
				int sx,
				int sy,
				int bpp,
				bool hasAlpha,
				uint8_t* data,
				int dataSize)
{
	m_Sx = sx;
	m_Sy = sy;
	m_Bpp = bpp;
	m_HasAlpha = hasAlpha;
	m_Data = data;
	m_DataSize = dataSize;
}
		
TexturePVRLevel::~TexturePVRLevel()
{
	if (m_Data)
	{
		delete[] m_Data;
		m_Data = 0;
	}
}

TexturePVR::TexturePVR()
{
	Initialize();
}

TexturePVR::~TexturePVR()
{
	for (int i = 0; i < m_Levels.size(); ++i)
	{
		delete m_Levels[i];
	}
	
	m_Levels.clear();
}

void TexturePVR::Initialize()
{
}

bool TexturePVR::Load(char* fileName)
{
	// uint8_t* bytes = 0; // todo: load data from file.
	
	return false;
}

bool TexturePVR::Load(uint8_t* bytes)
{	
	PVR_Header header = *(PVR_Header*)bytes;
	
	// Convert endianness of selected fields.
	
	header.pvrTag = CFSwapInt32LittleToHost(header.pvrTag);
	header.flags = CFSwapInt32LittleToHost(header.flags);
	header.bitmaskAlpha = CFSwapInt32LittleToHost(header.bitmaskAlpha);
	header.width = CFSwapInt32LittleToHost(header.width);
	header.height = CFSwapInt32LittleToHost(header.height);		
	header.dataLength = CFSwapInt32LittleToHost(header.dataLength);
	header.numMipmaps = CFSwapInt32LittleToHost(header.numMipmaps);
	
	// Check PVR identifier.
	
	if (g_PVR_Identifier[0] != ((header.pvrTag >>  0) & 0xff) ||
		g_PVR_Identifier[1] != ((header.pvrTag >>  8) & 0xff) ||
		g_PVR_Identifier[2] != ((header.pvrTag >> 16) & 0xff) ||
		g_PVR_Identifier[3] != ((header.pvrTag >> 24) & 0xff))
	{
		// todo: log.
		
		return false;
	}
	
	// Check format.
	
	uint32_t format = header.flags & PVR_TEXTURE_TYPE_MASK;
	
	if (format != PVR_TextureType_2BPP && format != PVR_TextureType_4BPP)
	{
		// todo: log.
		
		return false;
	}
	
	// Load MIP maps.
	
	m_Levels.clear();
	
	bytes += sizeof(PVR_Header);
	
	int sx = header.width;
	int sy = header.height;
	int bpp = 0;
	bool hasAlpha = header.bitmaskAlpha;
	
	if (format == PVR_TextureType_2BPP)
		bpp = 2;
	if (format == PVR_TextureType_4BPP)
		bpp = 4;
	
	// Calculate the data size for each texture level and respect the minimum number of blocks.
	
	int dataOffset = 0;
	int dataLength = header.dataLength;
	
	// todo: use for loop, use numMipmaps from header.
	while (dataOffset < dataLength)
	{
		int blockSize = 0;
		int blockCountX = 0;
		int blockCountY = 0;
		
		if (format == PVR_TextureType_2BPP)
		{
			blockSize = 8 * 4; // Pixel by pixel block size for 2bpp.
			blockCountX = sx / 8;
			blockCountY = sy / 4;
		}
		
		if (format == PVR_TextureType_4BPP)
		{
			blockSize = 4 * 4; // Pixel by pixel block size for 4bpp.
			blockCountX = sx / 4;
			blockCountY = sy / 4;
		}
		
		// Clamp to minimum number of blocks.
		
		if (blockCountX < 2)
			blockCountX = 2;
		if (blockCountY < 2)
			blockCountY = 2;
		
		// todo: Check if data is within memory bounds.
		
		int dataSize = blockCountX * blockCountY * ((blockSize  * bpp) / 8);
		
		uint8_t* data = new uint8_t[dataSize];
		memcpy(data, bytes + dataOffset, dataSize);
		
		TexturePVRLevel* level = new TexturePVRLevel(sx, sy, bpp, hasAlpha, data, dataSize);
		
		m_Levels.push_back(level);
		
		// Advance to next MIP map.
		
		dataOffset += dataSize;
		
		sx = sx >> 1;
		sy = sy >> 1;
		
		// fixme: should not really happen.. or should be handled differenly if it does.
		if (sx == 0)
			sx = 1;
		if (sy == 0)
			sy = 1;
	}
	
	// Sanity check: did we read all MIP maps?
	
	if (header.numMipmaps + 1 != m_Levels.size())
	{
		// todo: log.
		
		return false;
	}
	
	return true;
}

std::vector<TexturePVRLevel*> TexturePVR::Levels_get()
{
	return m_Levels;
}
