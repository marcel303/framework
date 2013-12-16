#include "Precompiled.h"

#ifndef NO_ALLEGRO
#define ALLEGRO_USE_CONSOLE
//#define ALLEGRO_NO_MAGIC_MAIN
#include <allegro.h>
#endif

#include <vector>
#include <string>
#include <string.h>
#include "Arguments.h"
#include "Atlas.h"
#include "AtlasBuilder.h"
#include "AtlasBuilderV2.h"
//#include "Directory.h"
#include "Exception.h"
#include "FileStream.h"
#include "FileStreamExtends.h"
#include "Image.h"
#include "Image_Allegro.h"
#include "ImageLoader_FreeImage.h"
#include "ImageLoader_Tga.h"
#include "MemoryStream.h"
#include "Parse.h"
#include "Path.h"
#include "Preview.h"
#include "ResIndex.h"
#include "ResIndexCompiler.h"
#include "StringEx.h"
#include "Types.h"

class Settings
{
public:
	Settings()
	{
		size = 0;
		borderSize = 2;
		preview = false;
	}

	int size;
	std::vector<std::string> src;
	std::string dst;
	int borderSize;
	bool preview;
	std::string resourcePath;
	std::string name;
	std::string dstHeader;
	std::string dstSource;
};

static Settings g_Settings;

int main(int argc, char* argv[])
{
	try
	{
		if (argc < 2)
		{
			printf("usage:\n");
			printf("atlcompiler -c <source> -o <destination> -s <size> -b <border-size> -rp <resource-path> -name <name> -o-header <header-file> -o-source <source-file> [-p]\n");
			return 0;
		}

		// parse arguments

		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-c"))
			{
				for (++i; i < argc && argv[i][0] != '-'; ++i)
				{
					g_Settings.src.push_back(argv[i]);
				}
			}
			else if (!strcmp(argv[i], "-o"))
			{
				ARGS_CHECKPARAM(1);

				g_Settings.dst = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-s"))
			{
				ARGS_CHECKPARAM(1);

				g_Settings.size = Parse::Int32(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-b"))
			{
				ARGS_CHECKPARAM(1);

				g_Settings.borderSize = Parse::Int32(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-p"))
			{
				g_Settings.preview = true;
				
				i += 1;
			}
			else if (!strcmp(argv[i], "-rp"))
			{
				ARGS_CHECKPARAM(1);

				g_Settings.resourcePath = argv[i + 1];

				i += 2;
			}
			else if (!strcmp(argv[i], "-name"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.name = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o-header"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.dstHeader = argv[i + 1];
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-o-source"))
			{
				ARGS_CHECKPARAM(1);
				
				g_Settings.dstSource = argv[i + 1];
				
				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}

		// validate settings

		if (g_Settings.dst.empty())
			throw ExceptionVA("destination not set");
		if (g_Settings.size == 0)
			throw ExceptionVA("size not set");
		if (g_Settings.name.empty())
			throw ExceptionVA("name not set");
		if (g_Settings.dstHeader.empty())
			throw ExceptionVA("dst header file not set");
		if (g_Settings.dstSource.empty())
			throw ExceptionVA("dst source file not set");
		
		//
		
		ImageLoader_FreeImage imageLoader;

		// build texture atlas

		AtlasBuilderV2* builder = new AtlasBuilderV2();

		builder->Setup(g_Settings.size, g_Settings.size, 1);

		std::vector<Atlas_ImageInfo*> images;

		for (size_t i = 0; i < g_Settings.src.size(); ++i)
		{
			if (String::EndsWith(g_Settings.src[i], ".txt"))
			{
				// read resource index

				ResIndex index;

				FileStream stream;

				stream.Open(g_Settings.src[i].c_str(), OpenMode_Read);

				index.Load(&stream);

				// add texture resources

				for (size_t i = 0; i < index.m_Resources.size(); ++i)
				{
					const ResInfo& res = index.m_Resources[i];

					// skip anything but textures

					if (res.m_Type != "texture")
						continue;

					// supplement file name with image extension

					std::string fileName = g_Settings.resourcePath + res.m_FileName;

					// create image, named

					Atlas_ImageInfo* image = new Atlas_ImageInfo(fileName, res.m_Name, g_Settings.borderSize, &imageLoader);

					// add image

					images.push_back(image);
				}
			}
			else
			{
				std::vector<std::string> uri = String::Split(g_Settings.src[i], ':');

				std::string fileName = uri[0];
				std::string name;

				if (uri.size() >= 2)
					name = uri[1];

				Atlas_ImageInfo* image = new Atlas_ImageInfo(fileName, name, g_Settings.borderSize, &imageLoader);

				images.push_back(image);
			}
		}

		ImageLoader_Tga imageSaver;
	
		Atlas* atlas = builder->Create(images);

		#if 0
		ImageLoader_FreeImage imageSaver2;
		
		atlas->Save("temp_atlas.png", &imageSaver2);
		#endif
		
		atlas->Save(g_Settings.dst.c_str(), &imageSaver);
		
		// todo: generate .cpp/.h files

		FileStream indexStream;
		MemoryStream headerStream;
		MemoryStream sourceStream;
		
		indexStream.Open("temp.idx", OpenMode_Write);
//		headerStream.Open(g_Settings.dstHeader.c_str(), OpenMode_Write);
//		sourceStream.Open(g_Settings.dstSource.c_str(), OpenMode_Write);
		
		ResIndexCompiler::Compile(atlas->m_ImageIndex, g_Settings.name, g_Settings.dstHeader, String::Empty, &indexStream, &headerStream, &sourceStream);

		FileStreamExtents::OverwriteIfChanged(&headerStream, g_Settings.dstHeader);
		FileStreamExtents::OverwriteIfChanged(&sourceStream, g_Settings.dstSource);
		
		//

#ifndef NO_ALLEGRO
		if (g_Settings.preview)
		{
			const int scrSx = 1024 / 2;
			const int scrSy = 1024 / 2;
	
			allegro_init();
			set_color_depth(32);
			set_gfx_mode(GFX_AUTODETECT_WINDOWED, scrSx, scrSy, 0, 0);
			install_keyboard();
			install_mouse();
	
			BITMAP* bitmap = Atlas_ToBitmap(atlas);
	
			blit(bitmap, screen, 0, 0, 0, 0, bitmap->w, bitmap->h);
	
			destroy_bitmap(bitmap);
	
			readkey();
		}
#endif

		delete atlas;
		delete builder;
	}
	catch (Exception e)
	{
		printf("%s\n", e.what());

		return -1;
	}

	return 0;
}
#ifndef NO_ALLEGRO
END_OF_MAIN();
#endif
