#include <SDL/SDL.h>
#include "syscode.h"
#include "id_heads.h"

#define BLOWUP 1

extern void Quit (char *error);
extern ScanCode	CurCode,LastCode;

static unsigned short _CRTC = 0;
static unsigned short _pelpan = 0;

void VW_SetScreen (unsigned short CRTC, unsigned short pelpan)
{
	// mstodo : move location. check CRTC/pelpan here

	_CRTC = CRTC;
	_pelpan = pelpan;

	SYS_Present();
}

unsigned char g0xA000[4][DISPLAY_BUFFER_SIZE];

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
static SDL_Thread * timeThread = 0;

static int __cdecl TimeThread(void * userData)
{
	while (true)
	{
		SDL_Delay(10);
		TimeCount++;
	}
	return 0;
}

void SYS_Init()
{
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		Quit("Failed to initialize SDL");

	if ((screen = SDL_SetVideoMode(320 * BLOWUP, 200 * BLOWUP, 32, SDL_SWSURFACE)) == 0)
		Quit("Failed to set video mode");

	if ((timeThread = SDL_CreateThread(TimeThread, 0)) == 0)
		Quit("Failed to create timer thread");

	memset(g0xA000, 0, sizeof(g0xA000));
}

void SYS_Present()
{
	int x, y, p, i;
	unsigned int *dst;

	//printf("CRTC=%d, pelpan=%d\n", _CRTC, _pelpan);

	if (SDL_LockSurface(screen) == 0)
	{
		// convert palette to surface compatible palette

		unsigned int palette2[16];

		for (i = 0; i < 16; ++i)
		{
			const unsigned int c = palette[i];
			const unsigned int r = (c >> 16) & 0xff;
			const unsigned int g = (c >> 8 ) & 0xff;
			const unsigned int b = (c >> 0 ) & 0xff;
			palette2[i] =
				(r << screen->format->Rshift) |
				(g << screen->format->Gshift) |
				(b << screen->format->Bshift);
		}

		for (y = 0; y < screen->h; ++y)
		{
			dst = (unsigned int*)(((unsigned char*)screen->pixels) + screen->pitch * y);

			for (x = 0; x < screen->w; x += 8)
			{
				unsigned int src;
				unsigned int sampx, sampy;

			#if BLOWUP == 1
				sampx = x;
				sampy = y;
			#else
				sampx = x / BLOWUP;
				sampy = y / BLOWUP;
			#endif

				src = gather32(_CRTC * 8 + sampy * 512 + sampx);
				//unsigned int src = gather32((displayofs + panadjust) * 8 + y * 512 + x);
				//unsigned int src = gather32(masterofs * 8 + y * 512 + x);
				//unsigned int src = gather32(bufferofs * 8 + y * 512 + x);

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

		SDL_UnlockSurface(screen);
	}

	SDL_Flip(screen);

	SYS_Update();
}

void SYS_Update()
{
	SDL_Event e;

	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
		{
			int code = e.key.keysym.scancode;

			if (code < NumCodes)
			{
				Keyboard[code] = (boolean)(e.key.state == SDL_PRESSED);

				if (Keyboard[code])
				{
					LastCode = CurCode;
					CurCode = LastScan = code;
				}
			}
		}
	}
}
