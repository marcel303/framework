#include <SDL/SDL.h>
#include <Windows.h>
#include "syscode.h"
#include "id_heads.h"

#define BLOWUP 4

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

		result |= bit << (31 - i);
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

static SDL_Surface * screen = 0;
static SDL_Surface * surface = 0;
static SDL_Thread * timeThread = 0;
static SDL_AudioSpec audioSpec;
static const struct SampledSound * soundSample = 0;

static int __cdecl TimeThread(void * userData)
{
	while (true)
	{
		SDL_Delay(1000/70); // increment at 70Hz, based on this loc: "if (TimeCount - time > 35)	// Half-second delays"
		TimeCount++;
	}
	return 0;
}

static void SoundThread(void * userData, Uint8 * stream, int length)
{
	// mstodo : need mutex (only around get sound ptr / offset)

	if (soundSample)
	{
		memset(stream, 0, length);
	}
	else
	{
		memset(stream, 0, length);
	}
}

void SYS_Init()
{
	int plane;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		Quit("Failed to initialize SDL");

	if ((screen = SDL_SetVideoMode(320 * BLOWUP, 200 * BLOWUP, 32, SDL_SWSURFACE | SDL_DOUBLEBUF)) == 0)
		Quit("Failed to set video mode");

	if ((surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320 + 8 /*max pelpan*/, 200, 32,
		0xff << screen->format->Rshift,
		0xff << screen->format->Gshift,
		0xff << screen->format->Bshift,
		0x00 << screen->format->Ashift)) == 0)
		Quit("Failed to create surface");

	SDL_ShowCursor(false);
	SDL_WM_GrabInput(SDL_GRAB_ON);
	
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
	audioSpec.freq = 11025;
	audioSpec.format = AUDIO_S8;
	audioSpec.channels = 1;
	audioSpec.samples = 4096; // mstodo : lower ?
	audioSpec.callback = SoundThread;
	if (SDL_OpenAudio(&audioSpec, 0) < 0) {
		//Quit("Failed to init sound playback!");
	}
	else
		SDL_PauseAudio(false);
}

void SYS_SetPalette(char * palette)
{
	// mstodo : SYS_SetPalette
}

void SYS_Present()
{

	if (SDL_LockSurface(surface) == 0)
	{
		// convert palette to surface compatible palette

		unsigned int palette2[16];
		unsigned short x, y, i;

		for (i = 0; i < 16; ++i)
		{
			const unsigned int c = palette[i];
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
					unsigned char b1, b2;
					
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
				unsigned char * srcbytes = (unsigned char*)surface->pixels + surface->pitch * (y / BLOWUP);
				unsigned char * dstbytes = (unsigned char*)screen->pixels + screen->pitch * y;
				unsigned int * __restrict src = (unsigned int*)srcbytes + _pelpan;
				unsigned int * __restrict dst = (unsigned int*)dstbytes;
				unsigned int step = (1 << 22) / BLOWUP;
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

	SYS_Update(); // called here, for convenient in draw loops, so we don't have to add it separately
}

extern void INL_HandleKey(byte k);

int MouseX, MouseY;
int MouseDXf, MouseDYf;
int MouseDX, MouseDY;
int MouseButtons = 0;

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

	MouseDX = MouseDXf / BLOWUP;
	MouseDY = MouseDYf / BLOWUP;
	MouseDXf -= MouseDX * BLOWUP;
	MouseDYf -= MouseDY * BLOWUP;

	//SDL_Delay(100);
}

void SYS_PlaySound(const struct SampledSound * sample)
{
	// mstodo : need mutex

	soundSample = sample;
}

void SYS_StopSound()
{
	// mstodo : need mutex

	//soundSample = 0;
}
