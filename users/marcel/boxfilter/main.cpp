#include <algorithm>
#include <cmath>
#include <CoreFoundation/CoreFoundation.h>
#include <SDL/SDL.h>
#include <xmmintrin.h>

#define SX            640 // display size X
#define SY            480 // display size Y
#define NUM_PARTICLES 100 // number of particles

typedef unsigned char uint8;
typedef unsigned int uint32;

static float sPosition[NUM_PARTICLES][2]; // particle position (x, y) in pixels
static float sSpeed[NUM_PARTICLES][2];    // particle speed (vx, vy) in  pixels per frame
static uint32 sColor[NUM_PARTICLES];      // particle color

static uint32 sKernelSize = 20; // box filter kernel size. effective sample area = size*2 + 1
static bool sClear = false;     // clear draw buffer every frame? (if false, fade the buffer to black)

static uint32 sBuffer[SY][SX] __attribute__((aligned(16))); // draw buffer. must be aligned so it can be aliased as __m128i
static __m128i sSAT[SY][SX];                                // summed area table

static void do_particles()
{
	// update particle positions and speed
	
	const float S[2] = { SX, SY };
	
	for (uint32 i = 0; i < NUM_PARTICLES; ++i)
	{
		for (uint32 j = 0; j < 2; ++j)
		{
			const float p = sPosition[i][j] + sSpeed[i][j];
			
			if (p < 0.f || p >= S[j])
				sSpeed[i][j] *= -1;
			else
				sPosition[i][j] = p;
		}
	}
}

static void do_draw()
{
	// clear or fade draw buffer?
	if (sClear)
	{
		// clear to black
		memset(sBuffer, 0, sizeof(sBuffer));
	}
	else
	{
		// fade to black by subtracting (with saturation) a constant value per frame
		for (uint32 y = 0; y < SY; ++y)
		{
			__m128i * __restrict line = (__m128i*)sBuffer[y];
			for (uint32 x = SX / 4; x != 0; --x, ++line)
				*line = _mm_subs_epu8(*line, _mm_set1_epi8(4));
		}
	}

	const int size = 7; // particle size. effective size = size*2 + 1
		
	for (uint32 i = 0; i < NUM_PARTICLES; ++i)
	{
		const int cx = (int)sPosition[i][0];
		const int cy = (int)sPosition[i][1];
		
		for (int dy = -size; dy <= +size; ++dy)
		{
			for (int dx = -size; dx <= +size; ++dx)
			{
				const int x = cx + dx;
				const int y = cy + dy;
				
				if (x >= 0 && y >= 0 && x < SX && y < SY)
				{
					sBuffer[y][x] = sColor[i];
				}
			}
		}
	}
}

