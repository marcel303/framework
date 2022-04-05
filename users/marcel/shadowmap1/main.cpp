#include "framework.h"
#include <algorithm>

#define GFX_SX 1200
#define GFX_SY 800

#define SHADOWMAP_SIZE 1024
#define LIGHT_ORTHO_SIZE 1.f // size of the orthographic viewport for lights when not perspective
#define PERSPECTIVE_LIGHT true // quick toggle to select if light #1 is perspective or not
#define LINEAR_DEPTH_FOR_CAMERA false // maps depth texture to linear depth texture. for debugging purposes only
#define DEPTH_TO_WORLD false // maps depth texture to XYZ texture. for debugging purposes only

struct Scene
{
	static const int kNumCubes = 256;
	Vec3 cube_positions[kNumCubes];
	Color cube_colors[kNumCubes];
	
	Scene()
	{
		for (int i = 0; i < kNumCubes; ++i)
		{
			cube_positions[i].Set(random<float>(-10.f, +10.f), random<float>(0.f, +2.f), random<float>(-10.f, +10.f));
			cube_colors[i] = Color::fromHSL(random<float>(0.f, 1.f), .3f, .8f);
			//cube_colors[i] = colorWhite;
		}
	}
	
	mutable Vec3 cube_cornerPositions[kNumCubes];
	mutable Vec2 cube_screenPositions[kNumCubes];
	mutable bool cube_screenVisible[kNumCubes] = { };
	
	void draw(const bool captureScreenPositions) const
	{
		const Vec3 cube_size_base(.5f, .3f, .5f);
	
	#if 0
		pushWireframe(true);
		beginCubeBatch();
		{
			for (int i = 0; i < kNumCubes; ++i)
			{
				setColor(40, 40, 40);
				fillCube(cube_positions[i], cube_size);
			}
		}
		endCubeBatch();
		popWireframe();
	#endif
	
		beginCubeBatch();
		{
			for (int i = 0; i < kNumCubes; ++i)
			{
				const float t = (cosf(framework.time * (1.f + i / 100.f)) + 1.f) / 2.f;
				//const Vec3 cube_size = cube_size_base * lerp<float>(.2f, 1.f, t);
				const Vec3 cube_size = cube_size_base;
				
				setColor(cube_colors[i]);
				fillCube(cube_positions[i], cube_size);
				//fillCylinder(cube_positions[i], cube_size[0], .3f, 3 + (i % 6));
				
				if (captureScreenPositions)
				{
					float w;
					cube_cornerPositions[i] = cube_positions[i] + cube_size;
					cube_screenPositions[i] = transformToScreen(cube_cornerPositions[i], w);
					cube_screenVisible[i] = w > 0.f;
				}
			}
		}
		endCubeBatch();
	
		// draw floor
		fillCube(Vec3(0, -1, 0), Vec3(10, .4f, 10));
	
		// draw walls
		fillCube(Vec3(-10, 10, 0), Vec3(.1f, 10, 10));
	
		clearShader();
	}
	
	void drawScreenOverlay(Vec3Arg cameraPosition)
	{
		// show distance markers

		beginTextBatch();
		{
			for (int i = 0; i < kNumCubes; ++i)
			{
				if (cube_screenVisible[i])
				{
					const Vec3 delta = cube_cornerPositions[i] - cameraPosition;
					const float distance = delta.CalcSize();
					
					//setColor(255, 0, 127);
					//fillCircle(cube_screenPositions[i][0], cube_screenPositions[i][1], 2.f, 20);
					
					setColor(100, 100, 100);
					drawText(cube_screenPositions[i][0], cube_screenPositions[i][1], 12.f, 0, 0, "distance: %f", distance);
				}
			}
		}
		endTextBatch();
	}
};

struct Light
{
	Mat4x4 lightToWorld_transform = Mat4x4(true);
	Mat4x4 worldToClip_transform = Mat4x4(true);
	
	struct
	{
		float min = .5f;
		float max = 10.f;
	} depthRange;
	
	bool isPerspective = true;
	float fov = 60.f;
	float orthoSize = 1.f;
	
	Color color = colorWhite;
	
	bool isShadowCasting = false;
	
