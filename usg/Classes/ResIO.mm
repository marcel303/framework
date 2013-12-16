/*#ifdef IPHONEOS
	#include <AudioToolbox/AudioFormat.h>
	#include <AudioToolbox/AudioToolbox.h>
	#include <CoreAudio/CoreAudioTypes.h>
#endif*/
#ifdef PSP
	#include <libdeflt.h>
#endif
#include "ArrayStream.h"
#include "Atlas.h"
#include "Benchmark.h"
#include "BinaryData.h"
#include "CompiledPackage.h"
#include "ConfigState.h"
#include "DdsLoader.h"
#include "Exception.h"
#include "FileStream.h"
#include "FixedSizeArray.h"
#include "FontMap.h"
#include "Heap.h"
#include "ImageLoader_Tga.h"
#include "Log.h"
#include "MemoryStream.h"
#include "Path.h"
#include "Res.h"
#include "ResIO.h"
#include "ResMgr.h"
#include "SoundEffect.h"
#include "StreamReader.h"
#include "StringBuilder.h"
#include "System.h"
#include "TextureAtlas.h"
#include "TextureDDS.h"
#include "TexturePVR.h"
#include "TextureRGBA.h"
#include "VectorComposition.h"
#include "VectorShape.h"

#include "GameState.h" // graphics hacks, screen scale

#if defined(WIN32) || defined(LINUX)// || defined(MACOS)
	#include "ImageLoader_FreeImage.h"
#endif

template <typename T>
class AutoPtr
{
public:
	AutoPtr(T* obj) : m_obj(obj) { }
	~AutoPtr() { delete m_obj; }
	inline T* Get() { return m_obj; }
	
private:
	T* m_obj;
};

#if ENABLE_RESOURCE_OVERRIDES
// when enabled, this allows individual resources to be overriden by placing
// them inside the data directory
static Stream* OpenResourceFromOverride(const char* fileName)
{
	// todo: apply extension overrides
	std::string fullName = g_System.GetResourcePath(fileName);
	if (Path::GetExtension(fullName) == "sfx")
		fullName = Path::ReplaceExtension(fullName, "wav");
	if (FileStream::Exists(fullName.c_str()))
	{
		FileStream* fileStream = new FileStream;
		fileStream->Open(fullName.c_str(), OpenMode_Read);
		return fileStream;
	}
	else
	{
		return 0;
	}
}
#else
static Stream* OpenResourceFromOverride(const char* fileName)
{
	return 0;
}
#endif

#if defined(PSP) || defined(WIN32) || defined(IPHONEOS) || defined(MACOS) || defined(BBOS)
// when enabled, this enables files being read from the package file
static CompiledPackage* sPackage = 0;
static void PackageFree()
{
	delete sPackage;
	sPackage = 0;
}
static void PackageLoad(const char* fileName)
{
	if (sPackage != 0)
		return;
	std::string fullFileName = g_System.GetResourcePath(fileName);
	sPackage = new CompiledPackage();
	FileStream stream(fullFileName.c_str(), OpenMode_Read);
	MemoryStream* memStream = new MemoryStream();
	memStream->StreamFrom(&stream, stream.Length_get());
	memStream->Seek(0, SeekMode_Begin);
	sPackage->Load(memStream, true);
	atexit(PackageFree);
}
static Stream* OpenResourceFromPackage(const char* fileName)
{
	PackageLoad("resources.pkg");
	if (sPackage->ContainsFile(fileName))
	{
		LOG_DBG("(cached: %s)", fileName);
		SubStream* subStream = new SubStream(0, false, 0, 0);
		*subStream = sPackage->Open(fileName);
		return subStream;
	}
	else
	{
		return 0;
	}
}
#else
static Stream* OpenResourceFromPackage(const char* fileName)
{
	return 0;
}
#endif

