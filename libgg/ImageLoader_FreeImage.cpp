#include <FreeImage.h>
#include "Exception.h"
#include "Image.h"
#include "ImageLoader_FreeImage.h"
#include "StringEx.h"

#ifdef FREEIMAGE_BIGENDIAN
#error freeimage uses incorrect endianness
#endif

void ImageLoader_FreeImage::Load(Image& image, const std::string& fileName)
{
	if (!FreeImage_IsLittleEndian())
	{
		throw ExceptionVA("freeimage uses incorrect endianness");
	}
	
	FIBITMAP* bmp = FreeImage_Load(FreeImage_GetFileType(fileName.c_str()), fileName.c_str());
	
	if (!bmp)
	{
		throw ExceptionVA("failed to load bitmap: %s", fileName.c_str());
	}
	
	FIBITMAP* bmp32 = FreeImage_ConvertTo32Bits(bmp);
	
	if (!bmp32)
	{
		throw ExceptionVA("failed to convert bitmap: %s", fileName.c_str());
	}

	int sx = FreeImage_GetWidth(bmp32);
	int sy = FreeImage_GetHeight(bmp32);

	image.SetSize(sx, sy);

	for (int y = 0; y < image.m_Sy; ++y)
	{
		uint32_t* sline = (uint32_t*)FreeImage_GetScanLine(bmp32, image.m_Sy - 1 - y);
		ImagePixel* dline = image.GetLine(y);
		
		for (int x = 0; x < image.m_Sx; ++x)
		{
			const int r = ((*sline) >> FI_RGBA_RED_SHIFT) & 0xFF;
			const int g = ((*sline) >> FI_RGBA_GREEN_SHIFT) & 0xFF;
			const int b = ((*sline) >> FI_RGBA_BLUE_SHIFT) & 0xFF;
			const int a = ((*sline) >> FI_RGBA_ALPHA_SHIFT) & 0xFF;

			dline->r = r;
			dline->g = g;
			dline->b = b;
			dline->a = a;

			sline++;
			dline++;
		}
	}
	
	FreeImage_Unload(bmp32);
	FreeImage_Unload(bmp);
}

void ImageLoader_FreeImage::Save(const Image& image, const std::string& fileName)
{
	FIBITMAP* bmp = FreeImage_AllocateT(FIT_BITMAP, image.m_Sx, image.m_Sy, 32, 0, 0, 0);
	
	if (!bmp)
	{
		throw ExceptionVA("failed to create bitmap");
	}
	
	FreeImage_SetTransparent(bmp, TRUE);
	
	FIBITMAP* bmp32 = FreeImage_ConvertTo32Bits(bmp);
	
	if (!bmp32)
	{
		throw ExceptionVA("failed to convert bitmap");
	}

	for (int y = 0; y < image.m_Sy; ++y)
	{
		const ImagePixel* sline = image.GetLine(y);
		unsigned int* dline = (unsigned int*)FreeImage_GetScanLine(bmp32, image.m_Sy - 1 - y);
		
		for (int x = 0; x < image.m_Sx; ++x)
		{
			*dline =
				(sline->r << FI_RGBA_RED_SHIFT) |
				(sline->g << FI_RGBA_GREEN_SHIFT) |
				(sline->b << FI_RGBA_BLUE_SHIFT) |
				(sline->a << FI_RGBA_ALPHA_SHIFT);
			
			sline++;
			dline++;
		}
	}
	
	/*FIBITMAP* bmp32 = FreeImage_ConvertTo32Bits(bmp);
	
	if (!bmp32)
	{
		throw ExceptionVA("failed to convert bitmap");
	}*/
	
	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(fileName.c_str());
	
	if (!FreeImage_Save(fif, bmp32, fileName.c_str(), 0))
	{
		throw ExceptionVA("failed to save bitmap: %s", fileName.c_str());
	}
	
	FreeImage_Unload(bmp32);
	FreeImage_Unload(bmp);
}
