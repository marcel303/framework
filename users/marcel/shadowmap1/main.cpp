#include "framework.h"

#define GFX_SX 1200
#define GFX_SY 800

#define SHADOWMAP_SIZE 1024
#define LIGHT_ORTHO_SIZE 1.f // size of the orthographic viewport for lights when not perspective
#define PERSPECTIVE_LIGHT true // quick toggle to select if light #1 is perspective or not
#define LINEAR_DEPTH_FOR_CAMERA false // maps depth texture to linear depth texture. for debugging purposes only
#define DEPTH_TO_WORLD false // maps depth texture to XYZ texture. for debugging purposes only

// depth to linear shader

static const char * s_depthToLinearVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_depthToLinearPs = R"SHADER(
	include engine/ShaderPS.txt

	uniform float projection_zNear;
	uniform float projection_zFar;
	uniform sampler2D depthTexture;

	shader_in vec2 texcoord;

	void main()
	{
		// depends on our implementation of MakePerspectiveGL actually,
		// so I should really have derived this myself. but taking a
		// shortcut and copy pasting the code from here,
		// http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#setting-up-the-rendertarget-and-the-mvp-matrix
		// works just as it should
		
		float z_b = texture(depthTexture, texcoord).x;
		float z_n = 2.0 * z_b - 1.0;
		float z_e = 2.0 * projection_zNear * projection_zFar / (projection_zFar + projection_zNear - z_n * (projection_zFar - projection_zNear));
		
		shader_fragColor = vec4(z_e);
	}
)SHADER";

// shaded object (forward lighting) shader

static const char * s_shadedObjectVs = R"SHADER(
	include engine/ShaderVS.txt

	uniform mat4x4 lightMVP;

	shader_out vec4 color;
	shader_out vec4 position_lightSpace;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		position_lightSpace = lightMVP * in_position4;
		
		color = unpackColor();
	}
)SHADER";

static const char * s_shadedObjectPs = R"SHADER(
	include engine/ShaderPS.txt

	uniform sampler2D depthTexture;

	shader_in vec4 color;
	shader_in vec4 position_lightSpace;

	void main()
	{
		shader_fragColor = color;
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		
		vec3 coords = projected * 0.5 + vec3(0.5);
		float depth = texture(depthTexture, coords.xy).x;
		
		if (projected.x < -1.0 || projected.x > +1.0)
			shader_fragColor.rgb = vec3(0.5, 0.0, 0.0);
		else if (projected.y < -1.0 || projected.y > +1.0)
			shader_fragColor.rgb = vec3(0.0, 0.5, 0.0);
		else if (projected.z < -1.0 || projected.z > +1.0)
			shader_fragColor.rgb = vec3(0.0, 0.0, 0.5);
		else if (false)
			shader_fragColor.rgb = vec3(projected.z);
		else if (false)
			shader_fragColor.rgb = vec3(coords); // RGB color cube
		else if (false)
			shader_fragColor.rgb = vec3(depth);
		else if (false)
			shader_fragColor.rgb = vec3(coords.z > depth ? 0.5 : 1.0); // correct shadow mapping with serious acne
		else
			shader_fragColor.rgb = vec3(coords.z > depth + 0.001 ? 0.3 : 1.0); // less acne with depth bias
		
		// apply some color
		
		shader_fragColor.rgb += color.rgb * 0.3;
		
		// apply a bit of lighting here
		
		float distance = length(projected.xy);
		shader_fragColor.rgb *= max(0.0, 1.0 - distance);
		shader_fragColor.rgb += vec3(0.1);
	}
)SHADER";

// shadow mapping utility functions include

static const char * s_shadowUtilsTxt = R"SHADER(
	vec3 depthToWorldPosition(float depth, vec2 texcoord, mat4x4 projectionToWorld)
	{
		vec3 coord = vec3(texcoord, depth) * 2.0 - vec3(1.0);

		vec4 position_projection = vec4(coord, 1.0);
		vec4 position_world = projectionToWorld * position_projection;

		position_world /= position_world.w;

		return position_world.xyz;
	}
)SHADER";

// depth to world shader

static const char * s_depthToWorldVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_depthToWorldPs = R"SHADER(
	include engine/ShaderPS.txt
	include shadowUtils.txt

	uniform sampler2D depthTexture;
	uniform mat4x4 projectionToWorld;

	shader_in vec2 texcoord;

	void main()
	{
		float depth = texture(depthTexture, texcoord).x;
		
		vec3 position_world = depthToWorldPosition(depth, texcoord, projectionToWorld);
		
		shader_fragColor = vec4(position_world, 1.0);
	}
)SHADER";

