#include "framework.h"
#include "gx_render.h"

// dither texture from: https://community.khronos.org/t/screen-door-transparency/621/5
unsigned char transparencytable[256] =
{ // 16*16 alpha only texture
 192, 11,183,125, 26,145, 44,244,  8,168,139, 38,174, 27,141, 43,
 115,211,150, 68,194, 88,177,131, 61,222, 87,238, 74,224,100,235,
  59, 33, 96,239, 51,232, 16,210,117, 32,187,  1,157,121, 14,165,
 248,128,217,  2,163,105,154, 81,247,149, 97,205, 52,182,209, 84,
  20,172, 80,140,202, 41,185, 55, 24,197, 65,129,252, 35, 70,147,
 201, 63,189, 28, 90,254,116,219,137,107,231, 17,144,119,228,109,
  46,245,103,229,134, 13, 67,162,  6,170, 47,178, 76,193,  4,167,
 133,  9,159, 54,175,124,225, 93,242, 79,214, 99,241, 56,221, 92,
 186,218, 78,208, 37,196, 25,188, 42,142, 29,158, 21,130,156, 40,
 102, 31,148,111,234, 85,151,120,207,113,255, 86,184,212, 69,236,
 176, 73,253,  0,138, 58,249, 71, 10,173, 62,200, 50,114, 12,123,
  23,204,118,191, 91,181, 19,164,216,101,233,  3,135,169,246,152,
 223, 60,143, 48,240, 34,220, 82,132, 36,146,106,227, 30, 95, 49,
  83,166, 18,199, 98,155,122, 53,237,179, 57,190, 77,195,127,180,
 230,108,215, 64,171,  5,206,161, 22, 94,251, 15,153, 45,243,  7,
  72,136, 39,250,104,226, 75,112,198,126, 66,213,110,203, 89,160
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);

	framework.enableRealTimeEditing = true;
	framework.allowHighDpi = true;
	framework.msaaLevel = 4;
	
	if (!framework.init(640, 480))
		return -1;

	const int bbScale = framework.getCurrentBackingScale();
	
	ColorTarget * temporal = new ColorTarget();
	temporal->init(bbScale * 640, bbScale * 480, SURFACE_RGBA16F, colorBlackTranslucent);
	
	ColorTarget * colorTarget1 = new ColorTarget();
	ColorTarget * colorTarget2 = new ColorTarget();
	colorTarget1->init(bbScale * 640, bbScale * 480, SURFACE_RGBA8, colorBlackTranslucent);
	colorTarget2->init(bbScale * 640, bbScale * 480, SURFACE_RGBA8, colorBlackTranslucent);
	
	ColorTarget * colorTarget[2] = { colorTarget1, colorTarget2 };
	int colorTargetIndex = 0;
	
	int frameIndex = 0;
	
	bool enableTemporalSmoothe = false;
	bool enableBoxBlur2x2 = false;
	bool enableDitherPatternAnimation = false;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		if (keyboard.wentDown(SDLK_t))
			enableTemporalSmoothe = !enableTemporalSmoothe;
		if (keyboard.wentDown(SDLK_b))
			enableBoxBlur2x2 = !enableBoxBlur2x2;
		if (keyboard.wentDown(SDLK_f))
			enableDitherPatternAnimation = !enableDitherPatternAnimation;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushRenderPass(colorTarget[colorTargetIndex], true, nullptr, false, "Color");
			{
				pushBlend(BLEND_OPAQUE);
				gxScalef(bbScale, bbScale, 1);
				
				// -- method 1: use a dither texture and perform alpha test --
				
				auto textureId = createTextureFromR8(transparencytable, 16, 16, false, false);
				
				setColor(colorWhite);
				gxSetTexture(textureId, GX_SAMPLE_NEAREST, true);
				drawRect(0, 0, 16*4, 16*4);
				gxClearTexture();
				
				Shader shader("alpha-test");
				setShader(shader);
				{
					shader.setTexture("source", 0, textureId, false, false);
					
					for (int i = 0; i < 10; ++i)
					{
						shader.setImmediate("alphaRef", (sinf(framework.time / (i + 1.f)) + 1.f) / 2.f);
						shader.setImmediate("alphaPatternSeed", i * 1234, i * 4321);
						shader.setImmediate("frameIndex", enableDitherPatternAnimation ? (frameIndex % 4) : 0);
						//shader.setImmediate("frameIndex", frameIndex % 2);
						
						const float x = 320 + cosf(framework.time / (i/4.f + 0.56f)) * 100.f;
						const float y = 240 + sinf(framework.time / (i/4.f + 1.67f)) * 100.f;
						
						const Color colors[5] =
						{
							colorRed,
							colorGreen,
							colorBlue,
							colorWhite,
							colorYellow,
						};
						
						setColor(colors[i % 5]);
						fillCircle(x, y, 100, 100);
					}
				}
				clearShader();
				
				freeTexture(textureId);
				
				popBlend();
			}
			popRenderPass();
			
			ColorTarget * buffer = colorTarget[colorTargetIndex];
			
			if (enableTemporalSmoothe)
			{
				pushRenderPass(temporal, false, nullptr, true, "Temporal smoothe");
				{
					gxScalef(bbScale, bbScale, 1);
					
					const float c = .75f;
					const float d = 1.f - c;
					
					pushBlend(BLEND_MUL);
					{
						setColorf(c, c, c, c);
						drawRect(0, 0, 640, 480);
					}
					popBlend();
					
					pushBlend(BLEND_ADD_OPAQUE);
					{
						setColorf(d, d, d, d);
						
						gxSetTexture(buffer->getTextureId(), GX_SAMPLE_NEAREST, true);
						drawRect(0, 0, 640, 480);
						gxClearTexture();
					}
					popBlend();
				}
				popRenderPass();
				
				buffer = temporal;
			}
		
			if (enableBoxBlur2x2)
			{
				const int nextColorTargetIndex = 1 - colorTargetIndex;
				
				pushRenderPass(colorTarget[nextColorTargetIndex], true, nullptr, false, "Box blur 2x2");
				{
					gxScalef(bbScale, bbScale, 1);
					
					pushBlend(BLEND_OPAQUE);
					{
						Shader shader("blur2x2");
						setShader(shader);
						shader.setTexture("source", 0, buffer->getTextureId(), false, true);
						drawRect(0, 0, 640, 480);
						clearShader();
					}
					popBlend();
				}
				popRenderPass();
				
				buffer = colorTarget[nextColorTargetIndex];
				colorTargetIndex = nextColorTargetIndex;
			}
			
			//
		
			pushBlend(BLEND_OPAQUE);
			{
				setColor(colorWhite);
				gxSetTexture(buffer->getTextureId(), GX_SAMPLE_NEAREST, true);
				drawRect(0, 0, 640, 480);
				gxClearTexture();
			}
			popBlend();
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			int y = 4;
			y += 20; drawText(4, y, 18, +1, -1, "Press 't' to toggle temporal AA");
			y += 20; drawText(4, y, 18, +1, -1, "Press 'b' to toggle box blur 2x2");
			y += 20; drawText(4, y, 18, +1, -1, "Press 'f' to toggle frame index based dither pattern animation");
			
			//
			
			pushAlphaToCoverage(true);
			{
				setColorf(1, 1, 1, .25f);
				drawRect(320, 240, 420, 340);
				
				setColorf(1, 1, 1, .5f);
				drawRect(320, 240, 360, 340);
				
				setColorf(1, 1, 1, .75f);
				drawRect(320, 240, 420, 280);
			}
			popAlphaToCoverage();
		}
		framework.endDraw();
		
		frameIndex++;
	}

	framework.shutdown();

	return 0;
}
