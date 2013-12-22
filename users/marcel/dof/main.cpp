#include <math.h>
#include <SDL/SDL.h>
#include "../../../Documents/projects/libgg/SIMD.h"

#include "../../../Documents/projects/libgg/Exception.cpp"
#include "../../../Documents/projects/libgg/Image.cpp"
#include "../../../Documents/projects/libgg/IImageLoader.cpp"
#include "../../../Documents/projects/libgg/ImageLoader_TGA.cpp"
#include "../../../Documents/projects/libgg/StringEx.cpp"
#include "../../../Documents/projects/libgg/Stream.cpp"
#include "../../../Documents/projects/libgg/FileStream.cpp"
#include "../../../Documents/projects/libgg/StreamReader.cpp"
#include "../../../Documents/projects/libgg/StreamWriter.cpp"

typedef SimdVec Vec;
typedef SimdVecArg VecArg;
inline Vec operator+(VecArg v1, VecArg v2) { return v1.Add(v2); }
inline Vec operator-(VecArg v1, VecArg v2) { return v1.Sub(v2); }
inline Vec operator*(VecArg v1, VecArg v2) { return v1.Mul(v2); }
inline Vec operator/(VecArg v1, VecArg v2) { return v1.Div(v2); }
inline Vec operator*(float v1, VecArg v2) { return Vec(v1).Mul(v2); }
inline Vec operator*(VecArg v1, float v2) { return v1.Mul(Vec(v2)); }
inline Vec operator/(VecArg v1, float v2) { return v1.Div(Vec(v2)); }
inline void operator+=(Vec & v1, VecArg v2) { v1 = v1.Add(v2); }
inline void operator-=(Vec & v1, VecArg v2) { v1 = v1.Sub(v2); }
inline void operator*=(Vec & v1, VecArg v2) { v1 = v1.Mul(v2); }
inline void operator/=(Vec & v1, VecArg v2) { v1 = v1.Div(v2); }
inline void operator*=(Vec & v1, float v2) { v1 = v1.Mul(Vec(v2)); }
inline void operator/=(Vec & v1, float v2) { v1 = v1.Div(Vec(v2)); }
inline Vec operator+(VecArg v) { return v; }
inline Vec operator-(VecArg v) { return v.Neg(); }

typedef SimdVec V4;

template <typename T> T Abs(T v) { return v < (T)0 ? -v : +v; }
template <typename T> T Min(T v1, float v2) { return v1 <= v2 ? v1 : v2; }
template <typename T> T Max(T v1, float v2) { return v1 >= v2 ? v1 : v2; }
template <typename T> T Clamp(T v, T vMin, T vMax) { return v < vMin ? vMin : v > vMax ? vMax : v; }
static float Rand(float min, float max) { return min + (max - min) * ((rand() & 1023) / 1023.f); }

template <typename V, int SX, int SY>
class Buffer
{
public:
	static const int kSX = SX;
	static const int kSY = SY;
	 
	void Clear(V v)
	{
		for (int x = 0; x < SX; ++x)
			for (int y = 0; y < SY; ++y)
				m_pixels[x][y] = v;
	}
	void Rect(int x1, int y1, int x2, int y2, V v)
	{
		x1 = Max(x1, 0);
		y1 = Max(y1, 0);
		x2 = Min(x2, SX);
		y2 = Min(y2, SY);
		for (int x = x1; x < x2; ++x)
			for (int y = y1; y < y2; ++y)
				m_pixels[x][y] = v;
	}
	V Load(int x, int y)
	{
		x = Clamp(x, 0, SX-1);
		y = Clamp(y, 0, SY-1);
		return m_pixels[x][y];
	}
	V Sample(float x, float y)
	{
		const float fx = floorf(x);
		const float fy = floorf(y);
		const float tx = x - fx;
		const float ty = y - fy;
		const int px = (int)fx;
		const int py = (int)fy;
		const V v00 = Load(px+0, py+0);
		const V v10 = Load(px+1, py+0);
		const V v01 = Load(px+0, py+1);
		const V v11 = Load(px+1, py+1);
		const V v0 = v00 * (1.f - tx) + v10 * tx;
		const V v1 = v01 * (1.f - tx) + v11 * tx;
		const V v = v0 * (1.f - ty) + v1 * ty;
		return v;
	}
	
