#include "avtypes.h"
#include "Calc.h"
#include "framework.h"
#include "StringEx.h"
#include "video.h"

#define GFX_SX 1024
#define GFX_SY 768

#define NUM_LAYERS 3

// todo : mask using rotating and scaling objects as mask alpha
// todo : grooop logo animation
// todo : think of ways to mix/vj
// todo : use touchpad for map-like moving and scaling videos?
// todo : multitouch touch pad? can SDL handle this? else look for api

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
	
	void draw()
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

static void applyFsfx(Surface & surface, const char * name, const float strength = 1.f, const float param1 = 0.f, const float param2 = 0.f, const float param3 = 0.f, const float param4 = 0.f)
{
	Shader shader(name, "fsfx/fsfx.vs", name);
	setShader(shader);
	{
		shader.setImmediate("params1", strength, 0.f, 0.f, 0.f);
		shader.setImmediate("params2", param1, param2, param3, param4);
		shader.setTexture("colormap", 0, surface.getTexture());
		surface.postprocess();
	}
	clearShader();
}

int main(int argc, char * argv[])
{
#if defined(DEBUG)
	framework.enableRealTimeEditing = true;
#endif

	//framework.fullscreen = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		Surface surface(GFX_SX, GFX_SY, false);
		
		Surface * layerAlphas[NUM_LAYERS] = { };
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			layerAlphas[i] = new Surface(GFX_SX, GFX_SY, false);
			layerAlphas[i]->clear();
		}
		
		MediaPlayer * mediaPlayers[NUM_LAYERS] = { };
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			mediaPlayers[i] = new MediaPlayer();
			
			std::string filename = String::Format("video%d.mpg", i + 1);
			
			mediaPlayers[i]->openAsync(filename.c_str(), false);
		}
		
		float mouseDownTime = 0.f;
		int activeLayer = 0;
		float presentTime = 0.f;
		float blurStrength = 0.f;
		float desiredBlurStrength = 0.f;
		FollowValue barAngle(0.f, .9f);
		FollowValue invertValue(0.f, .9f);
		
		GrainsEffect grainsEffect;
		int nextGrainIndex = 0;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			if (keyboard.wentDown(SDLK_1))
				activeLayer = 0;
			if (keyboard.wentDown(SDLK_2))
				activeLayer = 1;
			if (keyboard.wentDown(SDLK_3))
				activeLayer = 2;
			
			if (keyboard.wentDown(SDLK_b))
				desiredBlurStrength = 1.f - desiredBlurStrength;
			
			const float dt = framework.timeStep;
			
			presentTime += dt;
			
			blurStrength = Calc::Lerp(desiredBlurStrength, blurStrength, std::powf(.5f, dt));
			
			grainsEffect.tick(dt);
			
			for (int i = 0; i < NUM_LAYERS; ++i)
			{
				pushSurface(layerAlphas[i]);
				{
					if (i == 0)
					{
						setBlend(BLEND_OPAQUE);
						setColor(colorWhite);
						drawRect(0, 0, GFX_SX, GFX_SY);
						setBlend(BLEND_ALPHA);
					}
					else
					{
						setBlend(BLEND_SUBTRACT);
						setColor(1, 1, 1, 255);
						drawRect(0, 0, GFX_SX, GFX_SY);
						setBlend(BLEND_ALPHA);
						
						if (i == 1)
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
					}
				}
				popSurface();
			}
			
			for (int i = 0; i < NUM_LAYERS; ++i)
			{
				mediaPlayers[i]->tick(mediaPlayers[i]->context);
				
				if (mediaPlayers[i]->context->hasBegun)
					mediaPlayers[i]->presentTime = presentTime * (i == 0 ? .5f : 1.f);
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
						setBlend(BLEND_ADD);
						drawRect(mouse.x - radius, mouse.y - radius, mouse.x + radius, mouse.y + radius);
						setBlend(BLEND_ALPHA);
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
				setBlend(BLEND_ADD);
				grainsEffect.draw();
				setBlend(BLEND_ALPHA);
			}
			popSurface();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				GLuint layerColors[NUM_LAYERS] = { };
				
				for (int i = 0; i < NUM_LAYERS; ++i)
				{
					layerColors[i] = mediaPlayers[i]->getTexture();
				}
				
				pushSurface(&surface);
				{
					Shader shader("compose-layers");
					setShader(shader);
					{
						shader.setTexture("layerAlpha0", 0, layerAlphas[0]->getTexture());
						shader.setTexture("layerAlpha1", 1, layerAlphas[1]->getTexture());
						shader.setTexture("layerAlpha2", 2, layerAlphas[2]->getTexture());
						shader.setTexture("layerColor0", 3, layerColors[0]);
						shader.setTexture("layerColor1", 4, layerColors[1]);
						shader.setTexture("layerColor2", 5, layerColors[2]);
						
						setBlend(BLEND_OPAQUE);
						drawRect(0, 0, GFX_SX, GFX_SY);
						setBlend(BLEND_ALPHA);
					}
					clearShader();
				}
				popSurface();
				
			#if 1
				Surface mask(GFX_SX, GFX_SY, false);
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
				
				setShader_GaussianBlurH(surface.getTexture(), 5, blurStrength * std::cos(framework.time / 2.345f) * 5.f);
				surface.postprocess();
				clearShader();
				
			#if 0
				setShader_GaussianBlurV(surface.getTexture(), 5, blurStrength * std::cos(framework.time / 1.123f) * 20.f);
				surface.postprocess();
				clearShader();
			#else
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
						const float blurStrengthModifier = std::cos(i / 30.f + framework.time);
						setShader_GaussianBlurV(texture, 5, blurStrength * blurStrengthModifier * std::cos(framework.time / 3.456f) * 20.f);
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
			#endif
			
				applyFsfx(surface, "fsfx/invert.ps", (std::cosf(invertValue.value * Calc::m2PI) + 1.f) / 2.f);
				
				setBlend(BLEND_OPAQUE);
				applyMask(surface.getTexture(), layerColors[0], mask.getTexture());
				setBlend(BLEND_ALPHA);
			#endif
				
				//logDebug("presentTime: %g", presentTime);
			}
			framework.endDraw();
		}
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			delete mediaPlayers[i];
			mediaPlayers[i] = nullptr;
		}
		
		for (int i = 0; i < NUM_LAYERS; ++i)
		{
			delete layerAlphas[i];
			layerAlphas[i] = nullptr;
		}
		
		framework.shutdown();
	}
	
	return 0;
}
