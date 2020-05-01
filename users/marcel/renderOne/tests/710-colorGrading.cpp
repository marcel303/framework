#include "framework.h"
#include "imgui-framework.h"
#include "renderer.h"
#include <math.h>

using namespace rOne;

float hueShift = 0.f;
float gammaValue = 1.f;
Vec3 colorMultiplier(1.f);
Vec3 colorOffset;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 800))
		return -1;

	Renderer renderer;
	
	RenderOptions renderOptions;
	renderOptions.renderMode = kRenderMode_ForwardShaded;
	renderOptions.colorGrading.enabled = true;
	
	GxTextureId colorGradingTexture = 0;
	
	FrameworkImGuiContext guiCtx;
	guiCtx.init();
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		freeTexture(colorGradingTexture);
		colorGradingTexture = renderOptions.colorGrading.lookupTextureFromSrgbColorTransform(
			[](float r, float g, float b, float * out_rgb)
			{
			#if true
				Color colorTemp(r, g, b);
				colorTemp = colorTemp.hueShift(hueShift);
				Vec3 color(colorTemp.r, colorTemp.g, colorTemp.b);
				
				for (int i = 0; i < 3; ++i)
				{
					color[i] = powf(color[i], gammaValue);
					color[i] *= colorMultiplier[i];
					color[i] += colorOffset[i];
				}
				
				out_rgb[0] = color[0];
				out_rgb[1] = color[1];
				out_rgb[2] = color[2];
			#else
				if (r == 1.f && g == 1.f && b == 1.f)
					logDebug("xxx");
				Color color = Color(r, g, b);
				//color = color.hueShift(framework.time / 10.f);
				//color = color.mulRGB((cosf(framework.time) + 1.f) / 2.f);
				//color.r += (cosf(framework.time) + 1.f) / 2.f;
				
				const float p = (cosf(framework.time) + 1.f) / 2.f * 4.f;
				color.r = powf(color.r, p);
				color.g = powf(color.g, p);
				color.b = powf(color.b, p);
				
				color.r *= 1.2f;
				color.g *= 1.f;
				color.b *= .9f;
				
				out_rgb[0] = color.r;
				out_rgb[1] = color.g;
				out_rgb[2] = color.b;
			#endif
			});
		renderOptions.colorGrading.lookupTexture = colorGradingTexture;
		
		bool inputIsCaptured = false;
		guiCtx.processBegin(framework.timeStep, 800, 800, inputIsCaptured);
		{
			ImGui::Begin("Color grading", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
			{
				ImGui::SliderFloat("Gamma", &gammaValue, 0.f, 4.f);
				ImGui::SliderFloat3("Color Mult", &colorMultiplier[0], 0.f, 4.f);
				ImGui::SliderFloat3("Color Offset", &colorOffset[0], -1.f, +1.f);
				ImGui::SliderFloat("Hue Shift", &hueShift, -1.f, +1.f);
			}
			ImGui::End();
		}
		guiCtx.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			RenderFunctions renderFunctions;
			
			renderFunctions.drawOpaque = [&]()
			{
				gxBegin(GX_QUADS);
				{
					gxColor3f(1, 1, 1);
					gxVertex2f(-1, -1);
					gxVertex2f(+0, -1);
					
					gxColor3f(0, 0, 0);
					gxVertex2f(+0, +0);
					gxVertex2f(-1, +0);
				}
				gxEnd();
				
				gxBegin(GX_QUADS);
				{
					gxColor3f(1, 0, 0);
					gxVertex2f(+0, -1);
					gxVertex2f(+1, -1);
					
					gxColor3f(0, 0, 0);
					gxVertex2f(+1, +0);
					gxVertex2f(+0, +0);
				}
				gxEnd();
				
				gxBegin(GX_QUADS);
				{
					gxColor3f(0, 1, 0);
					gxVertex2f(-1, +0);
					gxVertex2f(+0, +0);
					
					gxColor3f(0, 0, 0);
					gxVertex2f(+0, +1);
					gxVertex2f(-1, +1);
				}
				gxEnd();
				
				gxBegin(GX_QUADS);
				{
					gxColor3f(0, 0, 1);
					gxVertex2f(+0, +0);
					gxVertex2f(+1, +0);
					
					gxColor3f(0, 0, 0);
					gxVertex2f(+1, +1);
					gxVertex2f(+0, +1);
				}
				gxEnd();
				
			#if 1
				Shader shader("710-colorCube");
				setShader(shader);
				shader.setImmediate("scale", .2f);
				gxRotatef(10.f*framework.time/1.23f, 1, 0, 0);
				gxRotatef(20.f*framework.time/2.34f, 0, 1, 0);
				gxRotatef(30.f*framework.time/3.45f, 0, 0, 1);
				fillCube(Vec3(), Vec3(1.f));
				clearShader();
			#endif
			};
			
			projectPerspective3d(90.f, .01f, 10.f);
			Mat4x4 cam;
			cam.MakeTranslation(0, 0, 1);
			gxSetMatrixf(GX_MODELVIEW, cam.m_v);
			
			renderer.render(renderFunctions, renderOptions, framework.timeStep);
			
			projectScreen2d();
			
			guiCtx.draw();
		}
		framework.endDraw();
	}
	
	guiCtx.shut();
	
	return 0;
}