static Stream* OpenResourceFromFile(const char* fileName)
{
#if 1
	// stream the entire file contents to a memory stream for faster reads
	FileStream fileStream;
	fileStream.Open(g_System.GetResourcePath(fileName).c_str(), OpenMode_Read);
	MemoryStream* memStream = new MemoryStream();
	StreamExtensions::StreamTo(&fileStream, memStream, 1024 * 64);
	memStream->Seek(0, SeekMode_Begin);
	return memStream;
#else
	FileStream* fileStream = new FileStream();
	fileStream->Open(g_System.GetResourcePath(fileName).c_str(), OpenMode_Read);
	return fileStream;
#endif
}

static Stream* QuickLoad(const char* fileName)
{
	Stream* stream = 0;
	if (stream == 0)
		stream = OpenResourceFromOverride(fileName);
	if (stream == 0)
		stream = OpenResourceFromPackage(fileName);
	if (stream == 0)
		stream = OpenResourceFromFile(fileName);
	
	if (stream == 0)
		throw ExceptionVA("failed to open resource: %s", fileName);
	
	return stream;
}

namespace ResIO
{
	// todo: reading from resource is assumed here.. is this alright?
	// todo: reading from Documents directory? (eg saved images, etc..)

#ifdef IPHONEOS
	static NSString* MakeBundleFileName(NSString* fileName);
//	static bool ImageToBytes(UIImage* image, int* out_Sx, int* out_Sy, uint8_t** out_Bytes);
	//static uint32_t AudioFile_GetSize(AudioFileID id);
	//static AudioStreamBasicDescription AudioFile_GetDescription(AudioFileID id);
	//static void LoadAudioFile(const char* fileName, int* out_ByteCount, uint8_t** out_Bytes, AudioStreamBasicDescription* out_Desc);
#endif

#if defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(PSP)
	static void* LoadTexture_PNG_FreeImage(const char* fileName);
#endif

#if defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(PSP) || defined(IPHONEOS) || defined(BBOS)
	static SoundEffect* LoadSound_Wave(const char* fileName);
#endif

#if defined(PSP)
	static TextureRGBA* CreateRandomTextureRGBA(int sx, int sy);
#endif

	//
	
	void* LoadBinaryData(const char* fileName)
	{
		FileStream stream;
		
		BinaryData* data = new BinaryData();
		
		try
		{
			stream.Open(g_System.GetResourcePath(fileName).c_str(), OpenMode_Read);
			
			data->Load(&stream);
		}
		catch (Exception& e)
		{
			LOG(LogLevel_Error, e.what());
			
			delete data;
			data = 0;
			
			throw e;
		}
		
		return data;
	}
	
	void* LoadFont(const char* fileName)
	{
		FontMap* font = new FontMap();
		
		try
		{
			AutoPtr<Stream> streamTmp(QuickLoad(fileName));
			Stream* stream = streamTmp.Get();
			
			font->m_Font.Load(stream);
		}
		catch (Exception& e)
		{
			delete font;
			font = 0;
			
			throw e;
		}
		
		return font;
	}
	
	static int GetDdsByteCount(DdsLoader& loader)
	{
		if (loader.IsFourCC("DXT1"))
		{
			return loader.mHeader.dwWidth * loader.mHeader.dwHeight * 4 / 8; // 4 bits per pixel
		}
		else if (loader.IsFourCC("DXT3"))
		{
			return loader.mHeader.dwWidth * loader.mHeader.dwHeight * 8 / 8; // 8 bits per pixel
		}
		else if (loader.IsFourCC("DXT5"))
		{
			return loader.mHeader.dwWidth * loader.mHeader.dwHeight * 8 / 8; // 8 bits per pixel
		}
		else
		{
			throw ExceptionVA("cannot determine DDS data size. unknown FOURCC");
		}
	}