	V m_pixels[SX][SY];
};

typedef Buffer<V4, 600, 400> BufferSrc;
typedef Buffer<float, 600, 400> BufferCoc;
typedef Buffer<V4, 600, 400> BufferDst;

static const float kDebugCocParam = 20.f;
static const int kMaxBlurSize = 20;

static float CalcCoc(float z)
{
	return (z - .5f) * 2.f * kDebugCocParam;
}

static void CalcMaxCoc(BufferSrc & src, BufferCoc & coc)
{
	for (int x = 0; x < src.kSX; ++x)
	{
		for (int y = 0; y < src.kSY; ++y)
		{
			const int x1 = Max(x - kMaxBlurSize, 0);
			const int y1 = Max(y - kMaxBlurSize, 0);
			const int x2 = Min(x + kMaxBlurSize, src.kSX - 1);
			const int y2 = Min(y + kMaxBlurSize, src.kSY - 1);
			
			float maxCoc = 0.f;
			for (int sx = x1; sx <= x2; ++sx)
				for (int sy = y1; sy <= y2; ++sy)
					maxCoc = Max(maxCoc, Abs(src.m_pixels[sx][sy].W()));
			
			coc.m_pixels[x][y] = maxCoc;
		}
	}
}

static float CalcSampleArea(int x, int y, float sx, float sy)
{
	return 1.f;
}

static void CalcDof(BufferSrc & srcBuffer, BufferCoc & cocBuffer, BufferDst & dstBuffer)
{
	for (int x = 0; x < srcBuffer.kSX; ++x)
	{
		for (int y = 0; y < srcBuffer.kSY; ++y)
		{
			const float myCoc = srcBuffer.m_pixels[x][y].W();
			const bool isNear = myCoc < 0.f;
			const float testCoc = Max(Abs(myCoc), cocBuffer.m_pixels[x][y]);
			const float testArea = M_PI * testCoc * testCoc;
			//const int numSamples = Min(100, (int)ceilf(testArea));
			//const int numSamples = 150;
			const int numSamples = 60;
			
			// summed
			int numPassed[2] = { 0, 0 };
			float passedArea[2] = { 0.f, 0.f };
			V4 passedSum[2] = { V4(0.f), V4(0.f) };
			
			for (int i = 0; i < numSamples; ++i)
			{
				const float sx = x + Rand(-testCoc, +testCoc);
				const float sy = y + Rand(-testCoc, +testCoc);
				const V4 otherVal = srcBuffer.Sample(sx, sy);
				const float otherCoc = Abs(otherVal.W());
				const float dx = sx - x;
				const float dy = sy - y;
				const float d = sqrtf(dx*dx + dy*dy);
				const int index = otherVal.W() >= 0.f ? 0 : 1;
				if ((index == 0 && Min(otherCoc, myCoc) >= d) || (index == 1 && otherCoc >= d))
				{
					numPassed[index]++;
					passedArea[index] += CalcSampleArea(x, y, sx, sy);
					passedSum[index] += otherVal;
				}
			}
			
			V4 result = srcBuffer.m_pixels[x][y];
			
			for (int i = 0; i < 2; ++i)
			{
				if (numPassed[i])
				{
					const float opacity = Clamp(passedArea[i] / numSamples * (i == 0 ? 1.f : 4.f), 0.f, 1.f);
					result *= (1.f - opacity);
					result += (      opacity) * (passedSum[i] / numPassed[i]);
				}
			}
			
			dstBuffer.m_pixels[x][y] = result;
		}
	}
}

static V4 RandSrcVal(float z)
{
	const float r = Rand(0.f, 1.f) * (1.f - z);
	const float g = Rand(0.f, 1.f) * (1.f - z);
	const float b = Rand(0.f, 1.f) * (1.f - z);
//	return V4(r, g, b, CalcCoc(z));
	return V4(z, z, z, CalcCoc(z));
}

