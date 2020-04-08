#include "forwardLighting.h"
#include "framework.h"
#include "shadowMapDrawer.h"
#include <vector>

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(800, 400))
		return -1;
	
	auto drawOpaque = [&]()
	{
		setColor(colorWhite);
		//fillCube(Vec3(), Vec3(1, 1, 1));
		fillCylinder(Vec3(), 1.f, 1.f, 100);
		
		gxPushMatrix();
		{
			gxTranslatef(0, -1, 0);
			gxScalef(10, 10, 10);
			setColor(200, 200, 200);
			drawGrid3d(1, 1, 0, 2);
		}
		gxPopMatrix();
	};
	
	auto drawTranslucent = [&]()
	{
		pushCullMode(CULL_BACK, CULL_CW);
		
		for (int i = 0; i < 0; ++i)
		{
			const float x = cosf(i / 1.23f) * 2.f;
			const float z = cosf(i / 2.34f) * 2.f;
			
			setColor(50, 100, 255, 127, 512);
			const float s = (sinf(framework.time + i) + 3.f) / 4.f * .2f;
			fillCube(Vec3(x, .5f, z), Vec3(s, s, s));
		}
		
		gxPushMatrix();
		{
			setColor(50, 100, 200, 127);
			const float s = (sinf(framework.time) + 1.f) / 2.f * (1.f/sqrt(2.f));
			gxTranslatef(0, 2, 0);
			gxRotatef(framework.time * 20.f, 1, 1, 1);
			fillCube(Vec3(), Vec3(s, s, s));
		}
		gxPopMatrix();
		
		popCullMode();
	};
	
	ShadowMapDrawer d;
	d.alloc(4, 512);

	d.drawOpaque = drawOpaque;
	d.drawTranslucent = drawTranslucent;

	d.enableColorShadows = false;

	Camera3d camera;
	
	struct SpotLight
	{
		Mat4x4 transform = Mat4x4(true);
		float angle = 90.f;
		float nearDistance = .01f;
		float farDistance = 1.f;
		Vec3 color = Vec3(1, 1, 1);
	};
	
	rOne::ForwardLightingHelper helper;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		if (keyboard.wentDown(SDLK_c))
			d.enableColorShadows = !d.enableColorShadows;
		
		// -- add animated spot lights --
		
		std::vector<SpotLight> spots;
		
		const Vec3 colors[3] =
		{
			Vec3(1, 1, 0),
			Vec3(0, 1, 1),
			Vec3(1, 0, 1)
		};

		for (int i = 0; i < 2; ++i)
		{
			SpotLight spot;
			spot.transform.MakeLookat(
				Vec3(
					sinf(framework.time / (2.34f + i)) * 2.f,
					sinf(framework.time / (1.23f + i)) * i / 2.f + 4.f,
					sinf(framework.time / (3.45f + i)) + (i == 0 ? -2.5f : +2.5f)),
				Vec3(0, 0, 0),
				Vec3(0, 1, 0));
			spot.transform = spot.transform.CalcInv();
			//spot.angle = 60.f;
			spot.angle = 60.f + sinf(framework.time/4.56f)*30.f;
			spot.nearDistance = .01f;
			spot.farDistance = 16.f;
			spot.color = colors[i % 3];
			
			spots.push_back(spot);
		}
		
		Mat4x4 directional;
		directional.MakeLookat(
			Vec3(
				sinf(framework.time / 1.23f) * 3.f,
				sinf(framework.time / 3.45f) + 3.f,
				sinf(framework.time / 2.34f) * 3.f),
			Vec3(0, 0, 0),
			Vec3(0, 1, 0));
		directional = directional.CalcInv();
		
		// -- determine view matrix --
		
		Mat4x4 worldToView = camera.getViewMatrix();
		
		if (keyboard.isDown(SDLK_1))
			worldToView = spots[0].transform.CalcInv();
		if (keyboard.isDown(SDLK_2))
			worldToView = spots[1].transform.CalcInv();
		if (keyboard.isDown(SDLK_3))
			worldToView = spots[2].transform.CalcInv();
		
		if (keyboard.isDown(SDLK_0))
			worldToView = directional;
		
		// -- draw shadow maps --
		
		size_t id = 0;
		
		for (auto & spot : spots)
		{
			d.addSpotLight(id++, spot.transform, spot.angle * float(M_PI/180.0), spot.nearDistance, spot.farDistance);
		}
		
		d.addDirectionalLight(id++, directional, 0.f, 100.f, 12.f);
		
		d.drawShadowMaps(worldToView);
		
		// -- prepare forward lighting --
		
		id = 0;
		
		for (auto & spot : spots)
		{
			const int shadowMapId = d.getShadowMapId(id);
			
			helper.addSpotLight(
				spot.transform.GetTranslation(),
				spot.transform.GetAxis(2).CalcNormalized(),
				spot.angle * float(M_PI/180.f),
				spot.farDistance,
				spot.color,
				.2f,
				shadowMapId);
			
			id++;
		}
		
		helper.addDirectionalLight(
			directional.GetAxis(2).CalcNormalized(),
			Vec3(1, 1, 1),
			(sinf(framework.time * 3.45f) + 1.f) / 2.f * .01f,
			d.getShadowMapId(id));
		id++;
		
		helper.prepareShaderData(16, 32.f, false, worldToView);
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			gxPushMatrix();
			gxLoadMatrixf(worldToView.m_v);
			{
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("210-light-with-shadow");
					setShader(shader);
					{
						int nextTextureUnit = 0;
						d.setShaderData(shader, nextTextureUnit, worldToView);
						helper.setShaderData(shader, nextTextureUnit);
						
						drawOpaque();
					}
					clearShader();
					
					for (auto & spot : spots)
					{
						pushLineSmooth(true);
						gxPushMatrix();
						{
							const float radius = tanf(spot.angle / 2.f * float(M_PI)/180.f);
							
							gxMultMatrixf(spot.transform.m_v);
							setColor(colorWhite);
							
							gxPushMatrix();
							gxTranslatef(0, 0, 1);
							drawCircle(0, 0, radius, 100);
							gxPopMatrix();
							
							gxBegin(GX_LINES);
							gxVertex3f(0, 0, 0);
							gxVertex3f(0, 0, 1);
							
							gxVertex3f(0, 0, 0); gxVertex3f(-radius, 0, 1);
							gxVertex3f(0, 0, 0); gxVertex3f(+radius, 0, 1);
							gxVertex3f(0, 0, 0); gxVertex3f(0, -radius, 1);
							gxVertex3f(0, 0, 0); gxVertex3f(0, +radius, 1);
							gxEnd();
						}
						gxPopMatrix();
						popLineSmooth();
					}
					
					pushLineSmooth(true);
					gxPushMatrix();
					{
						const float radius = 1.f;
						
						gxMultMatrixf(directional.m_v);
						setColor(colorYellow);
						
						gxPushMatrix();
						gxTranslatef(0, 0, 1);
						drawCircle(0, 0, radius, 100);
						gxPopMatrix();
					
						gxBegin(GX_LINES);
						gxVertex3f(0, 0, 0);
						gxVertex3f(0, 0, 1);
					
						gxVertex3f(0, 0, 0); gxVertex3f(-radius, 0, 1);
						gxVertex3f(0, 0, 0); gxVertex3f(+radius, 0, 1);
						gxVertex3f(0, 0, 0); gxVertex3f(0, -radius, 1);
						gxVertex3f(0, 0, 0); gxVertex3f(0, +radius, 1);
						gxEnd();
					}
					gxPopMatrix();
					popLineSmooth();
				}
				popBlend();
				popDepthTest();
				
				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ALPHA);
				{
				// todo : add light only shader
					Shader shader("210-light");
					setShader(shader);
					{
						int nextTextureUnit = 0;
						d.setShaderData(shader, nextTextureUnit, worldToView);
						helper.setShaderData(shader, nextTextureUnit);
						
						drawTranslucent();
					}
					clearShader();
				}
				popBlend();
				popDepthTest();
			}
			gxPopMatrix();
			
			projectScreen2d();
			
			//d.showRenderTargets();
			
		#if false
			setColor(colorWhite);
			drawText(4, 4, 12, +1, +1, "(%.2f, %.2f, %.2f)",
				camera.position[0],
				camera.position[1],
				camera.position[2]);
		#endif
		}
		framework.endDraw();
		
		helper.reset();
		
		d.reset();
	}
	
	d.free();
	
	framework.shutdown();
	
	return 0;
}