	void* LoadTexture_DDS(const char* fileName)
	{
		Benchmark bm("loading DDS");

#if defined(PSP) && 1
		StringBuilder<64> sb;
		sb.AppendFormat("%s.gz", fileName);
		FileStream tempStream(g_System.GetDocumentPath(sb.ToString()).c_str(), OpenMode_Read);
		StreamReader tempReader(&tempStream, false);
		uint8_t* srcBytes = tempReader.ReadAllBytes();

		FixedSizeArray<512 * 512 + 1024> dstArray;  // compensate for 512x512x8(1 byte per pixel) DDS texture data + header

		uint32_t crc32;
		int gzipResult;

		gzipResult = sceGzipDecompress(dstArray.Bytes, dstArray.Length, srcBytes, &crc32);

		delete[] srcBytes;
		srcBytes = 0;

		if (gzipResult < 0)
		{
			throw ExceptionVA("failed to decompress GZIP: %08x: %s", gzipResult, fileName);
		}

		ArrayStream stream(dstArray.Bytes, dstArray.Length);
#else
		FileStream stream;
		
		stream.Open(g_System.GetResourcePath(fileName).c_str(), OpenMode_Read);
#endif
		
		StreamReader reader(&stream, false);
		
		DdsLoader loader;
		
		loader.LoadHeader(reader);
		
		const int byteCount = GetDdsByteCount(loader);

		uint8_t* bytes = new uint8_t[byteCount];

		if (stream.Read(bytes, byteCount) != byteCount)
			throw ExceptionVA("unable to read DDS data");

		TextureDDSType type = TextureDDSType_DXT1;

		if (loader.IsFourCC("DXT1"))
			type = TextureDDSType_DXT1;
		else if (loader.IsFourCC("DXT3"))
			type = TextureDDSType_DXT3;
		else if (loader.IsFourCC("DXT5"))
			type = TextureDDSType_DXT5;
		else
		{
#ifndef DEPLOYMENT
			throw ExceptionVA("unknown FourCC");
#endif
		}

		return new TextureDDS(type, loader.mHeader.dwWidth, loader.mHeader.dwHeight, loader.mHeader.ddspf.dwFourCC, bytes, true, byteCount);
	}

	static inline uint32_t SafeSample(uint32_t* pixels, int sx, int sy, int x, int y)
	{
		if (x < 0) x = 0; else if (x >= sx) x = sx - 1;
		if (y < 0) y = 0; else if (y >= sy) y = sy - 1;
		return pixels[x + y * sx];
	}
	
	static inline int SafeSampleValue(uint32_t* pixels, int sx, int sy, int x, int y)
	{
		uint32_t c = SafeSample(pixels, sx, sy, x, y);
		
		uint8_t* rgba = (uint8_t*)&c;
		
		return (rgba[0] + rgba[1] + rgba[2]) / 3;
	}
	
