#define DO_VIDEOLOOPS 0
#define ENABLE_LEAPMOTION 1

#if DO_VIDEOLOOPS

#include "avtypes.h"
#include "Calc.h"
#include "framework.h"
#include "leapstate.h"
#include "Traveller.h"
#include "videoloop.h"

#if ENABLE_LEAPMOTION
	#include "leap/Leap.h"
#endif

#define MAX_LAYERS 3
#define NUM_LAYERS 1

extern const int GFX_SX;
extern const int GFX_SY;

extern void applyFsfx(Surface & surface, const char * name, const float strength = 1.f, const float param1 = 0.f, const float param2 = 0.f, const float param3 = 0.f, const float param4 = 0.f, GLuint texture1 = 0);

//

struct Grain
{
	float life;
	float lifeRcp;
	
	float px;
	float py;
	float vx;
	float vy;
	float vfalloff;
};

struct GrainsEffect
{
	const static int kMaxGrains = 1000;
	
	Grain grains[kMaxGrains];
	
	GrainsEffect()
	{
		memset(grains, 0, sizeof(grains));
	}
	
	void tick(const float dt)
	{
		for (int i = 0; i < kMaxGrains; ++i)
		{
			grains[i].life -= dt;
			grains[i].vx *= std::powf(1.f - grains[i].vfalloff, dt);
			grains[i].vy *= std::powf(1.f - grains[i].vfalloff, dt);
			grains[i].px += grains[i].vx * dt;
			grains[i].py += grains[i].vy * dt;
		}
	}
	
	void draw() const
	{
		Shader shader("gradient-circle");
		setShader(shader);
		{
			for (int i = 0; i < kMaxGrains; ++i)
			{
				if (grains[i].life > 0.f)
				{
					const float a = grains[i].life * grains[i].lifeRcp;
					const float radius = 40.f * a;
					
					setColorf(1.f, 1.f, 1.f, a * .2f);
					drawRect(
						grains[i].px - radius,
						grains[i].py - radius,
						grains[i].px + radius,
						grains[i].py + radius);
				}
			}
		}
		clearShader();
	}
};

static void applyMask(GLuint a, GLuint b, GLuint mask)
{
	Shader shader("mask");
	setShader(shader);
	{
		shader.setTexture("colormapA", 0, a);
		shader.setTexture("colormapB", 1, b);
		shader.setTexture("mask", 2, mask);
		gxBegin(GL_QUADS);
		{
			gxTexCoord2f(0.f, 1.f); gxVertex2f(0.f * GFX_SX, 0.f * GFX_SY);
			gxTexCoord2f(1.f, 1.f); gxVertex2f(1.f * GFX_SX, 0.f * GFX_SY);
			gxTexCoord2f(1.f, 0.f); gxVertex2f(1.f * GFX_SX, 1.f * GFX_SY);
			gxTexCoord2f(0.f, 0.f); gxVertex2f(0.f * GFX_SX, 1.f * GFX_SY);
		}
		gxEnd();
	}
	clearShader();
}

