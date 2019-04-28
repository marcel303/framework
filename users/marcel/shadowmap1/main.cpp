#include "framework.h"

#define SHADOWMAP_SIZE 1024
#define LIGHT_ORTHO_SIZE 4.f
#define PERSPECTIVE_LIGHT true

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
			shader_fragColor.rgb = vec3(coords.z > depth + 0.005 ? 0.5 : 1.0); // less acne with depth bias
	}
)SHADER";

int main(int argc, const char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	if (!framework.init(800, 600))
		return -1;
	
	Camera3d camera;
	
	const int kNumCubes = 128;
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
		kDrawMode_LightDepth
	};
	
	DrawMode drawMode = kDrawMode_CameraColor;
	
	const float znear = 1.f;
	const float zfar = 100.f;
	
	Surface view_camera(800, 600, true, false, SURFACE_RGBA8);
	Surface view_camera_linear(800, 600, false, false, SURFACE_R32F);
	view_camera_linear.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
	
	Surface view_light(SHADOWMAP_SIZE, SHADOWMAP_SIZE, true, false, SURFACE_RGBA8);
	Surface view_light_linear(SHADOWMAP_SIZE, SHADOWMAP_SIZE, false, false, SURFACE_R32F);
	view_light_linear.setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
	
	shaderSource("depthToLinear.vs", s_depthToLinearVs);
	shaderSource("depthToLinear.ps", s_depthToLinearPs);
	shaderSource("shadedObject.vs", s_shadedObjectVs);
	shaderSource("shadedObject.ps", s_shadedObjectPs);
	
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
		if (keyboard.wentDown(SDLK_o))
			depthLinearDrawScale /= 2.f;
		if (keyboard.wentDown(SDLK_p))
			depthLinearDrawScale *= 2.f;
		
		camera.tick(framework.timeStep, true);
		
		auto drawScene = [&](const bool isShadowPass, const bool captureScreenPositions)
		{
			Shader shader("shadedObject");
			
			if (isShadowPass == false)
			{
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
				
				setShader(shader);
				shader.setImmediateMatrix4x4("lightMVP", lightMVP.m_v);
				shader.setTexture("depthTexture", 0, view_light.getDepthTexture());
				shader.setTexture("linearDepthTexture", 1, view_light_linear.getTexture());
			}
			
			for (int i = 0; i < kNumCubes; ++i)
			{
				//const Vec3 size(.2f, .2f, .2f);
				const Vec3 size(.4f, .4f, .4f);
				
				setColor(40, 40, 40);
				lineCube(cube_positions[i], size);
				
				setColor(cube_colors[i]);
				fillCube(cube_positions[i], size);
				
				if (captureScreenPositions)
				{
					float w;
					cube_cornerPositions[i] = cube_positions[i] + size;
					cube_screenPositions[i] = transformToScreen(cube_cornerPositions[i], w);
					cube_screenVisible[i] = w > 0.f;
				}
			}
			
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
		
		if (mouse.isDown(BUTTON_LEFT))
			light.transform = camera.getWorldMatrix();
		
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
		
		// draw scene from the viewpoint of the camera
		
		pushSurface(&view_camera);
		{
			view_camera.clear();
			view_camera.clearDepth(1.f);
		
			projectPerspective3d(60.f, znear, zfar);
			
			camera.pushViewMatrix();
			pushDepthTest(true, DEPTH_LESS, true);
			pushBlend(BLEND_OPAQUE);
			{
				drawScene(false, true);
			}
			popBlend();
			popDepthTest();
			camera.popViewMatrix();
			
			projectScreen2d();
		}
		popSurface();
		
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
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_camera_linear.getTexture());
				setLumif(depthLinearDrawScale);
				drawRect(0, 0, 800, 600);
				gxSetTexture(0);
				popBlend();
				
				setColor(100, 100, 100);
				drawText(10, 10, 12.f, 0, 0, "linear draw range: %f .. %f", 0.f, 1.f / depthLinearDrawScale);
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
			
		#if 1
			{
				pushBlend(BLEND_OPAQUE);
				gxSetTexture(view_light.getDepthTexture());
				setColor(colorWhite);
				drawRect(0, 500, 100, 600);
				gxSetTexture(0);
				popBlend();
			}
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
			
			// show distance markers
			
			for (int i = 0; i < kNumCubes; ++i)
			{
				if (cube_screenVisible[i])
				{
					const Vec3 delta = cube_cornerPositions[i] - camera.position;
					const float distance = delta.CalcSize();
					
					setColor(255, 0, 127);
					fillCircle(cube_screenPositions[i][0], cube_screenPositions[i][1], 2.f, 20);
					setColor(100, 100, 100);
					drawText(cube_screenPositions[i][0], cube_screenPositions[i][1], 12.f, 0, 0, "distance: %f", distance);
				}
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
