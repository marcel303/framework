#include "Precompiled.h"
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <SDL/SDL_syswm.h>
#include "Calc.h"
#include "Debugging.h"
#include "Display_Sdl.h"
#include "EventManager.h"
#include "Exception.h"

// Translate key code from SDL to engine.
static int TranslateKey(int key);

// : Display(x, y, width, height, fullscreen)
DisplaySDL::DisplaySDL(int x, int y, int width, int height, bool fullscreen, bool openGL)
{
#ifdef WIN32
	_putenv("SDL_VIDEO_WINDOW_POS");
	_putenv("SDL_VIDEO_CENTERED=1");
#else
	//#warning putenv support not enabled: SDL window will not be configured
#endif

	uint32_t flags = /*SDL_INIT_EVENTTHREAD | */SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_VIDEO;
	//uint32_t flags = SDL_INIT_EVERYTHING;

	int ret = SDL_Init(flags);

	if (ret < 0)
		throw ExceptionVA("unable to initialize SDL: %d", ret);

	CreateDisplay(x, y, width, height, fullscreen, openGL);

	if (1)
	{
		SDL_ShowCursor(SDL_DISABLE);
		SDL_WM_GrabInput(SDL_GRAB_ON);
	}

	mJoystick = SDL_JoystickOpen(0);
	mJoyHat = SDL_HAT_CENTERED;
	
	for (int i = 0; i < 3; ++i)
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		SDL_GL_SwapBuffers();
	}
}

DisplaySDL::~DisplaySDL()
{
	DestroyDisplay();
}

int DisplaySDL::GetWidth() const
{
	return m_surface->w;
}

int DisplaySDL::GetHeight() const
{
	return m_surface->h;
}

void* DisplaySDL::Get(const std::string& name)
{
#ifdef WIN32
	if (name == "hWnd")
	{
		return GetHWnd();
	}
#endif

	return 0;
}

bool DisplaySDL::Update()
{
	SDL_PumpEvents();

	SDL_Event e;

	while (SDL_PollEvent(&e))
	{
		static int mouseX = 0;
		static int mouseY = 0;

		switch (e.type)
		{
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			{
				int key = TranslateKey(e.key.keysym.sym);
				int state = e.key.state;

				if (key != 0)
				{
					EventManager::I().AddEvent(Event(EVT_KEY, key, state == SDL_PRESSED ? 1 : 0, e.key.keysym.unicode));
				}
			}
			break;
		case SDL_MOUSEMOTION:
			{
				mouseX += e.motion.xrel;
				mouseY += e.motion.yrel;

				if (e.motion.xrel)
				{
					EventManager::I().AddEvent(Event(EVT_MOUSEMOVE, INPUT_AXIS_X, e.motion.xrel));
					EventManager::I().AddEvent(Event(EVT_MOUSEMOVE_ABS, INPUT_AXIS_X, mouseX));
				}
				if (e.motion.yrel)
				{
					EventManager::I().AddEvent(Event(EVT_MOUSEMOVE, INPUT_AXIS_Y, e.motion.yrel));
					EventManager::I().AddEvent(Event(EVT_MOUSEMOVE_ABS, INPUT_AXIS_Y, mouseY));
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			{
				int button = -1;

				if (e.button.button == SDL_BUTTON_LEFT)   button = INPUT_BUTTON1;
				if (e.button.button == SDL_BUTTON_RIGHT)  button = INPUT_BUTTON2;
				if (e.button.button == SDL_BUTTON_MIDDLE) button = INPUT_BUTTON3;

				if (button != -1)
					EventManager::I().AddEvent(Event(EVT_MOUSEBUTTON, button, e.button.state == SDL_PRESSED ? 1 : 0, mouseX, mouseY));
			}
			break;

		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			{
				EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jbutton.which, e.jbutton.button, e.button.state == SDL_PRESSED ? 1 : 0));
			}
			break;
			
		case SDL_JOYAXISMOTION:
			{
				EventManager::I().AddEvent(Event(EVT_JOYMOVE_ABS, e.jaxis.which, e.jaxis.axis, e.jaxis.value));
			}
			break;

		case SDL_JOYHATMOTION:
			{
				// note: doesn't consider joystick index
				
				int diff = mJoyHat ^ e.jhat.value;
				int to1 = e.jhat.value & diff;
				int to0 = diff ^ to1;
				mJoyHat = e.jhat.value;
				
				if (to0 & SDL_HAT_UP)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1000, 0));
				if (to0 & SDL_HAT_DOWN)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1001, 0));
				if (to0 & SDL_HAT_LEFT)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1002, 0));
				if (to0 & SDL_HAT_RIGHT)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1003, 0));
					
				if (to1 & SDL_HAT_UP)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1000, 1));
				if (to1 & SDL_HAT_DOWN)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1001, 1));
				if (to1 & SDL_HAT_LEFT)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1002, 1));
				if (to1 & SDL_HAT_RIGHT)
					EventManager::I().AddEvent(Event(EVT_JOYBUTTON, e.jhat.which, 1003, 1));
			}
			break;

		case SDL_QUIT:
			exit(0);
			break;

		default:
			break;
		}
	}

	return true;
}

