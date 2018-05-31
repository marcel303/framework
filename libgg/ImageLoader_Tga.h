#pragma once

#include <stdint.h>
#include "IImageLoader.h"

/*

the TGA loader is capable or reading and writing TGA files, with and without RLE encoding

note : the TGA loader _WILL THROW_ when it encounters any issue reading or writing files. be
       sure to capture <std::exception> when such error occurs
       
the TGA loader stored pixels directly in BGRA format

the first row of pixels in memory, will appear as the bottom row of pixels in the image when viewed
when an image has rows going top to bottom in memory (as is common for most images and apis), the
image should be vertically flipped first before wiring it to file, or after reading it from file

*/

class TgaHeader
{
public:
	void Load(Stream* stream);
	void Save(Stream* stream);
	
	void Make_Raw16(int sx, int sy);
	void Make_Raw32(int sx, int sy);
	void Make_Rle32(int sx, int sy);
	
	uint8_t id_lenght; // size of image id
	uint8_t colormap_type; // 1 if color map present
	uint8_t image_type; // compression type

	int16_t	cm_first_entry; // color map origin
	int16_t	cm_length; // color map length
	uint8_t cm_size; // color map size

	int16_t	origin_x; // bottom left x coord origin
	int16_t	origin_y; // bottom left y coord origin

	int16_t	sx; // size in x dimension
	int16_t	sy; // size in y dimension

	uint8_t pixel_depth; // bits per pixel (8, 16, 24, 32)
	uint8_t image_descriptor; // 24 bits = 0x00, 32 bits = 0x80
};

class TgaLoader
{
public:
	void LoadData(Stream* stream, const TgaHeader& header, uint8_t** out_Bytes);
	void LoadData_Raw16_Hack(Stream* stream, int sx, int sy, uint8_t* out_Bytes);
	void LoadData_Raw32(Stream* stream, int sx, int sy, uint8_t* out_Bytes);
	void LoadData_Rle32(Stream* stream, int sx, int sy, uint8_t* out_Bytes);
	
	void SaveData(Stream* stream, const TgaHeader& header, const uint8_t* bytes);
	void SaveData_Raw16(Stream* stream, int sx, int sy, const uint8_t* bytes);
	void SaveData_Raw32(Stream* stream, int sx, int sy, const uint8_t* bytes);
	void SaveData_Rle32(Stream* stream, int sx, int sy, const uint8_t* bytes);
};

//

class ImageLoader_Tga : public IImageLoader
{
public:
	ImageLoader_Tga();
	virtual ~ImageLoader_Tga();
	virtual void Load(Image& image, const std::string& fileName);
	virtual void Save(const Image& image, const std::string& fileName);
	
	void SaveBGRA_vflipped(const uint8_t* bytes, const int sx, const int sy, const std::string& fileName, const bool useRLE);
	
	int SaveColorDepth;
};
