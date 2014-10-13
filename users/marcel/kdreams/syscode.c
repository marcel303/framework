#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <Windows.h>
#include "syscode.h"
#include "syscode_xinput.h"
#include "id_heads.h"
#include "opl/opl.h"

#define MAX_SOUNDS 64
#define USE_OPENGL 1
#define PROFILING 0

extern void SDL_SoundFinished();
static void SYS_PollJoy();

extern void Quit (char *error);

// IN_*

extern boolean	CapsLock;
extern ScanCode	CurCode,LastCode;
extern byte		ASCIINames[], ShiftNames[], SpecialNames[];

// SD_*

extern volatile word SoundNumber;
extern volatile word SoundPriority;

//

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
	// return 32 adjacent bits (8 adjacent pixels), using planar VGA mode

	const unsigned int poffset = (offset >> 3);
	const unsigned char plane0 = g0xA000[0][poffset];
	const unsigned char plane1 = g0xA000[1][poffset];
	const unsigned char plane2 = g0xA000[2][poffset];
	const unsigned char plane3 = g0xA000[3][poffset];
	unsigned int result, i;

	result = 0;

	//result = _pdep_u32(plane0, (1 << 0) | (1 << 4) | (1 << 8) | (1 << 12) | ...);

	for (i = 0; i < 8; ++i)
	{
		const unsigned int mask = 1 << i;
		const unsigned int pixel =
			(plane0 & mask) << 0 |
			(plane1 & mask) << 1 |
			(plane2 & mask) << 2 |
			(plane3 & mask) << 3;
		result = (result << 4) | (pixel >> i);
	}

	return result;
}

static unsigned int s_palette[16] =
{
	0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
	0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
	0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
	0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF
};

static unsigned char s_palettemap[16] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

static SDL_mutex * s_mutex = 0;
static SDL_Surface * s_screen = 0;
static SDL_Joystick * s_joy = 0;
static SDL_Thread * s_timeThread = 0;
static SDL_AudioSpec s_audioSpec;
static int s_tickrate = 0;
static GLuint s_texture = 0;

//

static int __cdecl TimeThread(void * userData)
{
	while (true)
	{
		SDL_Delay(1000/s_tickrate); // increment at 70Hz, based on this loc: "if (TimeCount - time > 35)	// Half-second delays"
		TimeCount++;
	}
	return 0;
}

// 	AdLib Code

#define alOut adlib_write

// This table maps channel numbers to carrier and modulator op cells
static	byte			carriers[9] =  { 3, 4, 5,11,12,13,19,20,21},
						modifiers[9] = { 0, 1, 2, 8, 9,10,16,17,18};
static	ActiveTrack		*tracks[sqMaxTracks];
static	word			sqMode,sqFadeStep;

//	AdLib variables
static	boolean			alNoCheck;
static	byte			far *alSound;
static	byte			alBlock;
static	longword		alLengthLeft;

static void SDL_SetInstrument(int which, Instrument *inst)
{
	byte c,m;

	// DEBUG - what about percussive instruments?

	m = modifiers[which];
	c = carriers[which];
	tracks[which]->inst = *inst;

	alOut(m + alChar,inst->mChar);
	alOut(m + alScale,inst->mScale);
	alOut(m + alAttack,inst->mAttack);
	alOut(m + alSus,inst->mSus);
	alOut(m + alWave,inst->mWave);

	alOut(c + alChar,inst->cChar);
	alOut(c + alScale,inst->cScale);
	alOut(c + alAttack,inst->cAttack);
	alOut(c + alSus,inst->cSus);
	alOut(c + alWave,inst->cWave);
}

static void SDL_ALStopSound(void)
{
	alSound = 0;
	alOut(alFreqH + 0,0);
}

static void SDL_ALPlaySound(AdLibSound far *sound)
{
	byte		c,m;
	Instrument	far *inst;

	SDL_ALStopSound();

	alLengthLeft = sound->common.length;
	alSound = sound->data;
	alBlock = ((sound->block & 7) << 2) | 0x20;
	inst = &sound->inst;

	if (!(inst->mSus | inst->cSus))
		Quit("SDL_ALPlaySound() - Seriously suspicious instrument");

	m = modifiers[0];
	c = carriers[0];
	alOut(m + alChar,inst->mChar);
	alOut(m + alScale,inst->mScale);
	alOut(m + alAttack,inst->mAttack);
	alOut(m + alSus,inst->mSus);
	alOut(m + alWave,inst->mWave);
	alOut(c + alChar,inst->cChar);
	alOut(c + alScale,inst->cScale);
	alOut(c + alAttack,inst->cAttack);
	alOut(c + alSus,inst->cSus);
	alOut(c + alWave,inst->cWave);
}

