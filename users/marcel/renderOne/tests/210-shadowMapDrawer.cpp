#include "forwardLighting.h"
#include "framework.h"
#include "gx_render.h"
#include "renderer.h"
#include <limits>
#include <vector>

/*

ShadowMapDrawer assists in the drawing of shadow maps

it allows one to register a number of lights
and render functions for drawing geometry

on draw, it will prioritize lights to determine whichs ones to draw
shadow maps for. these shadow maps are then packed into a
shadow map atlas, containing linear depth values

the linear depth values from the shadow map atlas can be
fetched from a shader using the shader utils provided
by the library. these can then be directly compared with
distances from arbitrary points to a light

note : shadow maps are packed into an atlas, to make it easier
       to bind all of the shadow maps. this makes it suitable for
       forward rendering, with an arbitrary number of lights with
       optional shadows to be used from shaders

*/

class ShadowMapDrawer
{

private:

	enum LightType
	{
		kLightType_Spot,
		kLightType_Directional
	};

	struct Light
	{
		int id;

		LightType type;
		Mat4x4 lightToWorld;
		Mat4x4 worldToLight;
		float nearDistance;
		float farDistance;
		float spotAngle;
		float directionalExtents;
		
		float viewDistance; // for priority sort
	};
	
	std::vector<DepthTarget> depthTargets;
	std::vector<ColorTarget> colorTargets;
	
	std::vector<Light> lights;
	
	ShaderBuffer shaderBuffer;
	
	ColorTarget depthAtlas;
	ColorTarget colorAtlas;
	
	static void calculateProjectionMatrixForLight(const Light & light, Mat4x4 & projectionMatrix)
	{
		if (light.type == kLightType_Spot)
		{
			const float aspectRatio = 1.f;
			
		#if ENABLE_OPENGL
			projectionMatrix.MakePerspectiveGL(
				light.spotAngle / 180.f * float(M_PI),
				aspectRatio,
				light.nearDistance,
				light.farDistance);
		#else
			projectionMatrix.MakePerspectiveLH(
				light.spotAngle / 180.f * float(M_PI),
				aspectRatio,
				light.nearDistance,
				light.farDistance);
		#endif
		}
		else if (light.type == kLightType_Directional)
		{
		#if ENABLE_OPENGL
			projectionMatrix.MakeOrthoGL(
				-light.directionalExtents,
				+light.directionalExtents,
				-light.directionalExtents,
				+light.directionalExtents,
				light.nearDistance,
				light.farDistance);
		#else
			projectionMatrix.MakeOrthoLH(
				-light.directionalExtents,
				+light.directionalExtents,
				-light.directionalExtents,
				+light.directionalExtents,
				light.nearDistance,
				light.farDistance);
		#endif
		}
		else
		{
			Assert(false);
			projectionMatrix.MakeIdentity();
		}
	}
	
	void generateLinearDepthAtlas()
	{
		pushRenderPass(&depthAtlas, true, nullptr, false, "Shadow depth atlas");
		pushBlend(BLEND_OPAQUE);
		{
			projectScreen2d();
			
			int x = 0;
			
			for (size_t i = 0; i < lights.size() && i < depthTargets.size(); ++i)
			{
				auto & light = lights[i];
				
				gxLoadIdentity();
				gxTranslatef(x, 0, 0);
				
				auto & depthTarget = depthTargets[i];
				
				Mat4x4 viewToProjection;
				calculateProjectionMatrixForLight(light, viewToProjection);
				const Mat4x4 projectionToView = viewToProjection.CalcInv();
				
				Shader shader("renderOne/blit-texture");
				setShader(shader);
				{
					shader.setTexture("source", 0, depthTarget.getTextureId(), false, true);
					drawRect(0, 0, depthTarget.getWidth(), depthTarget.getHeight());
				}
				clearShader();
				
				x += depthTarget.getWidth();
			}
		}
		popBlend();
		popRenderPass();
	}
	
	void generateColorAtlas()
	{
		pushRenderPass(&colorAtlas, true, nullptr, false, "Shadow color atlas");
		pushBlend(BLEND_OPAQUE);
		{
			projectScreen2d();
			
			int x = 0;
		
			for (size_t i = 0; i < lights.size() && i < colorTargets.size(); ++i)
			{
				gxLoadIdentity();
				gxTranslatef(x, 0, 0);
				
				auto & colorTarget = colorTargets[i];
			
				Shader shader("renderOne/blit-texture");
				setShader(shader);
				{
					shader.setTexture("source", 0, colorTarget.getTextureId(), false, true);
					drawRect(0, 0, colorTarget.getWidth(), colorTarget.getHeight());
				}
				clearShader();
				
				x += colorTarget.getWidth();
			}
		}
		popBlend();
		popRenderPass();
	}
	
public:

