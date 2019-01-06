#include "framework.h"
#include "imgui-framework.h"

/*
Coding Challenge #86: Cube Wave by Bees and Bombs
https://www.youtube.com/watch?v=H81Tdrmz2LA
*/

static Color color1(1.f, 0.f, 0.f);
static Color color2(0.f, 1.f, 0.f);
static Color color3(0.f, 0.f, 1.f);

Vec3 computeSize(const float x, const float y, const float time)
{
	const float phase = hypotf(x, y) * 6.f - time * 2.f;
	
	const float sx = 1.f;
	const float sy = (cosf(phase) + 1.f) / 2.f * .6f + .2f;
	const float sz = 1.f;
	
	return Vec3(sx, sy, sz);
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	framework.enableDepthBuffer = true;
	
	if (!framework.init(1000, 800))
		return -1;
	
	Camera3d camera;
	camera.position = Vec3(0, 0, -4);
	
	FrameworkImGuiContext guiCtx;
	guiCtx.init();
	
	int halfSize = 16;
	
	float animTime = 0.f;
	float animSpeed = 1.f;
	
	while (!framework.quitRequested)
	{
		framework.process();
		
		bool inputIsCaptured = false;
		guiCtx.processBegin(framework.timeStep, 1000, 800, inputIsCaptured);
		{
			if (ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::SliderInt("Resolution", &halfSize, 0, 50);
				ImGui::SliderFloat("Animation speed", &animSpeed, 0.f, 40.f, "%.2f", 2.f);
				
				ImGui::PushItemWidth(ImGui::CalcItemWidth() * .6f);
				{
					ImGui::ColorPicker3("Color 1", (float*)&color1);
					ImGui::SameLine();
					ImGui::ColorPicker3("Color 2", (float*)&color2);
					ImGui::SameLine();
					ImGui::ColorPicker3("Color 3", (float*)&color3);
				}
				ImGui::PopItemWidth();
			}
			ImGui::End();
		}
		guiCtx.processEnd();
		
		animTime += animSpeed * framework.timeStep;
		
		Vec3 position(
			cosf(framework.time / 7.f) * 2.f,
			1.3f,
			sinf(framework.time / 7.f) * 2.f);
		
		Mat4x4 viewMatrix;
		viewMatrix.MakeLookat(position, Vec3(), Vec3(0, 1, 0));
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			pushBlend(BLEND_OPAQUE);
			
			gxPushMatrix();
			{
				gxMultMatrixf(viewMatrix.m_v);
				
				for (int xi = -halfSize; xi <= +halfSize; ++xi)
				{
					for (int zi = -halfSize; zi <= +halfSize; ++zi)
					{
						const float x = xi / float(halfSize);
						const float z = zi / float(halfSize);
						
						Vec3 size = computeSize(x, z, animTime);
						
						size[0] /= halfSize * 2.f;
						size[2] /= halfSize * 2.f;
						
						Shader shader("086-cube");
						shader.setImmediate("color1", color1.r, color1.g, color1.b, color1.a);
						shader.setImmediate("color2", color2.r, color2.g, color2.b, color2.a);
						shader.setImmediate("color3", color3.r, color3.g, color3.b, color3.a);
						
						setColor(colorWhite);
						fillCube(Vec3(x, 0, z), size);
					}
				}
			}
			gxPopMatrix();
			
			glDisable(GL_DEPTH_TEST);
			popBlend();
			
			projectScreen2d();
			
			guiCtx.draw();
		}
		framework.endDraw();
	}
	
	guiCtx.shut();
	
	framework.shutdown();
	
	return 0;
}