	void* LoadTexture_TGA(const char* fileName)
	{
		Benchmark bm("loading TGA");
		
#if defined(PSP) && 1
		StringBuilder<64> sb;
		sb.AppendFormat("%s.gz", fileName);
		FileStream tempStream(g_System.GetDocumentPath(sb.ToString()).c_str(), OpenMode_Read);
		StreamReader tempReader(&tempStream, false);
		uint8_t* srcBytes = tempReader.ReadAllBytes();

		FixedSizeArray<512 * 512 * 4 + 1024> dstArray;  // compensate for 512x512x32 texture data + header

		uint32_t crc32;
		int gzipResult;

		gzipResult = sceGzipDecompress(dstArray.Bytes, dstArray.Length, srcBytes, &crc32);

		delete[] srcBytes;
		srcBytes = 0;

		if (gzipResult < 0)
		{
			throw ExceptionVA("failed to decompress GZIP: %08x: %s", gzipResult, fileName);
		}

		ArrayStream stream2(dstArray.Bytes, dstArray.Length);
		Stream* stream = &stream2;
#else
		AutoPtr<Stream> streamTmp(QuickLoad(fileName));
		Stream* stream = streamTmp.Get();
#endif

		StreamReader reader(stream, false);
		
		TgaHeader header;
		
		header.Load(stream);
		
		TgaLoader loader;
		
		uint8_t* bytes;
		
		loader.LoadData(stream, header, &bytes);
	
		// resource hacks!
		
		if (ConfigGetIntEx("addonColorInvert", 0))
		{
			uint32_t pixelCount = header.sx * header.sy;
			
			for (uint32_t i = 0; i < pixelCount; ++i)
			{
				bytes[i * 4 + 0] = 255 - bytes[i * 4 + 0];
				bytes[i * 4 + 1] = 255 - bytes[i * 4 + 1];
				bytes[i * 4 + 2] = 255 - bytes[i * 4 + 2];
			}
		}

		if (ConfigGetIntEx("addonColorWhites", 0))
		{
			uint32_t pixelCount = header.sx * header.sy;
			
			for (uint32_t i = 0; i < pixelCount; ++i)
			{
				uint32_t v1 = (bytes[i * 4 + 0] + bytes[i * 4 + 0] * 2 + bytes[i * 4 + 0]) >> 2;
				uint32_t v2 = 32;
				uint32_t v3 = 256 - v2;
				uint32_t v4 = (v1 * v2 + bytes[i * 4 + 3] * v3) >> 8;
				bytes[i * 4 + 0] = v4;
				bytes[i * 4 + 1] = v4;
				bytes[i * 4 + 2] = v4;
			}
		}
		
		if (ConfigGetIntEx("addonEmboss", 0))
		{
			uint32_t pixelCount = header.sx * header.sy;
			uint32_t* pixels = (uint32_t*)bytes;
			
			uint8_t* bytes2 = new uint8_t[pixelCount * 4];
			
			for (int y = 0; y < header.sy; ++y)
			{
				uint8_t* srcLine = bytes + y * header.sx * 4;
				uint8_t* dstLine = bytes2 + y * header.sx * 4;
				
				for (int x = 0; x < header.sx; ++x)
				{
					int x1 = SafeSampleValue(pixels, header.sx, header.sy, x - 1, y);
					int x2 = SafeSampleValue(pixels, header.sx, header.sy, x + 1, y);
					int y1 = SafeSampleValue(pixels, header.sx, header.sy, x, y - 1);
					int y2 = SafeSampleValue(pixels, header.sx, header.sy, x, y + 1);
					
					int dx = x2 - x1;
					int dy = y2 - y1;
					
					// -510..510
					int v = 255 + dx + 255 + dy;
					
					if (g_GameState->m_ScreenScale >= 2)
						v = (v * v) >> 11;
					else
						v = v >> 2;
					
					if (v > 255)
						v = 255;
					
					*dstLine++ = v;
					*dstLine++ = v;
					*dstLine++ = v;
					*dstLine++ = srcLine[3];
					
					srcLine += 4;
				}
			}
			
			memcpy(bytes, bytes2, pixelCount * 4);
			
			delete[] bytes2;
			bytes2 = 0;
		}

		if (0 && ConfigGetIntEx("addonEdgeGlow", 0))
		{
			uint32_t pixelCount = header.sx * header.sy;
			uint32_t* pixels = (uint32_t*)bytes;
			
			uint8_t* bytes2 = new uint8_t[pixelCount * 4];
			
			for (int y = 0; y < header.sy; ++y)
			{
				uint8_t* srcLine = bytes + y * header.sx * 4;
				uint8_t* dstLine = bytes2 + y * header.sx * 4;
				
				for (int x = 0; x < header.sx; ++x)
				{
#if 0
					int x1 = SafeSampleValue(pixels, header.sx, header.sy, x - 1, y);
					int x2 = SafeSampleValue(pixels, header.sx, header.sy, x + 1, y);
					int y1 = SafeSampleValue(pixels, header.sx, header.sy, x, y - 1);
					int y2 = SafeSampleValue(pixels, header.sx, header.sy, x, y + 1);
					
					int dx = x2 - x1;
					int dy = y2 - y1;
					
					// -510..510
					int v = (Calc::Abs(dx) + Calc::Abs(dy)) / 2;
					
					*dstLine++ = v;
					*dstLine++ = v;
					*dstLine++ = v;
					*dstLine++ = srcLine[3];
					
					srcLine += 4;
#else
					int v[9];
					int idx = 0;
					for (int dx = -1; dx <= +1; ++dx)
					{
						for (int dy = -1; dy <= +1; ++dy)
						{
							v[idx++] = SafeSampleValue(pixels, header.sx, header.sy, x + dx, y + dy);
						}
					}
					
					int d =
						Calc::Abs(v[0] - v[4]) * 1 +
						Calc::Abs(v[1] - v[4]) * 1 +
						Calc::Abs(v[2] - v[4]) * 1 +
						Calc::Abs(v[3] - v[4]) * 1 +
						Calc::Abs(v[5] - v[4]) * 1 +
						Calc::Abs(v[6] - v[4]) * 1 +
						Calc::Abs(v[7] - v[4]) * 1 +
						Calc::Abs(v[8] - v[4]) * 1;
					
					if (g_GameState->m_ScreenScale >= 2)
					{
						d = d * d / 1500;
					}
					else
					{
						d = d >> 3;
					}
					
					if (d > 255)
						d = 255;
					
					*dstLine++ = d;
					*dstLine++ = d;
					*dstLine++ = d;
					*dstLine++ = srcLine[3];
					
					srcLine += 4;
#endif
				}
			}
			
			memcpy(bytes, bytes2, pixelCount * 4);
			
			delete[] bytes2;
			bytes2 = 0;
		}
		
		return new TextureRGBA(header.sx, header.sy, bytes, true);
	}
	
#if defined(WIN32) || defined(LINUX)// || defined(MACOS)
	void* LoadTexture_PNG_FreeImage(const char* fileName)
	{
		Benchmark bm("loading PNG");
		
		ImageLoader_FreeImage loader;
		
		Image image;
		
		loader.Load(image, g_System.GetResourcePath(Path::ReplaceExtension(fileName, "png").c_str()));
		
		const int count = image.m_Sx * image.m_Sy;
		const ImagePixel* pixel = image.GetLine(0);
		
		uint8_t* bytes = new uint8_t[count * 4];
		
		for (int i = 0; i < count; ++i)
		{
			bytes[i * 4 + 0] = (uint8_t)pixel[i].r;
			bytes[i * 4 + 1] = (uint8_t)pixel[i].g;
			bytes[i * 4 + 2] = (uint8_t)pixel[i].b;
			bytes[i * 4 + 3] = (uint8_t)pixel[i].a;
			//bytes[i * 4 + 3] = 255;
		}
		
		return new TextureRGBA(image.m_Sx, image.m_Sy, bytes, true);
	}
#endif

#if defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(PSP) || defined(IPHONEOS) || defined(BBOS)
	static SoundEffect* LoadSound_Wave(const char* fileName)
	{
/*#if defined(MACOS)
		SoundEffect* temp = new SoundEffect();
		
		temp->Initialize(SoundSampleFormat_S16, 1, 1, 44100, new uint8_t[2], true);

		return temp;
#endif*/
		AutoPtr<Stream> streamTmp(QuickLoad(fileName));
		Stream* stream = streamTmp.Get();

		//FileStream stream;
		
		//stream.Open(g_System.GetResourcePath(fileName).c_str(), OpenMode_Read);
		
		StreamReader reader(stream, false);
		
		char id[4];

		stream->Read(id, 4);
		
		if (id[0] != 'R' || id[1] != 'I' || id[2] != 'F' || id[3] != 'F')
			throw ExceptionVA("not a RIFF file");
			
		int32_t size = reader.ReadInt32();
		
		if (size < 0)
			throw ExceptionNA();

		stream->Read(id, 4);
		
		if (id[0] != 'W' || id[1] != 'A' || id[2] != 'V' || id[3] != 'E')
			throw ExceptionVA("not a WAVE file");
		
		stream->Read(id, 4); // "fmt "
		
		if (id[0] != 'f' || id[1] != 'm' || id[2] != 't' || id[3] != ' ')
			throw ExceptionVA("WAVE loader confused");
		
		int32_t fmtLength = reader.ReadInt32();
		int16_t fmtCompressionType = reader.ReadInt16();
		int16_t fmtChannelCount = reader.ReadInt16();
		int32_t fmtSampleRate = reader.ReadInt32();
		int32_t fmtByteRate = reader.ReadInt16();
		int16_t fmtBlockAlign = reader.ReadInt16();
		int16_t fmtBitDepth = reader.ReadInt16();
		int16_t fmtExtraLength = reader.ReadInt16();
		
		if (false)
		{
			// suppress unused variables warnings
			fmtLength = 0;
			fmtByteRate = 0;
			fmtBlockAlign = 0;
			fmtExtraLength = 0;
		}

		#if 1
		if (fmtBitDepth == 1)
			fmtBitDepth = 8;
		else if (fmtBitDepth == 2)
			fmtBitDepth = 16;
		#endif
		
		if (fmtCompressionType != 1)
			throw ExceptionVA("only PCM is supported. type: %d", fmtCompressionType);
		if (fmtChannelCount <= 0)
			throw ExceptionVA("invalid channel count: %d", fmtChannelCount);
		if (fmtChannelCount > 2)
			throw ExceptionVA("channel count not supported: %d", fmtChannelCount);
		if (fmtBitDepth != 8 && fmtBitDepth != 16)
			throw ExceptionVA("bit depth not supported: %d", fmtBitDepth);
			
		stream->Read(id, 4); // "fllr" or "data"
		
		if (id[0] == 'F' && id[1] == 'L' && id[2] == 'L' && id[3] == 'R')
		{
			int32_t byteCount = reader.ReadInt16();
			LOG_DBG("wave loader: skipping %d bytes of filler", byteCount);
			stream->Seek(byteCount + 2, SeekMode_Offset);
			
			stream->Read(id, 4); // "data"
		}
		
		if (id[0] != 'd' || id[1] != 'a' || id[2] != 't' || id[3] != 'a')
			throw ExceptionVA("WAVE loader confused");
			
		const int32_t byteCount = reader.ReadInt32();
		
		uint8_t* bytes = new uint8_t[byteCount];
		
		stream->Read(bytes, byteCount);
		
		SoundEffect* sound = new SoundEffect();
		
		SoundSampleFormat fmt = SoundSampleFormat_S8;
		
		int sampleSize = 0;
		
		if (fmtBitDepth == 8)
		{
			fmt = SoundSampleFormat_S8;
			sampleSize = 1;
		}
		else if (fmtBitDepth == 16)
		{
			fmt = SoundSampleFormat_S16;
			sampleSize = 2;
		}
		else
			throw ExceptionVA("sample size not supported");
		
		// todo: calculate sample count
		
		const int sampleCount = byteCount / fmtChannelCount / sampleSize;
		
		sound->Initialize(fmt, sampleCount, fmtChannelCount, fmtSampleRate, bytes, true);
		
		return sound;
	}
#endif

#if !defined(IPHONEOS)
	static TextureRGBA* CreateRandomTextureRGBA(int sx, int sy)
	{
		const int byteCount = sx * sy * 4;
		
		uint8_t* bytes = new uint8_t[byteCount];
		
		for (int y = 0; y < sy; ++y)
		{
			for (int x = 0; x < sx; ++x)
			{
				const int index = (x + y * sx) * 4;
				
				bytes[index + 0] = 128 + (rand() & 127);
				bytes[index + 1] = 128 + (rand() & 127);
				bytes[index + 2] = 128 + (rand() & 127);
				//bytes[index + 0] = (x * 8) & 255;
				//bytes[index + 1] = rand() & 255;
				//bytes[index + 2] = ((x + y) * 2) & 255;
				bytes[index + 3] = 255;
			}
		}
		
		return new TextureRGBA(sx, sy, bytes, true);
	}
#endif
	