	RenderFunction drawOpaque;
	RenderFunction drawTranslucent;
	
	bool enableColorShadows = false;

	~ShadowMapDrawer()
	{
		free();
	}
	
	void alloc(const int maxShadowMaps, const int resolution)
	{
		free();
		
		//
		
		depthTargets.resize(maxShadowMaps);
		for (auto & depthTarget : depthTargets)
			depthTarget.init(resolution, resolution, DEPTH_FLOAT32, true, 1.f);
		
		colorTargets.resize(maxShadowMaps);
		for (auto & colorTarget : colorTargets)
			colorTarget.init(resolution, resolution, SURFACE_RGBA8, colorWhite);
		
		depthAtlas.init(maxShadowMaps * resolution, resolution, SURFACE_R32F, colorBlackTranslucent);
		colorAtlas.init(maxShadowMaps * resolution, resolution, SURFACE_RGBA8, colorWhite);
	}
	
	void free()
	{
		for (auto & depthTarget : depthTargets)
			depthTarget.free();
		depthTargets.clear();
		
		for (auto & colorTarget : colorTargets)
			colorTarget.free();
		colorTargets.clear();
		
		depthAtlas.free();
		colorAtlas.free();
	}
	
	void addSpotLight(
		const int id,
		const Mat4x4 & lightToWorld,
		const float angle,
		const float nearDistance,
		const float farDistance)
	{
		Light light;
		light.type = kLightType_Spot;
		light.id = id;
		light.lightToWorld = lightToWorld;
		light.worldToLight = lightToWorld.CalcInv();
		light.spotAngle = angle;
		light.nearDistance = nearDistance;
		light.farDistance = farDistance;
		
		lights.push_back(light);
	}
	
	void addDirectionalLight(
		const int id,
		const Mat4x4 & lightToWorld,
		const float startDistance,
		const float endDistance,
		const float extents)
	{
		Light light;
		light.type = kLightType_Directional;
		light.id = id;
		light.lightToWorld = lightToWorld;
		light.worldToLight = lightToWorld.CalcInv();
		light.nearDistance = startDistance;
		light.farDistance = endDistance;
		light.directionalExtents = extents;
		
		lights.push_back(light);
	}
	
	void drawShadowMaps(const Mat4x4 & worldToView)
	{
		// prioritize lights
		
		for (auto & light : lights)
		{
			if (light.type == kLightType_Spot)
			{
				const Vec3 lightPosition_view = worldToView.Mul4(light.lightToWorld.GetTranslation());
				
				light.viewDistance = lightPosition_view.CalcSize();
			}
			else if (light.type == kLightType_Directional)
			{
				light.viewDistance = 0.f;
			}
			else
			{
				Assert(false);
			}
		}
	
		std::sort(lights.begin(), lights.end(),
			[](const Light & light1, const Light & light2)
			{
				return light1.viewDistance < light2.viewDistance;
			});
	
		std::vector<Light> lightsToDraw;
		
		for (size_t i = 0; i < lights.size() && i < depthTargets.size(); ++i)
		{
			lightsToDraw.push_back(lights[i]);
		}
		
		// draw shadow maps
		
		for (size_t i = 0; i < lights.size() && i < depthTargets.size(); ++i)
		{
			auto & light = lights[i];
			
			pushRenderPass(nullptr, false, &depthTargets[i], true, "Shadow depth");
			pushDepthTest(true, DEPTH_LESS);
			pushColorWriteMask(0, 0, 0, 0);
			pushBlend(BLEND_OPAQUE);
			pushDepthBias(1, 1);
			{
				if (light.type == kLightType_Spot)
				{
					Mat4x4 viewToProjection;
					calculateProjectionMatrixForLight(light, viewToProjection);
					
					gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
					gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
					
					if (drawOpaque != nullptr)
						drawOpaque();
				}
				else if (light.type == kLightType_Directional)
				{
					Mat4x4 viewToProjection;
					calculateProjectionMatrixForLight(light, viewToProjection);
					
					gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
					gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
					
					if (drawOpaque != nullptr)
						drawOpaque();
				}
			}
			popDepthBias();
			popBlend();
			popColorWriteMask();
			popDepthTest();
			popRenderPass();
		}
		
		if (enableColorShadows)
		{
			for (size_t i = 0; i < lights.size() && i < colorTargets.size(); ++i)
			{
				auto & light = lights[i];
				
				pushRenderPass(&colorTargets[i], true, &depthTargets[i], false, "Shadow color");
				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ALPHA); // todo : use blend mode where color and alpha are invert multiplied to generate an opacity mask
				{
					if (light.type == kLightType_Spot)
					{
						Mat4x4 viewToProjection;
						calculateProjectionMatrixForLight(light, viewToProjection);
						
						gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
						gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
						
						if (drawTranslucent != nullptr)
							drawTranslucent();
					}
					else if (light.type == kLightType_Directional)
					{
						Mat4x4 viewToProjection;
						calculateProjectionMatrixForLight(light, viewToProjection);
						
						gxSetMatrixf(GX_PROJECTION, viewToProjection.m_v);
						gxSetMatrixf(GX_MODELVIEW, light.worldToLight.m_v);
						
						if (drawOpaque != nullptr)
							drawOpaque();
					}
					else
					{
						Assert(false);
					}
				}
				popBlend();
				popDepthTest();
				popRenderPass();
			}
		}
		
