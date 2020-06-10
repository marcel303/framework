/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#if FRAMEWORK_USE_SDL
	#include <SDL2/SDL_events.h>
	#include <SDL2/SDL_keycode.h>
#else
	typedef int SDL_Event;
	
	enum
	{
		SDLK_LSHIFT,
		SDLK_RSHIFT,
		SDLK_LALT,
		SDLK_RALT,
		SDLK_LGUI,
		SDLK_RGUI,
		SDLK_LCTRL,
		SDLK_RCTRL,
		SDLK_UP,
		SDLK_DOWN,
		SDLK_LEFT,
		SDLK_RIGHT,
		SDLK_EQUALS,
		SDLK_MINUS,
		SDLK_HOME,
		SDLK_END,
		SDLK_DELETE,
		SDLK_BACKSPACE,
		SDLK_RETURN,
		SDLK_SPACE,
		SDLK_ESCAPE,
		SDLK_TAB,
		SDLK_a,
		SDLK_c,
		SDLK_k,
		SDLK_m,
		SDLK_z,
		SDLK_i,
		SDLK_o,
		SDLK_f,
		SDLK_g,
		SDLK_b,
		SDLK_s,
		SDLK_t,
		SDLK_p,
		SDLK_w,
		SDLK_d,
		SDLK_v,
		SDLK_LEFTBRACKET,
		SDLK_RIGHTBRACKET,
		SDLK_1,
		SDLK_2,
		SDLK_3,
		SDLK_0
	};
#endif