	void* LoadTexture_PVR(const char* fileName)
	{
#ifdef IPHONEOS
		Benchmark bm("loading PVR");
		
		TexturePVR* texture = new TexturePVR();
		
		if (!texture->Load(g_System.GetResourcePath(fileName).c_str()))
		{
			delete texture;
			texture = 0;
			
			throw ExceptionVA("failed to load PVR texture: %s", fileName);
		}
		
		return texture;
#elif defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
		//return LoadTexture_PNG_FreeImage(fileName);
		//return CreateRandomTextureRGBA(64, 64); // fixme!
		//return CreateRandomTextureRGBA(512, 512); // fixme!
		return CreateRandomTextureRGBA(1024, 1024);
#elif defined(PSP)
		return CreateRandomTextureRGBA(64, 64);
#endif
	}
	
	void* LoadTexture_Atlas(const char* fileName, ResMgr* resMgr)
	{
		Atlas* atlas = new Atlas();
		
		try
		{
			AutoPtr<Stream> streamTmp(QuickLoad(fileName));
			Stream* stream = streamTmp.Get();
			
			atlas->Load(stream, 0);
		}
		catch (Exception& e)
		{
			delete atlas;
			atlas = 0;
			
			throw e;
		}
		
		Res* texture = resMgr->CreateTextureRGBA(atlas->m_TextureFileName.c_str());
		
		TextureAtlas* textureAtlas = new TextureAtlas(texture, atlas);
		
		return textureAtlas;
	}
	
