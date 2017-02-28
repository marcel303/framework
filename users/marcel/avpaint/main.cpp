#include "Calc.h"
#include "framework.h"
#include "video.h"

#define GFX_SX 640
#define GFX_SY 480

#define NUM_LAYERS 3

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
			
			mediaPlayers[i]->openAsync("flowers.mpg", false);
		}
		
		float mouseDownTime = 0.f;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float dt = framework.timeStep;
			
			for (int i = 0; i < NUM_LAYERS; ++i)
			{
				pushSurface(layerAlphas[i]);
				{
					setBlend(BLEND_SUBTRACT);
					setColor(1, 1, 1, 255);
					drawRect(0, 0, GFX_SX, GFX_SY);
					setBlend(BLEND_ALPHA);
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
				
				pushSurface(layerAlphas[0]);
				{
					const float radius = Calc::Min(mouseDownTime / 2.f, 1.f) * 50.f;
					
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