	void calculateTransforms()
	{
		Mat4x4 projection;
	
		if (isPerspective)
		{
		#if ENABLE_OPENGL
			projection.MakePerspectiveGL(
				fov * float(M_PI) / 180.f, 1.f,
				depthRange.min,
				depthRange.max);
		#else
			projection.MakePerspectiveLH(
				fov * float(M_PI) / 180.f, 1.f,
				depthRange.min,
				depthRange.max);
		#endif
		}
		else
		{
		#if ENABLE_OPENGL
			projection.MakeOrthoGL(
				-orthoSize, +orthoSize,
				-orthoSize, +orthoSize,
				depthRange.min,
				depthRange.max);
		#else
			projection.MakeOrthoLH(
				-orthoSize, +orthoSize,
				-orthoSize, +orthoSize,
				depthRange.min,
				depthRange.max);
		#endif
		}
	
		const Mat4x4 worldToLight = lightToWorld_transform.CalcInv();
		
		worldToClip_transform = projection * worldToLight;
	}
};

class LightDrawer
{
public:
	struct Properties
	{
		int lightMapWidth = 0;
		int lightMapHeight = 0;
		
		int shadowMapSize = 2048;
		
		void setLightMapSize(const int in_lightMapWidth, const int in_lightMapHeight)
		{
			lightMapWidth = in_lightMapWidth;
			lightMapHeight = in_lightMapHeight;
		}
		
		void setShadowMapSize(const int in_shadowMapSize)
		{
			shadowMapSize = in_shadowMapSize;
		}
	};
	
	bool init(const Properties & in_properties)
	{
		properties = in_properties;
		
		//
		
		bool result = true;
		
		// allocate shadow map
		
		{
			SurfaceProperties surface_properties;
			surface_properties.dimensions.init(properties.shadowMapSize, properties.shadowMapSize);
			surface_properties.depthTarget.init(DEPTH_FLOAT32, false);
			shadowMap = new Surface();
			result &= shadowMap->init(surface_properties);
		}

		// allocate light map
		
		{
			SurfaceProperties surface_properties;
			surface_properties.dimensions.init(properties.lightMapWidth, properties.lightMapHeight);
			surface_properties.colorTarget.init(SURFACE_RGBA16F, false);
			lightMap = new Surface();
			result &= lightMap->init(surface_properties);
		}
		
		//
		
		if (result == false)
		{
			shut();
		}
		
		return result;
	}
	
	void shut()
	{
		delete lightMap;
		lightMap = nullptr;
		
		delete shadowMap;
		shadowMap = nullptr;
	}
	
	void captureSceneMatrices()
	{
		Mat4x4 projectionMatrix;
		Mat4x4 modelViewMatrix;
		
		gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
		gxGetMatrixf(GX_MODELVIEW, modelViewMatrix.m_v);
		
		drawState.worldToView = modelViewMatrix;
		drawState.projectionToWorld = (projectionMatrix * modelViewMatrix).CalcInv();
	}
	
	void drawBegin(const GxTextureId sceneDepthTexture, const GxTextureId sceneNormalTexture)
	{
		drawState.sceneDepthTexture = sceneDepthTexture;
		drawState.sceneNormalTexture = sceneNormalTexture;
		
		lightMap->clear();
	}
	
	void drawEnd()
	{
	}
	
	//
	
	void drawShadowMapBegin(const Light & light)
	{
		pushSurface(shadowMap);
		shadowMap->clearDepth(1.f);
		
		gxMatrixMode(GX_PROJECTION);
		gxPushMatrix();
		gxLoadIdentity();
	#if ENABLE_METAL
		gxScalef(1, -1, 1); // Metal NDC/clip-space bottom-y is -1
	#endif
		gxMultMatrixf(light.worldToClip_transform.m_v);
		gxMatrixMode(GX_MODELVIEW);
		
		pushDepthTest(true, DEPTH_LESS, true);
		pushColorWriteMask(0, 0, 0, 0);
		pushBlend(BLEND_OPAQUE);
		
		setDepthBias(2, 2);
	}
	
	void drawShadowMapEnd(const Light & light)
	{
		setDepthBias(0, 0);
		
		popBlend();
		popColorWriteMask();
		popDepthTest();
		
		gxMatrixMode(GX_PROJECTION);
		gxPopMatrix();
		gxMatrixMode(GX_MODELVIEW);
		
		popSurface();
	}
	
	//
	
	void drawShadowLightBegin(const Light & light)
	{
		drawShadowMapBegin(light);
	}
	
