#include "ByteString.h"
#include "DdsLoader.h"
#include "FileStream.h"
#include "Image.h"
#include "ImageLoader_Tga.h"
#include "MemoryStream.h"
#include "Parse.h"
#include "StreamReader.h"
#include "StreamWriter.h"

enum TextureFormat
{
	SCEGU_PFIDX4,
	SCEGU_PFIDX8,
	SCEGU_PF5650,
	SCEGU_PF5551,
	SCEGU_PF4444,
	SCEGU_PFIDX16,
	SCEGU_PF8888,
	SCEGU_PFIDX32
};

static void FixDDS(const char* src, const char* dst);
static void FixTGA(const char* src, const char* dst);

static void FixDXT1(uint8_t* bytes, int byteCount);
static void FixDXT3or5(uint8_t* bytes, int byteCount);
static void FixTexture(uint8_t* pDest, uint8_t* pSrc, int width, int height, TextureFormat format);

int main(int argc, char* argv[])
{
	try
	{
		if (argc != 4)
		{
			printf("usage: psp_fixdds <mode> <source> <destination>\n");
			printf("<mode>: dds = Fix DDS file\n");
			printf("<mode>: tga = Fix TGA file\n");
			printf("<source>: DDS file containing DXT1/DXT3/DXT5 data\n");
			printf("<destination>: DDS file optimized for PSP\n");
			return -1;
		}

		// parse arguments

		int mode = -1;
		if (!strcmp(argv[1], "dds"))
			mode = 0;
		else if (!strcmp(argv[1], "tga"))
			mode = 1;
		const char* src = argv[2];
		const char* dst = argv[3];

		// validate arguments
		
		if (mode == -1)
			throw ExceptionVA("unknown mode: %d", mode);
		
		// invoke DDS or TGA fix
		
		switch (mode)
		{
		case 0:
			FixDDS(src, dst);
			break;
		case 1:
			FixTGA(src, dst);
			break;
		default:
			throw ExceptionNA();
		}
		
		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s", e.what());
		return -1;
	}
}

static void FixDDS(const char* src, const char* dst)
{
	// open source stream

	FileStream srcStream;

	srcStream.Open(src, OpenMode_Read);

	// load DDS file

	DdsLoader loader;
	
	StreamReader reader(&srcStream, false);
	
	loader.LoadHeader(reader);

	MemoryStream dxtStream;
	
	if (loader.IsFourCC("DXT1"))
	{
		// load DXT1 data
		
		const int byteCount = loader.mHeader.dwWidth * loader.mHeader.dwHeight * 4 / 8;
		
		uint8_t* bytes = new uint8_t[byteCount];

		if (srcStream.Read(bytes, byteCount) != byteCount)
			throw ExceptionVA("unable to read DXT data");
		
		// fix DXT1 data
		
		FixDXT1(bytes, byteCount);
		
		dxtStream.Write(bytes, byteCount);
		
		delete[] bytes;
	}
	else if (loader.IsFourCC("DXT3") || loader.IsFourCC("DXT5"))
	{
		// load DXT3/DXT5 data
		
		const int byteCount = loader.mHeader.dwWidth * loader.mHeader.dwHeight * 8 / 8;
		
		uint8_t* bytes = new uint8_t[byteCount];

		if (srcStream.Read(bytes, byteCount) != byteCount)
			throw ExceptionVA("unable to read DXT data");
			
		// fix DXT3/DXT5 data
		
		FixDXT3or5(bytes, byteCount);
		//FixDXT1(bytes, byteCount);
		
		dxtStream.Write(bytes, byteCount);
		
		delete[] bytes;
	}

	// close src stream
	
	srcStream.Close();
	
	// open destination stream
	
	FileStream dstStream;

	dstStream.Open(dst, OpenMode_Write);
	
	// todo: write DDS
	
	DdsWriter ddsWriter;
	
	ddsWriter.mHeader = loader.mHeader;
	
	StreamWriter writer(&dstStream, false);
	
	ddsWriter.WriteHeader(writer);
	
	dxtStream.Seek(0, SeekMode_Begin);
	
	dxtStream.StreamTo(&dstStream, dxtStream.Length_get());

	// close dst stream

	dstStream.Close();
}