void DisplaySDL::CreateDisplay(int x, int y, int width, int height, bool fullscreen, bool openGL)
{
	int flags = 0;

	if (fullscreen)
		flags |= SDL_FULLSCREEN;

	if (openGL)
		flags |= SDL_DOUBLEBUF | SDL_OPENGL;

	if (openGL)
	{
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8  ); SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8  );
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8  ); SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8  );
		//SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   24 ); SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8  );
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1  );

		if (0)
		{
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
			SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 6);
		}

		SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);
	}

	m_surface = SDL_SetVideoMode(width, height, 32, flags);

	Assert(m_surface != 0);

	SDL_WM_SetCaption("Critical Wave", 0);
}

void DisplaySDL::DestroyDisplay()
{
	SDL_FreeSurface(m_surface);
	m_surface = 0;
}

#if defined(WIN32)
HWND DisplaySDL::GetHWnd()
{
	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);

	if (!SDL_GetWMInfo(&wminfo))
		return 0;

	return wminfo.window;
}
#endif

static int TranslateKey(int key)
{
	#define TRANSLATE(key1, key2) \
		if (key == key1) \
			return key2

	#define TRANSLATE_RANGE(begin, end, offset) \
		if (key >= begin && key <= end) \
			return offset + (key - begin)

	TRANSLATE_RANGE(SDLK_a, SDLK_z, IK_a);
	TRANSLATE_RANGE(SDLK_0, SDLK_9, IK_0);
	TRANSLATE_RANGE(SDLK_KP0, SDLK_KP9, IK_KP_0);
	TRANSLATE_RANGE(SDLK_F1, SDLK_F12, IK_F1);

	// symbols
	TRANSLATE(SDLK_PLUS, IK_PLUS);
	TRANSLATE(SDLK_MINUS, IK_MINUS);
	TRANSLATE(SDLK_ASTERISK, IK_ASTERISK);
	TRANSLATE(SDLK_SPACE, IK_SPACE);

	// special keys
	TRANSLATE(SDLK_ESCAPE, IK_ESCAPE);
	TRANSLATE(SDLK_RETURN, IK_ENTER);
	TRANSLATE(SDLK_DELETE, IK_DELETE);
	TRANSLATE(SDLK_BACKSPACE, IK_BACKSPACE);
	TRANSLATE(SDLK_LSHIFT, IK_SHIFTL);
	TRANSLATE(SDLK_RSHIFT, IK_SHIFTR);
	TRANSLATE(SDLK_LALT, IK_ALTL);
	TRANSLATE(SDLK_RALT, IK_ALTR);
	TRANSLATE(SDLK_LCTRL, IK_CONTROLL);
	TRANSLATE(SDLK_RCTRL, IK_CONTROLR);
	TRANSLATE(SDLK_HOME, IK_HOME);
	TRANSLATE(SDLK_END, IK_END);
	TRANSLATE(SDLK_PAGEUP, IK_PAGEUP);
	TRANSLATE(SDLK_PAGEDOWN, IK_PAGEDOWN);
	TRANSLATE(SDLK_UP, IK_UP);
	TRANSLATE(SDLK_DOWN, IK_DOWN);
	TRANSLATE(SDLK_LEFT, IK_LEFT);
	TRANSLATE(SDLK_RIGHT, IK_RIGHT);

	#undef TRANSLATE
	#undef TRANSLATE_RANGE

	return 0;
}
