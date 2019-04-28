#include "framework.h"

#define SHADOWMAP_SIZE 1024
#define LIGHT_ORTHO_SIZE 4.f
#define PERSPECTIVE_LIGHT true
#define LINEAR_DEPTH_FOR_CAMERA false
#define LINEAR_DEPTH_FOR_LIGHT false
#define DEPTH_TO_WORLD true

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
	uniform sampler2D linearDepthTexture;

	shader_in vec4 color;
	shader_in vec4 position_lightSpace;

	void main()
	{
		shader_fragColor = color;
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		
		vec3 coords = projected * 0.5 + vec3(0.5);
		float depth = texture(depthTexture, coords.xy).x;
		float linearDepth = texture(linearDepthTexture, coords.xy).x;
		
		//projected.z = (projected.z + 1.0) * 0.5;
		
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
			shader_fragColor.rgb = vec3(linearDepth * 0.1);
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

static const char * s_depthToWorldVs = R"SHADER(
	include engine/ShaderVS.txt

	shader_out vec2 texcoord;

	void main()
	{
		gl_Position = ModelViewProjectionMatrix * in_position4;
		
		texcoord = unpackTexcoord(0);
	}
)SHADER";

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

	uniform sampler2D depthTexture;
	uniform mat4x4 projectionToWorld;

	uniform sampler2D lightDepthTexture;
	uniform mat4x4 lightMVP;

	shader_in vec2 texcoord;

	void main()
	{
		vec4 color = vec4(1.0);
		
		shader_fragColor = color;
		
		float camera_view_depth = texture(depthTexture, texcoord).x;
		
		if (camera_view_depth == 1.0)
		{
			shader_fragColor = vec4(0.0);
			return;
		}
		
		vec3 position_world = depthToWorldPosition(camera_view_depth, texcoord, projectionToWorld);
		vec4 position_lightSpace = lightMVP * vec4(position_world, 1.0);
		
		vec3 projected = position_lightSpace.xyz / position_lightSpace.w;
		
		vec3 coords = projected * 0.5 + vec3(0.5);
		float depth = texture(lightDepthTexture, coords.xy).x;
		
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

int main(int argc, const char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	if (!framework.init(800, 600))
		return -1;
	
	Camera3d camera;
	
	const int kNumCubes = 256;
	Vec3 cube_positions[kNumCubes];
	Color cube_colors[kNumCubes];
	for (int i = 0; i < kNumCubes; ++i)
	{
		cube_positions[i].Set(random<float>(-10.f, +10.f), random<float>(0.f, +2.f), random<float>(-10.f, +10.f));
		cube_colors[i] = Color::fromHSL(random<float>(0.f, 1.f), .5f, .5f);
	}
	Vec3 cube_cornerPositions[kNumCubes];
	Vec2 cube_screenPositions[kNumCubes];
	bool cube_screenVisible[kNumCubes] = { };
	
	struct Light
	{
		Mat4x4 transform = Mat4x4(true);
		float zNear = .5f;
		float zFar = 10.f;
	};
	
	Light light;
	
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
		properties.dimensions.init(800, 600);
		properties.colorTarget.init(SURFACE_RGBA8, false);
		properties.depthTarget.init(DEPTH_FLOAT32, false);
		view_camera.init(properties);
	}
#if LINEAR_DEPTH_FOR_CAMERA
	Surface view_camera_linear;
	{
		SurfaceProperties properties;
		properties.dimensions.init(800, 600);
		properties.colorTarget.init(SURFACE_R32F, false);
		properties.colorTarget.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
		view_camera_linear.init(properties);
	}
#endif

	Surface view_light;
	{
		SurfaceProperties properties;
		properties.dimensions.init(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
		properties.depthTarget.init(DEPTH_FLOAT32, false);
		view_light.init(properties);
	}
#if LINEAR_DEPTH_FOR_LIGHT
	Surface view_light_linear;
	{
		SurfaceProperties properties;
		properties.dimensions.init(SHADOWMAP_SIZE, SHADOWMAP_SIZE);
		properties.colorTarget.init(SURFACE_R32F, false);
		properties.colorTarget.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
		view_light_linear.init(properties);
	}
#endif

#if DEPTH_TO_WORLD
	Surface view_camera_world_position;
	{
		SurfaceProperties properties;
		properties.dimensions.init(800, 600);
		properties.colorTarget.init(SURFACE_RGBA32F, false);
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
		
		// update light transform
		
		if (mouse.isDown(BUTTON_LEFT))
			light.transform = camera.getWorldMatrix();
		
		Mat4x4 lightMVP;
		{
			Mat4x4 projection;
		#if PERSPECTIVE_LIGHT
			projection.MakePerspectiveGL(60.f * float(M_PI) / 180.f, 1.f, light.zNear, light.zFar);
		#else
			projection.MakeOrthoGL(
				-LIGHT_ORTHO_SIZE, +LIGHT_ORTHO_SIZE,
				-LIGHT_ORTHO_SIZE, +LIGHT_ORTHO_SIZE,
				light.zNear, light.zFar);
		#endif
		
			Mat4x4 worldToView = light.transform.CalcInv();
			
			lightMVP = projection * worldToView;
		}
		
		auto drawScene = [&](const bool isShadowPass, const bool captureScreenPositions)
		{
			Shader shader("shadedObject");
			
			if (isShadowPass == false)
			{
				setShader(shader);
				shader.setImmediateMatrix4x4("lightMVP", lightMVP.m_v);
				shader.setTexture("depthTexture", 0, view_light.getDepthTexture());
			#if LINEAR_DEPTH_FOR_LIGHT
				shader.setTexture("linearDepthTexture", 1, view_light_linear.getTexture());
			#endif
			}
			
			const Vec3 cube_size(.4f, .4f, .4f);
			
		#if 1
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
			
			clearShader();
			
			// draw light volume and direction
			
			gxPushMatrix();
			{
				gxMultMatrixf(light.transform.m_v);
				
				setColor(100, 100, 100);
				lineCube(
					Vec3(0, 0, (light.zNear + light.zFar) / 2.f),
					Vec3(LIGHT_ORTHO_SIZE, LIGHT_ORTHO_SIZE, (light.zFar - light.zNear) / 2.f));
				
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
		
		// draw scene from the viewpoint of the light
		
		pushSurface(&view_light);
		{
			view_light.clear();
			view_light.clearDepth(1.f);
			
			Mat4x4 projection;
		#if PERSPECTIVE_LIGHT
			projection.MakePerspectiveGL(60.f * float(M_PI) / 180.f, 1.f, light.zNear, light.zFar);
		#else
			projection.MakeOrthoGL(
				-LIGHT_ORTHO_SIZE,
				+LIGHT_ORTHO_SIZE,
				-LIGHT_ORTHO_SIZE,
				+LIGHT_ORTHO_SIZE,
				light.zNear, light.zFar);
		#endif
		
			gxMatrixMode(GX_PROJECTION);
			gxPushMatrix();
			gxLoadMatrixf(projection.m_v);
			
			Mat4x4 worldToView = light.transform.CalcInv();
			gxMultMatrixf(worldToView.m_v);
			
			pushDepthTest(true, DEPTH_LESS, true);
			pushBlend(BLEND_OPAQUE);
			{
				drawScene(true, false);
			}
			popBlend();
			popDepthTest();
			
			gxPopMatrix();
			
			projectScreen2d();
		}
		popSurface();
		
	#if LINEAR_DEPTH_FOR_LIGHT
		// convert light depth image to linear depth
		
		pushSurface(&view_light_linear);
		{
			view_light_linear.clear();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader depthLinear("depthToLinear");
				depthLinear.setImmediate("projection_zNear", light.zNear);
				depthLinear.setImmediate("projection_zFar", light.zFar);
				depthLinear.setTexture("depthTexture", 0, view_light.getDepthTexture());
				drawRect(0, 0, view_light_linear.getWidth(), view_light_linear.getHeight());
			}
			popBlend();
		}
		popSurface();
	#endif
	
		// draw scene from the viewpoint of the camera
		
		Mat4x4 view_camera_world_to_projection_matrix;
		view_camera_world_to_projection_matrix.MakePerspectiveGL(60.f * float(M_PI) / 180.f, view_camera.getHeight() / float(view_camera.getWidth()), znear, zfar);
		view_camera_world_to_projection_matrix = view_camera_world_to_projection_matrix.Scale(1, -1, 1);
		view_camera_world_to_projection_matrix = view_camera_world_to_projection_matrix * camera.getViewMatrix();
		
		pushSurface(&view_camera);
		{
			view_camera.clear();
			view_camera.clearDepth(1.f);
		
			gxMatrixMode(GX_PROJECTION);
			gxLoadMatrixf(view_camera_world_to_projection_matrix.m_v);
			gxMatrixMode(GX_MODELVIEW);
			gxLoadIdentity();
			
			pushDepthTest(true, DEPTH_LESS, true);
			pushBlend(BLEND_OPAQUE);
			{
				drawScene(false, true);
			}
			popBlend();
			popDepthTest();
			
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
				drawRect(0, 0, 800, 600);
				gxSetTexture(0);
				popBlend();
			}
			else if (drawMode == kDrawMode_CameraDepth)
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera.getDepthTexture());
				setColor(colorWhite);
				drawRect(0, 0, 800, 600);
				gxSetTexture(0);
				popBlend();
			}
			else if (drawMode == kDrawMode_CameraDepthLinear)
			{
			#if LINEAR_DEPTH_FOR_CAMERA
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_linear.getTexture());
				setLumif(depthLinearDrawScale);
				drawRect(0, 0, 800, 600);
				gxSetTexture(0);
				popBlend();
				
				setColor(100, 100, 100);
				drawText(10, 10, 12.f, 0, 0, "linear draw range: %f .. %f", 0.f, 1.f / depthLinearDrawScale);
			#endif
			}
			else if (drawMode == kDrawMode_LightDepth)
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_light.getDepthTexture());
				setColor(colorWhite);
				drawRect(0, 0, 800, 600);
				gxSetTexture(0);
				popBlend();
			}
			else if (drawMode == kDrawMode_CameraWorldPosition)
			{
			#if DEPTH_TO_WORLD
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_world_position.getTexture());
				setColor(colorWhite);
				drawRect(0, 0, 800, 600);
				gxSetTexture(0);
				popBlend();
			#endif
			}
			else if (drawMode == kDrawMode_DeferredShadow)
			{
				const Mat4x4 projectionToWorld = view_camera_world_to_projection_matrix.CalcInv();
				
				pushBlend(BLEND_OPAQUE);
				Shader shader("deferredShadow");
				shader.setTexture("depthTexture", 0, view_camera.getDepthTexture());
				shader.setImmediateMatrix4x4("projectionToWorld", projectionToWorld.m_v);
				shader.setTexture("lightDepthTexture", 1, view_light.getDepthTexture());
				shader.setImmediateMatrix4x4("lightMVP", lightMVP.m_v);
				drawRect(0, 0, 800, 600);
				popBlend();
			}
			
		#if 1
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_light.getDepthTexture());
				setColor(colorWhite);
				drawRect(0, 500, 100, 600);
				gxSetTexture(0);
				popBlend();
			}
		#endif
		#if LINEAR_DEPTH_FOR_LIGHT
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_light_linear.getTexture());
				//setColor(colorWhite);
				setLumif(depthLinearDrawScale);
				drawRect(100, 500, 200, 600);
				gxSetTexture(0);
				popBlend();
			}
		#endif
			
			if (mouse.isDown(BUTTON_LEFT))
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