	void drawShadowLightEnd(const Light & light)
	{
		drawShadowMapEnd(light);

		//
		
		pushSurface(lightMap);
		setColorClamp(false);
		pushBlend(BLEND_ADD_OPAQUE);
		{
			projectScreen2d();
			
			Shader shader("shadowmap1/deferredLightWithShadow");
			setShader(shader);
			shader.setTexture("depthTexture", 0, drawState.sceneDepthTexture, false, true);
			shader.setTexture("normalTexture", 1, drawState.sceneNormalTexture, false, true);
			shader.setImmediateMatrix4x4("projectionToWorld", drawState.projectionToWorld.m_v);
			shader.setTexture("lightDepthTexture", 2, shadowMap->getDepthTexture(), false, true);
			shader.setImmediateMatrix4x4("lightMVP", light.worldToClip_transform.m_v);
			shader.setImmediate("lightColor",
				light.color.r * light.color.a,
				light.color.g * light.color.a,
				light.color.b * light.color.a);
			const Vec3 lightPosition_view = drawState.worldToView.Mul4(light.lightToWorld_transform.GetTranslation());
			shader.setImmediate("lightPosition_view", lightPosition_view[0], lightPosition_view[1], lightPosition_view[2]);
			shader.setImmediateMatrix4x4("viewMatrix", drawState.worldToView.m_v);
			
			drawRect(0, 0, GFX_SX, GFX_SY);
		}
		popBlend();
		setColorClamp(true);
		popSurface();
	}
	
	void drawLight(const Light & light)
	{
		setColorClamp(false);
		pushBlend(BLEND_ADD_OPAQUE);
		{
			Shader shader("shadowmap1/deferredLight");
			setShader(shader);
			shader.setTexture("depthTexture", 0, drawState.sceneDepthTexture, false, true);
			shader.setTexture("normalTexture", 1, drawState.sceneNormalTexture, false, true);
			shader.setImmediateMatrix4x4("projectionToWorld", drawState.projectionToWorld.m_v);
			shader.setImmediateMatrix4x4("lightMVP", light.worldToClip_transform.m_v);
			shader.setImmediate("lightColor",
				light.color.r * light.color.a,
				light.color.g * light.color.a,
				light.color.b * light.color.a);
			
			drawRect(0, 0, GFX_SX, GFX_SY);
		}
		popBlend();
		setColorClamp(true);
	}
	
	Surface * getLightMapSurface()
	{
		return lightMap;
	}
	
	Surface * getShadowMapSurface()
	{
		return shadowMap;
	}
	
private:
	Properties properties;
	
	Surface * lightMap = nullptr;
	Surface * shadowMap = nullptr;
	
	struct
	{
		GxTextureId sceneDepthTexture;
		GxTextureId sceneNormalTexture;
		Mat4x4 worldToView;
		Mat4x4 projectionToWorld;
	} drawState;
};

class LightManager
{
	std::vector<Light*> lights;
	
public:
	void registerLight(Light * light)
	{
		lights.push_back(light);
	}
	
	void unregisterLight(Light * light)
	{
		auto i = std::find(lights.begin(), lights.end(), light);
		
		Assert(i != lights.end());
		if (i != lights.end())
			lights.erase(i);
	}
	
