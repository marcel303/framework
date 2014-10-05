#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <Windows.h>
#include "syscode.h"
#include "syscode_xinput.h"
#include "id_heads.h"

#define BLOWUP 4 // mstodo : remove this define. make sure mouse is adjusted based on screen scale
#define MAX_SOUNDS 64
#define USE_OPENGL 1

static void SYS_PollJoy();

extern void Quit (char *error);
extern boolean	CapsLock;
extern ScanCode	CurCode,LastCode;
extern byte		ASCIINames[], ShiftNames[], SpecialNames[];

#if PROTECT_DISPLAY_BUFFER
	unsigned char * g0xA000[4];
#else
	unsigned char g0xA000[4][DISPLAY_BUFFER_SIZE];
#endif

static unsigned short _CRTC = 0;
static unsigned short _pelpan = 0;

void VW_SetScreen (unsigned short CRTC, unsigned short pelpan)
{
	_CRTC = CRTC;
	_pelpan = pelpan;

	SYS_Present();
}

static unsigned int gather32(int offset)
{
	unsigned int result;
	int poffset;
	int i;

	// return 32 adjacent bits (8 adjacent pixels), using planar VGA mode

	poffset = (offset >> 3);
	result = 0;

	for (i = 0; i < 32; ++i)
	{
		int plane = i & 3;
		int boffset = (i >> 2) & 7;
		int planeb = g0xA000[plane][poffset];
		int bit = (planeb >> boffset) & 1;

		result <<= 1;
		result |= bit;
	}

	return result;
}

static unsigned int palette[16] =
{
	0x000000,
	0x0000AA,
	0x00AA00,
	0x00AAAA,
	0xAA0000,
	0xAA00AA,
	0xAA5500,
	0xAAAAAA,
	0x555555,
	0x5555FF,
	0x55FF55,
	0x55FFFF,
	0xFF5555,
	0xFF55FF,
	0xFFFF55,
	0xFFFFFF
};

static unsigned char palettemap[16] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static SDL_mutex * mutex = 0;
static SDL_Surface * screen = 0;
#if !USE_OPENGL
static SDL_Surface * surface = 0;
#endif
static SDL_Joystick * joy = 0;
static SDL_Thread * timeThread = 0;
static SDL_AudioSpec audioSpec;

static int __cdecl TimeThread(void * userData)
{
	while (true)
	{
		SDL_Delay(1000/70); // increment at 70Hz, based on this loc: "if (TimeCount - time > 35)	// Half-second delays"
		TimeCount++;
	}
	return 0;
}

static struct
{
	boolean cached;
	short * buffer;
	short * bufferEnd;
} s_sounds[MAX_SOUNDS];
static short * s_soundPtr = 0;
static short * s_soundEnd = 0;

extern volatile word SoundNumber;
extern volatile word SoundPriority;

static void SoundThread(void * userData, Uint8 * stream, int length)
{
	SDL_LockMutex(mutex);
	{
		short * __restrict dest = (short*)stream;
		short * __restrict destEnd = (short*)(stream + length);

		while (dest < destEnd && s_soundPtr != s_soundEnd)
		{
			*dest++ = *s_soundPtr++;
		}
		while (dest < destEnd)
		{
			*dest++ = 0;
		}

		if (s_soundPtr == s_soundEnd)
		{
			SoundNumber = SoundPriority = 0;
		}
	}
	SDL_UnlockMutex(mutex);
}

void SYS_Init()
{
	int plane;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		Quit("Failed to initialize SDL");

	mutex = SDL_CreateMutex();

#if USE_OPENGL
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

	//if ((screen = SDL_SetVideoMode(640, 400, 32, SDL_OPENGL)) == 0)
	if ((screen = SDL_SetVideoMode(1680, 1050, 32, SDL_OPENGL | SDL_FULLSCREEN)) == 0)
		Quit("Failed to set video mode");
#else
	//if ((screen = SDL_SetVideoMode(320 * BLOWUP, 200 * BLOWUP, 32, SDL_SWSURFACE | SDL_DOUBLEBUF)) == 0)
	if ((screen = SDL_SetVideoMode(640, 400, 32, SDL_HWSURFACE | SDL_DOUBLEBUF)) == 0)
	//if ((screen = SDL_SetVideoMode(1680, 1050, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | SDL_FULLSCREEN)) == 0)
		Quit("Failed to set video mode");

	if ((surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320 + 8 /*max pelpan*/, 200, 32,
		0xff << screen->format->Rshift,
		0xff << screen->format->Gshift,
		0xff << screen->format->Bshift,
		0x00 << screen->format->Ashift)) == 0)
		Quit("Failed to create surface");
