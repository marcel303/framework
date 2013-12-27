#include <algorithm>
#include <cmath>
#include <CoreFoundation/CoreFoundation.h>
#include <SDL/SDL.h>
#include "../../../libgg/SIMD.h"

typedef SimdVec Vec;
typedef SimdVecArg VecArg;

#define SHOW_NONLINEAR 1

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

#if 1
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
#else
inline float GammaToLinear(float v)
{
	return v * v;
}

inline float LinearToGamma(float v)
{
	return std::sqrt(v);
}

inline Vec GammaToLinear(VecArg v)
{
	return v * v;
}

inline Vec LinearToGamma(VecArg v)
{
	return v.Sqrt();
}
#endif

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
	Vec & out_t,
	Vec & out_normalX,
	Vec & out_normalY,
	Vec & out_normalZ)
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

	const Vec hasSolution = a.CMP_GT(VEC_ZERO).AND(bb4ac.CMP_GE(VEC_ZERO));

	if (hasSolution.BITTEST())
	{
		const Vec bb4acSqrt = bb4ac.Sqrt();
		const Vec t1 = (-b + bb4acSqrt) / (2.f * a);
		const Vec t2 = (-b - bb4acSqrt) / (2.f * a);
		const Vec t = t1.Min(t2).Max(VEC_ZERO);
		
		const Vec positionX = rayOriginX + rayDirectionX * t;
		const Vec positionY = rayOriginY + rayDirectionY * t;
		const Vec positionZ = rayOriginZ + rayDirectionZ * t;

		const Vec normalX = positionX - sphereEq.ReplicateX();
		const Vec normalY = positionY - sphereEq.ReplicateY();
		const Vec normalZ = positionZ - sphereEq.ReplicateZ();
		
		const Vec mask = hasSolution.AND(t.CMP_LT(out_t));
		
		out_t = out_t.Select(mask, out_t, t);
		out_normalX = out_normalX.Select(mask, out_normalX, normalX);
		out_normalY = out_normalY.Select(mask, out_normalY, normalY);
		out_normalZ = out_normalZ.Select(mask, out_normalZ, normalZ);
		
		return mask.BITTEST();
	}
	else
	{
		return 0;
	}
}

