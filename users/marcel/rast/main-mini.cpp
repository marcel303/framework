#include <assert.h>
#include <emmintrin.h>
#include <exception>
#include <math.h>
#include <SDL/SDL.h>
#include <smmintrin.h>
#include <vector>
#include <xmmintrin.h>
#include "../../../libgg/SIMD.h"
#include "../../../libgg/SimdMat4x4.h"
#include "md3.h"
#include "util.h"

#define USE_COLOR 0
#define USE_TEXTURE 1
#define USE_SUBPIXEL 1

//

#define SY 1280

//

struct g_Surface
{
	int   sx;
	int   sy;
	int * pixels;
	int   pitch;
	int   shiftR;
	int   shiftG;
	int   shiftB;
} g_Surface;

//

static TextureLevel * g_Texture     = 0;
static DepthBuffer  * g_DepthBuffer = 0;

// struct which holds variables to be interpolated across surface
// note: we don't interpolate x/y in world space because they have no role in calculating the output colour

ALIGN_CLASS(16) VertInfo
{
public:
	union
	{
		struct
		{
			float z;
			float u;
			float v;
			float l; // mip level
		};
		vec128 zuvl;
	};

#if USE_COLOR == 1
	union
	{
		struct
		{
			float r;
			float g;
			float b;
		};
		vec128 rgb0;
	};
#endif
};

// vertex

ALIGN_CLASS(16) Vert
{
public:
	inline Vert()
	{
	}

	Vert(float x, float y, float z, float u, float v, float r, float g, float b)
	{
		this->x = x;
		this->y = y;
		info.z = z;
		info.u = u;
		info.v = v;
	#if USE_COLOR == 1
		info.r = r;
		info.g = g;
		info.b = b;
	#endif
	}

	float x;
	float y;
	VertInfo info;
};

// struct which holds a single scan line vertex

ALIGN_CLASS(16) ScanVert
{
public:
	VertInfo info;
};

// list of scan lines, 2 for each y, holding state at x1 and x2

typedef int ScanStateValue;

static ScanStateValue g_ScanState[SY];
static float          g_ScanExtentsX[SY][2];
static ScanVert       g_ScanVerts[SY][2];

// sample texture

static FORCEINLINE u32 Sample128_U32(vec128 nuvn) FORCEINLINE_BACK;

static FORCEINLINE u32 Sample128_U32(vec128 nuvn)
{
	const TextureLevel * __restrict texture = g_Texture;

	vec128  nuvnF = _mm_mul_ps(nuvn, texture->m_SizeVec);
	__m128i nuvnI = _mm_cvttps_epi32(nuvnF);

	const TexCoordValue tu = _MM_ACCESS_I32(nuvnI, 1) & texture->m_SizeMask;
	const TexCoordValue tv = _MM_ACCESS_I32(nuvnI, 2) & texture->m_SizeMask;

	return texture->GetPixel_U32(tu, tv);
}

static FORCEINLINE void Eval(
	  int * __restrict dst
	, __m128 zRcp
	, __m128 zuvlRcp
	)
{
#if 0
	vec128 zuvl = _mm_mul_ps(zuvlRcp, zRcp);
	*dst = Sample128_U32(zuvl);
#else
	__m128 zRcp255 = _mm_mul_ps(zRcp, kVec255);
	__m128 rgb0    = _mm_mul_ps(zuvlRcp, zRcp255);
	__m128i v32    = _mm_cvttps_epi32(rgb0);
	__m128i v16    = _mm_packus_epi32(v32, v32);
	__m128i v08    = _mm_packus_epi16(v16, v16);
//	*dst = v08.m128i_u32[0];
	*dst = ((int*)&v08)[0];
#endif
}

static FORCEINLINE void Eval4(
	  int * __restrict dst
	, const __m128 & zRcp
	, const __m128 & zuvl0Rcp
	, const __m128 & zuvl1Rcp
	, const __m128 & zuvl2Rcp
	, const __m128 & zuvl3Rcp
	)
{
	__m128 zRcp255   = _mm_mul_ps(zRcp, kVec255);
	__m128 rgb0      = _mm_mul_ps(zuvl0Rcp, _mm_shuffle_ps(zRcp255, zRcp255, _MM_SHUFFLE(0,0,0,0)));
	__m128 rgb1      = _mm_mul_ps(zuvl1Rcp, _mm_shuffle_ps(zRcp255, zRcp255, _MM_SHUFFLE(1,1,1,1)));
	__m128 rgb2      = _mm_mul_ps(zuvl2Rcp, _mm_shuffle_ps(zRcp255, zRcp255, _MM_SHUFFLE(2,2,2,2)));
	__m128 rgb3      = _mm_mul_ps(zuvl3Rcp, _mm_shuffle_ps(zRcp255, zRcp255, _MM_SHUFFLE(3,3,3,3)));
	__m128i v0_32    = _mm_cvttps_epi32(rgb0);
	__m128i v1_32    = _mm_cvttps_epi32(rgb1);
	__m128i v2_32    = _mm_cvttps_epi32(rgb2);
	__m128i v3_32    = _mm_cvttps_epi32(rgb3);
	__m128i v01_16   = _mm_packus_epi32(v0_32, v1_32);
	__m128i v23_16   = _mm_packus_epi32(v2_32, v3_32);
	__m128i v0123_08 = _mm_packus_epi16(v01_16, v23_16);
	_mm_storeu_si128((__m128i*)dst, v0123_08);
}