#endif

	if (SDL_NumJoysticks() > 0)
		joy = SDL_JoystickOpen(0);

	if ((timeThread = SDL_CreateThread(TimeThread, 0)) == 0)
		Quit("Failed to create timer thread");

	for (plane = 0; plane < 4; ++plane)
	{
	#if PROTECT_DISPLAY_BUFFER
		g0xA000[plane] = VirtualAlloc(NULL, DISPLAY_BUFFER_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	#endif
		memset(g0xA000[plane], 0, DISPLAY_BUFFER_SIZE);
	}

	memset(&audioSpec, 0, sizeof(audioSpec));
	audioSpec.freq = 44100;
	audioSpec.format = AUDIO_S16;
	audioSpec.channels = 1;
	audioSpec.samples = 1024; // mstodo : lower ?
	audioSpec.callback = SoundThread;
	if (SDL_OpenAudio(&audioSpec, 0) < 0) {
		//Quit("Failed to init sound playback!");
	}
	else
		SDL_PauseAudio(false);

	SDL_ShowCursor(false);
	SDL_WM_GrabInput(SDL_GRAB_ON);

	SYS_PollJoy();
}

void SYS_SetPalette(char * palette)
{
	unsigned int i;

	for (i = 0; i < 16; ++i)
		palettemap[i] = palette[i] & 0xf; // mstodo : sometimes the 5th bit is set. need 64 entry palette?
}

#if USE_OPENGL
static void CheckGL()
{
	GLenum error = glGetError();
	printf("GL error: %x\n", error);
}
#endif

