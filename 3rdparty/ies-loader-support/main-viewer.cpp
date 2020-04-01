#include "framework.h"
#include "ies_loader.h"
#include "Path.h"
#include "TextIO.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(400, 400))
		return -1;

	GxTextureId texture = 0;
	
	for (;;)
	{
		framework.waitForEvents = true;
		
		framework.process();

		if (framework.quitRequested)
			break;
		
		int viewSx;
		int viewSy;
		framework.getCurrentViewportSize(viewSx, viewSy);
		
		for (auto & file : framework.droppedFiles)
		{
			if (Path::GetExtension(file, true) == "ies")
			{
				char * text = nullptr;
				size_t size;
				if (TextIO::loadFileContents(file.c_str(), text, size))
				{
					IESLoadHelper helper;
					IESFileInfo info;
					
					if (helper.load(text, size, info))
					{
						uint8_t * data = new uint8_t[viewSx * viewSy];
						
						if (helper.saveAsPreview(info, data, viewSx, viewSy, 1))
						{
							freeTexture(texture);
							
							texture = createTextureFromR8(data, viewSx, viewSy, true, true);
						}
						
						delete [] data;
						data = nullptr;
					}
					
					delete [] text;
					text = nullptr;
				}
			}
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			if (texture != 0)
			{
				setColor(colorWhite);
				gxSetTexture(texture);
				pushColorPost(POST_SET_RGB_TO_R);
				drawRect(0, 0, viewSx, viewSy);
				popColorPost();
				gxSetTexture(0);
			}
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
