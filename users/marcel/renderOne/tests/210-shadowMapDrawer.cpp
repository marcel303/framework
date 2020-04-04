#include "framework.h"
#include "gx_render.h"
#include "renderer.h"
#include <vector>

/*

ShadowMapDrawer assists in the drawing of shadow maps

it allows one to register a number of lights
and render functions for drawing geometry

on draw, it will prioritise lights to determine whichs ones to draw
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
		kLightType_Spot
	};

	struct Light
	{
		int id;

		LightType type;
		Mat4x4 worldToLight;
		float nearDistance;
		float farDistance;
		float spotAngle;
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
			
			for (auto & light : lights)
			{
				gxLoadIdentity();
				gxTranslatef(x, 0, 0);
				
				auto & depthTarget = depthTargets[light.id];
				
				Mat4x4 viewToProjection;
				calculateProjectionMatrixForLight(light, viewToProjection);
				const Mat4x4 projectionToView = viewToProjection.CalcInv();
				
				Shader shader("renderOne/depth-to-linear");
				setShader(shader);
				{
					shader.setTexture("source", 0, depthTarget.getTextureId(), false, true);
					shader.setImmediateMatrix4x4("projectionToView", projectionToView.m_v);
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
		
			for (auto & light : lights)
			{
				gxLoadIdentity();
				gxTranslatef(x, 0, 0);
				
				auto & colorTarget = colorTargets[light.id];
			
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
		light.worldToLight = lightToWorld.CalcInv();
		light.spotAngle = angle;
		light.nearDistance = nearDistance;
		light.farDistance = farDistance;
		
		lights.push_back(light);
	}
	
	void drawShadowMaps()
	{
		for (auto & light : lights)
		{
			pushRenderPass(nullptr, false, &depthTargets[light.id], true, "Shadow depth");
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
			}
			popDepthBias();
			popBlend();
			popColorWriteMask();
			popDepthTest();
			popRenderPass();
		}
		
		if (enableColorShadows)
		{
			for (auto & light : lights)
			{
				pushRenderPass(&colorTargets[light.id], true, &depthTargets[light.id], false, "Shadow color");
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
	
		shader.setImmediate("numMaps", depthTargets.size());
		shader.setImmediate("enableColorShadows", enableColorShadows ? 1.f : 0.f);
	
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
	
	void reset()
	{
		lights.clear();
	}
	
	void showRenderTargets()
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
		fillCube(Vec3(), Vec3(1, 1, 1));
		
		gxPushMatrix();
		{
			gxTranslatef(0, -1, 0);
			gxScalef(10, 10, 10);
			setColor(colorWhite);
			drawGrid3d(10, 10, 0, 2);
		}
		gxPopMatrix();
	};
	
	auto drawTranslucent = [&]()
	{
		pushCullMode(CULL_BACK, CULL_CCW);
		
		for (int i = 0; i < 0; ++i)
		{
			const float x = cosf(i / 1.23f) * 2.f;
			const float z = cosf(i / 2.34f) * 2.f;
			
			setColor(0, 0, 0, 100);
			const float s = (sinf(framework.time) + 1.f) / 4.f;
			fillCube(Vec3(x, 2, z), Vec3(s, s, s));
		}
		
		gxPushMatrix();
		{
			setColor(0, 0, 0, 100);
			const float s = (sinf(framework.time) + 1.f) / 2.f * .5f;
			gxTranslatef(0, 2, 0);
			gxRotatef(framework.time * 20.f, 1, 1, 1);
			fillCube(Vec3(), Vec3(s, s, s));
		}
		gxPopMatrix();
		
		popCullMode();
	};
	
	ShadowMapDrawer d;
	d.alloc(4, 1024);

	d.drawOpaque = drawOpaque;
	d.drawTranslucent = drawTranslucent;

	d.enableColorShadows = true;

	Camera3d camera;
	
	struct SpotLight
	{
		Mat4x4 transform = Mat4x4(true);
		float angle = 90.f;
	};
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		// -- add animated spot lights --
		
		std::vector<SpotLight> spots;
		for (int i = 0; i < 3; ++i)
		{
			SpotLight spot;
			spot.transform.MakeLookat(
				Vec3(
					sinf(framework.time / (2.34f + i)) * 2.f,
					sinf(framework.time / (1.23f + i)) + 3.f,
					sinf(framework.time / (3.45f + i)) + (i == 0 ? -2.5f : +2.5f)),
				Vec3(0, 0, 0), Vec3(0, 1, 0));
			spot.transform = spot.transform.CalcInv();
			spot.angle = 90.f;
			//spot.angle = 60.f + sinf(framework.time/4.56f)*30.f;
			
			spots.push_back(spot);
		}
		
		// -- draw shadow maps --
		
		size_t id = 0;

		for (auto & spot : spots)
		{
			d.addSpotLight(id++, spot.transform, spot.angle, .01f, 6.f);
		}
		
		d.drawShadowMaps();
		
		// -- determine view matrix --
		
		Mat4x4 worldToView = camera.getViewMatrix();
		
		if (keyboard.isDown(SDLK_1))
			worldToView = spots[0].transform.CalcInv();
		if (keyboard.isDown(SDLK_2))
			worldToView = spots[1].transform.CalcInv();
		if (keyboard.isDown(SDLK_3))
			worldToView = spots[2].transform.CalcInv();
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			gxPushMatrix();
			gxMultMatrixf(worldToView.m_v);
			{
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("light-with-shadow");
					setShader(shader);
					{
						int nextTextureUnit = 0;
						d.setShaderData(shader, nextTextureUnit, worldToView);
						
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
							//lineCube(Vec3(), Vec3(1, 1, 1));
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
				}
				popBlend();
				popDepthTest();
				
				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ALPHA);
				{
				// todo : add light only shader
					Shader shader("light-with-shadow");
					setShader(shader);
					{
						int nextTextureUnit = 0;
						d.setShaderData(shader, nextTextureUnit, worldToView);
						
						drawTranslucent();
					}
					clearShader();
				}
				popBlend();
				popDepthTest();
			}
			gxPopMatrix();
			
			projectScreen2d();
			
			d.showRenderTargets();
			
			d.reset();
		}
		framework.endDraw();
	}
	
	d.free();
	
	framework.shutdown();
	
	return 0;

	//

	/*
	include renderOne/shadow-map.txt

	uniform vec3 v_position;

	void forEachLightId(int id)
	{
		params = lookupLightParams(id);

		float visibility = 1.0;

		if (params.shadowed)
		{
			// shadow lookup does to transform from view-space to light-space, and the projective shadow lookup

			visibility = lookupShadow(id, v_position);
		}
	}

	void main()
	{
		forEachLightIdAt(v_position);
	}

	-- renderOne/shadow-map.txt --

	uniform vec4 shadowParams[2 * kMaxShadowLights];
	uniform mat4x4 shadowMatrices[kMaxShadowLights];
	uniform sampler2D shadowDepthAtlas; // atlas texture with depth values for all shadow lights
	uniform sampler2D shadowColorAtlas; // atlas texture with color values for all shadow lights
	uniform float numMaps;
	uniform float enableColorShadows;

	*/
}