// draw a single horizontal span from x1 to x2

static void Fill(
	__m128 zuvl_scan,
	__m128 zuvl_dx,
#if USE_COLOR
	__m128 rgb0_scan,
	__m128 rgb0_dx,
#endif
	int   * __restrict color_scan,
	float * __restrict depth_scan,
	u32 sx)
{
	PREFETCH(color_scan);
	PREFETCH(depth_scan);

	const int shiftR = g_Surface.shiftR;
	const int shiftG = g_Surface.shiftG;
	const int shiftB = g_Surface.shiftB;

	u32 sx4 = sx >> 2;
	//u32 sx4 = 0;
	u32 sx1 = sx - (sx4 << 2);

	if (sx4 != 0)
	{
		__m128 zuvl1 = zuvl_scan;
		__m128 zuvl2 = _mm_add_ps(zuvl1, zuvl_dx);
		__m128 zuvl3 = _mm_add_ps(zuvl2, zuvl_dx);
		__m128 zuvl4 = _mm_add_ps(zuvl3, zuvl_dx);

		__m128 zuvl_dx4 = _mm_mul_ps(zuvl_dx, kVec4);

	#if 1
		__m128 t1 = _mm_unpacklo_ps(zuvl1, zuvl3); // | z1 | z3 | .. | .. |
		__m128 t2 = _mm_unpacklo_ps(zuvl2, zuvl4); // | z2 | z4 | .. | .. |
		__m128 z4 = _mm_unpacklo_ps(t1, t2);       // | z1 | z2 | z3 | z4 |
		ASSERT(_MM_ACCESS(z4, 0) == _MM_ACCESS(zuvl1, 0));
		ASSERT(_MM_ACCESS(z4, 1) == _MM_ACCESS(zuvl2, 0));
		ASSERT(_MM_ACCESS(z4, 2) == _MM_ACCESS(zuvl3, 0));
		ASSERT(_MM_ACCESS(z4, 3) == _MM_ACCESS(zuvl4, 0));
	#else
		__m128 z4;
		z4.m128_f32[0] = zuvl1.m128_f32[0];
		z4.m128_f32[1] = zuvl2.m128_f32[0];
		z4.m128_f32[2] = zuvl3.m128_f32[0];
		z4.m128_f32[3] = zuvl4.m128_f32[0];
	#endif

		__m128 z4_dx = _mm_mul_ps(_mm_shuffle_ps(zuvl_dx, zuvl_dx, _MM_SHUFFLE(0,0,0,0)), kVec4);

		while (sx4 != 0)
		{
			vec128 depth = _mm_loadu_ps(depth_scan);
			vec128 zPass = _mm_cmplt_ps(depth, z4);

			// flip bits where a != b and mask
			vec128 zWrite = _mm_xor_ps(_mm_and_ps(zPass, _mm_xor_ps(depth, z4)), depth);
			_mm_storeu_ps(depth_scan, zWrite);

			int zMask = _mm_movemask_ps(zPass);

			if (zMask != 0)
			{
				vec128 z4Rcp = _mm_rcp_ps(z4);

				if (zMask == 15)
				{
				#if 0
					Eval(color_scan + 0, zuvl1, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(0,0,0,0)));
					Eval(color_scan + 1, zuvl2, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(1,1,1,1)));
					Eval(color_scan + 2, zuvl3, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(2,2,2,2)));
					Eval(color_scan + 3, zuvl4, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(3,3,3,3)));
				#else
					Eval4(color_scan, z4Rcp, zuvl1, zuvl2, zuvl3, zuvl4);
				#endif
				}
				else
				{
					if (zMask & 1)
						Eval(color_scan + 0, zuvl1, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(0,0,0,0)));
					if (zMask & 2)
						Eval(color_scan + 1, zuvl2, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(1,1,1,1)));
					if (zMask & 4)
						Eval(color_scan + 2, zuvl3, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(2,2,2,2)));
					if (zMask & 8)
						Eval(color_scan + 3, zuvl4, _mm_shuffle_ps(z4Rcp, z4Rcp, _MM_SHUFFLE(3,3,3,3)));
				}
			}

			sx4--;

			z4    = _mm_add_ps(z4, z4_dx);
			zuvl1 = _mm_add_ps(zuvl1, zuvl_dx4);
			zuvl2 = _mm_add_ps(zuvl2, zuvl_dx4);
			zuvl3 = _mm_add_ps(zuvl3, zuvl_dx4);
			zuvl4 = _mm_add_ps(zuvl4, zuvl_dx4);
			color_scan += 4;
			depth_scan += 4;
		}

		zuvl_scan = zuvl1;
	}

	while (sx1 != 0)
	{
		vec128 depth = _mm_load_ss(depth_scan);
		vec128 zPass = _mm_cmplt_ss(depth, zuvl_scan);

		if (_mm_movemask_ps(zPass) & 1)
		{
			_mm_store_ss(depth_scan, zuvl_scan);
			vec128 zRcp = _mm_rcp_ss(zuvl_scan);
			Eval(color_scan + 0, zuvl_scan, _mm_shuffle_ps(zRcp, zRcp, _MM_SHUFFLE(0,0,0,0)));
		}

		zuvl_scan = _mm_add_ps(zuvl_scan, zuvl_dx);

		++depth_scan;
		++color_scan;
		--sx1;
	}
}

