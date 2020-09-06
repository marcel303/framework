#include "framework.h"
#include "gx_mesh.h"
#include "lightDrawer.h"
#include "Noise.h"
#include "renderer.h"
#include <vector>

using namespace rOne;

static std::vector<float> groove;

static const float depth = .1f;
static const float grooveWidth = .1f;
static const float grooveWidth_2 = grooveWidth / 2.f;

static void calculateGrooveFrame(const float sample, Mat4x4 & frame)
{
	const float angle = sample / 6000.f * float(2.0 * M_PI);
	const float radius = 6.f - sample / 40000.f;
	
	frame = Mat4x4(true).RotateY(angle).Translate(radius, 0, 0);
}

static Vec3 getGroovePositionAndVector(const int sample, Vec3 & vector)
{
#if 1
	Mat4x4 frame;
	calculateGrooveFrame(sample, frame);
	
	vector = frame.GetAxis(0).CalcNormalized();
	
	const float value = groove[sample];
	
	return frame.Mul4(Vec3(value, 0, 0));
#else
	const float x = groove[sample];
	const float z = sample / 100.f;
	
	vector = Vec3(1, 0, 0);
	
	return Vec3(x, 0.f, z);
#endif
}

static void drawGroove()
{
	setColor(colorWhite);
	
#if 1
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < groove.size() - 1; ++i)
		{
			Vec3 vector1;
			Vec3 vector2;
			
			const Vec3 p1 = getGroovePositionAndVector(i + 0, vector1);
			const Vec3 p2 = getGroovePositionAndVector(i + 1, vector2);
			
			const Vec3 p1_l = p1 - vector1 * grooveWidth_2;
			const Vec3 p2_l = p2 - vector2 * grooveWidth_2;
			
			const Vec3 p1_r = p1 + vector1 * grooveWidth_2;
			const Vec3 p2_r = p2 + vector2 * grooveWidth_2;
			
			const Vec3 v1_l(p1_l[0],    0.f, p1_l[2]);
			const Vec3 v2_l(  p1[0], -depth,   p1[2]);
			const Vec3 v3_l(  p2[0], -depth,   p2[2]);
			const Vec3 d1_l = v2_l - v1_l;
			const Vec3 d2_l = v3_l - v2_l;
			const Vec3 n_l = (d1_l % d2_l).CalcNormalized();
			
			const Vec3 v1_r(p1_r[0], 0.f,    p1_r[2]);
			const Vec3 v2_r(  p1[0], -depth,   p1[2]);
			const Vec3 v3_r(  p2[0], -depth,   p2[2]);
			const Vec3 d1_r = v2_r - v1_r;
			const Vec3 d2_r = v3_r - v2_r;
			const Vec3 n_r = (d1_r % d2_r).CalcNormalized();
			
			//gxColor3f((-n_l[0] + 1.f) / 2.f, (-n_l[1] + 1.f) / 2.f, (-n_l[2] + 1.f) / 2.f);
			gxNormal3f(-n_l[0], -n_l[1], -n_l[2]);
			gxVertex3f(p1_l[0],    0.f, p1_l[2]);
			gxVertex3f(  p1[0], -depth,   p1[2]);
			gxVertex3f(  p2[0], -depth,   p2[2]);
			gxVertex3f(p2_l[0],    0.f, p2_l[2]);
			
			//gxColor3f((+n_r[0] + 1.f) / 2.f, (+n_r[1] + 1.f) / 2.f, (+n_r[2] + 1.f) / 2.f);
			gxNormal3f(+n_r[0], +n_r[1], +n_r[2]);
			gxVertex3f(p2_r[0],    0.f, p2_r[2]);
			gxVertex3f(  p2[0], -depth,   p2[2]);
			gxVertex3f(  p1[0], -depth,   p1[2]);
			gxVertex3f(p1_r[0],    0.f, p1_r[2]);
		}
	}
	gxEnd();
#else
	gxBegin(GX_LINE_STRIP);
	{
		for (int i = 0; i < groove.size(); ++i)
		{
			gxVertex3f(groove[i] / 100.f, i / 100.f, 0.f);
		}
	}
	gxEnd();
#endif
}

