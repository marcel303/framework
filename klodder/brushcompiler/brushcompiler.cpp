#include "Arguments.h"
#include "Brush_Pattern.h"
#include "BrushLibrary.h"
#include "ximage.h"
#undef min
#undef max
#include "Exception.h"
#include "FileStream.h"
#include "Image.h"
#include "ImageLoader_FreeImage.h"
#include "Parse.h"
#include "Settings.h"
#include "StreamReader.h"
#include "StringEx.h"

#define MAX_PATTERN_ID 1000000000

enum ProcessingMode
{
	ProcessingMode_Compile,
	ProcessingMode_List
};

static void Validate(Brush_Pattern* brush);
static void PictureToFilter(std::string fileName, Filter* filter, float scale);
//static void FilterToPicture(Filter* filter, std::string fileName);

class Settings
{
public:
	Settings()
	{
		scale = 1;
		mode = ProcessingMode_Compile;
	}
	
	std::string src;
	std::string dst;
	int scale;
	ProcessingMode mode;
};

static Settings sSettings;

int main(int argc, char* argv[])
{
	try
	{
		// parse arguments
		
		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				ARGS_CHECKPARAM(1);
				
				sSettings.src = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);
				
				sSettings.dst = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-s"))
			{
				ARGS_CHECKPARAM(1);
				
				sSettings.scale = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-l"))
			{
				sSettings.mode = ProcessingMode_List;
				
				i += 1;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}
		
		// validate settings
		
		if (sSettings.src == String::Empty)
			throw ExceptionVA("src not set");
		if (sSettings.dst == String::Empty)
			throw ExceptionVA("dst not set");
		
		// read input file
		
		FileStream stream;
		
		stream.Open(sSettings.src.c_str(), OpenMode_Read);
		
		StreamReader reader(&stream, false);
		
		std::vector<Brush_Pattern*> brushList;
		
		Brush_Pattern* brush = 0;
		
		while (!reader.EOF_get())
		{
			std::string line = reader.ReadLine();

			if (String::StartsWith(line, "#"))
				continue;
			if (String::Trim(line) == String::Empty)
				continue;
			
			if (!String::StartsWith(line, "\t"))
			{
				line = String::Trim(line);
				
				if (line == "brush")
				{
					brush = new Brush_Pattern();
					
					brush->mPatternId = MAX_PATTERN_ID;
					
					brushList.push_back(brush);
				}
				else
				{
					throw ExceptionVA("unknown section: %s", line.c_str());
				}
			}
			else
			{
				if (brush == 0)
					throw ExceptionVA("expected brush");
				
				// parse parameter
				
				line = String::Trim(line);
				
				std::vector<std::string> args = String::Split(line, " \t");
				
				if (args[0] == "id")
				{
					brush->mPatternId = Parse::UInt32(args[1]);
				}
				else if (args[0] == "picture")
				{
					PictureToFilter(args[1], &brush->mFilter, (float)sSettings.scale);
					
#if 0
#pragma message("warning")
					for (int i = 0; i < 10; ++i)
					{
						float scale = 1.0f + i * i / 5.0f;

						Filter filter;

						PictureToFilter(args[1], &filter, scale);

						std::string fileName = String::Format("%s_%s_%03d_out.png", args[1].c_str(), sSettings.dst.c_str(), i);
					
						FilterToPicture(&filter, fileName);
					}
#endif
				}
				else if (args[0] == "oriented")
				{
					brush->mIsOriented = Parse::Bool(args[1]);
				}
				else if (args[0] == "filtered")
				{
					brush->mIsFiltered = Parse::Bool(args[1]);
				}
				else if (args[0] == "settings_size")
				{
					brush->mDefaultDiameter = Parse::Int32(args[1]);
				}
				else if (args[0] == "settings_spacing")
				{
					brush->mDefaultSpacing = Parse::Int32(args[1]);
				}
				else if (args[0] == "settings_strength")
				{
					brush->mDefaultStrength = Parse::Int32(args[1]);
				}
				else
				{
					throw ExceptionVA("unknown parameter: %s", args[0].c_str());
				}
			}
		}

		for (size_t i = 0; i < brushList.size(); ++i)
			Validate(brushList[i]);
		
		BrushLibrary library;

		for (size_t i = 0; i < brushList.size(); ++i)
			library.Append(brushList[i], true);

		FileStream dstStream;

		dstStream.Open(sSettings.dst.c_str(), OpenMode_Write);

		library.Save(&dstStream);

		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());
		
		return -1;
	}
}

