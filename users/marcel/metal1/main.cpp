#include <SDL2/SDL.h>

#include "metal.h"

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
		//for (int i = 0; i < 1000; ++i)
		metal_drawtest();
		metal_draw_end();
		
		metal_make_active(window2);
		metal_draw_begin(0.0f, 0.3f, 1.0f, 1.0f);
		metal_drawtest();
		metal_draw_end();
	}
	
	return 0;
}
