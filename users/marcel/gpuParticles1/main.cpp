#include "framework.h"
#include "imgui-framework.h"
#include "video.h"

/*

Optical flow,
GPU particles.

inspired by the work of Thomas Diewald's PixelFlow:
https://diwi.github.io/PixelFlow/

*/

#define VIEW_SX 1024
#define VIEW_SY 1024

static const int kNumParticles = 1024*16;

struct OpticalFlow
{
	Surface luminance[2];
	int current_luminance = 0;
	
	Surface sobel[2];
	int current_sobel = 0;
	
	Surface opticalFlow;
	
	struct
	{
		float blurRadius = 22.f;
	} sourceFilter;
	
	void init()
	{
		for (int i = 0; i < 2; ++i)
		{
			luminance[i].init(VIEW_SX, VIEW_SY, SURFACE_R8, false, true);
			luminance[i].clear();
			
			sobel[i].init(VIEW_SX, VIEW_SY, SURFACE_RGBA8, false, false);
			sobel[i].clear(127, 127, 0, 0);
		}
		
		opticalFlow.init(VIEW_SX, VIEW_SY, SURFACE_RG16F, false, false);
		opticalFlow.clear();
	}
	
	void shut()
	{
	/*
		for (int i = 0; i < 2; ++i)
		{
			luminance[i].shut();
			
			sobel[i].shut();
		}
		
		opticalFlow.free();
	*/
	}
	
	void update(const GxTextureId source)
	{
		// convert source image into luminance map
		
		current_luminance = (current_luminance + 1) % 2;
		
		pushSurface(&luminance[current_luminance]);
		{
			pushBlend(BLEND_OPAQUE);
			Shader shader("filter-rgbToLuminance");
			setShader(shader);
			{
				shader.setTexture("source", 0, source, false, true);
				drawRect(0, 0,
					luminance[current_luminance].getWidth(),
					luminance[current_luminance].getHeight());
			}
			clearShader();
			popBlend();
		}
		popSurface();
		
	#if 1
		{
			setShader_GaussianBlurH(luminance[current_luminance].getTexture(), 11, sourceFilter.blurRadius);
			luminance[current_luminance].postprocess();
			clearShader();
			
			setShader_GaussianBlurV(luminance[current_luminance].getTexture(), 11, sourceFilter.blurRadius);
			luminance[current_luminance].postprocess();
			clearShader();
		}
	#endif
		
		// apply horizontal + vertical sobel filter
		
		current_sobel = (current_sobel + 1) % 2;
		
		pushSurface(&sobel[current_sobel]);
		{
			pushBlend(BLEND_OPAQUE);
			Shader shader("filter-sobel");
			setShader(shader);
			{
				shader.setTexture("source", 0, luminance[current_luminance].getTexture(), false, true);
				drawRect(0, 0,
					sobel[current_sobel].getWidth(),
					sobel[current_sobel].getHeight());
			}
			clearShader();
			popBlend();
		}
		popSurface();
		
		// apply the optical flow filter
		
		const int previous_luminance = 1 - current_luminance;
		const int previous_sobel = 1 - current_sobel;
		
		pushSurface(&opticalFlow);
		{
			pushBlend(BLEND_OPAQUE);
			Shader shader("filter-opticalFlow");
			setShader(shader);
			{
				shader.setTexture("luminance_prev", 0, luminance[previous_luminance].getTexture(), false, true);
				shader.setTexture("luminance_curr", 1, luminance[current_luminance].getTexture(), false, true);
				shader.setTexture("sobel_prev", 2, sobel[previous_sobel].getTexture(), false, true);
				shader.setTexture("sobel_curr", 3, sobel[current_sobel].getTexture(), false, true);
				shader.setImmediate("scale", 10.f);
				
				drawRect(0, 0, sobel[current_sobel].getWidth(), sobel[current_sobel].getHeight());
			}
			clearShader();
			popBlend();
		}
		popSurface();
	}
};

struct ParticleSystem
{
	Surface flow_field;
	
	Surface p;
	Surface v;
	
	struct
	{
		float strength = 1.f;
	} gravity;
	
	struct
	{
		float strength = 1.f;
	} flow;
	
	void init()
	{
		flow_field.init(VIEW_SX, VIEW_SY, SURFACE_RGBA32F, false, false);
		
		p.init(kNumParticles, 1, SURFACE_RGBA32F, false, true);
		v.init(kNumParticles, 1, SURFACE_RGBA32F, false, true);
		
		for (int i = 0; i < 2; ++i)
		{
			p.clear();
			v.clear();
			
			p.swapBuffers();
			v.swapBuffers();
		}
		
		setColorClamp(false);
		pushSurface(&p);
		{
			pushBlend(BLEND_OPAQUE);
			gxBegin(GX_POINTS);
			for (int i = 0; i < p.getWidth(); ++i)
			{
				setColorf(rand() % VIEW_SX, rand() % VIEW_SY, 0, 0);
				gxVertex2f(i, 0.5f);
			}
			gxEnd();
			popBlend();
		}
		popSurface();
	}
	
	void shut()
	{
	/*
		flow_field.free();
	
		p.free();
		v.free();
	*/
	}
	
	void draw_flow_field()
	{
		pushSurface(&flow_field);
		{
			flow_field.clear();
			
			pushBlend(BLEND_ADD_OPAQUE);
			{
				Shader shader("particle-field");
				setShader(shader);
				{
					shader.setTexture("p", 0, p.getTexture(), false, true);
					gxEmitVertices(GX_TRIANGLES, p.getWidth() * 6);
				}
				clearShader();
			}
			popBlend();
		}
		popSurface();
	}
	
	void update_particles(const GxTextureId flowfield)
	{
		const GxTextureId pTex = p.getTexture();
		const GxTextureId vTex = v.getTexture();
		
		p.swapBuffers();
		v.swapBuffers();
		
		pushSurface(&v);
		{
			pushBlend(BLEND_OPAQUE);
			Shader shader("particle-update-velocity");
			setShader(shader);
			{
				shader.setTexture("p", 0, pTex, false, true);
				shader.setTexture("v", 1, vTex, false, true);
				shader.setTexture("flowfield", 2, flowfield, true, true);
				shader.setImmediate("drag", .99f);
				const float mouse_x = 256 + sinf(framework.time / 1.234f) * 100.f;
				const float mouse_y = 256 + sinf(framework.time / 2.345f) * 100.f;
				shader.setImmediate("grav_pos", mouse_x, mouse_y);
				shader.setImmediate("grav_force", gravity.strength);
				shader.setImmediate("flow_strength", flow.strength);
				drawRect(0, 0, v.getWidth(), v.getHeight());
			}
			clearShader();
			popBlend();
		}
		popSurface();
		
		pushSurface(&p);
		{
			pushBlend(BLEND_OPAQUE);
			Shader shader("particle-update-position");
			setShader(shader);
			{
				shader.setTexture("p", 0, pTex, false, true);
				shader.setTexture("v", 1, vTex, false, true);
				drawRect(0, 0, p.getWidth(), p.getHeight());
			}
			clearShader();
			popBlend();
		}
		popSurface();
	}
};

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
	opticalFlow.init();
	
	ParticleSystem ps;
	ps.init();
	
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
					ImGui::SliderFloat("Flow strength", &ps.flow.strength, 0.f, 10.f);
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
		
		ps.draw_flow_field();
		
		ps.update_particles(opticalFlow.opticalFlow.getTexture());
		
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
				gxSetTexture(ps.flow_field.getTexture());
				drawRect(0, 0, ps.flow_field.getWidth(), ps.flow_field.getHeight());
				gxSetTexture(0);
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