	void* LoadSoundEffect(const char* fileName)
	{
//#ifdef IPHONEOS
#if defined(IPHONEOS) && 0
		int byteCount = 0;
		uint8_t* bytes = 0;
		AudioStreamBasicDescription desc;
		
		LoadAudioFile(fileName, &byteCount, &bytes, &desc);
		
		SoundEffect* sound = new SoundEffect();
		
		SoundSampleFormat format = SoundSampleFormat_Undefined;
		
		// todo: create mac -> our own format conversion method
		
		switch (desc.mBitsPerChannel)
		{
		case 8:
			format = SoundSampleFormat_S8;
			break;
			
		case 16:
			format = SoundSampleFormat_S16;
			break;
			
		default:
			throw ExceptionVA("sample format not supported. bpc=%d", desc.mBitsPerChannel);
		}
		
		sound->Initialize(format, byteCount * 8 / desc.mBitsPerChannel, desc.mChannelsPerFrame, (int)desc.mSampleRate, bytes, true);
		
		return sound;
#else
#if 0
		SoundEffect* sound = new SoundEffect();
		
		int frameCount = 1024;
		int byteCount = frameCount * sizeof(int16_t);
		
		uint8_t* data = new uint8_t[byteCount];
		
		for (int i = 0; i < byteCount; ++i)
			data[i] = rand() & 0xFF;
		
		sound->Initialize(SoundSampleFormat_S16, frameCount, 1, 8000, data, true);

		return sound;
#endif
		return LoadSound_Wave(fileName);
#endif
	}
	