static void Fill(ScreenCoordValue y)
{
	ASSERT(y < g_Surface.sy);
	
	float x1f = g_ScanExtentsX[y][0];
	float x2f = g_ScanExtentsX[y][1];
	
	if (x1f == x2f)
		return;

	ScanVert* __restrict v1 = &g_ScanVerts[y][0];
	ScanVert* __restrict v2 = &g_ScanVerts[y][1];
	
	if (x1f > x2f)
	{
		ScanVert * temp = v1;
		v1 = v2;
		v2 = temp;
		
		float temp2 = x1f;
		x1f = x2f;
		x2f = temp2;
	}

	const vec128 dxSS     = _mm_set_ss(x2f - x1f);
	const vec128 dxRcpSS  = _mm_rcp_ss(dxSS);
	const vec128 dxRcpVec = _mm_shuffle_ps(dxRcpSS, dxRcpSS, _MM_SHUFFLE(0,0,0,0));

	vec128 v1_zuvl = v1->info.zuvl;
	vec128 v2_zuvl = v2->info.zuvl;
#if USE_COLOR == 1
	vec128 v1_rgb0 = _mm_mul_ps(v1->info.rgb0, kVec255);
	vec128 v2_rgb0 = _mm_mul_ps(v2->info.rgb0, kVec255);
#endif

	vec128 dzuvl_dx = _mm_mul_ps(_mm_sub_ps(v2_zuvl, v1_zuvl), dxRcpVec);
#if USE_COLOR == 1
	vec128 drgb0_dx = _mm_mul_ps(_mm_sub_ps(v2_rgb0, v1_rgb0), dxRcpVec);
#endif

	vec128 v_zuvl = v1_zuvl;
#if USE_COLOR == 1
	vec128 v_rgb0 = v1_rgb0;
#endif
	
	if (x1f < 0.0f)
	{
		vec128 dx = _mm_set_ps1(x1f);
		v_zuvl = _mm_sub_ps(v_zuvl, _mm_mul_ps(dzuvl_dx, dx));
	#if USE_COLOR == 1
		v_rgb0 = _mm_sub_ps(v_rgb0, _mm_mul_ps(drgb0_dx, dx));
	#endif

		x1f = 0.0f;
	}

	ScreenCoordValue x1 = FTOI(x1f);
	ScreenCoordValue x2 = FTOI(x2f);

	if (x2 > g_Surface.sx)
		x2 = g_Surface.sx;

	if (x2 <= x1)
		return;

	ASSERT(x1 <= 65535);
	ASSERT(x2 <= 65535);

#if USE_SUBPIXEL == 1
	const float xd = x1 - x1f + 1.0f;
	const vec128 xdVec = _mm_set_ps1(xd);

	v_zuvl = _mm_add_ps(v_zuvl, _mm_mul_ps(dzuvl_dx, xdVec));
#if USE_COLOR == 1
	v_rgb0 = _mm_add_ps(v_rgb0, _mm_mul_ps(drgb0_dx, xdVec));
#endif
#endif

	int   * __restrict color = g_Surface.pixels + g_Surface.pitch * y;
	float * __restrict depth = g_DepthBuffer->GetDepth(0, y);

	int   * __restrict color_scan;
	float * __restrict depth_scan;

	//

	vec128 v_zuvl_scan = v_zuvl;
#if USE_COLOR == 1
	vec128 v_rgb0_scan = v_rgb0;
#endif

	color_scan = color + x1;
	depth_scan = depth + x1;

	Fill(
		v_zuvl_scan,
		dzuvl_dx,
	#if USE_COLOR == 1
		v_rgb0_scan,
		drgb0_dx,
	#endif
		color_scan,
		depth_scan,
		x2 - x1);
}

