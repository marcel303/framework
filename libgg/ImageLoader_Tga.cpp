#include "Exception.h"
#include "FileStream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "Image.h"
#include "ImageLoader_Tga.h"

#define TGA_LOG(x, ...)
#define RLE_TRESHOLD 3

void TgaHeader::Load(Stream* stream)
{
	StreamReader reader(stream, false);
	
	id_lenght = reader.ReadUInt8();
	colormap_type = reader.ReadUInt8();
	image_type = reader.ReadUInt8();
	cm_first_entry = reader.ReadInt16();
	cm_length = reader.ReadInt16();
	cm_size = reader.ReadUInt8();
	origin_x = reader.ReadInt16();
	origin_y = reader.ReadInt16();
	sx = reader.ReadInt16();
	sy = reader.ReadInt16();
	pixel_depth = reader.ReadUInt8();
	image_descriptor = reader.ReadUInt8();
	
	TGA_LOG("load: id_length: %d", (int)id_lenght);
	TGA_LOG("load: colormap_type: %d", (int)colormap_type);
	TGA_LOG("load: image_type: %d", (int)image_type);
	TGA_LOG("load: cm_first_entry: %d", (int)cm_first_entry);
	TGA_LOG("load: cm_length: %d", (int)cm_length);
	TGA_LOG("load: cm_size: %d", (int)cm_size);
	TGA_LOG("load: origin_x: %d", (int)origin_x);
	TGA_LOG("load: origin_y: %d", (int)origin_y);
	TGA_LOG("load: sx: %d", (int)sx);
	TGA_LOG("load: sy: %d", (int)sy);
	TGA_LOG("load: pixel_depth: %d", (int)pixel_depth);
	TGA_LOG("load: image_descriptor: %d", (int)image_descriptor);
}

void TgaHeader::Save(Stream* stream)
{
	StreamWriter writer(stream, false);
	
	writer.WriteUInt8(id_lenght);
	writer.WriteUInt8(colormap_type);
	writer.WriteUInt8(image_type);
	writer.WriteInt16(cm_first_entry);
	writer.WriteInt16(cm_length);
	writer.WriteUInt8(cm_size);
	writer.WriteInt16(origin_x);
	writer.WriteInt16(origin_y);
	writer.WriteInt16(sx);
	writer.WriteInt16(sy);
	writer.WriteUInt8(pixel_depth);
	writer.WriteUInt8(image_descriptor);
}

void TgaHeader::Make_Raw16(int _sx, int _sy)
{
	id_lenght = 0;
	colormap_type = 0;
	image_type = 2;

	cm_first_entry = 0;
	cm_length = 0;
	cm_size = 0;

	origin_x = 0;
	origin_y = 0;

	sx = _sx;
	sy = _sy;

	pixel_depth = 16;
	//image_descriptor = 8;
	image_descriptor = 0;
}

void TgaHeader::Make_Raw32(int _sx, int _sy)
{
	id_lenght = 0;
	colormap_type = 0;
	image_type = 2;

	cm_first_entry = 0;
	cm_length = 0;
	cm_size = 0;

	origin_x = 0;
	origin_y = 0;

	sx = _sx;
	sy = _sy;

	pixel_depth = 32;
	//image_descriptor = 8;
	image_descriptor = 0;
}

void TgaHeader::Make_Rle32(int _sx, int _sy)
{
	id_lenght = 0;
	colormap_type = 0;
	image_type = 10;

	cm_first_entry = 0;
	cm_length = 0;
	cm_size = 0;

	origin_x = 0;
	origin_y = 0;

	sx = _sx;
	sy = _sy;

	pixel_depth = 32;
	image_descriptor = 8;
}

