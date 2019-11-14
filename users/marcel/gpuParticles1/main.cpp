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

#define VIEW_SX 1024
#define VIEW_SY 1024

static const int kNumParticles = 1024*128;

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
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
	
	MediaPlayer mediaPlayer;
	mediaPlayer.openAsync("video.mp4", MP::kOutputMode_RGBA);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		guiContext.processBegin(framework.timeStep, VIEW_SX, VIEW_SY, inputIsCaptured);
		{
			ImGui::SetNextWindowSize(ImVec2(370, 0));
			if (ImGui::Begin("Flowfield"))
			{
				ImGui::PushItemWidth(200.f);
				{
					ImGui::Text("Particle System");
					ImGui::SliderFloat("Gravity strength", &ps.gravity.strength, 0.f, 10.f);
					ImGui::SliderFloat("Repulsion strength", &ps.repulsion.strength, 0.f, 1.f);
					ImGui::SliderFloat("Flow strength", &ps.flow.strength, 0.f, 10.f);
					ImGui::Checkbox("Bounds enabled", &ps.simulation.applyBounds);
					ImGui::InputFloat2("Bounds min", &ps.bounds.min[0]);
					ImGui::InputFloat2("Bounds max", &ps.bounds.max[1]);
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
					ImGui::Separator();
					
					ImGui::Text("Optical Flow");
					ImGui::SliderFloat("Blur radius", &opticalFlow.sourceFilter.blurRadius, 0.f, 100.f);
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
		
		// todo : create a separate surface for the combined flow
		
		pushSurface(&opticalFlow.opticalFlow);
		{
			pushBlend(BLEND_ADD_OPAQUE);
			{
				ps.drawParticleVelocity();
			}
			popBlend();
		}
		popSurface();
		
		ps.updateParticles(opticalFlow.opticalFlow.getTexture());
		
		framework.beginDraw(0, 0, 0, 0);
		{
		#if 0
			pushBlend(BLEND_OPAQUE);
			{
				setColor(colorWhite);
				gxSetTexture(ps.flow_field.getTexture());
				drawRect(0, 0, ps.flow_field.getWidth(), ps.flow_field.getHeight());
				gxSetTexture(0);
			}
			popBlend();
		#elif 1
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
				setColor(255, 255, 255, 63);
				ps.drawParticleColor();
			}
			popBlend();
		#elif 0
			pushBlend(BLEND_OPAQUE);
			{
				setColor(colorWhite);
				//gxSetTexture(ps.flow_field.getTexture());
				//gxSetTexture(opticalFlow.luminance[opticalFlow.current_luminance].getTexture());
				//gxSetTexture(opticalFlow.sobel[opticalFlow.current_sobel].getTexture());
				gxSetTexture(opticalFlow.opticalFlow.getTexture());
				//gxSetTexture(mediaPlayer.getTexture());
				drawRect(0, 0, ps.flow_field.getWidth(), ps.flow_field.getHeight());
				gxSetTexture(0);
			}
			popBlend();
		#elif 0
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
		#else
			Shader shader("flowfield-draw-colors");
			setShader(shader);
			{
				shader.setTexture("flowfield", 0, opticalFlow.opticalFlow.getTexture(), true, true);
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