static void drawGrooveMask()
{
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < groove.size() - 1; ++i)
		{
			Vec3 vector1;
			Vec3 vector2;
			
			const Vec3 p1 = getGroovePositionAndVector(i + 0, vector1);
			const Vec3 p2 = getGroovePositionAndVector(i + 1, vector2);
			
			const Vec3 p1_l = p1 - vector1 * grooveWidth_2;
			const Vec3 p2_l = p2 - vector2 * grooveWidth_2;
			
			const Vec3 p1_r = p1 + vector1 * grooveWidth_2;
			const Vec3 p2_r = p2 + vector2 * grooveWidth_2;
			
			gxVertex3f(p1_l[0], 0.f, p1_l[2]);
			gxVertex3f(p1_r[0], 0.f, p1_r[2]);
			gxVertex3f(p2_r[0], 0.f, p2_r[2]);
			gxVertex3f(p2_l[0], 0.f, p2_l[2]);
		}
	}
	gxEnd();
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.allowHighDpi = false;
	
	if (!framework.init(800, 600))
		return -1;
	
	for (int i = 0; i < 180000; ++i)
	{
		const float value = raw_noise_1d(i / 10.f) / 40.f;
		
		groove.push_back(value);
	}
	
	GxVertexBuffer groove_vb;
	GxIndexBuffer groove_ib;
	GxMesh groove_mesh;
	
	gxCaptureMeshBegin(groove_mesh, groove_vb, groove_ib);
	{
		drawGroove();
	}
	gxCaptureMeshEnd();
	
	GxVertexBuffer grooveMask_vb;
	GxIndexBuffer grooveMask_ib;
	GxMesh grooveMask_mesh;
	
	gxCaptureMeshBegin(grooveMask_mesh, grooveMask_vb, grooveMask_ib);
	{
		drawGrooveMask();
	}
	gxCaptureMeshEnd();
	
	Camera3d camera;
	
	Renderer renderer;
	renderer.registerShaderOutputs();
	
	RenderOptions renderOptions;
	//renderOptions.renderMode = kRenderMode_Flat;
	renderOptions.renderMode = kRenderMode_DeferredShaded;
	renderOptions.linearColorSpace = true;
	//renderOptions.deferredLighting.enableStencilVolumes = false;
	//renderOptions.bloom.enabled = true;
	//renderOptions.chromaticAberration.enabled = true;
	//renderOptions.fxaa.enabled = true;
	renderOptions.colorGrading.enabled = true;
	
	mouse.showCursor(false);
	mouse.setRelative(true);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
				if (keyboard.isDown(SDLK_SPACE))
				{
					const float sample = framework.time * 100.f;
					
					Mat4x4 frame;
					calculateGrooveFrame(sample, frame);
					
					Mat4x4 cameraToWorld = frame;
					
					const float height = sinf(framework.time * 1.f) * depth / 6.f;
					cameraToWorld = cameraToWorld.Translate(frame.GetAxis(1) * height);
					
					const float value = groove[sample];
					cameraToWorld = cameraToWorld.Translate(cameraToWorld.GetAxis(0) * value * .2f);
					
					Mat4x4 worldToCamera = cameraToWorld.CalcInv();
					
					gxSetMatrixf(GX_MODELVIEW, worldToCamera.m_v);
				}
				
				auto drawOpaque = [&]()
				{
					setColor(colorWhite);
					//drawGrid3dLine(10, 10, 0, 2);
					
					gxPushMatrix();
					{
						//gxRotatef(framework.time * 10.f, 1.f, .5f, .25f);
					
						// draw the grooves
					
						groove_mesh.draw();
					
						// draw the surface mask
					
						// the surface mask sets stencil bits where the surface is NOT allowed to draw (where it's chiseled away due to the grooves being made)
					
						StencilSetter()
							.op(GX_STENCIL_OP_KEEP, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_REPLACE)
							.writeMask(0x01)
							.comparison(GX_STENCIL_FUNC_ALWAYS, 0x1, 0x1);

						pushColorWriteMask(0, 0, 0, 0);
						pushDepthWrite(false);
						{
							grooveMask_mesh.draw();
						}
						popDepthWrite();
						popColorWriteMask();
					
						// draw the surface fill
					
						StencilSetter()
							.op(GX_STENCIL_OP_KEEP, GX_STENCIL_OP_KEEP, GX_STENCIL_OP_ZERO)
							.writeMask(0x01)
							.comparison(GX_STENCIL_FUNC_NOTEQUAL, 0x01, 0x01);
					
						gxPushMatrix();
						{
							gxRotatef(-90, 1, 0, 0);
							fillCircle(0, 0, 6.1f, 100);
						}
						gxPopMatrix();
					
						clearStencilTest();
					}
					gxPopMatrix();
				};
				
				auto drawLights = [&]()
				{
					g_lightDrawer.drawDeferredDirectionalLight(Vec3(0, -1, 0), Vec3(1, 1, 1), Vec3(1, 0, 0), .01f);

					g_lightDrawer.drawDeferredSpotLight(
						Vec3(0, 5, 0),
						Vec3(
							sinf(framework.time / 1.23f) * .4f,
							-1,
							sinf(framework.time / 2.34f) * .4f).CalcNormalized(),
						M_PI/8.0,
						0.01f,
						20.f,
						Vec3(1, .5f, 0),
						10.f);
					
					g_lightDrawer.drawDeferredSpotLight(
						Vec3(0, 5, 0),
						Vec3(
							sinf(framework.time / 2.34f) * .3f,
							-1,
							sinf(framework.time / 3.45f) * .3f).CalcNormalized(),
						M_PI/8.0,
						0.01f,
						20.f,
						Vec3(0, .5f, 1),
						10.f);
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
	
	framework.shutdown();
	
	return 0;
}
