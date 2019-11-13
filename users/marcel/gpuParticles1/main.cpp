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
			// note : the luminance map is double buffered so it can be post processed/blurred
			
			luminance[i].init(VIEW_SX, VIEW_SY, SURFACE_R8, false, true);
			luminance[i].setName("OpticalFlow.Luminance");
			luminance[i].clear();
			
			sobel[i].init(VIEW_SX, VIEW_SY, SURFACE_RG8, false, false);
			sobel[i].setName("OpticalFlow.Sobel");
			sobel[i].clear(127, 127, 0, 0);
		}
		
		opticalFlow.init(VIEW_SX, VIEW_SY, SURFACE_RG16F, false, false);
		opticalFlow.setName("OpticalFlow.flow");
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
		
	#if 1 // todo : fix issue with Metal where shader buffer params are not set
		// blur the luminance map
		
		pushBlend(BLEND_OPAQUE);
		{
			setShader_GaussianBlurH(luminance[current_luminance].getTexture(), 11, sourceFilter.blurRadius);
			luminance[current_luminance].postprocess();
			clearShader();
			
			setShader_GaussianBlurV(luminance[current_luminance].getTexture(), 11, sourceFilter.blurRadius);
			luminance[current_luminance].postprocess();
			clearShader();
		}
		popBlend();
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
	
	Surface p; // xy = particle position, zw = particle velocity
	
	GxTextureId particleTexture = 0;
	
	struct
	{
		float strength = 1.f;
	} gravity;
	
	struct
	{
		float strength = .1f;
	} repulsion;
	
	struct
	{
		float strength = 1.f;
	} flow;
	
	struct
	{
		bool applyBounds = true;
	} simulation;
	
	void init()
	{
		flow_field.init(VIEW_SX, VIEW_SY, SURFACE_RGBA16F, false, false);
		flow_field.setName("Flowfield");
		
		// note : position and velocity need to be double buffered as they are feedbacking onto themselves
		
		p.init(kNumParticles, 1, SURFACE_RGBA32F, false, true);
		p.setName("ParticleSystem.positionsAndVelocities");
		
		for (int i = 0; i < 2; ++i)
		{
			p.clear();
			p.swapBuffers();
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
		
		particleTexture = generateParticleTexture();
	}
	
	void shut()
	{
	/*
		flow_field.free();
	
		p.free();
		v.free();
	*/
	}
	
	GxTextureId generateParticleTexture()
	{
		float data[32][32][2];
		
		for (int y = 0; y < 32; ++y)
		{
			for (int x = 0; x < 32; ++x)
			{
				const float dx = x / 31.f * 2.f - 1.f;
				const float dy = y / 31.f * 2.f - 1.f;
				const float d = hypotf(dx, dy);
				
				const float vx = d <= 1.f ? dx : 0.f;
				const float vy = d <= 1.f ? dy : 0.f;
				
				data[y][x][0] = vx;
				data[y][x][1] = vy;
			}
		}
		
		return createTextureFromRG32F(data, 32, 32, true, true);
	}
	
	void drawParticleVelocity()
	{
		Shader shader("particle-draw-field");
		setShader(shader);
		{
			shader.setTexture("p", 0, p.getTexture(), false, true);
			shader.setTexture("particleTexture", 1, particleTexture, true, true);
			shader.setImmediate("strength", repulsion.strength);
			gxEmitVertices(GX_TRIANGLES, p.getWidth() * 6);
		}
		clearShader();
	}
	
	void drawParticleColor() const
	{
		Shader shader("particle-draw-color");
		setShader(shader);
		{
			shader.setTexture("p", 0, p.getTexture(), false, true);
			gxEmitVertices(GX_TRIANGLES, p.getWidth() * 6);
		}
		clearShader();
	}
	
	void update_particles(const GxTextureId flowfield)
	{
		const GxTextureId pTex = p.getTexture();
		
		p.swapBuffers();
		
		pushSurface(&p);
		{
		// todo : rename shader to particle-update
			pushBlend(BLEND_OPAQUE);
			Shader shader("particle-update-velocity");
			setShader(shader);
			{
				shader.setTexture("p", 0, pTex, false, true);
				shader.setTexture("flowfield", 1, flowfield, true, true);
				shader.setImmediate("drag", .99f);
				const float mouse_x = 256 + sinf(framework.time / 1.234f) * 100.f;
				const float mouse_y = 256 + sinf(framework.time / 2.345f) * 100.f;
				shader.setImmediate("grav_pos", mouse_x, mouse_y);
				shader.setImmediate("grav_force", gravity.strength);
				shader.setImmediate("flow_strength", flow.strength);
				shader.setImmediate("bounds", 0.f, 0.f, VIEW_SX, VIEW_SY);
				shader.setImmediate("applyBounds", simulation.applyBounds ? 1.f : 0.f);
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
					ImGui::SliderFloat("Repulsion strength", &ps.repulsion.strength, 0.f, 1.f);
					ImGui::SliderFloat("Flow strength", &ps.flow.strength, 0.f, 10.f);
					ImGui::Checkbox("Apply bounds", &ps.simulation.applyBounds);
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
