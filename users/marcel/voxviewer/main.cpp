// viewer for MagicaVoxel .vox files
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
// https://ephtracy.github.io/

#include "framework.h"
#include "gx_mesh.h"

#include "magicavoxel/magicavoxel-framework.h"

#include "lightDrawer.h"
#include "renderer.h"
#include "renderOptions.h"

#include "FileStream.h"

#include <stdint.h>
#include <vector>

using namespace rOne;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	framework.filedrop = true;
	
	//if (!framework.init(400, 300))
	if (!framework.init(1200, 900))
		return 0;

	Renderer::registerShaderOutputs();
	
	MagicaWorld world;
	
	try
	{
		//FileStream stream("marcel-01.vox", OpenMode_Read);
		FileStream stream("monu7.vox", OpenMode_Read);
		//FileStream stream("room.vox", OpenMode_Read);
		StreamReader reader(&stream, false);
		
		readMagicaWorld(reader, world);
	}
	catch (std::exception & e)
	{
		logError("error: %s", e.what());
	}

	Camera3d camera;
	
	GxMesh drawMesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;
	
	gxCaptureMeshBegin(drawMesh, vb, ib);
	{
		drawMagicaWorld(world);
	}
	gxCaptureMeshEnd();
	
	RenderOptions renderOptions;
	renderOptions.renderMode = kRenderMode_DeferredShaded;
	//renderOptions.renderMode = kRenderMode_Flat;
	renderOptions.linearColorSpace = true;
	renderOptions.backgroundColor.Set(0, 0, 1);
	
	renderOptions.fog.enabled = true;
	renderOptions.fog.thickness = .1f;
	
	renderOptions.bloom.enabled = false;
	renderOptions.bloom.blurSize = 20.f;
	renderOptions.bloom.strength = .3f;
	
	renderOptions.lightScatter.enabled = false;
	
	renderOptions.depthSilhouette.enabled = true;
	renderOptions.depthSilhouette.color.Set(0, 0, 0, .5f);
	
	renderOptions.fxaa.enabled = true;
	
	Renderer renderer;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		for (auto & file : framework.droppedFiles)
		{
			try
			{
				world.free();
				
				FileStream stream(file.c_str(), OpenMode_Read);
				StreamReader reader(&stream, false);
				
				readMagicaWorld(reader, world);
			}
			catch (std::exception & e)
			{
				logError("error: %s", e.what());
			}
			
			gxCaptureMeshBegin(drawMesh, vb, ib);
			{
				drawMagicaWorld(world);
			}
			gxCaptureMeshEnd();
		}
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
				auto drawOpaque = [&]()
				{
					Shader shader("shader");
					setShader(shader);
					
					gxScalef(-1, 1, 1);
					gxRotatef(-90, 1, 0, 0);
					gxScalef(.1f, .1f, .1f);
					
				#if 1
					drawMesh.draw();
				#else
					drawVoxWorld(world);
				#endif
					
					clearShader();
				};
				
				auto drawLights = [&]()
				{
					g_lightDrawer.drawDeferredAmbientLight(Vec3(.1f, .1f, .1f));
					
					g_lightDrawer.drawDeferredPointLight(camera.position, 0.f, 4.f, Vec3(1, 1, 1));
				};
				
				RenderFunctions renderFunctions;
				renderFunctions.drawOpaque = drawOpaque;
				renderFunctions.drawLights = drawLights;
				
				renderer.render(renderFunctions, renderOptions, framework.timeStep);
			}
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	// todo : shutdown renderer
	
	framework.shutdown();

	return 0;
}
