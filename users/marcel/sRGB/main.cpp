#include <algorithm>
#include <cmath>
#include <CoreFoundation/CoreFoundation.h>
#include <SDL/SDL.h>
#include "../../../Documents/projects/libgg/SIMD.h"

typedef SimdVec Vec;
typedef SimdVecArg VecArg;

inline Vec operator+(VecArg v1, VecArg v2) { return v1.Add(v2); }
inline Vec operator-(VecArg v1, VecArg v2) { return v1.Sub(v2); }
inline Vec operator*(VecArg v1, VecArg v2) { return v1.Mul(v2); }
inline Vec operator/(VecArg v1, VecArg v2) { return v1.Div(v2); }
inline Vec operator*(float v1, VecArg v2) { return Vec(v1).Mul(v2); }
inline Vec operator*(VecArg v1, float v2) { return v1.Mul(Vec(v2)); }
inline void operator+=(Vec & v1, VecArg v2) { v1 = v1.Add(v2); }
inline void operator-=(Vec & v1, VecArg v2) { v1 = v1.Sub(v2); }
inline void operator*=(Vec & v1, VecArg v2) { v1 = v1.Mul(v2); }
inline void operator/=(Vec & v1, VecArg v2) { v1 = v1.Div(v2); }
inline Vec operator+(VecArg v) { return v; }
inline Vec operator-(VecArg v) { return v.Neg(); }

const static float kGamma = 2.2f;

inline float GammaToLinear(float v)
{
	return std::pow(v, kGamma);
}

inline float LinearToGamma(float v)
{
	return std::pow(v, 1.f / kGamma);
}

inline Vec GammaToLinear(VecArg v)
{
	return Vec(
		GammaToLinear(v.X()),
		GammaToLinear(v.Y()),
		GammaToLinear(v.Z()),
		GammaToLinear(v.W()));
}

inline Vec LinearToGamma(VecArg v)
{
	return Vec(
		LinearToGamma(v.X()),
		LinearToGamma(v.Y()),
		LinearToGamma(v.Z()),
		LinearToGamma(v.W()));
}

int LinearMix(int v1, int v2, float a)
{
	float linear1 = GammaToLinear(v1/255.f);
	float linear2 = GammaToLinear(v2/255.f);
	float linear = linear1 * (1.f - a) + linear2 * a;
	return (int)(LinearToGamma(linear) * 255.f);
}

int RaySphere(
	VecArg rayOriginX,
	VecArg rayOriginY,
	VecArg rayOriginZ,
	VecArg rayDirectionX,
	VecArg rayDirectionY,
	VecArg rayDirectionZ,
	VecArg sphereEq,
	Vec & t1, Vec & t2)
{ 
	const Vec a =
		rayDirectionX * rayDirectionX +
		rayDirectionY * rayDirectionY +
		rayDirectionZ * rayDirectionZ;

	const Vec b = 2.f * (
		rayDirectionX * (rayOriginX - sphereEq.ReplicateX()) +
		rayDirectionY * (rayOriginY - sphereEq.ReplicateY()) +
		rayDirectionZ * (rayOriginZ - sphereEq.ReplicateZ()));

	Vec c = sphereEq.Dot3(sphereEq);

	c +=
   		rayOriginX * rayOriginX +
   		rayOriginY * rayOriginY +
   		rayOriginZ * rayOriginZ;

	c -= 2.f * (
		sphereEq.ReplicateX() * rayOriginX +
		sphereEq.ReplicateY() * rayOriginY +
		sphereEq.ReplicateZ() * rayOriginZ);

	c -= sphereEq.ReplicateW() * sphereEq.ReplicateW();

	const Vec bb4ac = b * b - 4.f * a * c;

	const Vec hit = a.CMP_GT(VEC_ZERO).AND(bb4ac.CMP_GE(VEC_ZERO));

	const int result = hit.BITTEST();

	if (result)
	{
		t1 = (-b + bb4ac.Sqrt()) / (2.f * a);
		t2 = (-b - bb4ac.Sqrt()) / (2.f * a);
	}

	return result;
}

