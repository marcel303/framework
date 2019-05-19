#include "framework.h"
#include "shaders.h"

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
		Shader shader("shadedObject");
	
	/*
		// would be necessary to capture shadow map textures for forward shadow casting lights
	 
		if (applyForwardLighting)
		{
			setShader(shader);
			shader.setImmediateMatrix4x4("lightMVP", light.worldToClip_transform.m_v);
			shader.setTexture("depthTexture", 0, view_light.getDepthTexture());
		}
	*/
	
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
				const Vec3 cube_size = cube_size_base * lerp<float>(.2f, 1.f, t);
				
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
			projection.MakePerspectiveGL(
				fov * float(M_PI) / 180.f, 1.f,
				depthRange.min,
				depthRange.max);
		}
		else
		{
			projection.MakeOrthoGL(
				-orthoSize, +orthoSize,
				-orthoSize, +orthoSize,
				depthRange.min,
				depthRange.max);
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
		
		int shadowMapSize = 1024;
		
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
			surface_properties.depthTarget.init(DEPTH_FLOAT16, false);
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
		drawState.worldToProjection = projectionMatrix * modelViewMatrix;
		drawState.projectionToWorld = drawState.worldToProjection.CalcInv();
	}
	
	void drawBegin(const GxTextureId sceneDepthTexture, const GxTextureId sceneNormalTexture)
	{
		drawState.sceneDepthTexture = sceneDepthTexture;
		drawState.sceneNormalTexture = sceneNormalTexture;
		
		lightMap->clear();
		
		pushSurface(lightMap);
	}
	
	void drawEnd()
	{
		popSurface();
	}
	
	void drawShadowLightBegin(const Light & light)
	{
		pushSurface(shadowMap);
		shadowMap->clearDepth(1.f);
		
		gxMatrixMode(GX_PROJECTION);
		gxPushMatrix();
		gxLoadMatrixf(light.worldToClip_transform.m_v);
		
		pushDepthTest(true, DEPTH_LESS, true);
		pushBlend(BLEND_OPAQUE);
	}
	
	void drawShadowLightEnd(const Light & light)
	{
		popBlend();
		popDepthTest();
		gxPopMatrix();
		popSurface();

		//
		
		setColorClamp(false);
		pushBlend(BLEND_ADD);
		{
			Shader shader("deferredLightWithShadow");
			setShader(shader);
			shader.setTexture("depthTexture", 0, drawState.sceneDepthTexture);
			shader.setTexture("normalTexture", 1, drawState.sceneNormalTexture);
			shader.setImmediateMatrix4x4("projectionToWorld", drawState.projectionToWorld.m_v);
			shader.setTexture("lightDepthTexture", 2, shadowMap->getDepthTexture());
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
	}
	
	void drawLight(const Light & light)
	{
		setColorClamp(false);
		pushBlend(BLEND_ADD);
		{
			Shader shader("deferredLight");
			setShader(shader);
			shader.setTexture("depthTexture", 0, drawState.sceneDepthTexture);
			shader.setTexture("normalTexture", 1, drawState.sceneNormalTexture);
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
	
private:
	Properties properties;
	
	Surface * lightMap = nullptr;
	Surface * shadowMap = nullptr;
	
	struct
	{
		GxTextureId sceneDepthTexture;
		GxTextureId sceneNormalTexture;
		Mat4x4 worldToView;
		Mat4x4 worldToProjection; // todo : remove ?
		Mat4x4 projectionToWorld;
	} drawState;
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif
	
	framework.allowHighDpi = false;
	//framework.fullscreen = true;
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	Scene scene;
	
	Camera3d camera;
	camera.position.Set(0, 2, -2);
	
	LightDrawer lightDrawer;
	{
		LightDrawer::Properties properties;
		properties.setLightMapSize(GFX_SX, GFX_SY);
		properties.setShadowMapSize(SHADOWMAP_SIZE);
		lightDrawer.init(properties);
	}
	
	Light light;
	light.isPerspective = PERSPECTIVE_LIGHT;
	light.isShadowCasting = true;
	light.depthRange.min = .5f;
	light.depthRange.max = 10.f;
	light.fov = 60.f;
	light.orthoSize = LIGHT_ORTHO_SIZE;
	
	Light light2;
	light2.isPerspective = true;
	light2.isShadowCasting = true;
	light2.depthRange.min = .5f;
	light2.depthRange.max = 10.f;
	light2.fov = 60.f;
	light2.orthoSize = LIGHT_ORTHO_SIZE;
	
	Light light3;
	light3.isPerspective = false;
	light3.isShadowCasting = false;
	light3.depthRange.min = .1f;
	light3.depthRange.max = 1000.f;
	light3.orthoSize = 1000000.f;
	
	enum DrawMode
	{
		kDrawMode_CameraColor,
		kDrawMode_CameraDepth,
		kDrawMode_CameraDepthLinear,
		kDrawMode_LightDepth,
		kDrawMode_CameraWorldPosition,
		kDrawMode_DeferredShadow,
		kDrawMode_CameraNormal
	};
	
	DrawMode drawMode = kDrawMode_CameraColor;
	
	const float znear = .1f;
	const float zfar = 100.f;
	
	Surface view_camera;
	{
		SurfaceProperties properties;
		properties.dimensions.init(GFX_SX, GFX_SY);
		properties.colorTarget.init(SURFACE_RGBA8, false);
		properties.depthTarget.init(DEPTH_FLOAT16, false);
		view_camera.init(properties);
	}
	
	Surface view_camera_normal;
	{
		SurfaceProperties properties;
		properties.dimensions.init(GFX_SX, GFX_SY);
		properties.colorTarget.init(SURFACE_RGBA16F, false);
		properties.depthTarget.init(DEPTH_FLOAT16, false); // fixme : re-use depth buffer or support MRT
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

	shaderSource("depthToLinear.vs", s_depthToLinearVs);
	shaderSource("depthToLinear.ps", s_depthToLinearPs);
	shaderSource("shadedObject.vs", s_shadedObjectVs);
	shaderSource("shadedObject.ps", s_shadedObjectPs);
	shaderSource("shadowUtils.txt", s_shadowUtilsTxt);
	shaderSource("depthToWorld.vs", s_depthToWorldVs);
	shaderSource("depthToWorld.ps", s_depthToWorldPs);
	shaderSource("deferredLight.vs", s_deferredLightVs);
	shaderSource("deferredLight.ps", s_deferredLightPs);
	shaderSource("deferredLightWithShadow.vs", s_deferredLightWithShadowVs);
	shaderSource("deferredLightWithShadow.ps", s_deferredLightWithShadowPs);
	shaderSource("lightApplication.vs", s_lightApplicationVs);
	shaderSource("lightApplication.ps", s_lightApplicationPs);
	
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
		if (keyboard.wentDown(SDLK_o))
			depthLinearDrawScale /= 2.f;
		if (keyboard.wentDown(SDLK_p))
			depthLinearDrawScale *= 2.f;
		
		camera.tick(framework.timeStep, true);
		
		// update light transforms
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			light.lightToWorld_transform = camera.getWorldMatrix();
			light.color.a = 8.f;
		}
		//light.color.a = lerp<float>(.5f, 1.3f, (cosf(framework.time * 10.f) + 1.f) / 2.f);
		light.color.a *= powf(.4f, framework.timeStep);
		light.calculateTransforms();
		
		light2.lightToWorld_transform.MakeLookat(
				Vec3(cosf(framework.time / 2.34f) * 3.f, 4, sinf(framework.time / 3.45f) * 3.f),
				Vec3(cosf(framework.time / 1.23f) * 6.f, 0, sinf(framework.time / 4.56f) * 6.f),
				Vec3(0, 1, 0));
		light2.lightToWorld_transform = light2.lightToWorld_transform.CalcInv();
		light2.color = Color::fromHSL(framework.time / 1.45f, .1f, .8f);
		light2.color.a = 16.f;
		light2.calculateTransforms();
		
		light3.lightToWorld_transform.MakeLookat(
				Vec3(cosf(framework.time / 10.f) * 3.f, 6, sinf(framework.time / 10.f) * 3.f),
				Vec3(0, 0, 0),
				Vec3(0, 1, 0));
		light3.color = Color(255, 127, 63, 31);
		light3.calculateTransforms();
		
		auto drawLightVolume = [](const Light & light)
		{
			gxPushMatrix();
			{
				gxMultMatrixf(light.lightToWorld_transform.m_v);
				
				setColor(100, 100, 100);
				lineCube(
					Vec3(0, 0, (light.depthRange.min + light.depthRange.max) / 2.f),
					Vec3(LIGHT_ORTHO_SIZE, LIGHT_ORTHO_SIZE, (light.depthRange.max - light.depthRange.min) / 2.f));
				
				gxBegin(GX_LINES);
				gxVertex3f(0, 0, 0);
				gxVertex3f(0, 0, 2);
				gxEnd();
				
				gxPushMatrix();
				gxScalef(.2f, .2f, 1.f);
				drawGrid3d(1, 1, 0, 1);
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
				
				drawLightVolume(light);
				drawLightVolume(light2);
				drawLightVolume(light3);
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
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera.getTexture());
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				popBlend();
			}
			else if (drawMode == kDrawMode_CameraDepth)
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera.getDepthTexture());
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
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
			/*
			// todo : cache shadow maps for light somewhere ?
			
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_light.getDepthTexture());
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				popBlend();
			*/
			}
			else if (drawMode == kDrawMode_CameraWorldPosition)
			{
			#if DEPTH_TO_WORLD
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_world_position.getTexture());
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
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
					
					drawLight(light);
					drawLight(light2);
					drawLight(light3);
				}
				lightDrawer.drawEnd();
				
				// apply light to color image
				
				pushBlend(BLEND_OPAQUE);
				Shader shader("lightApplication");
				setShader(shader);
				shader.setTexture("colorTexture", 0, view_camera.getTexture());
				shader.setTexture("lightTexture", 1, lightDrawer.getLightMapSurface()->getTexture());
				shader.setImmediate("ambient", .1f, .08f, .06f);
				drawRect(0, 0, GFX_SX, GFX_SY);
				popBlend();
			}
			else if (drawMode == kDrawMode_CameraNormal)
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_normal.getTexture());
				setColor(colorWhite);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				popBlend();
			}
			
		#if 0
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_light.getDepthTexture());
				setColor(colorWhite);
				drawRect(0, 500, 100, 600);
				gxSetTexture(0);
				popBlend();
			}
		#endif
			
			if (keyboard.isDown(SDLK_RSHIFT))
			{
				scene.drawScreenOverlay(camera.position);
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