void SYS_Present()
{
#if !USE_OPENGL
	if (SDL_LockSurface(surface) == 0)
	{
		// convert palette to surface compatible palette

		unsigned int palette2[16];
		unsigned short x, y, i;

		for (i = 0; i < 16; ++i)
		{
			const unsigned int c = palette[palettemap[i]];
			const unsigned int r = (c >> 16) & 0xff;
			const unsigned int g = (c >> 8 ) & 0xff;
			const unsigned int b = (c >> 0 ) & 0xff;
			palette2[i] =
				(r << surface->format->Rshift) |
				(g << surface->format->Gshift) |
				(b << surface->format->Bshift);
		}

		for (y = 0; y < surface->h; ++y)
		{
			unsigned int * __restrict dst = (unsigned int*)(((unsigned char*)surface->pixels) + surface->pitch * y);

			for (x = 0; x < surface->w; x += 8)
			{
				unsigned int src = gather32(_CRTC * 8 + y * 512 + x);

				for (i = 0; i < 8; ++i)
				{
					unsigned char c = (src >> (i << 2)) & 15;
					
					c =
						((c & 1) >> 0) << 3 |
						((c & 2) >> 1) << 2 |
						((c & 4) >> 2) << 1 |
						((c & 8) >> 3) << 0;

					*dst++ = palette2[c];
				}
			}
		}

		SDL_UnlockSurface(surface);
	}

#if BLOWUP > 1
	if (SDL_LockSurface(surface) == 0)
	{
		if (SDL_LockSurface(screen) == 0)
		{
			unsigned int x, y;

			// do a scaled blit. use 10.22 fixed point for sampling the source in the horizontal direction

			for (y = 0; y < screen->h; ++y)
			{
				unsigned char * srcbytes = (unsigned char*)surface->pixels + surface->pitch * (y * (surface->h - 1) / (screen->h - 1));
				unsigned char * dstbytes = (unsigned char*)screen->pixels + screen->pitch * y;
				unsigned int * __restrict src = (unsigned int*)srcbytes + _pelpan;
				unsigned int * __restrict dst = (unsigned int*)dstbytes;
				unsigned int step = ((surface->w - 1 - 8) << 22) / (screen->w - 1);
				unsigned int pos = 0;

				for (x = 0; x < screen->w; ++x, pos += step)
				{
					dst[x] = src[pos >> 22];
				}
			}

			SDL_UnlockSurface(screen);
		}

		SDL_UnlockSurface(surface);
	}
#else
	{
		SDL_Rect srcrect;
		SDL_Rect dstrect;

		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = 328;
		srcrect.h = 200;

		dstrect.x = -_pelpan;
		dstrect.y = 0;
		dstrect.w = 320;
		dstrect.h = 200;

		SDL_BlitSurface(surface, &srcrect, screen, &dstrect);
	}
#endif

	SDL_Flip(screen); // mstodo : we need to guarantee we do the page flip with vsync. does SDL guarantee this?
#else
	static unsigned int __declspec(align(16)) buffer[200][328];
	static GLuint texture = 0;
	struct
	{
		union
		{
			unsigned int asInt;
			unsigned char rgba[4];
		};
	} palette2[16];
	unsigned short x, y, i;

	// convert palette to surface compatible palette

	for (i = 0; i < 16; ++i)
	{
		const unsigned int c = palette[palettemap[i]];
		const unsigned char r = (c >> 16) & 0xff;
		const unsigned char g = (c >> 8 ) & 0xff;
		const unsigned char b = (c >> 0 ) & 0xff;
		palette2[i].rgba[0] = r;
		palette2[i].rgba[1] = g;
		palette2[i].rgba[2] = b;
		palette2[i].rgba[3] = 0xff;
	}

	for (y = 0; y < 200; ++y)
	{
		unsigned int * __restrict dst = buffer[y];

		for (x = 0; x < 328; x += 8)
		{
			unsigned int src = gather32(_CRTC * 8 + y * 512 + x);

			for (i = 0; i < 8; ++i)
			{
				unsigned char c = (src >> (i << 2)) & 15;
				
				// mstodo : instead of swapping bits here, swap them in the palette
				//          or better yet, change gather32 to return them in the right order

				c =
					((c & 1) >> 0) << 3 |
					((c & 2) >> 1) << 2 |
					((c & 4) >> 2) << 1 |
					((c & 8) >> 3) << 0;

				*dst++ = palette2[c].asInt;
			}
		}
	}

	// setup viewport

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 320.0, 200.0, 0.0, -1.0, +1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// create texture

	if (texture == 0)
	{
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
	}

	// update texture contents

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 328, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//CheckGL();

	// draw a textured quad, taking the current pixel panning into account

	glBindTexture(GL_TEXTURE_2D, texture);
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.f, 0.f); glVertex3f(  0.f - _pelpan, 0.f,   0.f);
		glTexCoord2f(1.f, 0.f); glVertex3f(328.f - _pelpan, 0.f,   0.f);
		glTexCoord2f(1.f, 1.f); glVertex3f(328.f - _pelpan, 200.f, 0.f);
		glTexCoord2f(0.f, 1.f); glVertex3f(  0.f - _pelpan, 200.f, 0.f);
	}
	glEnd();

	// present!

	SDL_GL_SwapBuffers();
#endif

	SYS_Update(); // called here, for convenient in draw loops, so we don't have to add it separately
}

extern void INL_HandleKey(byte k);

int MouseX, MouseY;
int MouseDXf, MouseDYf;
int MouseDX, MouseDY;
int MouseButtons;
int JoyAbsX;
int JoyAbsY;
int JoyButtons;