static void do_sat_scan()
{
	#define SCAN_UNROLL_HORI 2
	
	for (uint32 y = 0; y < SY / SCAN_UNROLL_HORI; ++y)
	{
		// horizontal pass
		#define SETUP(N) \
			const __m128i * __restrict src ## N = (__m128i*)sBuffer[y * SCAN_UNROLL_HORI + N]; \
			      __m128i * __restrict dst ## N = sSAT[y * SCAN_UNROLL_HORI + N]; \
			      __m128i current ## N = src ## N [0]

		#if SCAN_UNROLL_HORI >= 1
			SETUP(0);
		#endif
		#if SCAN_UNROLL_HORI >= 2
			SETUP(1);
		#endif
		#if SCAN_UNROLL_HORI >= 3
			SETUP(2);
		#endif
		#if SCAN_UNROLL_HORI >= 4
			SETUP(3);
		#endif
		
		#undef SETUP

		for (uint32 x = SX / 4; x != 0; --x)
		{
			#define PROCESS(N) \
			{ \
				const __m128i src_16_0 = _mm_unpacklo_epi8(*src ## N, _mm_setzero_si128()); \
				const __m128i src_16_1 = _mm_unpackhi_epi8(*src ## N, _mm_setzero_si128()); \
				const __m128i src_32_0 = _mm_unpacklo_epi16(src_16_0, _mm_setzero_si128()); \
				const __m128i src_32_1 = _mm_unpackhi_epi16(src_16_0, _mm_setzero_si128()); \
				const __m128i src_32_2 = _mm_unpacklo_epi16(src_16_1, _mm_setzero_si128()); \
				const __m128i src_32_3 = _mm_unpackhi_epi16(src_16_1, _mm_setzero_si128()); \
				*dst ## N ++ = current ## N; current ## N += src_32_0; \
				*dst ## N ++ = current ## N; current ## N += src_32_1; \
				*dst ## N ++ = current ## N; current ## N += src_32_2; \
				*dst ## N ++ = current ## N; current ## N += src_32_3; \
				++src ## N; \
			}
			
			#if SCAN_UNROLL_HORI >= 1
				PROCESS(0);
			#endif
			#if SCAN_UNROLL_HORI >= 2
				PROCESS(1);
			#endif
			#if SCAN_UNROLL_HORI >= 3
				PROCESS(2);
			#endif
			#if SCAN_UNROLL_HORI >= 4
				PROCESS(3);
			#endif
			
			#undef PROCESS
		}
	}
	
	// vertical pass
	for (uint32 y = 1; y < SY; ++y)
	{
		const __m128i * __restrict src = sSAT[y - 1];
		      __m128i * __restrict dst = sSAT[y - 0];
		
		for (uint32 x = SX / 4; x != 0; --x)
		{
			*dst++ += *src++;
			*dst++ += *src++;
			*dst++ += *src++;
			*dst++ += *src++;
		}
	}
}

static void do_sat_boxfilter()
{
	const uint32 kernelSize = sKernelSize;
	const uint32 kernelArea = (kernelSize * 2 + 1) * (kernelSize * 2 + 1);
	
	for (uint32 y = kernelSize + 1; y < SY - kernelSize; ++y)
	{
		const uint32 offset1 = kernelSize * 1 + 0;
		const uint32 offset2 = kernelSize * 2 + 1;
		
		const __m128i * __restrict v00 = &sSAT[y - kernelSize - 1][0      ];
		const __m128i * __restrict v10 = &sSAT[y - kernelSize - 1][offset2];
		const __m128i * __restrict v01 = &sSAT[y + kernelSize + 0][0      ];
		const __m128i * __restrict v11 = &sSAT[y + kernelSize + 0][offset2];
		
		uint8 * __restrict dst = (uint8*)&sBuffer[y][offset1];
		
		for (uint32 i = 0, todo = SX - offset2; todo != 0; --todo, ++i)
		{
			// calculate total value for area by sampling SAT
			const __m128i value = + v11[i] + v00[i] - v10[i] - v01[i];
			
			// normalize the value by kernel area and convert to 32 bits per pixel
			// note: SSE doesn't feature an integer divide instruction, so we use
			//       the ALU instead. alternatively, we could have multiplied the
			//       total area value by "(1 << shift) / kernelArea" and subsequently
			//       shifted the result back. however, when i tried this the
			//       maximum shift was around 18 before overflowing, and the result
			//       wasn't very accurate with large kernel sizes.. so yeah.. use
			//       a simple ALU divide for now
			const uint32 * __restrict src = (uint32*)&value;
			for (uint32 c = 0; c < 3; ++c)
				dst[c] = src[c] / kernelArea;
			dst += 4;
		}
	}
}

int main(int argc, char * argv[])
{
	printf(
		"controls:\n"
		"UP: increase boxfilter kernel size\n"
		"DOWN: decrease boxfilter kernel size\n"
		"SPACE: toggle clearing the background\n"
		"ESCAPE: quit\n");
		
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_EnableKeyRepeat(200, 50);
	SDL_Surface * surface = SDL_SetVideoMode(SX, SY, 32, SDL_DOUBLEBUF);
	if (!surface)
	{
		printf("failed to set video mode\n");
		return 0;
	}
	
	// initialize particles
	for (int i = 0; i < NUM_PARTICLES; ++i)
	{
		sPosition[i][0] = rand() % SX;
		sPosition[i][1] = rand() % SY;
		sSpeed[i][0] = (((rand() % 1001) / 1000.f) - .5) * 5.f;
		sSpeed[i][1] = (((rand() % 1001) / 1000.f) - .5) * 5.f;
		uint8 * color = (uint8*)&sColor[i];
		color[0] = rand();
		color[1] = rand();
		color[2] = rand();
		color[3] = 0;
	}
	
	bool stop = false;
	
	while (!stop)
	{
		// handle SDL events
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				switch (e.key.keysym.sym)
				{
					case SDLK_UP:
						if (sKernelSize < 100)
							sKernelSize++;
						break;
					case SDLK_DOWN:
						if (sKernelSize > 0)
							sKernelSize--;
						break;
					case SDLK_SPACE:
						sClear = !sClear;
						break;
					case SDLK_ESCAPE:
						stop = true;
						break;
					default:
						break;
				}
			}
		}
		
		// update particles and draw them
		do_particles();
		do_draw();

		// apply box filter using SAT
		do_sat_scan();
		do_sat_boxfilter();
	
		// copy draw buffer into SDL surface
		SDL_LockSurface(surface);
		
		uint32 * pixels = (uint32*)surface->pixels;
		const uint32 pitch = surface->pitch >> 2;
		
		const uint32 shiftR = surface->format->Rshift;
		const uint32 shiftG = surface->format->Gshift;
		const uint32 shiftB = surface->format->Bshift;
		
	#define PUTPIX(x, y, r, g, b) pixels[(x) + (y) * pitch] = ((r) << shiftR) | ((g) << shiftG) | ((b) << shiftB)
	#define MAKE_RGB(r, g, b) (((r) << shiftR) | ((g) << shiftG) | ((b) << shiftB))
		
		for (uint32 y = 0; y < SY; ++y)
		{
			const uint8 * __restrict src = (uint8*)sBuffer[y];
			uint32 * __restrict dst = pixels + y * pitch;
			
			for (uint32 x = SX >> 2; x != 0; --x)
			{
				*dst++ = MAKE_RGB(src[0], src[1], src[2]); src += 4;
				*dst++ = MAKE_RGB(src[0], src[1], src[2]); src += 4;
				*dst++ = MAKE_RGB(src[0], src[1], src[2]); src += 4;
				*dst++ = MAKE_RGB(src[0], src[1], src[2]); src += 4;
			}
		}
		
		SDL_UnlockSurface(surface);
		SDL_Flip(surface);
	}
	
	SDL_Quit();
	
	return 0;	
}