void TgaLoader::LoadData(Stream* stream, const TgaHeader& header, uint8_t** out_Bytes)
{
	*out_Bytes = new uint8_t[header.sx * header.sy * 4];
	
	stream->Seek(header.id_lenght, SeekMode_Offset);
	
	if (header.colormap_type)
	{
		throw ExceptionVA("not implemented: TGA color map");
	}
	
	switch (header.image_type)
	{
		case 0:
		{
			break;
		}
		case 1:
		{
			// 8 bpp indexed
			throw ExceptionVA("not implemented: 8 BPP");
		}
		case 2:
		{
			// 16, 24, 32 bpp raw
			switch (header.pixel_depth)
			{
				case 16:
					LoadData_Raw16_Hack(stream, header.sx, header.sy, *out_Bytes);
					break;
				case 24:
					throw ExceptionVA("not implemented: 24 BPP");
				case 32:
					LoadData_Raw32_Hack(stream, header.sx, header.sy, *out_Bytes);
					break;
			}
			break;
		}
		
		case 3:
		{
			// 8, 16 bpp gray
			switch (header.pixel_depth)
			{
				case 8:
				case 16:
					throw ExceptionVA("not implemented: 8/16 BPP");
			}
			break;
		}
		case 9:
		{
			// 8 bpp index RLE
			throw ExceptionVA("not implemented");
		}
		case 10:
		{
			// 16, 24, 32 bpp RLE
			switch (header.pixel_depth)
			{
				case 16:
				case 24:
					throw ExceptionVA("not implemented: 16/24 BPP RLE");
				case 32:
				{
					//LoadData_Rle32(stream, header.sx, header.sy, *out_Bytes);
					//LoadData_Rle32_Hack(stream, header.sx, header.sy, *out_Bytes);
					//break;
					throw ExceptionVA("not implemented: 32 BPP RLE");
				}
			}
			break;
		}
		case 11:
		{
			// 8, 16 bpp gray RLE
			switch (header.pixel_depth)
			{
				case 8:
				case 16:
					throw ExceptionVA("not implemented: 8/16 BPP");
			}
			break;
		}
		default:
		{
			throw ExceptionVA("not implemented: image type: %d", (int)header.image_type);
		}
	}
}

#ifdef DEPLOYMENT
#pragma message("fixme: TGA hack disabled..")
//#define TGA_HACK 1
#define TGA_HACK 0
#else
#define TGA_HACK 0
#endif

#if TGA_HACK == 0
//#define READ8() reader.ReadUInt8()
//#define READ32(v) v = reader.ReadUInt32()
#define READ(v, x) reader.Stream_get()->Read(v, x);
#else
#include "FileStream.h"
#define READ8() getc(file)
#define READ32(v) if (fread(&v, 1, 4, file) != 4) { Assert(false); }
#define READ(v, x) if (fread(v, 1, x, file) != (size_t)(x)) { Assert(false); }
#endif

#if defined(WIN32) || defined(LINUX) || defined(MACOS)
#define TGA_TO_RGBA(rgba) rgba
#else
#define TGA_TO_RGBA(rgba) rgba
//#define TGA_TO_RGBA(rgba) (((rgba >> 16) & 0xFF) << 0) | (((rgba >> 8) & 0xFF) << 8) | (((rgba >> 0) & 0xFF) << 16) | (((rgba >> 24) & 0xFF) << 24)
//#define TGA_TO_RGBA(rgba) (((rgba >> 16)) << 0) | (((rgba >> 8)) << 8) | (((rgba >> 0)) << 16) | (((rgba >> 24)) << 24)
#endif

void TgaLoader::LoadData_Raw16_Hack(Stream* stream, int sx, int sy, uint8_t* out_Bytes)
{
#if TGA_HACK == 0
	StreamReader reader(stream, false);
#else
	FILE* file = ((FileStream*)stream)->FileHandle_get();
#endif

	uint32_t* ptr = (uint32_t*)out_Bytes;

	const int count = sx * sy;
	
	uint16_t* pixels = new uint16_t[count];

	READ(pixels, count * 2);

	for (int i = 0; i < count; ++i)
	{
		const uint16_t c = pixels[i];

		const int r = c >> 10;
		const int g = c >> 5;
		const int b = c >> 0;

		*ptr = ((r << 3) << 0) | ((g << 3) << 8) | ((b << 3) << 16) | (255 << 24);

		++ptr;
	}

	delete[] pixels;
	pixels = 0;
}