void SYS_Update()
{
	SDL_Event e;

	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
		{
			int key = e.key.keysym.scancode;

			if (key < NumCodes)
			{
				if (e.type == SDL_KEYUP)
					key |= 0x80;

				INL_HandleKey(key);
			}
		}

		if (e.type == SDL_MOUSEMOTION)
		{
			MouseX = e.motion.x / BLOWUP;
			MouseY = e.motion.y / BLOWUP;

			MouseDXf += e.motion.xrel;
			MouseDYf += e.motion.yrel;
		}

		if (e.type == SDL_MOUSEBUTTONDOWN)
			MouseButtons |= 1 << e.button.button;
		if (e.type == SDL_MOUSEBUTTONUP)
			MouseButtons &= ~(1 << e.button.button);
	}

	// export mouse values to game

	MouseDX = MouseDXf / BLOWUP;
	MouseDY = MouseDYf / BLOWUP;
	MouseDXf -= MouseDX * BLOWUP;
	MouseDYf -= MouseDY * BLOWUP;

	SYS_PollJoy();

	//SDL_Delay(100);
}

static void SYS_PollJoy()
{
	// joystick/gamepad support

	boolean hasjoy = false;
	float x = 0.f, y = 0.f;
	unsigned short buttons = 0;

	// try to poll XInput first. if that fails, use SDL fallback

	if (SYS_PollXInput(0, &x, &y, &buttons))
	{
		hasjoy = true;
	}
	else if (joy)
	{
		unsigned int i;
		unsigned char axismap[2] = { 0, 1 }; // mstodo : axis and button mapping should be in a config file

		hasjoy = true;

		SDL_JoystickUpdate();

		for (i = 0; i < 2; ++i)
		{
			float v = 0.f;

			if (axismap[i] < SDL_JoystickNumAxes(joy))
			{
				v = SDL_JoystickGetAxis(joy, axismap[i]) / 32768.f;
			}

			if (i == 0)
				x = v;
			else
				y = v;
		}

		for (i = 0; i < SDL_JoystickNumButtons(joy); ++i)
			buttons |= (SDL_JoystickGetButton(joy, i) == 0 ? 0 : 1) << i;
	}

	// export joystick/gamepad values to game

	if (hasjoy)
	{
		JoyAbsX = (int)((x + 1.f) / 2.f * 4095);
		JoyAbsY = (int)((y + 1.f) / 2.f * 4095);
		JoyButtons = buttons;
	}
	else
	{
		JoyAbsX = 0;
		JoyAbsY = 0;
		JoyButtons = 0;
	}
}

void SYS_PlaySound(unsigned short sound)
{
	if (sound >= MAX_SOUNDS)
		return;

	if (!s_sounds[sound].cached)
	{
		char filename[64];
		SDL_AudioSpec waveSpec;
		Uint8 * buffer;
		Uint32 bufferSize;

		s_sounds[sound].cached = true;

		sprintf_s(filename, sizeof(filename), "sfx_%02d.wav", sound);

		if (SDL_LoadWAV(filename, &waveSpec, &buffer, &bufferSize))
		{
			SDL_AudioCVT cvt;

			if (SDL_BuildAudioCVT(
				&cvt,
				waveSpec.format, waveSpec.channels, waveSpec.freq,
				audioSpec.format, audioSpec.channels, audioSpec.freq) == 1)
			{
				cvt.buf = malloc(bufferSize * cvt.len_mult);
				cvt.len = bufferSize;
				memcpy(cvt.buf, buffer, bufferSize);

				SDL_ConvertAudio(&cvt);

				s_sounds[sound].buffer = (short*)cvt.buf;
				s_sounds[sound].bufferEnd = (short*)(cvt.buf + cvt.len * cvt.len_mult);
			}

			SDL_FreeWAV(buffer);
		}
		else
		{
			printf("PlaySound: failed to find %s\n", filename);
		}
	}

	SDL_LockMutex(mutex);
	{
		s_soundPtr = s_sounds[sound].buffer;
		s_soundEnd = s_sounds[sound].bufferEnd;
	}
	SDL_UnlockMutex(mutex);
}

void SYS_StopSound()
{
	SDL_LockMutex(mutex);
	{
		s_soundPtr = s_soundEnd = 0;
	}
	SDL_UnlockMutex(mutex);
}
