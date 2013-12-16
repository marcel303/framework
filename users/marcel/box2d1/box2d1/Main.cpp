#include <Box2D/Box2D.h>
#include <gl/GL.h>
#include <SDL/SDL.h>
#include <vector>

static void DrawLine(float x1, float y1, float x2, float y2)
{
	glBegin(GL_LINES);
	glVertex3f(x1, y1, 1.0f);
	glVertex3f(x2, y2, 1.0f);
	glEnd();
}

int main(int argc, char* argv[])
{
	SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_VIDEO);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8  ); SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8  );
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8  ); SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8  );
	//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24 ); SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8  );
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

	//SDL_Surface* screen = SDL_SetVideoMode(640, 480, 32, SDL_DOUBLEBUF | SDL_OPENGL);
	SDL_Surface* screen = SDL_SetVideoMode(1280, 960, 32, SDL_DOUBLEBUF | SDL_OPENGL);

	SDL_WM_SetCaption("Box2D", 0);

	Sim sim;
	
	sim.Setup();
	
	//

	float dt = 1.0f / 60.0f;

	bool stop = false;

	float t = 0.0f;

	while (!stop)
	{
		//printf("time: %f\n", t += dt);

		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				SDL_Event q;
				q.type = SDL_QUIT;
				SDL_PushEvent(&q);
			}
			if (e.type == SDL_QUIT)
			{
				stop = true;
			}
		}
		
		sim.Update(dt);

		glClearColor(0.0f, 0.1f, 0.2f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		float s = 1.0f / 20.0f;
		glOrtho(-640.0f * s, 640.0f * s, 480.0f * s, -480.0f * s, -1000.0f, 1000.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glColor3f(1.0f, 1.0f, 1.0f);

		sim.RenderGL();
		
		b2Vec2 pos0 = staticBody->GetPosition();

		for (size_t i = 0; i < sim.bodyList.size(); ++i)
		{
			b2Body* body = sim.bodyList[i];
			b2Vec2 pos = body->GetPosition();
			DrawLine(pos.x, pos.y, pos0.x, pos0.y);
		}

		SDL_GL_SwapBuffers();
	}

	SDL_Quit();

	return 0;
}