void TgaLoader::LoadData_Raw32_Hack(Stream* stream, int sx, int sy, uint8_t* out_Bytes)
{
#if TGA_HACK == 0
	StreamReader reader(stream, false);
#else
	FILE* file = ((FileStream*)stream)->FileHandle_get();
#endif

	int32_t* ptr = (int32_t*)out_Bytes;

	int count = sx * sy;
	
#if 1
	READ(ptr, count * 4);
#else
	for (int i = 0; i < count; ++i)
	{
		int32_t rgba;
			
		READ32(rgba);

		ptr[i] = TGA_TO_RGBA(rgba);
	}
#endif
}

#if 0
void TgaLoader::LoadData_Rle32(Stream* stream, int sx, int sy, uint8_t* out_Bytes)
{
#if TGA_HACK == 0
	StreamReader reader(stream, false);
#else
	FILE* file = ((FileStream*)stream)->FileHandle_get();
#endif
	
	int32_t* ptr = (int32_t*)out_Bytes;
	const int32_t* stop = ptr + sx * sy;
	
	while (ptr < stop)
	{
		// read header
		const int header = READ8();
		
		// determine packet size
		const int size = 1 + (header & 0x7f);

		if (header & 0x80)
		{
			// RLE encoded
			
			int32_t rgba;
			
			READ32(rgba);
			
			rgba = TGA_TO_RGBA(rgba);
			
#if 1
			for (int i = 0; i < size; ++i)
			{
				ptr[i] = rgba;
			}
#else
			memset_pattern4(ptr, &rgba, size * 4);
#endif
			
			ptr += size;
		}
		else
		{
			// raw
			
			for (int i = 0; i < size; ++i)
			{
				// read BGRA
				
				int32_t rgba;
				
				READ32(rgba);
				
				ptr[i] = TGA_TO_RGBA(rgba);
			}
			
			ptr += size;
		}
	}
}
#endif

#if 0
void TgaLoader::LoadData_Rle32_Hack(Stream* stream, int sx, int sy, uint8_t* out_Bytes)
{
#if TGA_HACK == 0
	StreamReader reader(stream, false);
#else
	FILE* file = ((FileStream*)stream)->FileHandle_get();
#endif
	
	int32_t* ptr = (int32_t*)out_Bytes;
	const int32_t* stop = ptr + sx * sy;
	
	while (ptr < stop)
	{
		// read header
		const int header = READ8();
		
		// determine packet size
		const int size = 1 + (header & 0x7f);

		if (header & 0x80)
		{
			// RLE encoded
			
			int32_t rgba;
			
			READ32(rgba);
			
			for (int i = 0; i < size; ++i)
			{
				ptr[i] = rgba;
			}
			
			ptr += size;
		}
		else
		{
			// raw
			
#if 0
			for (int i = 0; i < size; ++i)
			{
				// read BGRA
				
				READ32(ptr[i]);
			}
#else
			READ(ptr, size * 4);
#endif
			
			ptr += size;
		}
	}
}
#endif

void TgaLoader::SaveData(Stream* stream, const TgaHeader& header, uint8_t* bytes)
{
	switch (header.image_type)
	{
		case 2:
		{
			switch (header.pixel_depth)
			{
				case 16:
				{
					SaveData_Raw16(stream, header.sx, header.sy, bytes);
					break;
				}
				case 32:
				{
					SaveData_Raw32(stream, header.sx, header.sy, bytes);
					break;
				}
				default:
					throw ExceptionVA("not implemented: %d BPP", (int)header.pixel_depth);
			}
			break;
		}

		case 10:
		{
			switch (header.pixel_depth)
			{
				case 32:
				{
					SaveData_Rle32(stream, header.sx, header.sy, bytes);
					break;
				}
				default:
					throw ExceptionVA("not implemented: %d BPP", (int)header.pixel_depth);
			}
			break;
		}

		default:
		{
			throw ExceptionVA("not implemented: image type: %d", header.image_type);
		}
	}
}

static int Convert8to5_Forward(int v)
{
	 v >>= 3;
	 
	 if (v < 0)
		v = 0;
	else if (v > 31)
		v = 31;
	
	return v;
}

