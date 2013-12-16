#include <cmath>
//#include <gl/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "input.h"
#include "render_gl.h"

#define BLEND SetBlendEnabled(color.a != 1.0f)

static void SetBlendEnabled(bool enabled);

static PFNGLBLENDEQUATIONPROC _glBlendEquation = 0;

void RenderGL::Init(int sx, int sy)
{
	SDL_Init(SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_VIDEO);

	SDL_putenv("SDL_VIDEO_CENTERED=center");

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8); SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8); SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 0);

	mSurface = SDL_SetVideoMode(sx, sy, 32, SDL_OPENGL | SDL_FULLSCREEN);

	_glBlendEquation = (PFNGLBLENDEQUATIONPROC)wglGetProcAddress("glBlendEquation");

	glDisable(GL_DEPTH_TEST);

	SDL_WM_SetCaption("shooter", 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, sx, sy, 0.0f, -1000.0f, +1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void RenderGL::Clear()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void RenderGL::Present()
{
	SDL_Event e;

	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			{
				int code = e.key.keysym.sym;
				int state = e.key.state;
				KeyCode code2 = KeyCode_Undefined;
				if (code == SDLK_LEFT)
					code2 = KeyCode_Left;
				if (code == SDLK_RIGHT)
					code2 = KeyCode_Right;
				if (code == SDLK_UP)
					code2 = KeyCode_Up;
				if (code == SDLK_DOWN)
					code2 = KeyCode_Down;
				if (code == SDLK_ESCAPE)
					code2 = KeyCode_Escape;
				if (code2 != KeyCode_Undefined)
					gKeyboard.Set(code2, state);
				/*if (code == SDLK_ESCAPE)
				{
					SDL_Event quit;
					quit.type = SDL_QUIT;
					quit.quit.type = 0;
					SDL_PushEvent(&quit);
				}*/
				break;
			}
		case SDL_QUIT:
			exit(0);
			break;
		}
	}

	SDL_GL_SwapBuffers();

	SDL_Delay(1000 / 65);
}

void RenderGL::Fade(int amount)
{
	_glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glEnable(GL_BLEND);

	float x1 = 0.0f;
	float y1 = 0.0f;
	float x2 = (float)mSurface->w;
	float y2 = (float)mSurface->h;

	float c = amount / 255.0f;

	glBegin(GL_QUADS);
	glColor4f(1.0f, 1.0f, 1.0f, c);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();

	_glBlendEquation(GL_FUNC_ADD);
}

void RenderGL::Point(float x, float y, Color color)
{
	BLEND;

	glBegin(GL_POINTS);
	glColor4f(color.r, color.g, color.b, color.a);
	glVertex2f(x, y);
	glEnd();
}

void RenderGL::Line(float x1, float y1, float x2, float y2, Color color)
{
	BLEND;

	glBegin(GL_LINES);
	glColor4f(color.r, color.g, color.b, color.a);
	glVertex2f(x1, y1);
	glVertex2f(x2, y2);
	glEnd();
}

void RenderGL::Circle(float x, float y, float r, Color color)
{
	r = std::floor(r);

	BLEND;

	int n = (int)std::ceil(r * 3.14f);
	
	glBegin(GL_TRIANGLE_FAN);
	glColor4f(color.r, color.g, color.b, color.a);
	glVertex2f(x, y);
	for (int i = 0; i <= n; ++i)
	{
		float a = i / (float)n * (float)M_PI * 2.0f;
		float x2 = x + std::sin(a) * r;
		float y2 = y + std::cos(a) * r;
		glVertex2f(x2, y2);
	}
	glEnd();
}

void RenderGL::Arc(float x, float y, float angle1, float angle2, float r, Color color)
{
	r = std::floor(r);

	int n = (int)std::ceil(r * 3.14f);
	
	for (int i = 0; i < n; ++i)
	{
		float a1 = angle1 + (angle2 - angle1) / (float)n * (i + 0);
		float a2 = angle1 + (angle2 - angle1) / (float)n * (i + 1);
		float x1 = x + std::sin(a1) * r;
		float y1 = y + std::cos(a1) * r;
		float x2 = x + std::sin(a2) * r;
		float y2 = y + std::cos(a2) * r;
		Line(x1, y1, x2, y2, color);
	}
}

void RenderGL::Quad(float x1, float y1, float x2, float y2, Color color)
{
	BLEND;

	glBegin(GL_QUADS);
	glColor4f(color.r, color.g, color.b, color.a);
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
	glEnd();
}

void RenderGL::Text(float x, float y, Color color, const char* format, ...)
{
}

static void SetBlendEnabled(bool enabled)
{
	if (enabled)
	{
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
	}
	else
	{
		glDisable(GL_BLEND);
	}
}
