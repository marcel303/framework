#include "Debugging.h"
#include "Event_SDL.h"

static int TranslateKey(int key);

Event_SDL::Event_SDL()
{
	mMouseX = 0;
	mMouseY = 0;
}

int Event_SDL::TranslateEvent(const SDL_Event& e, Event* out_Events, int out_EventsLength)
{
	Assert(out_EventsLength >= 4);

	int result = 0;

	switch (e.type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		{
			int key = TranslateKey(e.key.keysym.sym);
			int state = e.key.state;

			if (key != 0)
			{
				out_Events[result++] = Event(EVT_KEY, key, state == SDL_PRESSED ? 1 : 0, e.key.keysym.unicode);
			}
		}
		break;
	case SDL_MOUSEMOTION:
		{
			//mMouseX += e.motion.xrel;
			//mMouseY += e.motion.yrel;
			mMouseX = e.motion.x;
			mMouseY = e.motion.y;

			//printf("SDL mouse: %d, %d\n", mMouseX, mMouseY);

			if (e.motion.xrel)
			{
				//out_Events[result++] = Event(EVT_MOUSEMOVE, INPUT_AXIS_X, e.motion.xrel);
				out_Events[result++] = Event(EVT_MOUSEMOVE_ABS, INPUT_AXIS_X, mMouseX);
			}
			if (e.motion.yrel)
			{
				//out_Events[result++] = Event(EVT_MOUSEMOVE, INPUT_AXIS_Y, e.motion.yrel);
				out_Events[result++] = Event(EVT_MOUSEMOVE_ABS, INPUT_AXIS_Y, mMouseY);
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
				out_Events[result++] = Event(EVT_MOUSEBUTTON, button, e.button.state == SDL_PRESSED ? 1 : 0, mMouseX, mMouseY);
		}
		break;
	case SDL_QUIT:
		out_Events[result++] = Event(EVT_QUIT);
		break;
	}

	return result;
}

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