int main(int argc, char * argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	//SDL_Surface * surface = SDL_SetVideoMode(300, 200, 32, SDL_DOUBLEBUF | SDL_SWSURFACE);
	SDL_Surface * surface = SDL_SetVideoMode(600, 400, 32, SDL_DOUBLEBUF | SDL_SWSURFACE);
	if (!surface)
		return 0;

	BufferSrc & src = *new BufferSrc;
	const float clearZ = 0.75f;
	src.Clear(V4(clearZ, clearZ, clearZ, CalcCoc(clearZ)));
	for (int i = 0; i < 20; ++i)
	{
		float z = 1.f - i / 20.f;
		float coc = CalcCoc(z);
		{
			int x1 = rand() % 600;
			int y1 = rand() % 400;
			int x2 = x1 + 400;
			int y2 = y1 + 20;
			src.Rect(x1, y1, x2, y2, RandSrcVal(z));
		}
		{
			int x1 = rand() % 600;
			int y1 = rand() % 400;
			int x2 = x1 + 20;
			int y2 = y1 + 400;
			src.Rect(x1, y1, x2, y2, V4(z, z, z, coc));
		}
	}
	src.Rect(0, 200, 600, 220, V4(.5f, .5f, .5f, 0.f));
	
	ImageLoader_Tga loader;
	Image imageColor;
	Image imageDepth;
	loader.Load(imageColor, "dof_color.tga");
	loader.Load(imageDepth, "dof_depth.tga");
	int sx = Min(src.kSX, Min(imageColor.m_Sx/2, imageDepth.m_Sx/2));
	int sy = Min(src.kSY, Min(imageColor.m_Sy/2, imageDepth.m_Sy/2));
	for (int x = 0; x < sx; ++x)
	{
		for (int y = 0; y < sy; ++y)
		{
			ImagePixel color = imageColor.GetPixel(x*2, y*2);
			ImagePixel depth = imageDepth.GetPixel(x*2, y*2);
			src.m_pixels[x][y] = V4(color.b / 255.f, color.g / 255.f, color.r / 255.f, CalcCoc(1.f - depth.r / 255.f));
		}
	}
	
	BufferCoc & coc = *new BufferCoc;
	BufferDst & dst = *new BufferDst;

	int mode = 0;
		
	bool stop = false;

	while (!stop)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_ESCAPE)
					stop = true;
				else
					mode = (mode + 1) % 5;
			}
		}
			
		switch (mode)
		{
		case 0:
			CalcMaxCoc(src, coc);	
			CalcDof(src, coc, dst);
			break;
		case 1:
			break;
		case 2:
			dst = src;
			break;
		case 3:
			for (int x = 0; x < coc.kSX; ++x)
				for (int y = 0; y < coc.kSY; ++y)
					dst.m_pixels[x][y] = V4(Abs(src.m_pixels[x][y].W()) / kMaxBlurSize);
			break;
		case 4:
			for (int x = 0; x < coc.kSX; ++x)
				for (int y = 0; y < coc.kSY; ++y)
					dst.m_pixels[x][y] = V4(coc.m_pixels[x][y] / kMaxBlurSize);
			break;
		}
		
		if (mode == 0)
			mode = 1;
			
		SDL_LockSurface(surface);

		int * pixels = reinterpret_cast<int*>(surface->pixels);
		const int pitch = surface->pitch >> 2;

		const int shiftR = surface->format->Rshift;
		const int shiftG = surface->format->Gshift;
		const int shiftB = surface->format->Bshift;

	#define PUTPIX(x, y, r, g, b) pixels[(x) + (y) * pitch] = (int(r) << shiftR) | (int(g) << shiftG) | ((int)b << shiftB)

		for (int y = 0; y < src.kSY; ++y)
		{
			for (int x = 0; x < src.kSX; ++x)
			{
				V4 c = dst.m_pixels[x][y];
				int cr = c.X() * 255.f;
				int cg = c.Y() * 255.f;
				int cb = c.Z() * 255.f;
				PUTPIX(x, y, cr, cg, cb);
			}
		}
		
		SDL_UnlockSurface(surface);
		SDL_Flip(surface);
	}

	SDL_Quit();
        
	return 0;
}