//

#define EXT_UNDEFINED -10000

static int g_ScanExtents[2];

// draw horizontal spans

static FORCEINLINE void Fill(int y1, int y2)
{
	ASSERT(y1 >= 0);
	ASSERT(y2 <= g_Surface.sy);

	if (y1 < 0)
		y1 = 0;
	if (y2 > g_Surface.sy)
		y2 = g_Surface.sy;
	
	for (int y = y1; y < y2; ++y)
	{
		ASSERT(g_ScanState[y] == 2);
		Fill(y);
		g_ScanState[y] = 0;
	}
}

static FORCEINLINE void Fill()
{
	if (g_ScanExtents[0] != EXT_UNDEFINED)
	{
		Fill(g_ScanExtents[0], g_ScanExtents[1]);
	}
}

// fill span structure across line

static FORCEINLINE void ScanBegin()
{
	g_ScanExtents[0] = EXT_UNDEFINED;
	g_ScanExtents[1] = EXT_UNDEFINED;
}

static void Scan(const Vert * __restrict v1, const Vert * __restrict v2)
{
	if (v1->y > v2->y)
		std::swap(v1, v2);

	//int y1 = FTOI_PTR(&v1->y);
	//int y2 = FTOI_PTR(&v2->y);
	int y1 = (int)v1->y;
	int y2 = (int)v2->y;

	if (y1 == y2)
		return;
	if (y1 >= g_Surface.sy || y2 <= 0)
		return;
	
	vec128 v1_zuvl = _mm_set_xyzw_ps(1.0f, v1->info.u, v1->info.v, v1->info.l);
	vec128 v2_zuvl = _mm_set_xyzw_ps(1.0f, v2->info.u, v2->info.v, v2->info.l);

#if USE_COLOR == 1
	vec128 v1_rgb0 = v1->info.rgb0;
	vec128 v2_rgb0 = v2->info.rgb0;
#endif

	//

	const float  dy       = v2->y - v1->y;
	const float  dyRcp    = 1.0f / dy;
	const vec128 dyRcpVec = _mm_set_ps1(dyRcp);

	// move to 1/z space

	const vec128 v1_zRcp    = _mm_rcp_ss(_mm_load_ss(&v1->info.z));
	const vec128 v2_zRcp    = _mm_rcp_ss(_mm_load_ss(&v2->info.z));
	const vec128 v1_zRcpVec = _mm_shuffle_ps(v1_zRcp, v1_zRcp, _MM_SHUFFLE(0,0,0,0));
	const vec128 v2_zRcpVec = _mm_shuffle_ps(v2_zRcp, v2_zRcp, _MM_SHUFFLE(0,0,0,0));

	v1_zuvl = _mm_mul_ps(v1_zuvl, v1_zRcpVec);
	v2_zuvl = _mm_mul_ps(v2_zuvl, v2_zRcpVec);

#if USE_COLOR == 1
	v1_rgb0 = _mm_mul_ps(v1_rgb0, v1_zRcpVec);
	v2_rgb0 = _mm_mul_ps(v2_rgb0, v2_zRcpVec);
#endif

	// calculate line segment deltas

	const float dx_dy = (v2->x - v1->x) * dyRcp;
	vec128 dzuvl_dy = _mm_mul_ps(_mm_sub_ps(v2_zuvl, v1_zuvl), dyRcpVec);
#if USE_COLOR == 1
	vec128 drgb0_dy = _mm_mul_ps(_mm_sub_ps(v2_rgb0, v1_rgb0), dyRcpVec);
#endif

	// trace line from y1 to y2

	float x = v1->x;

	vec128 v_zuvl = v1_zuvl;
#if USE_COLOR == 1
	vec128 v_rgb0 = v1_rgb0;
#endif

#if USE_SUBPIXEL == 1
	const float yd = y1 - v1->y + 1.0f;
	vec128 ydVec = _mm_set_ps1(yd);

	x += dx_dy * yd;

	v_zuvl = _mm_add_ps(v_zuvl, _mm_mul_ps(dzuvl_dy, ydVec));
#if USE_COLOR == 1
	v_rgb0 = _mm_add_ps(v_rgb0, _mm_mul_ps(drgb0_dy, ydVec));
#endif
#endif

	if (y1 < 0)
	{
		vec128 ydVec = _mm_set_ps1(float(y1));
		x -= dx_dy * y1;
		v_zuvl = _mm_sub_ps(v_zuvl, _mm_mul_ps(dzuvl_dy, ydVec));
	#if USE_COLOR == 1
		v_rgb0 = _mm_sub_ps(v_rgb0, _mm_mul_ps(drgb0_dy, ydVec));
	#endif
		y1 = 0;
	}
	if (y2 > g_Surface.sy)
		y2 = g_Surface.sy;

	ASSERT(y1 < y2);

	if (y1 < g_ScanExtents[0] || g_ScanExtents[0] == EXT_UNDEFINED)
		g_ScanExtents[0] = y1;
	if (y2 > g_ScanExtents[1] || g_ScanExtents[1] == EXT_UNDEFINED)
		g_ScanExtents[1] = y2;

	for (int y = y1; y < y2; ++y)
	{
		// skip lines above or below the destination bitmap

		ScanStateValue state = g_ScanState[y];
		
		ASSERT(state <= 1);

		g_ScanExtentsX[y][state] = x;
		
		ScanVert * __restrict sv = g_ScanVerts[y] + state;
		
		sv->info.zuvl = v_zuvl;
	#if USE_COLOR == 1
		sv->info.rgb0 = v_rgb0;
	#endif
	
		g_ScanState[y] = state + 1;

		// advance

		x += dx_dy;
		v_zuvl = _mm_add_ps(v_zuvl, dzuvl_dy);
#if USE_COLOR == 1
		v_rgb0 = _mm_add_ps(v_rgb0, drgb0_dy);
#endif
	}
}

