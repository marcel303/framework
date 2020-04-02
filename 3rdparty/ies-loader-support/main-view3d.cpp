#include "framework.h"
#include "ies_loader.h"
#include "Path.h"
#include "TextIO.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableRealTimeEditing = true;
	framework.msaaLevel = 4;
	
	if (!framework.init(400, 400))
		return -1;

	GxTextureId texture = 0;
	
	Camera3d camera;
	camera.position[2] = -1.f;
	camera.position[1] = 1.f;
	
	bool firstFrame = true;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		if (firstFrame)
		{
			firstFrame = false;
			
			framework.droppedFiles.push_back("defined-spot.ies");
		}
		
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
						float * data = new float[viewSx * viewSy];
						
						if (helper.saveAs2D(info, data, viewSx, viewSy, 1))
						{
							freeTexture(texture);
							
							texture = createTextureFromR32F(data, viewSx, viewSy, true, true);
						}
						
						delete [] data;
						data = nullptr;
					}
					
					delete [] text;
					text = nullptr;
				}
			}
		}
		
		const bool enableCamera = mouse.isDown(BUTTON_LEFT);
		
		mouse.showCursor(!enableCamera);
		SDL_CaptureMouse(enableCamera ? SDL_TRUE : SDL_FALSE);
		
		camera.tick(framework.timeStep, enableCamera);

		framework.beginDraw(0, 0, 0, 0);
		{
			const Vec3 lightPosition_world = Vec3(0, 1, 0);
			const Mat4x4 viewToWorld = camera.getWorldMatrix();
			
		#if true
			projectPerspective3d(90.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("ies-light");
				setShader(shader);
				{
					shader.setTexture("ies_texture", 0, texture, true, true);
					shader.setImmediateMatrix4x4("viewToWorld", viewToWorld.m_v);
					shader.setImmediate("lightPosition_world",
						lightPosition_world[0],
						lightPosition_world[1],
						lightPosition_world[2]);
					
					gxPushMatrix();
					{
						gxScalef(4, 4, 4);
						
						setColor(colorWhite);
						drawGrid3dLine(100, 100, 0, 2);
					}
					gxPopMatrix();
				}
				clearShader();
			}
			popBlend();
			camera.popViewMatrix();
		#endif
		
			projectScreen2d();
			
			pushBlend(BLEND_ADD);
			{
				Shader shader("ies-light-volume");
				setShader(shader);
				{
					shader.setTexture("ies_texture", 0, texture, true, true);
					shader.setImmediateMatrix4x4("viewToWorld", viewToWorld.m_v);
					shader.setImmediate("lightPosition_world",
						lightPosition_world[0],
						lightPosition_world[1],
						lightPosition_world[2]);
					
					drawRect(0, 0, viewSx, viewSy);
				}
				clearShader();
			}
			popBlend();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
