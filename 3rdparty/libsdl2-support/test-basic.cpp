#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

int main(int argc, char * argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	
	SDL_Window * window = SDL_CreateWindow("Hello World", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 600, 400, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
	
	if (window != nullptr)
	{
		SDL_GLContext context = SDL_GL_CreateContext(window);
		
		SDL_GL_MakeCurrent(window, context);
		
		SDL_GL_SetSwapInterval(1);
		
		SDL_CaptureMouse(SDL_TRUE);
		
		bool stop = false;
		
		while (stop == false)
		{
			SDL_Event e;
			
			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_QUIT)
			 		stop = true;
				else if (e.type == SDL_MOUSEBUTTONDOWN)
					printf("mouse down: %x\n", e.button.windowID);
				else if (e.type == SDL_MOUSEBUTTONUP)
					printf("mouse up: %x\n", e.button.windowID);
				else if (e.type == SDL_MOUSEMOTION)
					printf("mouse motion: %x: %d, %d\n", e.motion.windowID, e.motion.x, e.motion.y);
			}
			
			glClearColor((rand() % 100) / 99.f, 0.f, 0.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT);
			
			SDL_GL_SwapWindow(window);
		}
		
		SDL_GL_DeleteContext(context);
	}
	
	return 0;
}