void testAvpaint()
{
	changeDirectory("/Users/thecat/Google Drive/The Grooop - Welcome");

	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
	#if ENABLE_LEAPMOTION
		// initialise LeapMotion controller

		Leap::Controller leapController;
		leapController.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

		LeapListener * leapListener = new LeapListener();
		leapController.addListener(*leapListener);
	#endif
	
		Surface surface(GFX_SX, GFX_SY, false);
		surface.clear();
		
		Surface mask(GFX_SX, GFX_SY, false);
		
		Surface * layerColors[NUM_LAYERS] = { };
		Surface * layerAlphas[NUM_LAYERS] = { };
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			layerColors[i] = new Surface(GFX_SX, GFX_SY, true);
			layerColors[i]->clear();
			
			layerAlphas[i] = new Surface(GFX_SX, GFX_SY, false);
			layerAlphas[i]->clear();
		}
		
	#if 0
		const char * videoFilenames[MAX_LAYERS] =
		{
			"testvideos/video5.mpg",
			"testvideos/video5.mpg",
			"testvideos/video5.mpg",
		};
	#elif 0
		const char * videoFilenames[MAX_LAYERS] =
		{
			"testvideos/video4.mp4",
			"testvideos/video4.mp4",
			"testvideos/video4.mp4",
		};
	#else
		const char * videoFilenames[MAX_LAYERS] =
		{
			//"testvideos/video1.mpg",
			"testvideos/video6.mpg",
			"testvideos/video2.mpg",
			"testvideos/video4.mp4",
		};
	#endif
		
		VideoLoop * videoLoops[NUM_LAYERS] = { };
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			videoLoops[i] = new VideoLoop(videoFilenames[i]);
			
			videoLoops[i]->tick(0.f);
		}
		
		int activeLayer = 0;
		
		float mouseDownTime = 0.f;
		
		float blurStrength = 0.f;
		float desiredBlurStrength = 0.f;
		
		FollowValue barAngle(0.f, .9f);
		FollowValue invertValue(0.f, .9f);
		
		bool showBackgroundVideo = true;
		bool showGrooopCircles = true;
		
		GrainsEffect grainsEffect;
		int nextGrainIndex = 0;
		
		while (!framework.quitRequested)
		{
			framework.process();

		#if ENABLE_LEAPMOTION
			// process LeapMotion input

			leapListener->tick();
		#endif
		
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.wentDown(SDLK_1))
				activeLayer = 0;
			if (keyboard.wentDown(SDLK_2))
				activeLayer = 1;
			if (keyboard.wentDown(SDLK_3))
				activeLayer = 2;
			if (keyboard.wentDown(SDLK_3))
				activeLayer = 3;
			
			if (activeLayer > NUM_LAYERS - 1)
				activeLayer = NUM_LAYERS - 1;
			
			if (keyboard.wentDown(SDLK_b))
				desiredBlurStrength = 1.f - desiredBlurStrength;
			
			if (keyboard.wentDown(SDLK_c))
				showGrooopCircles = !showGrooopCircles;
			
			if (keyboard.wentDown(SDLK_v))
				showBackgroundVideo = !showBackgroundVideo;

			const float dt = framework.timeStep;
			
			blurStrength = Calc::Lerp(desiredBlurStrength, blurStrength, std::powf(.5f, dt));
			
			grainsEffect.tick(dt);
			
			for (int i = 0; i < NUM_LAYERS; ++i)
			{
				if (i == 0)
				{
					pushSurface(layerAlphas[i]);
					{
						pushBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						drawRect(0, 0, GFX_SX, GFX_SY);
						popBlend();
					}
					popSurface();
				}
				
				{
					if (i != 0)
					{
						pushSurface(layerAlphas[i]);
						{
							pushBlend(BLEND_SUBTRACT);
							setColor(1, 1, 1, 255);
							drawRect(0, 0, GFX_SX, GFX_SY);
							popBlend();
						}
						popSurface();
					}
					
					if (1 && i == 1)
					{
						pushSurface(layerAlphas[i]);
						{
							static Traveller traveller;
							
							pushBlend(BLEND_ADD);
							setColor(255, 255, 255, 255, 31);
							
							if (!traveller.m_Active)
							{
								static float x = mouse.x;
								static float y = mouse.y;
								
								auto callback = [](void * obj, const TravelEvent & e)
								{
									x = Calc::Lerp(x, e.x, .05f);
									y = Calc::Lerp(y, e.y, .05f);
									
									Sprite brush("brushes/brush1006.png");
									brush.pivotX = brush.getWidth() / 2.f;
									brush.pivotY = brush.getHeight() / 2.f;
									//brush.drawEx(e.x, e.y, 0.f, .2f);
									brush.drawEx(x, y, 0.f, .2f);
								};
								
								traveller.Setup(2.f, callback, nullptr);
								traveller.Begin(mouse.x, mouse.y);
							}
							
							traveller.Update(mouse.x, mouse.y);
							
							popBlend();
						}
						popSurface();
					}
					
					if (0 && i == 0)
					{
						static Traveller traveller;
						static Surface * surface = nullptr;
						
						surface = layerColors[i];
						//surface = layerAlphas[i];
						
						pushBlend(BLEND_ALPHA);
						{
							if (!traveller.m_Active)
							{
								static float x = mouse.x;
								static float y = mouse.y;
								
								auto callback = [](void * obj, const TravelEvent & e)
								{
									x = Calc::Lerp(x, e.x, .05f);
									y = Calc::Lerp(y, e.y, .05f);
									
									// todo : use (x, y) and calculate deltas
									
									Shader shader("paint-smudge");
									setShader(shader);
									{
										const GLuint brush = getTexture("brushes/brush1006.png");
										shader.setTexture("brush", 0, brush);
										shader.setTexture("colormap", 1, surface->getTexture());
										
										surface->swapBuffers();
										
										pushSurface(surface);
										{
											setColor(colorWhite);
											gxBegin(GL_QUADS);
											{
												const float s = 30.f;
												gxTexCoord2f(0.f, 0.f); gxVertex4f(e.x - s, e.y - s, e.dx, e.dy);
												gxTexCoord2f(1.f, 0.f); gxVertex4f(e.x + s, e.y - s, e.dx, e.dy);
												gxTexCoord2f(1.f, 1.f); gxVertex4f(e.x + s, e.y + s, e.dx, e.dy);
												gxTexCoord2f(0.f, 1.f); gxVertex4f(e.x - s, e.y + s, e.dx, e.dy);
												//gxTexCoord2f(0.f, 0.f); gxVertex2f(e.x - s, e.y - s);
												//gxTexCoord2f(1.f, 0.f); gxVertex2f(e.x + s, e.y - s);
												//gxTexCoord2f(1.f, 1.f); gxVertex2f(e.x + s, e.y + s);
												//gxTexCoord2f(0.f, 1.f); gxVertex2f(e.x - s, e.y + s);
											}
											gxEnd();
										}
										popSurface();
									}
									clearShader();
								};
								
								traveller.Setup(2.f, callback, nullptr);
								traveller.Begin(mouse.x, mouse.y);
							}
							
							traveller.Update(mouse.x, mouse.y);
						}
						popBlend();
					}
					
					if (showGrooopCircles && i == 1)
					{
						pushSurface(layerAlphas[i]);
						{
							gxPushMatrix();
							{
								gxTranslatef(GFX_SX/2.f, GFX_SY/2.f, 0.f);
								
								const float scale = std::cos(framework.time * .1f) + 1.2f;
								gxScalef(scale, scale, 1.f);
								
								const float angle = std::cos(framework.time * .1f) * 360.f;
								gxRotatef(angle, 0.f, 0.f, 1.f);
								
								hqBegin(HQ_STROKED_CIRCLES);
								{
									setColor(colorBlack);
									hqStrokeCircle(0, 0, 100, 30.f);
									
									setColor(colorWhite);
									hqStrokeCircle(0, 0, 100, 5.f);
								}
								hqEnd();
								
								for (int x = -3; x <= +3; ++x)
								{
									gxPushMatrix();
									gxTranslatef(x * 150 * scale, 0, 0);
									gxScalef(scale, scale, 1.f);
									
									hqBegin(HQ_STROKED_CIRCLES);
									{
										setColor(colorBlack);
										hqStrokeCircle(0, 0, 50, 5.f);
										
										setColor(colorWhite);
										hqStrokeCircle(0, 0, 50, 3.f);
									}
									hqEnd();
									
									gxPopMatrix();
								}
							}
							gxPopMatrix();
						}
						popSurface();
					}
				}
			}
			
			for (int i = 0; i < NUM_LAYERS; ++i)
			{
				videoLoops[i]->tick(dt);
			}

			if (mouse.isDown(BUTTON_LEFT))
			{
				mouseDownTime += dt;
				
				Grain & grain = grainsEffect.grains[nextGrainIndex];
				nextGrainIndex = (nextGrainIndex + 1) % GrainsEffect::kMaxGrains;
				grain.life = 4.f;
				grain.lifeRcp = 1.f / grain.life;
				grain.px = mouse.x;
				grain.py = mouse.y;
				const float speed = random(5.f, 15.f);
				grain.vx = mouse.dx * speed;
				grain.vy = mouse.dy * speed;
				grain.vfalloff = .9;
				
			#if 1
				pushSurface(layerAlphas[activeLayer]);
				{
					const float radius = Calc::Min(mouseDownTime / .2f, 1.f) * 50.f;
					
					Shader shader("gradient-circle");
					setShader(shader);
					{
						pushBlend(BLEND_ADD);
						drawRect(mouse.x - radius, mouse.y - radius, mouse.x + radius, mouse.y + radius);
						popBlend();
					}
					clearShader();
				}
				popSurface();
			#endif
			}
			else
			{
				mouseDownTime = 0.f;
			}
			
			if (keyboard.wentDown(SDLK_r))
				barAngle.targetValue += 45.f;
			if (keyboard.wentDown(SDLK_i))
				invertValue.targetValue += .2f;
			
			barAngle.tick(dt);
			
			invertValue.tick(dt);
			
			pushSurface(layerAlphas[activeLayer]);
			{
				pushBlend(BLEND_ADD);
				grainsEffect.draw();
				popBlend();
			}
			popSurface();

			framework.beginDraw(0, 0, 0, 0);
			{
				if (showBackgroundVideo == false)
				{
					//
				}
				else
				{
					GLuint layerVideos[NUM_LAYERS] = { };
					
					for (int i = 0; i < NUM_LAYERS; ++i)
					{
						layerVideos[i] = videoLoops[i]->getTexture();
						
						pushSurface(layerColors[i]);
						{
							//setColorf(1.f, 1.f, 1.f, 1.f);
							setColorf(1.f, 1.f, 1.f, Calc::Lerp(0.f, 1.f, mouse.x / float(GFX_SX)));
							gxSetTexture(layerVideos[i]);
							drawRect(0, 0, GFX_SX, GFX_SY);
							gxSetTexture(0);
						}
						popSurface();
						
						pushBlend(BLEND_OPAQUE);
						applyFsfx(*layerColors[i], "fsfx/godrays.ps", Calc::Lerp(0.f, 1.f, mouse.y / float(GFX_SY)));
						popBlend();
					}
					
					{
						Shader shader("compose-layers");
						setShader(shader);
						{
							shader.setTexture("layerAlpha0", 0, layerAlphas[0]->getTexture());
							shader.setTexture("layerColor0", 1, layerColors[0]->getTexture());
							
						#if NUM_LAYERS >= 2
							shader.setTexture("layerAlpha1", 2, layerAlphas[1]->getTexture());
							shader.setTexture("layerColor1", 3, layerColors[1]->getTexture());
						#else
							shader.setTexture("layerAlpha1", 2, 0);
							shader.setTexture("layerColor1", 3, 0);
						#endif
						
						#if NUM_LAYERS >= 3
							shader.setTexture("layerAlpha2", 4, layerAlphas[2]->getTexture());
							shader.setTexture("layerColor2", 5, layerColors[2]->getTexture());
						#else
							shader.setTexture("layerAlpha2", 4, 0);
							shader.setTexture("layerColor2", 5, 0);
						#endif
							
							pushBlend(BLEND_OPAQUE);
							surface.postprocess();
							popBlend();
						}
						clearShader();
					}
					
				#if 1
					mask.clear();
					pushSurface(&mask);
					{
						gxPushMatrix();
						{
							gxTranslatef(GFX_SX/3.f, GFX_SY/3.f, 0.f);
							//gxRotatef(framework.time * 10.f, 0.f, 0.f, 1.f);
							gxRotatef(barAngle.value, 0.f, 0.f, 1.f);
							
							setColor(colorWhite);
							hqBegin(HQ_FILLED_RECTS);
							{
								hqFillRect(-100, -10000, 0, +10000);
								
								hqFillRect(20, -10000, 25, +10000);
								hqFillRect(40, -10000, 45, +10000);
							}
							hqEnd();
							
							hqBegin(HQ_STROKED_CIRCLES);
							{
								for (int y = -30; y <= +30; ++y)
								{
									hqStrokeCircle(60, y * 20, 3.5f, 2.f);
								}
							}
							hqEnd();
						}
						gxPopMatrix();
					}
					popSurface();
					
				#if ENABLE_LEAPMOTION
					if (leapController.isConnected() && leapController.hasFocus())
					{
						if (g_leapState.hands[0].active)
						{
							pushBlend(BLEND_OPAQUE);
							setShader_GaussianBlurH(surface.getTexture(), 60, std::cosf(g_leapState.hands[0].fingers[2].position[1] / 255.f * 5.f) * 100.f);
							surface.postprocess();
							clearShader();
							popBlend();
						}
						
						if (g_leapState.hands[0].active)
						{
							pushBlend(BLEND_OPAQUE);
							setShader_GaussianBlurV(surface.getTexture(), 60, std::cosf(g_leapState.hands[0].fingers[1].position[1] / 255.f * 5.f) * 100.f);
							surface.postprocess();
							clearShader();
							popBlend();
						}
					}
				#endif
					
				#if 0
					pushBlend(BLEND_OPAQUE);
					setShader_GaussianBlurH(surface.getTexture(), 5, blurStrength * std::cos(framework.time / 2.345f) * 40.f);
					surface.postprocess();
					clearShader();
					popBlend();
				#elif 1
					{
						pushBlend(BLEND_OPAQUE);
						GLuint texture = surface.getTexture();
						surface.swapBuffers();
						pushSurface(&surface);
						{
							for (int i = 0; i < 20; ++i)
							{
								const float t1 = (i + 0) / 20.f;
								const float t2 = (i + 1) / 20.f;
								const float y1 = t1 * GFX_SY;
								const float y2 = t2 * GFX_SY;
								const float blurStrengthModifier = std::cos(i / 10.f + framework.time);
								const float radius = blurStrength * blurStrengthModifier * std::cos(framework.time / 6.789f) * 200.f;
								setShader_GaussianBlurH(texture, 63, radius);
								gxBegin(GL_QUADS);
								{
									gxTexCoord2f(0.f, 1.f - t1); gxVertex2f(0,      y1);
									gxTexCoord2f(1.f, 1.f - t1); gxVertex2f(GFX_SX, y1);
									gxTexCoord2f(1.f, 1.f - t2); gxVertex2f(GFX_SX, y2);
									gxTexCoord2f(0.f, 1.f - t2); gxVertex2f(0,      y2);
								}
								gxEnd();
								clearShader();
							}
						}
						popSurface();
						popBlend();
					}
				#endif
					
				#if 0
					pushBlend(BLEND_OPAQUE);
					setShader_GaussianBlurV(surface.getTexture(), 5, blurStrength * std::cos(framework.time / 1.123f) * 20.f);
					surface.postprocess();
					clearShader();
					popBlend();
				#elif 1
					pushBlend(BLEND_OPAQUE);
					GLuint texture = surface.getTexture();
					surface.swapBuffers();
					pushSurface(&surface);
					{
						for (int i = 0; i < 30; ++i)
						{
							const float t1 = (i + 0) / 30.f;
							const float t2 = (i + 1) / 30.f;
							const float x1 = t1 * GFX_SX;
							const float x2 = t2 * GFX_SX;
							const float blurStrengthModifier = std::cos(i / 10.f + framework.time);
							const float radius = blurStrength * blurStrengthModifier * std::cos(framework.time / 3.456f) * 100.f;
							setShader_GaussianBlurV(texture, 63, radius);
							gxBegin(GL_QUADS);
							{
								gxTexCoord2f(t1, 1.f); gxVertex2f(x1, 0.f);
								gxTexCoord2f(t2, 1.f); gxVertex2f(x2, 0.f);
								gxTexCoord2f(t2, 0.f); gxVertex2f(x2, GFX_SY);
								gxTexCoord2f(t1, 0.f); gxVertex2f(x1, GFX_SY);
							}
							gxEnd();
							clearShader();
						}
					}
					popSurface();
					popBlend();
				
					applyFsfx(surface, "fsfx/invert.ps", (-std::cosf(invertValue.value * Calc::m2PI) + 1.f) / 2.f);
					
					pushBlend(BLEND_OPAQUE);
					setShader_ColorTemperature(surface.getTexture(), (std::cosf(framework.time / 2.f) + 1.f) / 2.f, 1.f);
					surface.postprocess();
					clearShader();
					popBlend();
					
					pushBlend(BLEND_OPAQUE);
					applyMask(surface.getTexture(), layerColors[0]->getTexture(), mask.getTexture());
					popBlend();
				#endif
				#endif
				}
			}
			framework.endDraw();
		}

		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			delete videoLoops[i];
			videoLoops[i] = nullptr;
		}
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			delete layerAlphas[i];
			layerAlphas[i] = nullptr;
			
			delete layerColors[i];
			layerColors[i] = nullptr;
		}

	#if ENABLE_LEAPMOTION
		leapController.removeListener(*leapListener);

		delete leapListener;
		leapListener = nullptr;
	#endif
	
		framework.shutdown();
	}
}

#else

void testAvpaint()
{
}

#endif