int RayPlane(
	VecArg rayOriginX,
	VecArg rayOriginY,
	VecArg rayOriginZ,
	VecArg rayDirectionX,
	VecArg rayDirectionY,
	VecArg rayDirectionZ,
	VecArg planeEq,
	Vec & out_t,
	Vec & out_normalX,
	Vec & out_normalY,
	Vec & out_normalZ)
{
	Vec d =
		rayOriginX.Mul(planeEq.ReplicateX()).Add(
		rayOriginY.Mul(planeEq.ReplicateY()).Add(
		rayOriginZ.Mul(planeEq.ReplicateZ()).Add(
		planeEq.ReplicateW())));
		
	Vec dd =
		rayDirectionX.Mul(planeEq.ReplicateX()).Add(
		rayDirectionY.Mul(planeEq.ReplicateY()).Add(
		rayDirectionZ.Mul(planeEq.ReplicateZ()).Add(
		planeEq.ReplicateW())));

	// d + dd * t = 0 => t = (0 - d) / dd
	
	const Vec t = d.Neg().Div(dd);
	
	const Vec hasSolution = t.CMP_GE(VEC_ZERO);
		
	if (hasSolution.BITTEST())
	{	
		const Vec normalX = planeEq.ReplicateX();
		const Vec normalY = planeEq.ReplicateY();
		const Vec normalZ = planeEq.ReplicateZ();
		
		const Vec mask = hasSolution.AND(t.CMP_LT(out_t));
		
		out_t = out_t.Select(mask, out_t, t);
		out_normalX = out_normalX.Select(mask, out_normalX, normalX);
		out_normalY = out_normalY.Select(mask, out_normalY, normalY);
		out_normalZ = out_normalZ.Select(mask, out_normalZ, normalZ);
		
		return mask.BITTEST();
	}
	else
	{
		return 0;
	}
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
		
		const int numLights = 3;
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
		
		const Vec sphereEq[2] =
			{
				Vec(-.6f, 0.f, 1.f, .5f),
				Vec(+.6f, 0.f, 1.f, .5f)
			};
					
		{
			const Vec materialColor = GammaToLinear(materialColorBase);
			Vec lightColor[numLights];
			for (int i = 0; i < numLights; ++i)
				lightColor[i] = GammaToLinear(lightColorBase[i]);
			
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
					Vec t(1000000.f);
					Vec normalX;
					Vec normalY;
					Vec normalZ;
					int hit = 0;
				#if SHOW_NONLINEAR
					int nonLinear = 0;
				#endif
					
					for (int r = 0; r < 2; ++r)
					{
						const int temp = RaySphere(
							rayOriginX,
							rayOriginY,
							rayOriginZ,
							rayDirectionX,
							rayDirectionY,
							rayDirectionZ,
							sphereEq[r],
							t,
							normalX,
							normalY,
							normalZ);
						
					#if SHOW_NONLINEAR
						if (r == 0)
							nonLinear = temp;
					#endif
						
						hit |= temp;
					}

				#if 1
					if (y >= s)
					{
						const Vec planeEq(0.f, 0.f, -1.f, .5f);
						
						hit |= RayPlane(
							rayOriginX,
							rayOriginY,
							rayOriginZ,
							rayDirectionX,
							rayDirectionY,
							rayDirectionZ,
							planeEq,
							t,
							normalX,
							normalY,
							normalZ);
					}
				#endif
				
					if (hit)
					{
						// lighting
						Normalize3(normalX, normalY, normalZ);
						Vec diffuseX = VEC_ZERO;
						Vec diffuseY = VEC_ZERO;
						Vec diffuseZ = VEC_ZERO;
					#if 1
						for (int i = 0; i < numLights; ++i)
						{
							Vec lightIntensity =
								lightDirection[i].ReplicateX() * normalX +
								lightDirection[i].ReplicateY() * normalY +
								lightDirection[i].ReplicateZ() * normalZ;
							if (i == 0 && numLights != 1)
								lightIntensity += Vec(.2f);
							//lightIntensity = lightIntensity.Add(Vec(.2f)).Mul(Vec(1.f/1.2f)); // rim lighting
							lightIntensity = lightIntensity.Mul(Vec(.5));
							lightIntensity = lightIntensity.Max(VEC_ZERO);
							
						#if SHOW_NONLINEAR
							const Vec lightColorTemp = nonLinear ? LinearToGamma(lightColor[i]) : lightColor[i];
						#else
							const Vec & lightColorTemp = lightColor[i];
						#endif
							diffuseX += lightIntensity * lightColorTemp.ReplicateX();
							diffuseY += lightIntensity * lightColorTemp.ReplicateY();
							diffuseZ += lightIntensity * lightColorTemp.ReplicateZ();
						}
						// mix material color
					#if SHOW_NONLINEAR
						const Vec materialColorTemp = nonLinear ? LinearToGamma(materialColor) : materialColor;
					#else
						const Vec & materialColorTemp = materialColor;
					#endif
						diffuseX *= materialColorTemp.ReplicateX();
						diffuseY *= materialColorTemp.ReplicateY();
						diffuseZ *= materialColorTemp.ReplicateZ();
					#else
						diffuseX = normalX;
						diffuseY = normalY;
						diffuseZ = normalZ;
					#endif
						// output
						diffuseX = diffuseX.Saturate();
						diffuseY = diffuseY.Saturate();
						diffuseZ = diffuseZ.Saturate();
						for (int i = 0; i < 4; ++i)
						{
							if (hit & (1 << i))
							{
								Vec diffuse(diffuseX(i), diffuseY(i), diffuseZ(i));
							#if SHOW_NONLINEAR
								if (!nonLinear)
							#endif
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