	const std::vector<Light*> access_lights() const
	{
		return lights;
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.allowHighDpi = false;
	//framework.fullscreen = true;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	Scene scene;
	
	Camera3d camera;
	camera.position.Set(0, 2, -2);
	//camera.maxForwardSpeed *= 2.f;
	//camera.maxStrafeSpeed *= 2.f;
	//camera.maxUpSpeed *= 2.f;
	
	LightDrawer lightDrawer;
	{
		LightDrawer::Properties properties;
		properties.setLightMapSize(GFX_SX, GFX_SY);
		properties.setShadowMapSize(SHADOWMAP_SIZE);
		lightDrawer.init(properties);
	}
	
	LightManager lightManager;
	
	Light light1;
	light1.isPerspective = PERSPECTIVE_LIGHT;
	light1.isShadowCasting = true;
	light1.depthRange.min = .5f;
	light1.depthRange.max = 20.f;
	light1.fov = 60.f;
	light1.orthoSize = LIGHT_ORTHO_SIZE;
	lightManager.registerLight(&light1);
	
	Light light2;
	light2.isPerspective = true;
	light2.isShadowCasting = true;
	light2.depthRange.min = .5f;
	light2.depthRange.max = 10.f;
	light2.fov = 30.f;
	light2.orthoSize = LIGHT_ORTHO_SIZE;
	lightManager.registerLight(&light2);
	
	Light light3;
	light3.isPerspective = true;
	light3.isShadowCasting = true;
	light3.depthRange.min = .5f;
	light3.depthRange.max = 20.f;
	light3.fov = 60.f;
	light3.orthoSize = 20.f;
	lightManager.registerLight(&light3);
	
	enum DrawMode
	{
		kDrawMode_CameraColor,
		kDrawMode_CameraDepth,
		kDrawMode_CameraDepthLinear,
		kDrawMode_LightDepth,
		kDrawMode_CameraWorldPosition,
		kDrawMode_DeferredShadow,
		kDrawMode_CameraNormal,
		kDrawMode_LightBuffer
	};
	
	DrawMode drawMode = kDrawMode_CameraColor;
	
	const float znear = .1f;
	const float zfar = 100.f;
	
	Surface view_camera;
	{
		SurfaceProperties properties;
		properties.dimensions.init(GFX_SX, GFX_SY);
		properties.colorTarget.init(SURFACE_RGBA8, false);
		properties.depthTarget.init(DEPTH_FLOAT32, false);
		view_camera.init(properties);
	}
	
	Surface view_camera_normal;
	{
		SurfaceProperties properties;
		properties.dimensions.init(GFX_SX, GFX_SY);
		properties.colorTarget.init(SURFACE_RGBA16F, false);
		properties.depthTarget.init(DEPTH_FLOAT32, false); // fixme : re-use depth buffer or support MRT
		view_camera_normal.init(properties);
	}
	
#if LINEAR_DEPTH_FOR_CAMERA
	Surface view_camera_linear;
	{
		SurfaceProperties properties;
		properties.dimensions.init(GFX_SX, GFX_SY);
		properties.colorTarget.init(SURFACE_R16F, false);
		properties.colorTarget.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
		view_camera_linear.init(properties);
	}
#endif

#if DEPTH_TO_WORLD
	Surface view_camera_world_position;
	{
		SurfaceProperties properties;
		properties.dimensions.init(GFX_SX, GFX_SY);
		properties.colorTarget.init(SURFACE_RGBA16F, false);
		view_camera_world_position.init(properties);
	}
#endif

	setFont("calibri.ttf");
	
	float depthLinearDrawScale = 1.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (keyboard.wentDown(SDLK_1))
			drawMode = kDrawMode_CameraColor;
		if (keyboard.wentDown(SDLK_2))
			drawMode = kDrawMode_CameraDepth;
		if (keyboard.wentDown(SDLK_3))
			drawMode = kDrawMode_CameraDepthLinear;
		if (keyboard.wentDown(SDLK_4))
			drawMode = kDrawMode_LightDepth;
		if (keyboard.wentDown(SDLK_5))
			drawMode = kDrawMode_CameraWorldPosition;
		if (keyboard.wentDown(SDLK_6))
			drawMode = kDrawMode_DeferredShadow;
		if (keyboard.wentDown(SDLK_7))
			drawMode = kDrawMode_CameraNormal;
		if (keyboard.wentDown(SDLK_8))
			drawMode = kDrawMode_LightBuffer;
		if (keyboard.wentDown(SDLK_o))
			depthLinearDrawScale /= 2.f;
		if (keyboard.wentDown(SDLK_p))
			depthLinearDrawScale *= 2.f;
		
		camera.tick(framework.timeStep, true);
		
		// update light transforms
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			light1.lightToWorld_transform = camera.getWorldMatrix();
			light1.color.a = 8.f;
		}
		light1.color.a = lerp<float>(10.f, 23.f, (cosf(framework.time * 10.f) + 1.f) / 2.f);
		//light1.color.a *= powf(.4f, framework.timeStep);
		//light1.depthRange.max = (cosf(framework.time) + 2.f) / 3.f * 10.f;
		light1.calculateTransforms();
		
		light2.lightToWorld_transform.MakeLookat(
				Vec3(cosf(framework.time / 2.34f) * 3.f, 4, sinf(framework.time / 3.45f) * 3.f),
				Vec3(cosf(framework.time / 1.23f) * 6.f, 0, sinf(framework.time / 4.56f) * 6.f),
				Vec3(0, 1, 0));
		light2.lightToWorld_transform = light2.lightToWorld_transform.CalcInv();
		light2.color = Color::fromHSL(framework.time / 6.54f, .4f, .6f);
		light2.color.a = 32.f;
		light2.calculateTransforms();
		