// fill span structure for triangle

static FORCEINLINE void Scan(const Vert * __restrict v)
{
	Scan(v + 0, v + 1);
	Scan(v + 1, v + 2);
	Scan(v + 2, v + 0);
}

static FORCEINLINE void Scan(const Vert * __restrict v, u32 n)
{
	for (u32 prev = n - 1, curr = 0; curr < n; prev = curr++)
	{
		Scan(v + prev, v + curr);
	}
}

#if 1
static SimdVec screenSize(240.0f, 240.0f);
static SimdVec mid;

static FORCEINLINE void WriteVert(Vert * __restrict rv, const DataVert * __restrict dv, SimdVecArg v, SimdVecArg p) FORCEINLINE_BACK;

static FORCEINLINE void WriteVert(Vert * __restrict rv, const DataVert * __restrict dv, SimdVecArg v, SimdVecArg p)
{
	ASSERT(v.Z() > 0.0f);
#if 0
	SimdVec p2 = p.Mul(screenSize).Add(mid);
	rv->x = p2.X();
	rv->y = p2.Y();
#elif 0
	//rv->x = p.X() * screenSize.X() + mid.X();
	//rv->y = p.Y() * screenSize.Y() + mid.Y();
	rv->x = v.X() / v.Z() * screenSize.X() + mid.X();
	rv->y = v.Y() / v.Z() * screenSize.Y() + mid.Y();
#else
	rv->x = v.X() / v.Z() * 4.0f * screenSize.X() + mid.X();
	rv->y = v.Y() / v.Z() * 4.0f * screenSize.Y() + mid.Y();
	//rv->x = p.X() * 4.0f * screenSize.X() + mid.X();
	//rv->y = p.Y() * 4.0f * screenSize.Y() + mid.Y();
#endif
	rv->info.z = v.Z();

#if USE_TEXTURE
	rv->info.u = dv->uv[0];
	rv->info.v = dv->uv[1];
#endif

#if USE_COLOR == 1
#if 1
	rv->info.r = 1.0f;
	rv->info.g = 1.0f;
	rv->info.b = 1.0f;
#else
	SimdVec n = v.UnitVec3() ;
	rv->info.r = (n.X() + 1.0f) * 0.5f;
	rv->info.g = (n.Y() + 1.0f) * 0.5f;
	rv->info.b = (n.Z() + 1.0f) * 0.5f;
#endif
#endif
}

static void RenderMd3(const SimdMat4x4 & mat, const Md3 & md3)
{
	mid = SimdVec(g_Surface.sx / 2.0f, g_Surface.sy / 2.0f, 0.0f, 0.0f);

#if 1
	SimdMat4x4 temp = mat.CalcTranspose();
#endif

	Vert tri[3];

	for (unsigned int i = 0; i < md3.SurfCount; ++i)
	{
		const DataSurf * __restrict s = &md3.SurfList[i];
		const DataVert * __restrict dv = s->VertList;

		const unsigned int * __restrict indexPtr = s->IndexList;

		for (unsigned int j = s->IndexCount / 3; j != 0; --j, indexPtr += 3)
		{
			const unsigned int index1 = indexPtr[0];
			const unsigned int index2 = indexPtr[1];
			const unsigned int index3 = indexPtr[2];
			
			const DataVert * __restrict dv1 = dv + index1;
			const DataVert * __restrict dv2 = dv + index2;
			const DataVert * __restrict dv3 = dv + index3;
			
			SimdVec v1 = mat.Mul4x4(dv1->p);
			SimdVec v2 = mat.Mul4x4(dv2->p);
			SimdVec v3 = mat.Mul4x4(dv3->p);

			//printf("%g, %g, %g\n", v1(0), v1(1), v1(2));
	
			SimdVec p1 = v1.Div(v1.ReplicateZ());
			SimdVec p2 = v2.Div(v2.ReplicateZ());
			SimdVec p3 = v3.Div(v3.ReplicateZ());

#if 1
			SimdVec d1 = p2.Sub(p1);
			SimdVec d2 = p3.Sub(p2);
			SimdVec dc = d1.Cross3(d2);
			
#if 1
			SimdVec zzzz = dc.ReplicateZ();
			if (zzzz.ANY_LE4(VEC_ZERO))
				continue;
#else
			if (dc.Z() <= 0.0f)
				continue;
#endif
#endif

			WriteVert(tri + 0, dv1, v1, p1);
			WriteVert(tri + 1, dv2, v2, p2);
			WriteVert(tri + 2, dv3, v3, p3);
			
			ScanBegin();
			Scan(tri);
			Fill();
		}
	}
}
#endif

