#include "framework.h"
#include "gpuParticleSystem.h"
#include "imgui-framework.h"
#include "opticalFlow.h"
#include "video.h"

/*

Optical flow,
GPU particles.

inspired by the work of Thomas Diewald's PixelFlow:
https://diwi.github.io/PixelFlow/

*/

#define VIEW_SX 1080
#define VIEW_SY 1920

static const int kNumParticles = 1024*16;

enum DebugDraw
{
	kDebugDraw_Off,
	kDebugDraw_OpticalFlow_Luminance,
	kDebugDraw_OpticalFlow_Sobel,
	kDebugDraw_OpticalFlow_Flow,
	kDebugDraw_FlowField,
	kDebugDraw_COUNT
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.allowHighDpi = false;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	OpticalFlow opticalFlow;
	opticalFlow.init(VIEW_SX, VIEW_SY);
	
	GpuParticleSystem ps;
	ps.init(kNumParticles, VIEW_SX, VIEW_SY);
	ps.repulsion.strength = .1f;
	ps.dimensions.velocitySize = 2.f;
	ps.dimensions.colorSize = 4.f;
	ps.setBounds(-10., -10., VIEW_SX + 10.f, VIEW_SY + 10.f);
	
	Surface flowField;
	flowField.init(VIEW_SX, VIEW_SY, SURFACE_RGBA16F, false, false);
	flowField.setName("Flowfield");
	flowField.clear();
	
	MediaPlayer mediaPlayer;
	mediaPlayer.openAsync("video.mp4", MP::kOutputMode_RGBA);
	
	DebugDraw debugDraw = kDebugDraw_Off;
	
	float fpsTimer = 0.f;
	int fpsFrame = 0;
	int fps = 0;

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		fpsTimer += framework.timeStep;
		if (fpsTimer >= 1.f)
		{
			fpsTimer = 0.f;
			fps = fpsFrame;
			fpsFrame = 0;
		}

		fpsFrame++;

