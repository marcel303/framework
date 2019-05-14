#include <SDL2/SDL.h>

#include "Mat4x4.h"
#include "metal.h"
#include "shader.h"
#include <stdlib.h>

int main(int arg, char * argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	metal_init();
	
	gxInitialize();
	
	SDL_Window * window1 = SDL_CreateWindow("Hello Metal", 100, 100, 300, 600, SDL_WINDOW_RESIZABLE);
	SDL_Window * window2 = SDL_CreateWindow("Hello Metal", 400, 100, 300, 600, SDL_WINDOW_RESIZABLE);
	
	metal_attach(window1);
	metal_attach(window2);
	
	uint8_t * texture1_data = new uint8_t[128 * 128 * 4];
	for (int i = 0; i < 128 * 128 * 4; ++i)
		texture1_data[i] = i;
	
	GxTextureId texture1 = createTextureFromRGBA8(texture1_data, 128, 128, true, true);
	
	delete [] texture1_data;
	texture1_data = nullptr;
	
	for (;;)
	{
		bool stop = false;
		
	#if 1
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
				stop = true;
		}
	#else
		SDL_Event e;
		SDL_WaitEvent(&e);
		
		do
		{
			if (e.type == SDL_QUIT)
				stop = true;
		} while (SDL_PollEvent(&e));
	#endif
	
		if (stop)
			break;
		
		metal_make_active(window1);
		metal_draw_begin(1.0f, 0.3f, 0.0f, 1.0f);
		{
			//setDepthTest(false, DEPTH_ALWAYS);
			setDepthTest(true, DEPTH_LESS);
			setWireframe(false);
			
			Mat4x4 projectionMatrix;
			projectionMatrix.MakePerspectiveLH(90.f * float(M_PI) / 180.f, 600.f / 300.f, .01f, 100.f);
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			
			gxPushMatrix();
			{
				static float t = 0.f;
				t += 1.f / 60.f;
				
				gxRotatef(sinf(t * 1.23f) * 30.f, 0, 1, 0);
				gxTranslatef(0, 0, 1);
				
				const float scale = (cosf(t * 8.90f) * .2f + 1.f) * .5f;
				gxRotatef(t * 220.f, 0, 0, 1);
				gxRotatef(t * 22.f, 0, 1, 0);
				gxScalef(scale, scale, .1f);
				
				setBlend(BLEND_OPAQUE);
				gxSetTexture(texture1);
				gxBegin(GX_QUADS);
				{
					const float alpha = 1.f;
					
					for (int i = 0; i < 10; ++i)
					{
						const float x = (i % 3) / 2.f - .5f;
						const float y = (i % 5) / 4.f - .5f;
						
						gxTexCoord2f(0, 0);
						gxColor4f(1, 0, 0, alpha);
						gxVertex3f(x-1.f, y-1.f, i);
						
						gxTexCoord2f(1, 0);
						gxColor4f(0, 1, 0, alpha);
						gxVertex3f(x+1.f, y-1.f, i);
						
						gxTexCoord2f(1, 1);
						gxColor4f(0, 0, 1, alpha);
						gxVertex3f(x+1.f, y+1.f, i);
						
						gxTexCoord2f(0, 1);
						gxColor4f(1, 1, 1, alpha);
						gxVertex3f(x-1.f, y+1.f, i);
					}
				}
				gxEnd();
				gxSetTexture(0);
				setBlend(BLEND_OPAQUE);
				
				gxPushMatrix();
				gxTranslatef(0, 0, -.1f);
				Shader shader("test");
				setShader(shader);
				shader.setImmediate("imms", 0, .5f, 0, 0);
				gxBegin(GX_TRIANGLE_STRIP);
				{
					gxColor4f(1, 1, 1, 1);
					gxVertex2f(0.f, 0.f);
					gxVertex2f(.3f, 0.f);
					gxVertex2f(.3f, .3f);
				}
				gxEnd();
				clearShader();
				gxPopMatrix();
			}
			gxPopMatrix();
		}
		metal_draw_end();
		
	#if 1
		metal_make_active(window2);
		metal_draw_begin(0.0f, 0.3f, 1.0f, 1.0f);
		{
			Mat4x4 projectionMatrix;
			projectionMatrix.MakePerspectiveLH(90.f * float(M_PI) / 180.f, 600.f / 300.f, .01f, 100.f);
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			
			gxPushMatrix();
			{
				gxTranslatef(0, 0, 2);
				
				uint8_t texture2_data[16*16*4];
				for (int i = 0; i < 16 * 16; ++i)
				{
					texture2_data[i * 4 + 0] = 0;
					texture2_data[i * 4 + 1] = 0;
					texture2_data[i * 4 + 2] = rand() % 256;
					texture2_data[i * 4 + 3] = 255;
				}
				
				GxTextureId texture2 = createTextureFromRGBA8(texture2_data, 16, 16, true, true);
				
				gxSetTexture(texture2);
				gxBegin(GX_TRIANGLE_STRIP);
				{
					for (int i = 0; i < 1000; ++i)
					{
						const float x = (rand() % 100) / 100.f;
						const float y = (rand() % 100) / 100.f;
						
						gxTexCoord2f(x, y);
						gxColor4f(1, 1, 1, 1);
						gxVertex2f(x, y);
					}
				}
				gxEnd();
				gxSetTexture(0);
				
				freeTexture(texture2);
			}
			gxPopMatrix();
		}
		metal_draw_end();
	#endif
	}
	
	freeTexture(texture1);
	
	gxShutdown();
	
// todo : add metal shutdown
	//metal_shutdown();
	
	return 0;
}