class Damper
{
public:
	Damper(double falloffPerSec)
		: mValue(0.0)
		, mDesiredValue(0.0)
		, mFalloff(falloffPerSec)
	{
	}

	void Update(double dt)
	{
		const double t1 = pow(mFalloff, dt);
		const double t2 = 1.0 - t1;
		mValue = mDesiredValue * t2 + mValue * t1;
	}

	float GetDesiredValue() const
	{
		return static_cast<float>(mDesiredValue);
	}

	void SetDesiredValue(float value, bool force = false)
	{
		mDesiredValue = value;
		if (force)
			mValue = mDesiredValue;
	}

	float GetValue() const
	{
		return static_cast<float>(mValue);
	}

private:
	double mValue;
	double mDesiredValue;
	double mFalloff;
};

static void Execute()
{
#if 1
	SimdVec plane1;
	SimdVec plane2a;
	SimdVec plane2b;
	SimdVec plane3;

	SimdMat4x4 m1;
	SimdMat4x4 m1i;
	SimdMat4x4 m2;

	SimdVec norm1;
	SimdVec norm3;

#define RF(x) ((((rand() % 1000) / 999.0f) - 0.5f) * x)

	for (int i = 0; i < 100; ++i)
	{
		m1.MakeLookat(
			SimdVec(RF(10.0f), RF(10.0f), RF(10.0f)),
			SimdVec(RF(10.0f), RF(10.0f), RF(10.0f)),
			SimdVec(0.0f, 1.0f, 0.0f));
		m1i = m1.CalcInv();
		m2 = m1.CalcInv().CalcTranspose();

		SimdVec norm1 = SimdVec(RF(1.0f), RF(1.0f), RF(1.0f), 0.0f).UnitVec3();
		plane1 = SimdVec(norm1(0), norm1(1), norm1(2), RF(10.0f));

		plane2a = m1.Mul4x4(plane1);
		plane2b = m2.Mul4x4(plane1);

		norm3 = m1.Mul3x3(norm1);
		SimdVec p3a = plane1.Mul(plane1.ReplicateW().Neg());
		p3a(3) = 1.0f;
		float d = -norm1.Dot3(p3a)(0);
		SimdVec p3b = m1.Mul4x4(p3a);
		SimdVec d3 = p3b.Dot3(norm3);
		plane3 = norm3;
		plane3(3) = -d3(0);

		printf("plane1: %g, %g, %g, %g\n", plane1(0), plane1(1), plane1(2), plane1(3));
		printf("plane2: %g, %g, %g, %g\n", plane2b(0), plane2b(1), plane2b(2), plane2b(3));
		printf("plane3: %g, %g, %g, %g\n", plane3(0), plane3(1), plane3(2), plane3(3));
	}
#endif

#if defined(WINDOWS)
	HANDLE process = GetCurrentProcess();
	SetPriorityClass(process, HIGH_PRIORITY_CLASS);
	HANDLE thread = GetCurrentThread();
	if (!SetThreadPriority(thread, THREAD_PRIORITY_HIGHEST))
		throw_exception("failed to set thread priority");
	if (!SetThreadPriorityBoost(thread, FALSE))
		throw_exception("failed to set thread priority boost");
	//if (!SetThreadAffinityMask(thread, 2))
	//	throw_exception("failed to set thread affinity");
#endif

	if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO) < 0)
		throw_exception("failed to initialize SDL");

	SDL_Surface * surface = SDL_SetVideoMode(640, 480, 32, 0);
	//SDL_Surface * surface = SDL_SetVideoMode(1024, 768, 32, 0);
	//SDL_Surface * surface = SDL_SetVideoMode(320, 240, 32, 0);
	//SDL_Surface * surface = SDL_SetVideoMode(1920, 1080, 32, SDL_FULLSCREEN);
	//SDL_Surface * surface = SDL_SetVideoMode(1280, 1024, 32, SDL_FULLSCREEN);

	if (surface == 0)
		throw_exception("failed to set video mode");

	g_Surface.sx = surface->w;
	g_Surface.sy = surface->h;
	g_Surface.shiftR = surface->format->Rshift;
	g_Surface.shiftG = surface->format->Gshift;
	g_Surface.shiftB = surface->format->Bshift;

	g_DepthBuffer = new DepthBuffer();
	g_DepthBuffer->Initialize(g_Surface.sx, g_Surface.sy);

	TextureLevel * texture1 = new TextureLevel();
	TextureLevel * texture2 = new TextureLevel();
	texture1->Load("texture.bmp", g_Surface.shiftR, g_Surface.shiftG, g_Surface.shiftB);
	//texture2->Load("shotgun.bmp", g_Surface.shiftR, g_Surface.shiftG, g_Surface.shiftB);
	texture2->Load("briareos.bmp", g_Surface.shiftR, g_Surface.shiftG, g_Surface.shiftB);
	g_Texture = texture2;

	Md3 md3;

	md3.Load("shotgun.md3");
	md3.Load("briareos-upper.md3");
	u32 surfCount = md3.SurfCount;
	u32 vertexCount = 0;
	u32 indexCount = 0;
	for (u32 i = 0; i < surfCount; ++i)
	{
		vertexCount += md3.SurfList[i].VertCount;
		indexCount += md3.SurfList[i].IndexCount;
	}
	printf("MD3: %d surfaces, %d total vertices, %d total indices\n", surfCount, vertexCount, indexCount);

	Vert backQuad[4] =
	{
		Vert(-1.0f, -1.0f, 100, 0.0f, 0.0f, 1.0f, 0.5f, 0.5f),
		Vert(+1.0f, -1.0f, 100, 1.0f, 0.0f, 0.5f, 1.0f, 0.5f),
		Vert(+1.0f, +1.0f, 100, 1.0f, 1.0f, 0.5f, 0.5f, 1.0f),
		Vert(-1.0f, +1.0f, 100, 0.0f, 1.0f, 0.5f, 0.5f, 1.0f),
	};

	Vert quad[4];

	Damper mrx(0.00001f);
	Damper mry(0.00001f);

	u32 frame = 0;

	bool stop = false;
	bool clear = true;
	bool flat = false;
	bool back = false;

	while (stop == false)
	{
#if 1
		if ((frame % 100) == 0)
		{
			Timer etime;
			
			etime.Start();

			SDL_Event e;

			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_q)
						stop = true;
					else if (e.key.keysym.sym == SDLK_LSHIFT)
						g_Texture = g_Texture == texture1 ? texture2 : texture1;
					else if (e.key.keysym.sym == SDLK_SPACE)
						flat = true;
					else if (e.key.keysym.sym == SDLK_b)
						back = true;
					else if (e.key.keysym.sym == SDLK_c)
						clear = false;
				}
				else if (e.type == SDL_KEYUP)
				{
					if (e.key.keysym.sym == SDLK_SPACE)
						flat = false;
					else if (e.key.keysym.sym == SDLK_b)
						back = false;
					else if (e.key.keysym.sym == SDLK_c)
						clear = true;
				}
				else if (e.type == SDL_MOUSEMOTION)
				{
					mrx.SetDesiredValue(mrx.GetDesiredValue() + e.motion.xrel / 20.0f);
					mry.SetDesiredValue(mry.GetDesiredValue() + e.motion.yrel / 20.0f);
				}
				else if (e.type == SDL_QUIT)
				{
					stop = true;
				}
			}

			etime.Stop();

			printf("etime: %gms\n" , etime.usec() / 1000.0f);
		}
