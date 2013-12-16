#include <stdlib.h>
#include <string.h>
#include "Arguments.h"
#include "Exception.h"
#include "FontCompiler.h"
#include "FileStream.h"
#include "ImageLoader_FreeImage.h"
#include "Parse.h"
#include "StringEx.h"

/*

fontcompile

Takes a 16x16 character map image as input and generates a tidied up texture and compiled font file

- source image is read
- glyphs are generated for non-empty tiles
- glyph width is automatically determined
- glyphs are merged into single texture
- texture is output
- compiled font is generated, which contains character to texture rect mappings
- compiled font is generated

Integration into build process

- build fonts before texture atlas is generated
- include texture in texture atlas
- include font map in project build (copy resource)

*/

class Settings
{
public:
	Settings()
	{
		downScale = 1;
		space_sx = 10;
		atlasSx = 0;
		atlasSy = 0;
		range1 = 0;
		range2 = 255;
		shrinkAtlas = false;
	}
	
	std::string src;
	std::string dst;
	int downScale;
	int space_sx;
	int atlasSx;
	int atlasSy;
	int range1;
	int range2;
	bool shrinkAtlas;
};

static Settings settings;

int main(int argc, char* argv[])
{
	try
	{
		if (argc == 1)
		{
			printf("usage: fontcompile -c <src> -o <dst> -d <scale> -p <space_size> -s <atlas_sx> <atlas_sy> [-r <first_char> <last_char>] [-w <shrink>]\n");
			printf("src: source image, containing 16x16 glyphs\n");
			printf("dst: destination font file\n");
			printf("scale: downscaling factor. if 2, the font size will be halved\n");
			printf("space_size: width of the space character\n");
			printf("atlas_sx: width of the generated texture atlas\n");
			printf("atlas_sy: height of the generated texture atlas\n");
			printf("first_char: first character (ASCII code) in the range of glyphs to export\n");
			printf("last_char: last character (ASCII code) in the range of glyphs to export\n");
			printf("shrink: if set to 1, the size of the texture atlas will be minimized\n");
			exit(-1);
		}
		
		for (int i = 1; i < argc; )
		{
			if (!strcmp(argv[i], "-c"))
			{
				ARGS_CHECKPARAM(1);

				settings.src = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);

				settings.dst = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-p"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.space_sx = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-s"))
			{
				ARGS_CHECKPARAM(2);
				
				settings.atlasSx = Parse::Int32(argv[i + 1]);
				settings.atlasSy = Parse::Int32(argv[i + 2]);
				
				i += 3;
			}
			else if (!strcmp(argv[i], "-r"))
			{
				ARGS_CHECKPARAM(2);

				settings.range1 = Parse::Int32(argv[i + 1]);
				settings.range2 = Parse::Int32(argv[i + 2]);

				i += 3;
			}
			else if (!strcmp(argv[i], "-w"))
			{
				settings.shrinkAtlas = true;
				
				i += 1;
			}
			else if (!strcmp(argv[i], "-d"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.downScale = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else
			{
				throw ExceptionVA(String::Format("unknown parameter: %s", argv[i]).c_str());
			}
		}
		
		//
		
		if (settings.src.empty())
		{
			throw ExceptionVA("error: src not set");
		}
		if (settings.dst.empty())
		{
			throw ExceptionVA("error: dst not set");
		}
		if (settings.downScale <= 0)
		{
			throw ExceptionVA("error: downScale <= 0");
		}
		if (settings.atlasSx <= 0 || settings.atlasSy <= 0)
		{
			throw ExceptionVA("error: invalid atlas size: %dx%d", settings.atlasSx, settings.atlasSy);
		}
		
		//
		
		ImageLoader_FreeImage loader;
		
		//
		
		Font font;
		
		FontCompiler::Create(settings.src, settings.dst, settings.downScale, settings.space_sx, settings.range1, settings.range2, true, settings.atlasSx, settings.atlasSy, settings.shrinkAtlas, font, &loader);
		
		CompiledFont compiledFont;
		
		FontCompiler::Compile(font, compiledFont);
		
		{
			std::string dst = settings.dst;
			
			FileStream stream;
			
			stream.Open(dst.c_str(), OpenMode_Write);
			
			compiledFont.Save(&stream);
		}
		
		return 0;
	}
	catch (Exception& e)
	{
		printf("error: %s\n", e.what());
		
		return -1;
	}
}