static void SDL_ALSoundService(void)
{
	byte	s;

	if (alSound)
	{
		s = *alSound++;
		if (!s)
			alOut(alFreqH + 0,0);
		else
		{
			alOut(alFreqL + 0,s);
			alOut(alFreqH + 0,alBlock);
		}

		if (!(--alLengthLeft))
		{
			(long)alSound = 0;
			alOut(alFreqH + 0,0);
			SDL_SoundFinished();
		}
	}
}

// digitized audio code

static struct
{
	boolean cached;
	short * buffer;
	short * bufferEnd;
} s_sounds[MAX_SOUNDS];

static short * s_soundPtr = 0;
static short * s_soundEnd = 0;

// audio thread

static void SoundThread(void * userData, Uint8 * stream, int length)
{
	if (SoundMode == sdm_AdLib)
	{
		SDL_LockMutex(s_mutex);
		{
			// original sound service runs at 140 Hz. at 44.1kHz, that's 44100 / 140 = 315 samples per update
			int adlibRate = AUDIO_SAMPLE_RATE / 140;
			static int adlibCount = 0;

			// keep fetching samples from the adlib core until we're done. meanwhile, make sure we're
			// updating the adlib sound code 140 times per second
			const int numSamples = length / 2;
			Bit16s * __restrict samplePtr = ((Bit16s*)stream);
			int todo = numSamples;
			do
			{
				int num = adlibRate - adlibCount;

				if (num > todo)
					num = todo;

				// fetch samples from the adlib core
				adlib_getsample(samplePtr, num);

				adlibCount += num;
				samplePtr += num;
				todo -= num;

				if (adlibCount == adlibRate)
				{
					// it's time to check the adlib params
					adlibCount = 0;
					SDL_ALSoundService();
				}
			}
			while (todo != 0);
		}
		SDL_UnlockMutex(s_mutex);
	}
	else if (SoundMode == sdm_SoundBlaster)
	{
		SDL_LockMutex(s_mutex);
		{
			short * __restrict dest = (short*)stream;
			short * __restrict destEnd = (short*)(stream + length);

			// keep outputting digitized sound data, until we're at the end of the buffer

			while (dest < destEnd && s_soundPtr != s_soundEnd)
			{
				*dest++ = *s_soundPtr++;
			}

			// fill the remainder of the destination buffer with silence

			while (dest < destEnd)
			{
				*dest++ = 0;
			}

			// are we done with this sound?

			if (s_soundPtr == s_soundEnd)
			{
				SDL_SoundFinished();
			}
		}
		SDL_UnlockMutex(s_mutex);
	}
}

//