static void Validate(Brush_Pattern* brush)
{
	if (brush->mPatternId == MAX_PATTERN_ID)
		throw ExceptionVA("pattern id not set");
	if (brush->mFilter.Sx_get() == 0 || brush->mFilter.Sy_get() == 0)
		throw ExceptionVA("picture is empty");
}

static void PictureToFilter(std::string fileName, Filter* filter, float scale)
{
	ImageLoader_FreeImage loader;

	Image image;

	loader.Load(image, fileName);

	if (image.m_Sx != image.m_Sy)
		throw ExceptionVA("image not square");
	
	const int size = image.m_Sx;
	
	CxImage cxImage(size, size, 24);

	for (int y = 0; y < size; ++y)
	{
		const ImagePixel* srcLine = image.GetLine(y);
		BYTE* dstLine = cxImage.GetBits(y);

		for (int x = 0; x < size; ++x)
		{
			dstLine[0] = (BYTE)srcLine[x].r;
			dstLine[1] = (BYTE)srcLine[x].g;
			dstLine[2] = (BYTE)srcLine[x].b;
			dstLine += 3;
		}
	}
	
	const int scaledSize = (int)((size - 1) / scale) + 1;
	
	bool disableAveraging = false;
	
	//cxImage.Resample2(scaledSize, scaledSize, CxImage::IM_LANCZOS, CxImage::OM_REPEAT, NULL, disableAveraging);	
	cxImage.Resample2(scaledSize, scaledSize, CxImage::IM_BICUBIC2, CxImage::OM_REPEAT, NULL, disableAveraging);	
	//cxImage.Resample2(scaledSize, scaledSize, CxImage::IM_BILINEAR, CxImage::OM_REPEAT, NULL, disableAveraging);	
	//cxImage.Resample2(scaledSize, scaledSize, CxImage::IM_NEAREST_NEIGHBOUR, CxImage::OM_REPEAT, NULL, disableAveraging);	
	
	LOG_DBG("cxWidth: %d", (int)cxImage.GetWidth());
	
	filter->Size_set(scaledSize, scaledSize);
	
	for (int y = 0; y < scaledSize; ++y)
	{
		const BYTE* srcLine = cxImage.GetBits(y);
		float* dstLine = filter->Line_get(y);
		
		for (int x = 0; x < scaledSize; ++x)
		{
			dstLine[x] = (srcLine[0] + srcLine[1] + srcLine[2]) / 3.0f / 255.0f;
			srcLine += 3;
		}
	}
}

/*
static void FilterToPicture(Filter* filter, std::string fileName)
{
	Image image;
	
	image.SetSize(filter->Sx_get(), filter->Sy_get());
	
	for (int y = 0; y < filter->Sy_get(); ++y)
	{
		const float* srcLine = filter->Line_get(y);
		ImagePixel* dstLine = image.GetLine(y);
		
		for (int x = 0; x < filter->Sx_get(); ++x)
		{
			dstLine[x].r = (int)(srcLine[x] * 255.0f);
			dstLine[x].g = (int)(srcLine[x] * 255.0f);
			dstLine[x].b = (int)(srcLine[x] * 255.0f);
			dstLine[x].a = 255;
		}
	}
	
	ImageLoader_FreeImage loader;
	
	loader.Save(image, fileName);
}
*/
