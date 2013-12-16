#include "veccompile_pch.h"
#include <FreeImage.h>
#ifdef WIN32
#include <direct.h>
#endif
#include <stdlib.h>
#include <string>
#include <string.h>
#include <vector>
#include "Arguments.h"
#include "FileStream.h"
#include "ImageLoader_FreeImage.h"
#include "Parse.h"
#include "Path.h"
#include "Shape.h"
#include "ShapeIO.h"
#include "StringEx.h"
#include "Types.h"
#include "VecRend.h"
#include "VecRend_FreeImage.h"

using namespace std;

class Settings
{
public:
	Settings()
	{
		quality = 5;
		drawMode = DrawMode_Regular;
		border = 0;
		demultiplyAlpha = false;
		scale = 1;
	}

	int quality;
	string src;
	string dst;
	DrawMode drawMode;
	int border;
	bool demultiplyAlpha;
	int scale;
};

Settings settings;

int main(int argc, char* argv[])
{
	FreeImage_Initialise(TRUE);
	
	// parse command line

	try
	{
		if (argc == 1)
		{
			printf("usage: veccompile -c <src> -o <dst> [-q <quality] [-m <mode>] [-b <border>] [-n <demultiply_alpha>] [-s <scale>]\n");
			printf("\tmode = regular, stencil\n");

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
			else if (!strcmp(argv[i], "-q"))
			{
				ARGS_CHECKPARAM(1);

				settings.quality = Parse::Int32(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-m"))
			{
				ARGS_CHECKPARAM(1);
					 
				if (!strcmp(argv[i + 1], "regular"))
					settings.drawMode = DrawMode_Regular;
				else if (!strcmp(argv[i + 1], "stencil"))
					settings.drawMode = DrawMode_Stencil;
				else
					throw ExceptionVA("unknown draw mode: %s", argv[i + 1]);
				 
				 i += 2;
			}
			else if (!strcmp(argv[i], "-b"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.border = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else if (!strcmp(argv[i], "-n"))
			{
				settings.demultiplyAlpha = true;
				
				i += 1;
			}
			else if (!strcmp(argv[i], "-s"))
			{
				ARGS_CHECKPARAM(1);
				
				settings.scale = Parse::Int32(argv[i + 1]);
				
				i += 2;
			}
			else
			{
				throw ExceptionVA(String::Format("unknown parameter: %s", argv[i]).c_str());
			}
		}

		if (settings.src.empty())
		{
			throw ExceptionVA("error: src not set");
		}

		if (settings.dst.empty())
		{
			throw ExceptionVA("error: dst not set");
		}
		
		if (settings.scale <= 0)
		{
			throw ExceptionVA("scale must be > 0");
		}
		
		//
		
		std::string cwd;
		
		char cwdTemp[1000];
		
		if (getcwd(cwdTemp, 1000) == 0)
			assert(false);
		
		cwd = cwdTemp;
		
		std::string directory = Path::GetDirectory(settings.src);
		
		if (directory != String::Empty)
		{
			if (chdir(directory.c_str()) < 0)
				assert(false);
		}
		
//		printf("cwd: %s\n", directory.c_str());
		
		settings.src = Path::GetFileName(settings.src);
		
		//
		
		ImageLoader_FreeImage imageLoader;

		// read shape
		
		Shape shape;
		
		if (!ShapeIO::LoadVG(settings.src.c_str(), shape, &imageLoader))
		{
			throw ExceptionVA("failed to load file");
		}
		
		shape.Scale(settings.scale);
		
		if (chdir(cwd.c_str()) < 0)
			assert(false);
		
//		printf("cwd: %s\n", cwd.c_str());
				
		// todo: border..

		//printf("shape: item count=%d\n", (int)shape.m_Items.size());

		std::string fileVGC = settings.dst;
		std::string filePNG = settings.dst + ".png";
		std::string fileSIL = settings.dst + ".sil.png";

		// render shape
		
		Shape shapeX2 = shape;
		
		//shapeX2.Scale(2);
		
		Buffer* buffer = VecRend_CreateBuffer(shapeX2, settings.quality, settings.drawMode);

		//printf("buffer: sx=%d, sy=%d\n", buffer->m_Sx, buffer->m_Sy);

		if (settings.demultiplyAlpha)
		{
			buffer->DemultiplyAlpha();
		}

		// write image to PNG file
		
		VecRend_Export(buffer, filePNG.c_str(), &imageLoader);
		
		delete buffer;
		
		if (shape.m_HasShadow)
		{
			// render shape silhouette
			
			Buffer* temp = VecRend_CreateBuffer(shape, settings.quality, DrawMode_Stencil);
			
			buffer = temp->Blur(2);
			
			delete temp;
			
			VecRend_Export(buffer, fileSIL.c_str(), &imageLoader);
			
			delete buffer;
		}
		else
		{
			// use a stub 1x1 silhouette
			
			buffer = new Buffer();
			buffer->SetSize(1, 1);
			buffer->Clear(0.0f);
			VecRend_Export(buffer, fileSIL.c_str(), &imageLoader);
			delete buffer;
		}

		// compile shape and save

		Stream* stream = new FileStream(fileVGC.c_str(), OpenMode_Write);

		VecCompile_Compile(shape, filePNG, fileSIL, stream);

		delete stream;

		return 0;
	}
	catch (const Exception& e)
	{
		printf("error: %s\n", e.what());

		return -1;
	}
}