void RayPlane(
	VecArg rayOriginX,
	VecArg rayOriginY,
	VecArg rayOriginZ,
	VecArg rayDirectionX,
	VecArg rayDirectionY,
	VecArg rayDirectionZ,
	VecArg planeEquationX,
	VecArg planeEquationY,
	VecArg planeEquationZ,
	VecArg planeEquationW,
	Vec & t)
{
	Vec d =
		rayOriginX.Mul(planeEquationX).Add(
		rayOriginY.Mul(planeEquationY).Add(
		rayOriginZ.Mul(planeEquationZ)));
		
	Vec dd =
		rayDirectionX.Mul(planeEquationX).Add(
		rayDirectionY.Mul(planeEquationY).Add(
		rayDirectionZ.Mul(planeEquationZ)));

	// d + dd * t = 0 => t = (0 - d) / dd
	
	t = d.Neg().Div(dd);
}

int main(int argc, char * argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	//SDL_Surface * surface = SDL_SetVideoMode(300, 200, 32, SDL_DOUBLEBUF | SDL_SWSURFACE);
	SDL_Surface * surface = SDL_SetVideoMode(600, 400, 32, SDL_DOUBLEBUF | SDL_SWSURFACE);
	if (!surface)
		return 0;
	
	bool stop = false;
	
	while (!stop)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
				stop = true;
		}
		
		SDL_LockSurface(surface);
		
		int * pixels = reinterpret_cast<int*>(surface->pixels);
		const int pitch = surface->pitch >> 2;
			
		const int shiftR = surface->format->Rshift;
		const int shiftG = surface->format->Gshift;
		const int shiftB = surface->format->Bshift;

	#define PUTPIX(x, y, r, g, b) pixels[(x) + (y) * pitch] = (int(r) << shiftR) | (int(g) << shiftG) | ((int)b << shiftB)
		
		static int l = 0;
		l = (l + 1) & 255;
		
		int s = surface->w/3;
		for (int y = 0; y < s; ++y)
		{
			for (int x = 0; x < s; ++x)
			{
				const int v1 = (x + y) & 1 ? l : 0;
				const int v2 = l / 2;
				const int v3 = LinearMix(0, l, .5f);
				
				PUTPIX(x + s*0, y, v1, v1, v1);
				PUTPIX(x + s*1, y, v2, v2, v2);
				PUTPIX(x + s*2, y, v3, v3, v3);
			}
		}
		
		double time = 0.0;
		time -= CFAbsoluteTimeGetCurrent();
		
		static float animationTime = 0.f;
		animationTime += 1.f / 160.f;
		
		//usleep(1000*50);
		
		Vec materialColorBase(.5f, .7f, .9f);
		
		const int numLights = 2;
		Vec lightDirection[numLights];
		static Vec lightColorBase[numLights];
		
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			for (int i = 0; i < numLights; ++i)
			{
				lightDirection[i] = Vec(
					std::sin(animationTime * (i + 1)),
					std::sin(animationTime * (i + 1) * 1.234f),
					std::sin(animationTime * (i + 1) * 2.345f), 0.f).UnitVec3();
				lightColorBase[i] = Vec((rand() % 1024) / 1023.f, (rand() % 1024) / 1023.f, (rand() % 1024) / 1023.f);
			}
		}
		
		for (int i = 0; i < numLights; ++i)
		{
			lightDirection[i] = Vec(
				std::sin(animationTime * (i + 1)),
				std::sin(animationTime * (i + 1) * 1.234f),
				std::sin(animationTime * (i + 1) * 2.345f), 0.f).UnitVec3();
		}
						
		for (int r = 0; r < 2; ++r)
		{
			const Vec materialColor = (r == 1) ? GammaToLinear(materialColorBase) : materialColorBase;
			Vec lightColor[numLights];
			for (int i = 0; i < numLights; ++i)
				lightColor[i] = (r == 1) ? GammaToLinear(lightColorBase[i]) : lightColorBase[i];
			
			const Vec sphereEq(r == 0 ? -.6f : +.6f, 0.f, 1.f, .5f);

			const float viewScaleRcp = 1.f / (surface->h / 2.f);
			const float viewCenterX = surface->w / 2.f;
			const float viewCenterY = surface->h / 2.f;
			
			const Vec rayDirectionX(0.f);
			const Vec rayDirectionY(0.f);
			const Vec rayDirectionZ(1.f);
			
			//#pragma omp parallel for
			for (int y = 0; y < surface->h; ++y)			
			{
				Vec rayOriginX;
				Vec rayOriginY;
				Vec rayOriginZ(0.f);
				for (int i = 0; i < 4; ++i)
					rayOriginY(i) = (y - viewCenterY) * viewScaleRcp;
										
				for (int x = 0; x < surface->w / 4; ++x)
				{				
					for (int i = 0; i < 4; ++i)
						rayOriginX(i) = ((x * 4 + i) - viewCenterX) * viewScaleRcp;
					
					// intersection
					Vec t1, t2;
					const int hit = RaySphere(
						rayOriginX,
						rayOriginY,
						rayOriginZ,
						rayDirectionX,
						rayDirectionY,
						rayDirectionZ,
						sphereEq,
						t1, t2);
					
					if (hit)
					{
						// intersection point, normal
						const Vec t = t1.Min(t2);
						const Vec positionX = rayOriginX + rayDirectionX * t;
						const Vec positionY = rayOriginY + rayDirectionY * t;
						const Vec positionZ = rayOriginZ + rayDirectionZ * t;
						Vec normalX = positionX - sphereEq.ReplicateX();
						Vec normalY = positionY - sphereEq.ReplicateY();
						Vec normalZ = positionZ - sphereEq.ReplicateZ();
						Normalize3(normalX, normalY, normalZ);
						// lighting
						Vec diffuseX = VEC_ZERO;
						Vec diffuseY = VEC_ZERO;
						Vec diffuseZ = VEC_ZERO;
						for (int i = 0; i < numLights; ++i)
						{
							Vec lightIntensity =
								lightDirection[i].ReplicateX() * normalX +
								lightDirection[i].ReplicateY() * normalY +
								lightDirection[i].ReplicateZ() * normalZ;
							if (i == 0 && numLights != 1)
								lightIntensity = Vec(.5f);
							//lightIntensity = lightIntensity.Add(Vec(.5f)).Mul(Vec(1.f/1.5f));
							lightIntensity = lightIntensity.Mul(Vec(.5));
							lightIntensity = lightIntensity.Max(VEC_ZERO);
							diffuseX += lightIntensity * lightColor[i].ReplicateX();
							diffuseY += lightIntensity * lightColor[i].ReplicateY();
							diffuseZ += lightIntensity * lightColor[i].ReplicateZ();
						}
						// mix material color
						diffuseX *= materialColor.ReplicateX();
						diffuseY *= materialColor.ReplicateY();
						diffuseZ *= materialColor.ReplicateZ();
						// output
						diffuseX = diffuseX.Saturate();
						diffuseY = diffuseY.Saturate();
						diffuseZ = diffuseZ.Saturate();
						for (int i = 0; i < 4; ++i)
						{
							if (hit & (1 << i))
							{
								Vec diffuse(diffuseX(i), diffuseY(i), diffuseZ(i));
								if (r == 1)
									diffuse = LinearToGamma(diffuse);
								diffuse = diffuse.Mul(Vec(255.f));
								const int cr = (int)diffuse(0);
								const int cg = (int)diffuse(1);
								const int cb = (int)diffuse(2);
								PUTPIX(x * 4 + i, y, cr, cg, cb);
							}
						}
					}
				}
			}
		}
		
		time += CFAbsoluteTimeGetCurrent();
		
		printf("trace took %f ms\n", float(time * 1000.0));
	
		SDL_UnlockSurface(surface);
		SDL_Flip(surface);
	}
	
	SDL_Quit();
	
	return 0;	
}