void SYS_Init(int tickrate, int displaySx, int displaySy, int fullscreen)
{
	int i, plane;

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
		Quit("Failed to initialize SDL");

	SDL_WM_SetCaption("Keen Dreams", NULL);
#ifndef _DEBUG
	SDL_ShowCursor(false);
	SDL_WM_GrabInput(SDL_GRAB_ON);
#endif

	s_mutex = SDL_CreateMutex();

	if (!fullscreen)
	{
		if (displaySx == 0) displaySx = 320*3;
		if (displaySy == 0) displaySy = 200*3;
	}

	_putenv("SDL_VIDEO_CENTERED=1");

#if USE_OPENGL
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	if ((s_screen = SDL_SetVideoMode(displaySx, displaySy, 32, SDL_OPENGL | (fullscreen ? SDL_FULLSCREEN : 0))) == 0)
		Quit("Failed to set video mode");

	// create texture
	glGenTextures(1, &s_texture);
	glBindTexture(GL_TEXTURE_2D, s_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
#else
	if ((s_screen = SDL_SetVideoMode(displaySx, displaySy, 32, SDL_HWSURFACE | SDL_DOUBLEBUF | (fullscreen ? SDL_FULLSCREEN : 0))) == 0)
		Quit("Failed to set video mode");
#endif

	if (SDL_NumJoysticks() > 0)
		s_joy = SDL_JoystickOpen(0);

	s_tickrate = tickrate;

	if ((s_timeThread = SDL_CreateThread(TimeThread, 0)) == 0)
		Quit("Failed to create timer thread");

	for (plane = 0; plane < 4; ++plane)
	{
	#if PROTECT_DISPLAY_BUFFER
		g0xA000[plane] = VirtualAlloc(NULL, DISPLAY_BUFFER_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	#endif
		memset(g0xA000[plane], 0, DISPLAY_BUFFER_SIZE);
	}

	memset(&s_audioSpec, 0, sizeof(s_audioSpec));
	s_audioSpec.freq = 44100;
	s_audioSpec.format = AUDIO_S16;
	s_audioSpec.channels = 1;
	s_audioSpec.samples = 1024;
	s_audioSpec.callback = SoundThread;
	if (SDL_OpenAudio(&s_audioSpec, 0) < 0) {
		//Quit("Failed to init sound playback!");
	}
	else
		SDL_PauseAudio(false);

	adlib_init(44100);

	SYS_PollJoy();

	for (i = 0; i < 3; ++i)
		SYS_Present();
}

void SYS_SetPalette(char * palette)
{
	unsigned int i;

	for (i = 0; i < 16; ++i)
		s_palettemap[i] = palette[i] & 0xf; // mstodo : sometimes the 5th bit is set. need 64 entry palette?
}

static void SYS_Present_Software()
{
	// convert data in EGA memory to RGBA buffer

	unsigned int __declspec(align(16)) buffer[200][328];
	LARGE_INTEGER freq, t1, t2, t3;

#if PROFILING
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&t1);
#endif

	{
		// convert palette to surface compatible palette

		unsigned int nativePalette[16];
		unsigned short x, y, i;

		for (i = 0; i < 16; ++i)
		{
			const unsigned int c = s_palette[s_palettemap[i]];
			const unsigned int r = (c >> 16) & 0xff;
			const unsigned int g = (c >> 8 ) & 0xff;
			const unsigned int b = (c >> 0 ) & 0xff;
			nativePalette[i] =
				(r << s_screen->format->Rshift) |
				(g << s_screen->format->Gshift) |
				(b << s_screen->format->Bshift);
		}

		for (y = 0; y < 200; ++y)
		{
			unsigned int * __restrict dst = buffer[y];

			for (x = 0; x < 328; x += 8)
			{
				unsigned int src = gather32(_CRTC * 8 + y * 512 + x);

				for (i = 0; i < 8; ++i)
				{
					*dst++ = nativePalette[src & 15];

					src >>= 4;
				}
			}
		}
	}

#if PROFILING
	QueryPerformanceCounter(&t2);

	printf("conversion time: %g ms\n", 1000.f * (t2.QuadPart - t1.QuadPart) / freq.QuadPart);
#endif

	// upscale RGBA buffer to SDL screen

	if (SDL_LockSurface(s_screen) == 0)
	{
		unsigned int x, y;

		// do a scaled blit. use 10.22 fixed point for sampling the source in the horizontal direction

		for (y = 0; y < s_screen->h; ++y)
		{
			unsigned char * srcbytes = (unsigned char*)buffer[y * 200 / s_screen->h];
			unsigned char * dstbytes = (unsigned char*)s_screen->pixels + s_screen->pitch * y;
			unsigned int * __restrict src = (unsigned int*)srcbytes + _pelpan;
			unsigned int * __restrict dst = (unsigned int*)dstbytes;
			unsigned int step = (320 << 22) / s_screen->w;
			unsigned int pos = 0;

			for (x = 0; x < s_screen->w; ++x, pos += step)
			{
				*dst++ = src[pos >> 22];
			}
		}

		SDL_UnlockSurface(s_screen);
	}

#if PROFILING
	QueryPerformanceCounter(&t3);

	printf("upscale time: %g ms\n", 1000.f * (t3.QuadPart - t2.QuadPart) / freq.QuadPart);
#endif

	SDL_Flip(s_screen); // mstodo : we need to guarantee we do the page flip with vsync. does SDL guarantee this?
}

static void SYS_Present_OpenGL()
{
	// blit/convert EGA memory to OpenGL texture

	unsigned int __declspec(align(16)) buffer[200][328];
	LARGE_INTEGER freq, t1, t2;

	struct
	{
		union
		{
			unsigned int asInt;
			unsigned char rgba[4];
		};
	} nativePalette[16];

	unsigned short x, y, i;

#if PROFILING
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&t1);
#endif

	// convert palette to surface compatible palette

	for (i = 0; i < 16; ++i)
	{
		const unsigned int c = s_palette[s_palettemap[i]];
		const unsigned char r = (c >> 16) & 0xff;
		const unsigned char g = (c >> 8 ) & 0xff;
		const unsigned char b = (c >> 0 ) & 0xff;
		nativePalette[i].rgba[0] = r;
		nativePalette[i].rgba[1] = g;
		nativePalette[i].rgba[2] = b;
		nativePalette[i].rgba[3] = 0xff;
	}

	for (y = 0; y < 200; ++y)
	{
		unsigned int * __restrict dst = buffer[y];

		for (x = 0; x < 328; x += 8)
		{
			unsigned int src = gather32(_CRTC * 8 + y * 512 + x);

			for (i = 0; i < 8; ++i)
			{
				*dst++ = nativePalette[src & 15].asInt;

				src >>= 4;
			}
		}
	}

#if PROFILING
	QueryPerformanceCounter(&t2);

	printf("conversion time: %g ms\n", 1000.f * (t2.QuadPart - t1.QuadPart) / freq.QuadPart);
#endif

	// setup viewport

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, 320.0, 200.0, 0.0, -1.0, +1.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// update texture contents

	glBindTexture(GL_TEXTURE_2D, s_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 328, 200, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	// draw a textured quad, taking the current pixel panning into account

	glBindTexture(GL_TEXTURE_2D, s_texture);
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
}

void SYS_Present()
{
#if !USE_OPENGL
	SYS_Present_Software();
#else
	SYS_Present_OpenGL();
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
			MouseX = e.motion.x * 320 / s_screen->w;
			MouseY = e.motion.y * 200 / s_screen->h;

			MouseDXf += e.motion.xrel;
			MouseDYf += e.motion.yrel;
		}

		if (e.type == SDL_MOUSEBUTTONDOWN)
			MouseButtons |= 1 << e.button.button;
		if (e.type == SDL_MOUSEBUTTONUP)
			MouseButtons &= ~(1 << e.button.button);
	}

	// export mouse values to game

	MouseDX = MouseDXf * 320 / s_screen->w;
	MouseDY = MouseDYf * 200 / s_screen->h;
	MouseDXf -= MouseDX * s_screen->w / 320;
	MouseDYf -= MouseDY * s_screen->h / 200;

	SYS_PollJoy();
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
	else if (s_joy)
	{
		unsigned int i;
		unsigned char axismap[2] = { 0, 1 }; // mstodo : axis and button mapping should be in a config file

		hasjoy = true;

		SDL_JoystickUpdate();

		for (i = 0; i < 2; ++i)
		{
			float v = 0.f;

			if (axismap[i] < SDL_JoystickNumAxes(s_joy))
			{
				v = SDL_JoystickGetAxis(s_joy, axismap[i]) / 32768.f;
			}

			if (i == 0)
				x = v;
			else
				y = v;
		}

		for (i = 0; i < SDL_JoystickNumButtons(s_joy); ++i)
			buttons |= (SDL_JoystickGetButton(s_joy, i) == 0 ? 0 : 1) << i;
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

	if (SoundMode == sdm_AdLib)
	{
		byte ** SoundTable = &audiosegs[STARTADLIBSOUNDS];
		AdLibSound * Sound = (AdLibSound*)SoundTable[sound];

		if (Sound)
		{
			SDL_LockMutex(s_mutex);
			{
				SDL_ALPlaySound(Sound);
			}
			SDL_UnlockMutex(s_mutex);
		}
	}
	else if (SoundMode == sdm_SoundBlaster)
	{
		if (!s_sounds[sound].cached)
		{
			char filename[64];
			SDL_AudioSpec waveSpec;
			Uint8 * buffer;
			Uint32 bufferSize;

			s_sounds[sound].cached = true;

			sprintf_s(filename, sizeof(filename), "SFX%03d.WAV", sound);

			if (SDL_LoadWAV(filename, &waveSpec, &buffer, &bufferSize))
			{
				SDL_AudioCVT cvt;

				if (SDL_BuildAudioCVT(
					&cvt,
					waveSpec.format, waveSpec.channels, waveSpec.freq,
					s_audioSpec.format, s_audioSpec.channels, s_audioSpec.freq) == 1)
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
				printf("PlaySound: failed to open %s\n", filename);
			}
		}

		SDL_LockMutex(s_mutex);
		{
			s_soundPtr = s_sounds[sound].buffer;
			s_soundEnd = s_sounds[sound].bufferEnd;
		}
		SDL_UnlockMutex(s_mutex);
	}
}

void SYS_StopSound()
{
	SDL_LockMutex(s_mutex);
	{
		s_soundPtr = s_soundEnd = 0;
	}
	SDL_UnlockMutex(s_mutex);
}