static int Convert8to5_Backward(int v)
{
	v <<= 3;
	
	if (v < 0)
		v = 0;
	else if (v > 255)
		v = 255;
	
	return v;
}

void TgaLoader::SaveData_Raw16(Stream* stream, int sx, int sy, uint8_t* bytes)
{
	// todo: convert to 565 range w/ error diffusion
	
	StreamWriter writer(stream, false);

	int count = sx * sy;

	uint8_t* temp = new uint8_t[count * 4];

	ErrorDiffusion ed;

	for (int i = 0; i < 4; ++i)
	{
		for (int y = 0; y < sy; ++y)
		{
			SampleGetter getter(bytes + y * sx * 4 + i, 4, 1);
			SampleSetter setter(temp + y * sx * 4 + i, 4, 1);

			if (i == 1)
				ed.Apply(&getter, &setter, sx, Convert8to5_Forward, Convert8to5_Backward);
			else
				ed.Apply(&getter, &setter, sx, Convert8to5_Forward, Convert8to5_Backward);
		}
	}

	uint8_t* ptr = temp;

	for (int i = 0; i < count; ++i)
	{
		const int r = ptr[0];
		const int g = ptr[1];
		const int b = ptr[2];
		
		uint16_t c = (b << 0) | (g << 5) | (r << 10);
		
		writer.WriteUInt16(c);

		ptr += 4;
	}

	delete[] temp;
	temp = 0;
}

void TgaLoader::SaveData_Raw32(Stream* stream, int sx, int sy, uint8_t* bytes)
{
	StreamWriter writer(stream, false);

	int count = sx * sy;

	uint8_t* ptr = bytes;

	for (int i = 0; i < count; ++i)
	{
		writer.WriteUInt8(ptr[0]);
		writer.WriteUInt8(ptr[1]);
		writer.WriteUInt8(ptr[2]);
		writer.WriteUInt8(ptr[3]);

		ptr += 4;
	}
}

static int RepeatCount_32(const uint8_t* bytes, int _x, int sx)
{
	// scan 128 pixels at most
	
	int scanX1 = _x;
	int scanX2 = _x + 127;
	
	if (scanX2 >= sx)
		scanX2 = sx - 1;
	
	const uint32_t* pixels = (uint32_t*)bytes;
	const uint32_t value = pixels[_x];
	
	int size = 0;
	
	for (int x = scanX1; x <= scanX2; ++x)
	{
		if (pixels[x] != value)
			break;
		else
			size++;
	}
	
	return size;
}

static int NoRepeatCount_32(const uint8_t* bytes, int _x, int sx)
{
	// scan 128 pixels at most
	
	int scanX1 = _x;
	int scanX2 = _x + 127;
	
	if (scanX2 >= sx)
		scanX2 = sx - 1;
	
	int size = 0;
	
	for (int x = scanX1; x <= scanX2; ++x)
	{
		if (RepeatCount_32(bytes, x, sx) >= RLE_TRESHOLD)
			break;
		else
			size++;
	}
	
	return size;
}