	void* LoadSound3D(const char* fileName)
	{
		SoundEffect* sound = (SoundEffect*)LoadSoundEffect(fileName);
		
		if (!sound)
			return 0;

		bool ok = true;
		
		ok &= sound->m_Info.ChannelCount_get() == 1;
		ok &= sound->m_Info.SampleRate_get() <= 11050; // todo
		ok &= sound->m_Info.SampleFormat_get() == SoundSampleFormat_S16;
		
		if (!ok)
		{
			delete sound;
			sound = 0;
			
			throw ExceptionVA("unsupported audio spec for 3D: %s", fileName);
		}
		
		return sound;
	}
	
	void* LoadVectorGraphic(const char* fileName)
	{
		VectorShape* vectorShape = new VectorShape();
		
		try
		{
			AutoPtr<Stream> streamTmp(QuickLoad(fileName));
			Stream* stream = streamTmp.Get();
			
			vectorShape->m_Shape.Load(stream);
		}
		catch (Exception& e)
		{
			LOG(LogLevel_Error, e.what());
			
			delete vectorShape;
			vectorShape = 0;
			
			throw e;
		}
		
		return vectorShape;
	}
	
	void* LoadVectorComposition(const char* fileName)
	{
		VectorComposition* vectorComposition = new VectorComposition();
		
		try
		{
			AutoPtr<Stream> streamTmp(QuickLoad(fileName));
			Stream* stream = streamTmp.Get();
			
			vectorComposition->m_Composition.Load(stream);
		}
		catch (Exception& e)
		{
			LOG(LogLevel_Error, e.what());
			
			delete vectorComposition;
			vectorComposition = 0;
			
			throw e;
		}
		
		return vectorComposition;
	}
	
#ifdef IPHONEOS
	std::string GetBundleFileName(const char* fileName)
	{
		NSString* temp1 = [NSString stringWithCString:fileName encoding:NSASCIIStringEncoding];
		
		NSString* temp2 = MakeBundleFileName(temp1);
		
		return [temp2 cStringUsingEncoding:NSASCIIStringEncoding];
	}
	
