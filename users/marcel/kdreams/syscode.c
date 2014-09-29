#include <SDL/SDL.h>
#include <Windows.h>
#include "syscode.h"
#include "id_heads.h"

#define BLOWUP 1

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
	// mstodo : move location. check CRTC/pelpan here

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

	if ((surface = SDL_CreateRGBSurface(SDL_SWSURFACE, 320 + 8 /*max pelpan*/, 200, 32, 0xff << 0, 0xff << 8, 0xff << 16, 0x00 << 24)) == 0)
		Quit("Failed to create surface");

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
	int x, y, p, i;
	unsigned int *dst;

	//printf("CRTC=%d, pelpan=%d\n", _CRTC, _pelpan);

	if (SDL_LockSurface(surface) == 0)
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
				(r << surface->format->Rshift) |
				(g << surface->format->Gshift) |
				(b << surface->format->Bshift);
		}

		for (y = 0; y < surface->h; ++y)
		{
			dst = (unsigned int*)(((unsigned char*)surface->pixels) + surface->pitch * y);

			for (x = 0; x < surface->w; x += 8)
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

	{
		SDL_Rect srcrect;
		SDL_Rect dstrect;

		srcrect.x = 0;
		srcrect.y = 0;
		srcrect.w = 328;
		srcrect.h = 200;

		dstrect.x = -_pelpan;
		dstrect.y = 0;
		dstrect.w = 0;
		dstrect.h = 0;

		SDL_BlitSurface(surface, &srcrect, screen, &dstrect);
	}

	SDL_Flip(screen); // mstodo : we need to guarantee we do the page flip with vsync. does SDL guarantee this?

	SYS_Update(); // called here, for convenient in draw loops, so we don't have to add it separately
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
#if 0
				Keyboard[code] = (boolean)(e.key.state == SDL_PRESSED);

				if (Keyboard[code])
				{
					LastCode = CurCode;
					CurCode = LastScan = code;
				}
#else
				// from INL_KeyService

				static	boolean	special;
						byte	k,c,
								temp;

				k = code;

				if (k == 0xe0)		// Special key prefix
					special = true;
				else if (k == 0xe1)	// Handle Pause key
					Paused = true;
				else
				{
					//if (k & 0x80)	// Break code
					if (e.type == SDL_KEYUP)
					{
						//k &= 0x7f;

			// DEBUG - handle special keys: ctl-alt-delete, print scrn

						Keyboard[k] = false;
					}
					else			// Make code
					{
						LastCode = CurCode;
						CurCode = LastScan = k;
						Keyboard[k] = true;

						if (special)
							c = SpecialNames[k];
						else
						{
							if (k == sc_CapsLock)
							{
								CapsLock ^= true;
								// DEBUG - make caps lock light work
							}

							if (Keyboard[sc_LShift] || Keyboard[sc_RShift])	// If shifted
							{
								c = ShiftNames[k];
								if ((c >= 'A') && (c <= 'Z') && CapsLock)
									c += 'a' - 'A';
							}
							else
							{
								c = ASCIINames[k];
								if ((c >= 'a') && (c <= 'z') && CapsLock)
									c -= 'a' - 'A';
							}
						}
						if (c)
							LastASCII = c;
					}

					special = false;
				}
#endif
			}
		}
	}

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