		light3.lightToWorld_transform.MakeLookat(
				Vec3(cosf(framework.time / 10.f) * 3.f, 6, sinf(framework.time / 10.f) * 3.f),
				Vec3(0, 0, 0),
				Vec3(0, 1, 0));
		light3.lightToWorld_transform = light3.lightToWorld_transform.CalcInv();
		light3.color = Color(255, 127, 63, 31);
		light3.color.a = 1.f;
		light3.calculateTransforms();
		
		auto drawLightVolume = [](const Light & light)
		{
			gxPushMatrix();
			{
				gxMultMatrixf(light.lightToWorld_transform.m_v);
				
				setColor(100, 100, 100);
				
				if (light.isPerspective)
				{
					const int resolution = 20;
					
					gxBegin(GX_TRIANGLES);
					{
						const float fov_rad = light.fov * float(M_PI) / 180.f;
						const float w = tanf(fov_rad / 2.f);
						
						for (int i = 0; i < resolution; ++i)
						{
							if ((i % 2) != 0)
								continue;
							
							const float t1 = (i + 0) / float(resolution);
							const float t2 = (i + .1f) / float(resolution);
							
							const float angle1 = t1 * 2.f * float(M_PI);
							const float angle2 = t2 * 2.f * float(M_PI);
							
							const Vec3 p1 = Vec3(0, 0, 0);
							const Vec3 p2 = Vec3(
								cosf(angle1) * w * light.depthRange.max,
								sinf(angle1) * w * light.depthRange.max,
								light.depthRange.max);
							const Vec3 p3 = Vec3(
								cosf(angle2) * w * light.depthRange.max,
								sinf(angle2) * w * light.depthRange.max,
								light.depthRange.max);
							
							const Vec3 d1 = p2 - p1;
							const Vec3 d2 = p3 - p1;
							const Vec3 n = -(d1 % d2).CalcNormalized();
							
							gxNormal3fv(&n[0]);
							gxVertex3fv(&p1[0]);
							gxVertex3fv(&p2[0]);
							gxVertex3fv(&p3[0]);
						}
					}
					gxEnd();
				}
				else
				{
					lineCube(
						Vec3(0, 0, (light.depthRange.min + light.depthRange.max) / 2.f),
						Vec3(light.orthoSize, light.orthoSize, (light.depthRange.max - light.depthRange.min) / 2.f));
				}
				
				
				gxBegin(GX_LINES);
				gxVertex3f(0, 0, 0);
				gxVertex3f(0, 0, 2);
				gxEnd();
				
				gxPushMatrix();
				gxScalef(.2f, .2f, 1.f);
				fillCube(
					Vec3(0, 0, 0),
					Vec3(1, 1, fminf(.02f, light.depthRange.min / 2.f)));
				gxPopMatrix();
			}
			gxPopMatrix();
		};
		
		auto drawScene = [&](const bool captureScreenPositions)
		{
			scene.draw(captureScreenPositions);
			
			if (keyboard.isDown(SDLK_RSHIFT))
			{
				// draw light volume and direction
				
				for (auto * light : lightManager.access_lights())
				{
					drawLightVolume(*light);
				}
			}
		};
	
		// draw scene from the viewpoint of the camera
		
	#if DEPTH_TO_WORLD
		Mat4x4 view_camera_world_to_projection_matrix;
	#endif
		
		pushSurface(&view_camera);
		{
			view_camera.clear();
			view_camera.clearDepth(1.f);
		
			projectPerspective3d(60.f, znear, zfar);
			
			camera.pushViewMatrix();
			{
			#if DEPTH_TO_WORLD
				Mat4x4 mat_p;
				Mat4x4 mat_v;
				gxGetMatrixf(GX_PROJECTION, mat_p.m_v);
				gxGetMatrixf(GX_MODELVIEW, mat_v.m_v);
				view_camera_world_to_projection_matrix = mat_p * mat_v;
			#endif
			
				lightDrawer.captureSceneMatrices();
				
				pushDepthTest(true, DEPTH_LESS, true);
				pushBlend(BLEND_OPAQUE);
				{
					drawScene(true);
				}
				popBlend();
				popDepthTest();
			}
			camera.popViewMatrix();
			
			projectScreen2d();
		}
		popSurface();