	static NSString* MakeBundleFileName(NSString* fileName)
	{
		NSString* nsFileName = [fileName stringByDeletingPathExtension];
		NSString* nsFileExtension = [fileName pathExtension];
		
		return [[NSBundle mainBundle] pathForResource:nsFileName ofType:nsFileExtension];
	}
	
#if 0
	static bool ImageToBytes(UIImage* image, int* out_Sx, int* out_Sy, uint8_t** out_Bytes)
	{
		const int sx = (int)image.size.width;
		const int sy = (int)image.size.height;
		const int byteCount = sx * sy * 4;
		
		uint8_t* bytes = new uint8_t[byteCount];
		
		bzero(bytes, byteCount);
		
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		CGContextRef ctx = CGBitmapContextCreate(bytes, sx, sy, 8, 4 * sx, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
		CGColorSpaceRelease(colorSpace);
		CGContextDrawImage(ctx, CGRectMake(0, 0, sx, sy), [image CGImage]);
		CGContextRelease(ctx);
		
		*out_Sx = sx;
		*out_Sy = sy;
		*out_Bytes = bytes;
		
		return true;
	}
#endif
	
#if 0
	static uint32_t AudioFile_GetSize(AudioFileID id)
	{
		UInt64 size = 0;
		UInt32 propSize = sizeof(size);
		
		OSStatus retval = AudioFileGetProperty(id, kAudioFilePropertyAudioDataByteCount, &propSize, &size);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to get audio size: %d", retval);
		}
		
		return (uint32_t)size;
	}

	static AudioStreamBasicDescription AudioFile_GetDescription(AudioFileID id)
	{
		AudioStreamBasicDescription dataFormat;

		UInt32 size = sizeof(dataFormat);
		
		OSStatus retval = AudioFileGetProperty(id, kAudioFilePropertyDataFormat, &size, &dataFormat);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to get audio description: %d", retval);
		}
		
		return dataFormat;
	}
	
	static void LoadAudioFile(const char* fileName, int* out_ByteCount, uint8_t** out_Bytes, AudioStreamBasicDescription* out_Desc)
	{
		NSString* path = MakeBundleFileName([NSString stringWithCString:fileName encoding:NSASCIIStringEncoding]);
		
		if (path == nil)
		{
			throw ExceptionVA("unable to get path to file: %s", fileName);
		}
		
		AudioFileID id;
		OSStatus retval;
		
		NSURL* url = [NSURL fileURLWithPath:path];
 
		#if TARGET_OS_IPHONE
		retval = AudioFileOpenURL((CFURLRef)url, kAudioFileReadPermission, 0, &id);
		#else
		retval = AudioFileOpenURL((CFURLRef)url, fsRdPerm, 0, &id);
		#endif
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to open audio file: %d", retval);
		}
		
		UInt32 byteCount = AudioFile_GetSize(id);
		uint8_t* bytes = new uint8_t[byteCount];
		const AudioStreamBasicDescription desc = AudioFile_GetDescription(id);
		 
		retval = AudioFileReadBytes(id, false, 0, &byteCount, bytes);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to read audio data: %d", retval);
		}
		
		retval = AudioFileClose(id);
		
		if (retval != 0)
		{
			throw ExceptionVA("unable to close audio file: %d", retval);
		}
		
		*out_ByteCount = byteCount;
		*out_Bytes = bytes;
		*out_Desc = desc;
	}
#endif
#endif
}