#endif

		const float dt = 1.0f / 60.0f;

		mrx.Update(dt);
		mry.Update(dt);

		if (SDL_LockSurface(surface) < 0)
			throw_exception("failed to lock surface");

		g_Surface.pixels = reinterpret_cast<int*>(surface->pixels);
		g_Surface.pitch = surface->pitch >> 2;
		
		if (clear)
		{
			for (int i = 0; i < g_Surface.sy; ++i)
			{
				int * color = g_Surface.pixels + g_Surface.pitch * i;
				memset(color, 0, sizeof(u32) * g_Surface.sx);
			}
		}

		g_DepthBuffer->ClearZero();

		SimdMat4x4 backR;
		//backR.MakeRotationZ(frame / 1000.0f);
		backR.MakeIdentity();

		for (u32 i = 0; i < 4; ++i)
		{
			SimdVec vert(backQuad[i].x, backQuad[i].y, backQuad[i].info.z);
			vert = backR.Mul4x4(vert);

			const float s = g_Surface.sx / 2.0f;

			quad[i] = backQuad[i];
			quad[i].x = vert.X();
			quad[i].y = vert.Y();
			quad[i].info.z = 1000.0f;

			quad[i].x *= s;
			quad[i].y *= s;

			quad[i].x += g_Surface.sx / 2.0f;
			quad[i].y += g_Surface.sy / 2.0f;
		}