void TgaLoader::SaveData_Rle32(Stream* stream, int sx, int sy, uint8_t* bytes)
{
	StreamWriter writer(stream, false);
	
	for (int i = 0; i < sx * sy; ++i)
	{
		uint8_t* pixel = bytes + i * 4;
		
		if (pixel[3] == 0)
		{
			pixel[0] = pixel[1] = pixel[2] = 0;
		}
	}
	
	for (int y = 0; y < sy; ++y)
	{
		TGA_LOG("y=%d", y);
		
		int x = 0;
		
		while (x < sx)
		{
			// find # non repeating pixels
			
			int count = 0;
			
			count = NoRepeatCount_32(bytes, x, sx);
			
			if (count >= 1)
			{
				TGA_LOG("save: raw: %d", (int)count);
				
				// write header
				
				writer.WriteUInt8(count - 1);
				
				// write pixels
				
				for (int i = 0; i < count; ++i, ++x)
				{
					// save BGRA
					
#if 0
					writer.WriteUInt8(bytes[x * 4 + 2]);
					writer.WriteUInt8(bytes[x * 4 + 1]);
					writer.WriteUInt8(bytes[x * 4 + 0]);
					writer.WriteUInt8(bytes[x * 4 + 3]);
#else
					writer.WriteUInt8(bytes[x * 4 + 0]);
					writer.WriteUInt8(bytes[x * 4 + 1]);
					writer.WriteUInt8(bytes[x * 4 + 2]);
					writer.WriteUInt8(bytes[x * 4 + 3]);
#endif
				}
			}
			
			// find # repeating pixels
			
			count = RepeatCount_32(bytes, x, sx);
			
			if (count >= RLE_TRESHOLD)
			{
				TGA_LOG("save: RLE: %d", (int)count);
				
				// write header
				
				writer.WriteUInt8((count - 1) | 0x80);
				
				// write RLE pixel (BGRA)

#if 0
				writer.WriteUInt8(bytes[x * 4 + 2]);
				writer.WriteUInt8(bytes[x * 4 + 1]);
				writer.WriteUInt8(bytes[x * 4 + 0]);
				writer.WriteUInt8(bytes[x * 4 + 3]);
#else
				writer.WriteUInt8(bytes[x * 4 + 0]);
				writer.WriteUInt8(bytes[x * 4 + 1]);
				writer.WriteUInt8(bytes[x * 4 + 2]);
				writer.WriteUInt8(bytes[x * 4 + 3]);
#endif
				
				x += count;
			}
		}
		
		bytes += sx * 4;
	}
}

//

ImageLoader_Tga::ImageLoader_Tga()
{
	SaveColorDepth = 32;
}

ImageLoader_Tga::~ImageLoader_Tga()
{
}

void ImageLoader_Tga::Load(Image& image, const std::string& fileName)
{
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Read);
	
	TgaHeader header;
	
	header.Load(&stream);
	
	uint8_t* bytes = 0;
	
	TgaLoader loader;
	
	loader.LoadData(&stream, header, &bytes);
	
	image.SetSize(header.sx, header.sy);

	uint8_t* bytesItr = bytes;

	for (int y = 0; y < image.m_Sy; ++y)
	{
		ImagePixel* dline = image.GetLine(y);
		
		for (int x = 0; x < image.m_Sx; ++x)
		{
			dline[x].r = bytesItr[0];
			dline[x].g = bytesItr[1];
			dline[x].b = bytesItr[2];
			dline[x].a = bytesItr[3];
			
			bytesItr += 4;
		}
	}
	
	delete[] bytes;
	bytes = 0;
}

void ImageLoader_Tga::Save(const Image& image, const std::string& fileName)
{
	FileStream stream;
	
	stream.Open(fileName.c_str(), OpenMode_Write);
	
	TgaHeader header;
	
	if (SaveColorDepth == 16)
	{
		header.Make_Raw16(image.m_Sx, image.m_Sy);
	}
	else if (SaveColorDepth == 32)
	{
		//header.Make_Rle32(image.m_Sx, image.m_Sy);
		header.Make_Raw32(image.m_Sx, image.m_Sy);
	}
	else
	{
		throw ExceptionVA("unable to save with selected BPP: %d", SaveColorDepth);
	}
	
	header.Save(&stream);
	
	uint8_t* bytes = new uint8_t[image.m_Sx * image.m_Sy * 4];
	
	uint8_t* bytePtr = bytes;
	
	for (int y = 0; y < image.m_Sy; ++y)
	{
		const ImagePixel* sline = image.GetLine(y);
		
		for (int x = 0; x < image.m_Sx; ++x)
		{
			bytePtr[0] = (uint8_t)sline[x].r;
			bytePtr[1] = (uint8_t)sline[x].g;
			bytePtr[2] = (uint8_t)sline[x].b;
			bytePtr[3] = (uint8_t)sline[x].a;
			
			bytePtr += 4;
		}
	}
	
	TgaLoader loader;
	
	loader.SaveData(&stream, header, bytes);
	
	delete[] bytes;
	bytes = 0;
}
