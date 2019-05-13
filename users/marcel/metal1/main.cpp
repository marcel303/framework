#include <SDL2/SDL.h>

#include "Mat4x4.h"
#include "metal.h"
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
			Mat4x4 projectionMatrix;
			projectionMatrix.MakePerspectiveLH(90.f * float(M_PI) / 180.f, 600.f / 300.f, .01f, 100.f);
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			
			gxPushMatrix();
			{
				static float t = 0.f;
				t += 1.f / 60.f;
				
				gxRotatef(sinf(t * 1.23f) * 30.f, 0, 1, 0);
				gxTranslatef(0, 0, 1);
				
				const float scale = cosf(t * 8.90f) * .2f;
				gxScalef(scale, scale, scale);
				gxRotatef(t * 220.f, 0, 0, 1);
				
			#if 0
				gxBegin(GX_TRIANGLES);
				{
					gxColor4f(1, 0, 0, 1);
					gxVertex2f(-1.f, -1.f);
					gxColor4f(0, 1, 0, 1);
					gxVertex2f(+1.f, -1.f);
					gxColor4f(0, 0, 1, 1);
					gxVertex2f(+1.f, +1.f);
					
					gxColor4f(1, 0, 0, 1);
					gxVertex2f(-1.f, -1.f);
					gxColor4f(0, 1, 0, 1);
					gxVertex2f(+1.f, +1.f);
					gxColor4f(0, 0, 1, 1);
					gxVertex2f(-1.f, +1.f);
				}
				gxEnd();
			#else
				gxBegin(GX_QUADS);
				{
					gxColor4f(1, 0, 0, 1);
					gxVertex2f(-1.f, -1.f);
					gxColor4f(0, 1, 0, 1);
					gxVertex2f(+1.f, -1.f);
					gxColor4f(0, 0, 1, 1);
					gxVertex2f(+1.f, +1.f);
					gxColor4f(1, 1, 1, 1);
					gxVertex2f(-1.f, +1.f);
				}
				gxEnd();
			#endif
			}
			gxPopMatrix();
		}
		metal_draw_end();
		
		metal_make_active(window2);
		metal_draw_begin(0.0f, 0.3f, 1.0f, 1.0f);
		{
			Mat4x4 projectionMatrix;
			projectionMatrix.MakePerspectiveLH(90.f * float(M_PI) / 180.f, 600.f / 300.f, .01f, 100.f);
			gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
			
			gxPushMatrix();
			{
				gxTranslatef(0, 0, 2);
				
				gxBegin(GX_LINES);
				{
					for (int i = 0; i < 100; ++i)
					{
						const float x = (rand() % 100) / 100.f;
						const float y = (rand() % 100) / 100.f;
						
						gxColor4f(1, 0, 0, 1);
						gxVertex2f(x, y);
						gxColor4f(0, 1, 0, 1);
						gxVertex2f(1.f, 1.f);
					}
				}
				gxEnd();
			}
			gxPopMatrix();
		}
		metal_draw_end();
	}
	
	gxShutdown();
	
// todo : add metal shutdown
	//metal_shutdown();
	
	return 0;
}