#if 1
		if (back)
		{
			ScanBegin();
			Scan(quad, 4);
			Fill();
		}
#endif

		//const u32 tn = 1;
		//const u32 tn = 5;
		//const u32 tn = 3;
		const u32 tn = 0;
		const u32 qn = 3;

		for (u32 t = 0; t < tn; ++t)
		{
			for (u32 i = 0; i < qn; ++i)
			{
				const float r1 = t * 2.0f * (float)M_PI / (tn + 1e-10f) + frame / 100.0f;
				const float r = t - i * 2.0f * (float)M_PI / qn + frame * ((t + 1) / (float)tn) / 10.0f;

				//const float size1 = 300.0f;
				const float size1 = 256.0f;
				//const float size = 512.0f;
				//const float size = 256.0f;
				//const float size = 128.0f;
				const float size = 64.0f;

				const float x = sinf(r1) * size1;
				const float y = cosf(r1) * size1;

				quad[i].x = x + sinf(r) * size;
				quad[i].y = y + cosf(r) * size;

				if (flat == false)
					quad[i].info.z = i == 0 ? 2.0f : 1.0f;
				else
					quad[i].info.z = 1.0f;

				quad[i].x /= quad[i].info.z;
				quad[i].y /= quad[i].info.z;

				quad[i].x += g_Surface.sx / 2.0f;
				quad[i].y += g_Surface.sy / 2.0f;

#if USE_COLOR == 1
				quad[i].info.r = (cosf(r * 10.0f) + 1.0f) / 2.0f;
				quad[i].info.g = (cosf(r * 7.5f) + 1.0f) / 2.0f;
				quad[i].info.b = (cosf(r * 5.5f) + 1.0f) / 2.0f;
#endif
			}

			ScanBegin();
			Scan(quad, qn);
			Fill();
		}
		
		Timer time;
		
		time.Start();

#if 1
		{
		//const int mn = 1;
		//const int mn = 25;
		const int mn = 49;
		//const int mn = 9;
		static float rx[mn];
		static float ry[mn];
		static float rz[mn];
		for (int i = 0; i < mn; ++i)
		{
			SimdMat4x4 matV;
			SimdMat4x4 matR;
			SimdMat4x4 matT;
			SimdMat4x4 matS;
			float speed = i / 10.0f;
#if 1
			rx[i] += 0.01f * speed;
			ry[i] += 0.02f * speed;
			rz[i] += 0.03f * speed;
#else
			rx[i] = i * 100.0f;
			ry[i] = i * 100.0f;
			rz[i] = i * 100.0f;
#endif
			SimdMat4x4 matRx;
			SimdMat4x4 matRy;
			SimdMat4x4 matRz;
			matRx.MakeRotationX(mrx.GetValue() + rx[i]);
			matRy.MakeRotationY(                 ry[i]);
			matRz.MakeRotationZ(mry.GetValue() + rz[i]);
			matR = matRz * matRy * matRx;
			int is = FTOI(sqrtf((float)mn));
			int ix = i / is;
			int iy = i - ix * is;
			ix -= (is - 1) / 2;
			iy -= (is - 1) / 2;
			//matT.MakeTranslation(ix * 5.0f, iy * 5.0f, 0.0f);
			//matT.MakeTranslation(ix * 2.0f, iy * 2.0f, 10.0f);
			matT.MakeTranslation(ix * 0.7f, iy * 0.7f, 10.0f);
			//float scale = 4.0f;
			//float scale = 2.0f;
			//float scale = 1.0f;
			//float scale = 0.8f;
			//float scale = 0.25f;
			//float scale = 0.1f;
			//float scale = 0.05f;
			//float scale = 0.03f;
			float scale = 0.02f;
			matS.MakeScaling3f(scale, scale, scale);
			matV.MakeIdentity();
			SimdMat4x4 mat = matV * matT * matR * matS;

			RenderMd3(mat, md3);
		}
		}
#endif

		time.Stop();

#if 1
		//if (frame % 100 == 0)
		{
			//printf("time: %gms\n", time / 100.0f);
			printf("time: %gms\n", time.usec() / 1000.0f);
			//time = 0;
		}
#endif

		SDL_UnlockSurface(surface);

		if (SDL_Flip(surface) < 0)
			throw_exception("failed to flip back buffer");

		++frame;
	}

	g_Texture = 0;

	delete texture1;
	texture1 = 0;
	delete texture2;
	texture2 = 0;

	delete g_DepthBuffer;
	g_DepthBuffer = 0;

	SDL_FreeSurface(surface);

	SDL_Quit();
}

int main(int argc, char * argv[])
{
	try
	{
		Execute();

		return 0;
	}
	catch (std::exception & e)
	{
		printf("error: %s\n", e.what());
		return -1;
	}
}