		pushSurface(&view_camera_normal);
		pushShaderOutputs("n");
		{
			view_camera_normal.clear();
			view_camera_normal.clearDepth(1.f);
		
			projectPerspective3d(60.f, znear, zfar);
			
			camera.pushViewMatrix();
			{
				pushDepthTest(true, DEPTH_LESS, true);
				pushBlend(BLEND_OPAQUE);
				{
					drawScene(true);
				}
				popBlend();
				popDepthTest();
			}
			camera.popViewMatrix();
			
			projectScreen2d();
		}
		popShaderOutputs();
		popSurface();
		
	#if LINEAR_DEPTH_FOR_CAMERA
		// convert camera depth image to linear depth
		
		pushSurface(&view_camera_linear);
		{
			view_camera_linear.clear();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader depthLinear("depthToLinear");
				setShader(depthLinear);
				depthLinear.setImmediate("projection_zNear", znear);
				depthLinear.setImmediate("projection_zFar", zfar);
				depthLinear.setTexture("depthTexture", 0, view_camera.getDepthTexture());
				drawRect(0, 0, view_camera_linear.getWidth(), view_camera_linear.getHeight());
			}
			popBlend();
		}
		popSurface();
	#endif
	
		//
		
	#if DEPTH_TO_WORLD
		// convert camera depth image to XYZ world coordinates
		
		pushSurface(&view_camera_world_position);
		{
			view_camera_world_position.clear();
			
			const Mat4x4 projectionToWorld = view_camera_world_to_projection_matrix.CalcInv();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader depthLinear("depthToWorld");
				setShader(depthLinear);
				depthLinear.setImmediateMatrix4x4("projectionToWorld", projectionToWorld.m_v);
				depthLinear.setTexture("depthTexture", 0, view_camera.getDepthTexture());
				drawRect(0, 0, view_camera_world_position.getWidth(), view_camera_world_position.getHeight());
			}
			popBlend();
		}
		popSurface();
	#endif
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (drawMode == kDrawMode_CameraColor)
			{
			#if 1
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera.getTexture(), GX_SAMPLE_NEAREST, true);
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxClearTexture();
				popBlend();
			#else
				// would be necessary to capture shadow map textures for forward shadow casting lights
				
			// this is some old coded for drawing forward lit shaded objects
			// currently not in use, but will want to get this back working again
			// requires the light drawer to cache shadow maps first
				Shader shader("shadowmap1/shadedObject");
				setShader(shader);
				shader.setImmediateMatrix4x4("lightMVP", light.worldToClip_transform.m_v);
				shader.setTexture("depthTexture", 0, view_camera.getTexture());
				
				projectPerspective3d(60.f, znear, zfar);
			
				camera.pushViewMatrix();
				{
					pushDepthTest(true, DEPTH_LESS, true);
					pushBlend(BLEND_OPAQUE);
					{
						drawScene(false);
					}
					popBlend();
					popDepthTest();
				}
				camera.popViewMatrix();
				
				projectScreen2d();
				
				clearShader();
			#endif
			}
			else if (drawMode == kDrawMode_CameraDepth)
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera.getDepthTexture(), GX_SAMPLE_NEAREST, true);
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxClearTexture();
				popBlend();
			}
			else if (drawMode == kDrawMode_CameraDepthLinear)
			{
			#if LINEAR_DEPTH_FOR_CAMERA
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_linear.getTexture());
				setLumif(depthLinearDrawScale);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				popBlend();
				
				setColor(100, 100, 100);
				drawText(10, 10, 12.f, 0, 0, "linear draw range: %f .. %f", 0.f, 1.f / depthLinearDrawScale);
			#endif
			}
			else if (drawMode == kDrawMode_LightDepth)
			{
				auto & lights = lightManager.access_lights();
				
				if (!lights.empty())
				{
					const int size = SHADOWMAP_SIZE / lights.size();
					
					int x = 0;
					
					for (auto * light : lightManager.access_lights())
					{
						if (light->isShadowCasting == false)
							continue;
						
						lightDrawer.drawShadowMapBegin(*light);
						{
							drawScene(false);
						}
						lightDrawer.drawShadowMapEnd(*light);
						
						pushBlend(BLEND_OPAQUE);
						gxSetTexture(lightDrawer.getShadowMapSurface()->getTexture(), GX_SAMPLE_LINEAR, true);
						setColor(colorWhite);
						drawRect(x, 0, x + size, size);
						gxClearTexture();
						popBlend();
						
						x += size;
					}
				}
			}
			else if (drawMode == kDrawMode_CameraWorldPosition)
			{
			#if DEPTH_TO_WORLD
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_world_position.getTexture(), GX_SAMPLE_NEAREST, true);
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxClearTexture();
				popBlend();
			#endif
			}
			else if (drawMode == kDrawMode_DeferredShadow)
			{
				// accumulate lights
				
				lightDrawer.drawBegin(view_camera.getDepthTexture(), view_camera_normal.getTexture());
				{
					auto drawLight = [&](const Light & light)
					{
						if (light.isShadowCasting)
						{
							lightDrawer.drawShadowLightBegin(light);
							{
								drawScene(false);
							}
							lightDrawer.drawShadowLightEnd(light);
						}
						else
						{
							lightDrawer.drawLight(light);
						}
					};
					
					for (auto * light : lightManager.access_lights())
					{
						drawLight(*light);
					}
				}
				lightDrawer.drawEnd();
				
				// apply light to color image
				
				pushBlend(BLEND_OPAQUE);
				Shader shader("shadowmap1/lightApplication");
				setShader(shader);
				shader.setTexture("colorTexture", 0, view_camera.getTexture(), false, true);
				shader.setTexture("lightTexture", 1, lightDrawer.getLightMapSurface()->getTexture(), false, true);
				shader.setImmediate("ambient", .02f, .02f, .02f);
				drawRect(0, 0, GFX_SX, GFX_SY);
				popBlend();
			}
			else if (drawMode == kDrawMode_CameraNormal)
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_normal.getTexture(), GX_SAMPLE_NEAREST, true);
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxClearTexture();
				popBlend();
			}
			else if (drawMode == kDrawMode_LightBuffer)
			{
				// accumulate lights
				
				lightDrawer.drawBegin(view_camera.getDepthTexture(), view_camera_normal.getTexture());
				{
					auto drawLight = [&](const Light & light)
					{
						if (light.isShadowCasting)
						{
							lightDrawer.drawShadowLightBegin(light);
							{
								drawScene(false);
							}
							lightDrawer.drawShadowLightEnd(light);
						}
						else
						{
							lightDrawer.drawLight(light);
						}
					};
					
					for (auto * light : lightManager.access_lights())
					{
						drawLight(*light);
					}
				}
				lightDrawer.drawEnd();
				
				// apply light to color image
				
				pushBlend(BLEND_OPAQUE);
				setColor(colorWhite);
				gxSetTexture(lightDrawer.getLightMapSurface()->getTexture(), GX_SAMPLE_NEAREST, true);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxClearTexture();
				popBlend();
			}
			
		#if 0
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_light.getDepthTexture(), GX_SAMPLE_LINEAR, true);
				setColor(colorWhite);
				drawRect(0, 500, 100, 600);
				gxClearTexture();
				popBlend();
			}
		#endif
			
			if (keyboard.isDown(SDLK_RSHIFT))
			{
				scene.drawScreenOverlay(camera.position);
			}
			
			setColor(255, 200, 200);
			
			switch (drawMode)
			{
			case kDrawMode_CameraColor:
				drawText(10, 10, 12, +1, +1, "Draw mode: Camera Color");
				break;
			case kDrawMode_CameraDepth:
				drawText(10, 10, 12, +1, +1, "Draw mode: Camera Depth");
				break;
			case kDrawMode_CameraDepthLinear:
				drawText(10, 10, 12, +1, +1, "Draw mode: Camera Depth Linear");
				if (LINEAR_DEPTH_FOR_CAMERA == false)
					drawText(10, 30, 12, +1, +1, "(DISABLED at compile-time!)");
				break;
			case kDrawMode_LightDepth:
				drawText(10, 10, 12, +1, +1, "Draw mode: Light Depth");
				break;
			case kDrawMode_CameraWorldPosition:
				drawText(10, 10, 12, +1, +1, "Draw mode: Camera World Position");
				if (DEPTH_TO_WORLD == false)
					drawText(10, 30, 12, +1, +1, "(DISABLED at compile-time!)");
				break;
			case kDrawMode_DeferredShadow:
				drawText(10, 10, 12, +1, +1, "Draw mode: Deferred Shadow");
				break;
			case kDrawMode_CameraNormal:
				drawText(10, 10, 12, +1, +1, "Draw mode: Camera Normal");
				break;
			case kDrawMode_LightBuffer:
				drawText(10, 10, 12, +1, +1, "Draw mode: Light Buffer");
				break;
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