static void FixTGA(const char* src, const char* dst)
{
	ImageLoader_Tga loaderSrc;
	
	Image image;
	
	loaderSrc.Load(image, src);
	
	ByteString bytesSrc(image.m_Sx * image.m_Sy * 4);
	
	for (int y = 0; y < image.m_Sy; ++y)
	{
		ImagePixel* srcLine = image.GetLine(y);
		uint8_t* dstLine = &bytesSrc.m_Bytes[y * image.m_Sx * 4];
		
		for (int x = 0; x < image.m_Sx; ++x)
		{
			*dstLine++ = (uint8_t)srcLine->r;
			*dstLine++ = (uint8_t)srcLine->g;
			*dstLine++ = (uint8_t)srcLine->b;
			*dstLine++ = (uint8_t)srcLine->a;
			
			srcLine++;
		}
	}
	
	ByteString bytesDst(image.m_Sx * image.m_Sy * 4);
	
	//memcpy(&bytesDst.m_Bytes[0], &bytesSrc.m_Bytes[0], image.m_Sx * image.m_Sy * 4);
	FixTexture(&bytesDst.m_Bytes[0], &bytesSrc.m_Bytes[0], image.m_Sx, image.m_Sy, SCEGU_PF8888);
	
	for (int y = 0; y < image.m_Sy; ++y)
	{
		uint8_t* srcLine = &bytesDst.m_Bytes[y * image.m_Sx * 4];
		ImagePixel* dstLine = image.GetLine(y);
		
		for (int x = 0; x < image.m_Sx; ++x)
		{
			dstLine->r = *srcLine++;
			dstLine->g = *srcLine++;
			dstLine->b = *srcLine++;
			dstLine->a = *srcLine++;
			
			dstLine++;
		}
	}
	
	ImageLoader_Tga loaderDst;
	
	loaderDst.SaveColorDepth = 32;
	
	loaderDst.Save(image, dst);
}

static void FixDXT1(uint8_t* bytes, int byteCount)
{
	if (byteCount <= 0)
		throw ExceptionVA("invalid byte count: %d", byteCount);
	if (bytes == 0)
		throw ExceptionVA("byte array not set");

	// swap 32 elements in the entire byte array

	const int blockSize = 8;
	const int blockCount = byteCount / blockSize;

	uint32_t* ptr = (uint32_t*)bytes;

	for (int i = 0; i < blockCount; ++i)
	{
		uint32_t temp = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = temp;

		ptr += 2;
	}
}

static void FixDXT3or5(uint8_t* bytes, int byteCount)
{
	if (byteCount <= 0)
		throw ExceptionVA("invalid byte count: %d", byteCount);
	if (bytes == 0)
		throw ExceptionVA("byte array not set");

	// swap 64 elements in the entire byte array

	const int blockSize = 16;
	const int blockCount = byteCount / blockSize;

	uint64_t* ptr = (uint64_t*)bytes;

	for (int i = 0; i < blockCount; ++i)
	{
		uint64_t temp = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = temp;

		ptr += 2;
	}
}

// code snipped gratefully taken from the PSP devnet forums
static void FixTexture(uint8_t* pDest, uint8_t* pSrc, int width, int height, TextureFormat format)
{
	int pixelsize = 0;

	switch (format)
	{
	case SCEGU_PFIDX4:
		pixelsize = 1;
		width >>= 1;
		break;

	case SCEGU_PFIDX8:
		pixelsize = 1;
		break;

	case SCEGU_PF5650:
	case SCEGU_PF5551:
	case SCEGU_PF4444:
	case SCEGU_PFIDX16:
		pixelsize = 2;
		break;

	case SCEGU_PF8888:
	case SCEGU_PFIDX32:
		pixelsize = 4;
		break;
		
	default:
		throw ExceptionVA("unknown texture format: %d", format);
	}

	const int xchunks = (width * pixelsize) >> 4;
	const int ychunks = height >> 3;
	
	for(int y = 0; y < ychunks; y ++)
	{	
		for(int x = 0; x < xchunks; x ++)
		{
			uint8_t* pCopy = pSrc + (x * 16) + (y * xchunks * 16 * 8);
			
			for(int h = 0; h < 8; h ++)
			{
				for(int bytes = 16; bytes; --bytes)
					*pDest++ = *pCopy++;

				pCopy += (xchunks - 1) * 16;
			}
		}
	}
}
