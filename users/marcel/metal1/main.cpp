#include <SDL2/SDL.h>

#include "metal.h"
#include <stdlib.h>

int main(int arg, char * argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);

	metal_init();
	
	SDL_Window * window1 = SDL_CreateWindow("Hello Metal", 100, 100, 300, 600, SDL_WINDOW_RESIZABLE);
	SDL_Window * window2 = SDL_CreateWindow("Hello Metal", 400, 100, 300, 600, SDL_WINDOW_RESIZABLE);
	
	metal_attach(window1);
	metal_attach(window2);
	
	for (;;)
	{
		SDL_Event e;
		SDL_WaitEvent(&e);

		bool stop = false;
		
		do
		{
			if (e.type == SDL_QUIT)
				stop = true;
		} while (SDL_PollEvent(&e));
		
		if (stop)
			break;
		
		metal_make_active(window1);
		metal_draw_begin(1.0f, 0.3f, 0.0f, 1.0f);
		{
			gxPushMatrix();
			{
				static float t = 0.f;
				t += 1.f / 60.f;
				
				const float scale = cosf(t);
				gxScalef(scale, scale, scale);
				
				gxBegin(GX_TRIANGLES);
				{
					for (int i = 0; i < 100; ++i)
					{
						const float x = (rand() % 100) / 100.f;
						const float y = (rand() % 100) / 100.f;
						
						gxColor4f(1, 0, 0, 1);
						gxVertex2f(x, y);
						gxColor4f(0, 1, 0, 1);
						gxVertex2f(0.f, 1.f);
						gxColor4f(0, 0, 1, 1);
						gxVertex2f(1.f, 1.f);
					}
				}
				gxEnd();
			}
			gxPopMatrix();
		}
		metal_draw_end();
		
		metal_make_active(window2);
		metal_draw_begin(0.0f, 0.3f, 1.0f, 1.0f);
		{
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
		metal_draw_end();
	}
	
	return 0;
}
