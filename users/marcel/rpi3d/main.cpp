#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

int main(int argc, char * argv[])
{
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("failed to initialize SDL2\n");
		return -1;
	}

	int windowFlags = SDL_WINDOW_OPENGL;

	auto window = SDL_CreateWindow(
		"My Window",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		640, 480,
		windowFlags);
	
	if (window == nullptr)
	{
		printf("failed to create window\n");
		return -1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
	//SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	auto context = SDL_GL_CreateContext(window);
	
	if (context == nullptr)
	{
		printf("failed to create OpenGL context\n");
		return -1;
	}

	bool stop = false;

	float time = 0.f;

	while (stop == false)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
				stop = true;
		}

		glClearColor((sinf(time) + 1.f) / 2.f, 0.f, 0.f, 0.f);
		glClear(GL_COLOR_BUFFER_BIT);

		SDL_GL_SwapWindow(window);

		time += 1.f / 60.f;
	}

	return 0;
}