		bool inputIsCaptured = false;
		guiContext.processBegin(framework.timeStep, VIEW_SX, VIEW_SY, inputIsCaptured);
		{
			ImGui::SetNextWindowSize(ImVec2(370, 0));
			if (ImGui::Begin("Flowfield"))
			{
				ImGui::PushItemWidth(200.f);
				{
					ImGui::Text("%d fps", fps);

					if (ImGui::TreeNodeEx("Particle System", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SliderFloat("Gravity strength", &ps.gravity.strength, 0.f, 10.f);
						ImGui::SliderFloat("Repulsion strength", &ps.repulsion.strength, 0.f, 1.f);
						ImGui::SliderFloat("Flow strength", &ps.flow.strength, 0.f, 10.f);
						ImGui::SliderFloat("Draw velocity size", &ps.dimensions.velocitySize, 0.f, 40.f);
						ImGui::SliderFloat("Draw color size", &ps.dimensions.colorSize, 0.f, 40.f);
						
						ImGui::Checkbox("Bounds enabled", &ps.bounds.enabled);
						ImGui::InputFloat2("Bounds min", &ps.bounds.min[0]);
						ImGui::InputFloat2("Bounds max", &ps.bounds.max[0]);
						const char * modes[] =
						{
							"Off",
							"Bounce",
							"Wrap"
						};
						{
							int mode = ps.bounds.xMode;
							if (ImGui::Combo("Bounds X Mode", &mode, modes, 3))
								ps.bounds.xMode = (GpuParticleSystem::Bounds::Mode)mode;
						}
						{
							int mode = ps.bounds.yMode;
							if (ImGui::Combo("Bounds Y Mode", &mode, modes, 3))
								ps.bounds.yMode = (GpuParticleSystem::Bounds::Mode)mode;
						}
						
						ImGui::TreePop();
					}
					
					if (ImGui::TreeNodeEx("Optical Flow", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::SliderFloat("Blur radius", &opticalFlow.sourceFilter.blurRadius, 0.f, 100.f);
						ImGui::SliderFloat("Flow strength", &opticalFlow.flowFilter.strength, 0.f, 1000.f);
						
						ImGui::TreePop();
					}
					
					ImGui::Text("Draw");
					{
						const char * modes[kDebugDraw_COUNT] =
						{
							"Off",
							"Optical Flow: Luminance",
							"Optical Flow: Sobel",
							"Optical Flow: Flow",
							"Flow Field"
						};
						int mode = debugDraw;
						if (ImGui::Combo("Debug Draw", &mode, modes, kDebugDraw_COUNT))
							debugDraw = (DebugDraw)mode;
					}
				}
				ImGui::PopItemWidth();
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		if (mediaPlayer.presentedLastFrame(mediaPlayer.context))
		{
			auto openParams = mediaPlayer.context->openParams;
			
			mediaPlayer.close(false);
			mediaPlayer.presentTime = 0.0;
			
			mediaPlayer.openAsync(openParams);
		}
		
		mediaPlayer.presentTime += framework.timeStep;
		const bool gotFrame = mediaPlayer.tick(mediaPlayer.context, true);
		
		if (gotFrame)
		{
			opticalFlow.update(mediaPlayer.getTexture());
		}
		
		pushSurface(&flowField);
		{
			pushBlend(BLEND_OPAQUE);
			{
				setColor(colorWhite);
				gxSetTexture(opticalFlow.opticalFlow.getTexture());
				drawRect(0, 0, VIEW_SX, VIEW_SY);
				gxSetTexture(0);
			}
			popBlend();
			
			pushBlend(BLEND_ADD_OPAQUE);
			{
				ps.drawParticleVelocity();
			}
			popBlend();
		}
		popSurface();
		
		ps.updateParticles(flowField.getTexture(), framework.timeStep);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (debugDraw == kDebugDraw_Off)
			{
				pushBlend(BLEND_OPAQUE);
				{
					setColor(255, 255, 255, 63);
					gxSetTexture(mediaPlayer.getTexture());
					drawRect(0, 0, VIEW_SX, VIEW_SY);
					gxSetTexture(0);
				}
				popBlend();
				
				pushBlend(BLEND_ALPHA);
				{
					setColor(255, 255, 255, 255);
					ps.drawParticleColor();
				}
				popBlend();
			}
			else if (debugDraw == kDebugDraw_OpticalFlow_Luminance)
			{
				pushBlend(BLEND_OPAQUE);
				{
					auto & texture = opticalFlow.luminance[opticalFlow.current_luminance];
					setColor(colorWhite);
					gxSetTexture(texture.getTexture());
					drawRect(0, 0, texture.getWidth(), texture.getHeight());
					gxSetTexture(0);
				}
				popBlend();
			}
			else if (debugDraw == kDebugDraw_OpticalFlow_Sobel)
			{
				pushBlend(BLEND_OPAQUE);
				{
					auto & texture = opticalFlow.sobel[opticalFlow.current_sobel];
					setColor(colorWhite);
					gxSetTexture(texture.getTexture());
					drawRect(0, 0, texture.getWidth(), texture.getHeight());
					gxSetTexture(0);
				}
				popBlend();
			}
			else if (debugDraw == kDebugDraw_OpticalFlow_Flow)
			{
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("flowfield-draw-colors");
					setShader(shader);
					{
						shader.setTexture("flowfield", 0, opticalFlow.opticalFlow.getTexture(), true, true);
						pushBlend(BLEND_OPAQUE);
						drawRect(0, 0, VIEW_SX, VIEW_SY);
						popBlend();
					}
					clearShader();
				}
				popBlend();
			}
			else if (debugDraw == kDebugDraw_FlowField)
			{
				Shader shader("flowfield-draw-colors");
				setShader(shader);
				{
					shader.setTexture("flowfield", 0, flowField.getTexture(), true, true);
					pushBlend(BLEND_OPAQUE);
					drawRect(0, 0, VIEW_SX, VIEW_SY);
					popBlend();
				}
				clearShader();
			}

		#if 0
			Shader shader("flowfield-draw-tracers");
			setShader(shader);
			{
				shader.setTexture("flowfield", 0, opticalFlow.opticalFlow.getTexture(), true, true);
				shader.setTexture("colormap", 1, mediaPlayer.getTexture(), true, true);
				pushBlend(BLEND_OPAQUE);
				drawRect(0, 0, VIEW_SX, VIEW_SY);
				popBlend();
			}
			clearShader();
		#endif

			guiContext.draw();
		}
		framework.endDraw();
	}
	
	mediaPlayer.close(true);
	
	ps.shut();
	
	opticalFlow.shut();
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
