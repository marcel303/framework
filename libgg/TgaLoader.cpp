#if 0

#include "Exception.h"
#include "Stream.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "TgaLoader.h"

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
	image_descriptor = 0x80;
}

void TgaLoader::LoadData(Stream* stream, const TgaHeader& header, uint8_t** out_Bytes)
{
	*out_Bytes = new uint8_t[header.sx * header.sy * 4];
	
	stream->Seek(header.id_lenght, SeekMode_Offset);
	
	if (header.colormap_type)
	{
		throw ExceptionVA("not implemented");
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
			throw ExceptionVA("not implemented");
		}
		case 2:
		{
			// 16, 24, 32 bpp raw
			switch (header.pixel_depth)
			{
				case 16:
				case 24:
				case 32:
					throw ExceptionVA("not implemented");
					throw ExceptionVA("not implemented");
					throw ExceptionVA("not implemented");
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
					throw ExceptionVA("not implemented");
			}
			break;
		}
		case 9:
		{
			// 8 bpp index RLE
			throw ExceptionVA("not implemented");
			break;
		}
		case 10:
		{
			// 16, 24, 32 bpp RLE
			switch (header.pixel_depth)
			{
				case 16:
				case 24:
					throw ExceptionVA("not implemented");
					throw ExceptionVA("not implemented");
				case 32:
				{
					LoadData_Rle32(stream, header.sx, header.sy, *out_Bytes);
					break;
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
					throw ExceptionVA("not implemented");
					throw ExceptionVA("not implemented");
			}
			break;
		}
		default:
		{
			throw ExceptionVA("not implemented");
		}
	}
}

void TgaLoader::LoadData_Rle32(Stream* stream, int sx, int sy, uint8_t* out_Bytes)
{
	StreamReader reader(stream, false);
	
	uint8_t* ptr = out_Bytes;

	while (ptr < out_Bytes + (sx * sy) * 4)
	{
		// read header
		const uint8_t header = reader.ReadUInt8();
		
		// determine packet size
		const int size = 1 + (header & 0x7f);

		if (header & 0x80)
		{
			// RLE encoded
			
			uint8_t rgba[4];
			
			rgba[0] = reader.ReadUInt8();
			rgba[1] = reader.ReadUInt8();
			rgba[2] = reader.ReadUInt8();
			rgba[3] = reader.ReadUInt8();

			for (int i = 0; i < size; ++i, ptr += 4)
			{
				ptr[0] = rgba[2];
				ptr[1] = rgba[1];
				ptr[2] = rgba[0];
				ptr[3] = rgba[3];
			}
		}
		else
		{
			// raw
			
			for (int i = 0; i < size; ++i, ptr += 4)
			{
				ptr[2] = reader.ReadUInt8();
				ptr[1] = reader.ReadUInt8();
				ptr[0] = reader.ReadUInt8();
				ptr[3] = reader.ReadUInt8();
			}
		}
	}
}

void TgaLoader::SaveData(Stream* stream, const TgaHeader& header, uint8_t* bytes)
{
	switch (header.image_type)
	{
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
					throw ExceptionVA("not implemented");
			}
			break;
		}

		default:
		{
			throw ExceptionVA("not implemented");
		}
	}
}

static int RepeatCount_32(uint8_t* bytes, int _x, int sx)
{
	// scan 127 pixels at best
	
	int scanX1 = _x;
	int scanX2 = _x + 127;
	
	if (scanX2 >= sx)
		scanX2 = sx - 1;
	
	const uint32_t* pixels = (uint32_t*)bytes;
	const uint32_t value = pixels[_x];
	
	int size = 1;
	
	for (int x = scanX1; x <= scanX2; ++x)
	{
		if (pixels[x] != value)
			break;
		else
			size++;
	}
	
	return size;
}

static int NoRepeatCount_32(uint8_t* bytes, int _x, int sx)
{
	// scan 127 pixels at best
	
	int scanX1 = _x;
	int scanX2 = _x + 127;
	
	if (scanX2 >= sx)
		scanX2 = sx - 1;
	
	int size = 0;
	
	for (int x = scanX1; x <= scanX2; ++x)
	{
		if (RepeatCount_32(bytes, x, sx) >= 3)
			break;
		else
			size++;
	}
	
	return sx - _x;
}

void TgaLoader::SaveData_Rle32(Stream* stream, int sx, int sy, uint8_t* bytes)
{
	StreamWriter writer(stream, false);
	
	for (int y = 0; y < sy; ++y)
	{
		int x = 0;
		
		while (x < sx)
		{
			// find # non repeating pixels
			
			int count = 0;
			
			count = NoRepeatCount_32(bytes, x, sx);
			
			if (count >= 1)
			{
				// write header
				
				writer.WriteUInt8(count);
				
				// write pixels
				
				for (int i = 0; i < count; ++i)
				{
					writer.WriteUInt8(bytes[x * 4 + 2]);
					writer.WriteUInt8(bytes[x * 4 + 1]);
					writer.WriteUInt8(bytes[x * 4 + 0]);
					writer.WriteUInt8(bytes[x * 4 + 3]);
				}
				
				x += count;
			}
			
			// find # repeating pixels
			
			count = RepeatCount_32(bytes, x, sx);
			
			if (count >= 3)
			{
				// write header
				
				writer.WriteUInt8(count | 0x80);
				
				// write RLE pixel

				writer.WriteUInt8(bytes[x * 4 + 2]);
				writer.WriteUInt8(bytes[x * 4 + 1]);
				writer.WriteUInt8(bytes[x * 4 + 0]);
				writer.WriteUInt8(bytes[x * 4 + 3]);
				
				x += count;
			}
		}
	}
}

#endif