// deferred shadow light shader

static const char * s_deferredShadowVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_deferredShadowPs = R"SHADER(
	include engine/ShaderPS.txt
	include shadowUtils.txt

	#define DEBUG 0

	uniform sampler2D depthTexture;
	uniform mat4x4 projectionToWorld;

	uniform sampler2D lightDepthTexture;
	uniform mat4x4 lightMVP;
	uniform vec3 lightColor;

	shader_in vec2 texcoord;

	void main()
	{
		float camera_view_depth = texture(depthTexture, texcoord).x;
		
		if (camera_view_depth == 1.0)
		{
			// scene background
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 position_world = depthToWorldPosition(camera_view_depth, texcoord, projectionToWorld);
		vec4 position_lightSpace = lightMVP * vec4(position_world, 1.0);
	
		if (position_lightSpace.w <= 0.0)
		{
			// facing in the other direction
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		
		//
		
		vec3 coords = projected * 0.5 + vec3(0.5);
		float depth = texture(lightDepthTexture, coords.xy).x;
		
		vec3 shadowColor;
		
		if (depth == 1.0)
		{
			// maximum
			shadowColor = vec3(1.0);
		}
		else
		{
		#if DEBUG
			if (projected.x < -1.0 || projected.x > +1.0)
				shadowColor = vec3(0.5, 0.0, 0.0);
			else if (projected.y < -1.0 || projected.y > +1.0)
				shadowColor = vec3(0.0, 0.5, 0.0);
			else if (projected.z < -1.0 || projected.z > +1.0)
				shadowColor = vec3(0.0, 0.0, 0.5);
			else if (false)
				shadowColor = vec3(projected.z);
			else if (false)
				shadowColor = vec3(coords); // RGB color cube
			else if (false)
				shadowColor = vec3(depth);
			else if (false)
				shadowColor = vec3(coords.z > depth ? 0.5 : 1.0); // correct shadow mapping with serious acne
			else
				shadowColor = vec3(coords.z > depth + 0.001 ? 0.3 : 1.0); // less acne with depth bias
		#else
			shadowColor = vec3(coords.z > depth + 0.001 ? 0.3 : 1.0); // less acne with depth bias
		#endif
		}
	
		// apply a bit of lighting here
		
		shader_fragColor.rgb = shadowColor * lightColor;
		
		float distanceXY = length(projected.xy);
		float distanceZ = max(0.0, coords.z);
	// todo : make curve value a uniform
		float curveValue = 0.5;
		shader_fragColor.rgb *= pow(max(0.0, 1.0 - distanceXY), curveValue) * max(0.0, 1.0 - distanceZ);
		
		shader_fragColor.a = 1.0; // has to be 1.0 because BLEND_ADD multiplies the rgb with this value before addition
	}
)SHADER";

// deferred light shader

static const char * s_deferredLightVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_deferredLightPs = R"SHADER(
	include engine/ShaderPS.txt
	include shadowUtils.txt

	#define DEBUG 0

	uniform sampler2D depthTexture;
	uniform mat4x4 projectionToWorld;

	uniform mat4x4 lightMVP;
	uniform vec3 lightColor;

	shader_in vec2 texcoord;

	void main()
	{
		float camera_view_depth = texture(depthTexture, texcoord).x;
		
		if (camera_view_depth == 1.0)
		{
			// scene background
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 position_world = depthToWorldPosition(camera_view_depth, texcoord, projectionToWorld);
		vec4 position_lightSpace = lightMVP * vec4(position_world, 1.0);
		
		if (position_lightSpace.w <= 0.0)
		{
			// facing in the other direction
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		
		vec3 coords = projected * 0.5 + vec3(0.5);
	
		// apply a bit of lighting here
		
		shader_fragColor.rgb = lightColor;
		
		float distanceXY = length(projected.xy);
		float distanceZ = max(0.0, coords.z);
	// todo : make curve value a uniform
		float curveValue = 0.5;
		shader_fragColor.rgb *= pow(max(0.0, 1.0 - distanceXY), 0.5) * max(0.0, 1.0 - distanceZ);
		
		shader_fragColor.a = 1.0; // has to be 1.0 because BLEND_ADD multiplies the rgb with this value before addition
	}
)SHADER";

// light application shader

static const char * s_lightApplicationVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

static const char * s_lightApplicationPs = R"SHADER(
	include engine/ShaderPS.txt

	uniform sampler2D colorTexture;
	uniform sampler2D lightTexture;

	uniform vec3 ambient;

	shader_in vec2 texcoord;

	void main()
	{
		vec3 color = texture(colorTexture, texcoord).xyz;
		vec3 light = texture(lightTexture, texcoord).xyz;
		
		light += ambient;
		
		vec3 result = color * light;
	
		shader_fragColor = vec4(result, 1.0);
	}
)SHADER";

int main(int argc, const char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	framework.allowHighDpi = false;
	//framework.fullscreen = true;
	
	if (!framework.init(GFX_SX, GFX_SY))
		return -1;
	
	Camera3d camera;
	camera.position.Set(0, 2, -2);
	
	const int kNumCubes = 256;
	Vec3 cube_positions[kNumCubes];
	Color cube_colors[kNumCubes];
	for (int i = 0; i < kNumCubes; ++i)
	{
		cube_positions[i].Set(random<float>(-10.f, +10.f), random<float>(0.f, +2.f), random<float>(-10.f, +10.f));
		cube_colors[i] = Color::fromHSL(random<float>(0.f, 1.f), .3f, .8f);
		//cube_colors[i] = colorWhite;
	}
	Vec3 cube_cornerPositions[kNumCubes];
	Vec2 cube_screenPositions[kNumCubes];
	bool cube_screenVisible[kNumCubes] = { };
	
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
			
			drawState.worldToProjection = projectionMatrix * modelViewMatrix;
			drawState.projectionToWorld = drawState.worldToProjection.CalcInv();
		}
		
		void drawBegin(const GxTextureId sceneDepthTexture)
		{
			drawState.sceneDepthTexture = sceneDepthTexture;
			
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
				Shader shader("deferredShadow");
				setShader(shader);
				shader.setTexture("depthTexture", 0, drawState.sceneDepthTexture);
				shader.setImmediateMatrix4x4("projectionToWorld", drawState.projectionToWorld.m_v);
				shader.setTexture("lightDepthTexture", 1, shadowMap->getDepthTexture());
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
		
		void drawLight(const Light & light)
		{
			setColorClamp(false);
			pushBlend(BLEND_ADD);
			{
				Shader shader("deferredLight");
				setShader(shader);
				shader.setTexture("depthTexture", 0, drawState.sceneDepthTexture);
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
			Mat4x4 worldToProjection; // todo : remove ?
			Mat4x4 projectionToWorld;
		} drawState;
	};
	
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
		kDrawMode_DeferredShadow
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
	shaderSource("deferredShadow.vs", s_deferredShadowVs);
	shaderSource("deferredShadow.ps", s_deferredShadowPs);
	shaderSource("deferredLight.vs", s_deferredLightVs);
	shaderSource("deferredLight.ps", s_deferredLightPs);
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
		if (keyboard.wentDown(SDLK_o))
			depthLinearDrawScale /= 2.f;
		if (keyboard.wentDown(SDLK_p))
			depthLinearDrawScale *= 2.f;
		
		camera.tick(framework.timeStep, true);
		
		// update light transforms
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			light.lightToWorld_transform = camera.getWorldMatrix();
			light.color.a = 16.f;
		}
		//light.color.a = lerp<float>(.5f, 1.3f, (cosf(framework.time * 10.f) + 1.f) / 2.f);
		light.color.a *= powf(.4f, framework.timeStep);
		light.calculateTransforms();
		
		light2.lightToWorld_transform.MakeLookat(
				Vec3(cosf(framework.time / 2.34f) * 3.f, 6, sinf(framework.time / 3.45f) * 3.f),
				Vec3(cosf(framework.time) * 6.f, 0, sinf(framework.time) * 6.f),
				Vec3(0, 1, 0));
		light2.lightToWorld_transform = light2.lightToWorld_transform.CalcInv();
		light2.color = Color::fromHSL(framework.time / 1.45f, .1f, .8f);
		light2.color.a = 8.f;
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
		
			const Vec3 cube_size_base(.5f, .5f, .5f);
			
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
		
	#if LINEAR_DEPTH_FOR_CAMERA
		// convert camera depth image to linear depth
		
		pushSurface(&view_camera_linear);
		{
			view_camera_linear.clear();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader depthLinear("depthToLinear");
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
				
				lightDrawer.drawBegin(view_camera.getDepthTexture());
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
				shader.setTexture("colorTexture", 0, view_camera.getTexture());
				shader.setTexture("lightTexture", 1, lightDrawer.getLightMapSurface()->getTexture());
				shader.setImmediate("ambient", .1f, .08f, .06f);
				drawRect(0, 0, GFX_SX, GFX_SY);
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
				// show distance markers
				
				beginTextBatch();
				{
					for (int i = 0; i < kNumCubes; ++i)
					{
						if (cube_screenVisible[i])
						{
							const Vec3 delta = cube_cornerPositions[i] - camera.position;
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
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
