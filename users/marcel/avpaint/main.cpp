#include "Calc.h"
#include "framework.h"
#include "StringEx.h"
#include "video.h"

#define GFX_SX 640
#define GFX_SY 480

#define NUM_LAYERS 3

// todo : mask using rotating and scaling objects as mask alpha
// todo : grooop logo animation
// todo : think of ways to mix/vj
// todo : use touchpad for map-like moving and scaling videos?
// todo : multitouch touch pad? can SDL handle this? else look for api

int main(int argc, char * argv[])
{
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
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
			
			const float dt = framework.timeStep;
			
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
					mediaPlayers[i]->presentTime += dt;
			}
			
			if (mouse.isDown(BUTTON_LEFT))
			{
				mouseDownTime += dt;
				
				pushSurface(layerAlphas[activeLayer]);
				{
					const float radius = Calc::Min(mouseDownTime / .5f, 1.f) * 50.f;
					
					setColor(colorWhite);
					fillCircle(mouse.x, mouse.y, radius, 100);
				}
				popSurface();
			}
			else
			{
				mouseDownTime = 0.f;
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				GLuint layerColors[NUM_LAYERS] = { };
				
				for (int i = 0; i < NUM_LAYERS; ++i)
				{
					layerColors[i] = mediaPlayers[i]->getTexture();
				}
				
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
					
					hqBegin(HQ_LINES);
					{
						setColor(colorWhite);
						hqLine(50, 50, 0, GFX_SX - 50, 50, 50);
						hqLine(50, GFX_SY - 50, 50, GFX_SX - 50, GFX_SY - 50, 0);
					}
					hqEnd();
				}
				clearShader();
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