		generateLinearDepthAtlas();
		
		if (enableColorShadows)
		{
			generateColorAtlas();
		}
	}
	
	void setShaderData(Shader & shader, int & nextTextureUnit, const Mat4x4 & worldToView)
	{
		shader.setTexture("shadowDepthAtlas", nextTextureUnit++, depthAtlas.getTextureId(), false);
		shader.setTexture("shadowColorAtlas", nextTextureUnit++, enableColorShadows ? colorAtlas.getTextureId() : 0, true);
	
		shader.setImmediate("numShadowMaps", depthTargets.size());
		shader.setImmediate("enableColorShadows", enableColorShadows ? 1.f : 0.f);
	
	// todo : compute shadow matrices only once and store in a buffer
		const Mat4x4 viewToWorld = worldToView.CalcInv();
		
		const int numShadowMatrices = lights.size();
		Mat4x4 * shadowMatrices = (Mat4x4*)alloca(numShadowMatrices * sizeof(Mat4x4));
		
		for (size_t i = 0; i < lights.size(); ++i)
		{
			auto & light = lights[i];
			auto & shadowMatrix = shadowMatrices[i];
			
			Mat4x4 lightToProjection;
			calculateProjectionMatrixForLight(light, lightToProjection);
			
			shadowMatrix = lightToProjection * light.worldToLight * viewToWorld;
		}
		
		shader.setImmediateMatrix4x4Array("shadowMatrices", (float*)shadowMatrices, numShadowMatrices);
	}
	
	int getShadowMapId(const int id) const
	{
		for (size_t i = 0; i < lights.size(); ++i)
			if (lights[i].id == id)
				return i;
		
		return -1;
	}
	
	void reset()
	{
		lights.clear();
	}
	
	void showRenderTargets() const
	{
		const int sx = 40;
		const int sy = 40;
		
		pushBlend(BLEND_OPAQUE);
		gxPushMatrix();
		{
			setColor(colorWhite);
			
			gxPushMatrix();
			{
				for (auto & depthTarget : depthTargets)
				{
					gxSetTexture(depthTarget.getTextureId());
					setLumif(.1f);
					drawRect(0, 0, sx, sy);
					setLumif(1.f);
					gxSetTexture(0);
					gxTranslatef(sx, 0, 0);
				}
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
			
			gxPushMatrix();
			{
				for (auto & colorTarget : colorTargets)
				{
					gxSetTexture(colorTarget.getTextureId());
					drawRect(0, 0, sx, sy);
					gxSetTexture(0);
					gxTranslatef(sx, 0, 0);
				}
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
			
			gxPushMatrix();
			{
				pushColorPost(POST_SET_RGB_TO_R);
				gxSetTexture(depthAtlas.getTextureId());
				setLumif(.2f);
				drawRect(0, 0, sx * depthTargets.size(), sy);
				setLumif(1.f);
				gxSetTexture(0);
				popColorPost();
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
		
			gxPushMatrix();
			{
				gxSetTexture(colorAtlas.getTextureId());
				drawRect(0, 0, sx * depthTargets.size(), sy);
				gxSetTexture(0);
			}
			gxPopMatrix();
			gxTranslatef(0, sy, 0);
		}
		gxPopMatrix();
		popBlend();
	}
};

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
			d.addSpotLight(id++, spot.transform, spot.angle, spot.nearDistance, spot.farDistance);
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
