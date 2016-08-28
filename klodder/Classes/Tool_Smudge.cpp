#ifdef __ARM_NEON__
	#include <arm_neon.h>
#endif
#include <string.h>
#include "Bitmap.h"
#include "Calc.h"
#include "Filter.h"
#include "Tool_Smudge.h"
#include "Types.h"

#ifdef WIN32
#define MAX_SCALE 4
#endif
#ifdef MACOS
#define MAX_SCALE 4
#endif
#ifdef IPHONEOS
#define MAX_SCALE 1
#endif

const static uint32_t TEMPBUF_SX = MAX_BRUSH_RADIUS * MAX_SCALE * 2 + 1;
const static uint32_t TEMPBUF_SY = MAX_BRUSH_RADIUS * MAX_SCALE * 2 + 1;

static Rgba gTempBuf[TEMPBUF_SY][TEMPBUF_SX]; // a respectable 4 meg on the iPad with 256x2 x 256x2 x 16..

Tool_Smudge::Tool_Smudge()
{
	Setup(1.0f);
}

void Tool_Smudge::Setup(float strength)
{
	mStrength = strength;
}

void Tool_Smudge::Apply(Bitmap* bmp, const Filter* filter, float _x, float _y, float dx, float dy, AreaI& dirty)
{
	int x = (int)floorf(_x);
	int y = (int)floorf(_y);
//	dx = (int)dx;
//	dy = (int)dy;

	x -= (filter->Sx_get() - 1) / 2.f;
	y -= (filter->Sy_get() - 1) / 2.f;

	AreaI area;
	area.m_Min[0] = x;
	area.m_Min[1] = y;
	area.m_Max[0] = x + filter->Sx_get() - 1;
	area.m_Max[1] = y + filter->Sy_get() - 1;

	const uint32_t filterSx = filter->Sx_get();
	const uint32_t filterSy = filter->Sy_get();
	
	dx *= mStrength;
	dy *= mStrength;

#ifdef __ARM_NEON__
	const float32_t s_dxdy[2] = { dx,   dy   };
	const float32_t s_10[2]   = { 1.0f, 0.0f };
	
	float32x2_t v_dxdy = vld1_f32(s_dxdy);
	float32x2_t v_incx = vld1_f32(s_10);
	
	for (uint32_t py = 0; py < filterSy; ++py)
	{
		const float32_t s_xy[2] = { x, y + (int)py };
		float32x2_t v_xy   = vld1_f32(s_xy);
		
		const float * __restrict bpix = filter->Line_get(py);
		Rgba * __restrict cpix1 = &gTempBuf[py][0];
		
		for (uint32_t px = filterSx; px != 0; --px)
		{
			float32x2_t v_b = vld1_dup_f32(reinterpret_cast<const float32_t*>(bpix));
			float32x2_t v_s = vmls_f32(v_xy, v_b, v_dxdy);

			bmp->SampleAA(v_s, *cpix1);
			
			bpix++;
			cpix1++;
			v_xy = vadd_f32(v_xy, v_incx);
		}
	}
#else
	for (uint32_t py = 0; py < filterSy; ++py)
	{
		const float * __restrict bpix = filter->Line_get(py);
		Rgba * __restrict cpix1 = &gTempBuf[py][0];
		
		for (uint32_t px = 0; px < filterSx; ++px)
		{
			const float s = *bpix;
			bmp->SampleAA(x + px - s * dx, y + py - s * dy, *cpix1);
			
			bpix++;
			cpix1++;
		}
	}
#endif
	
	if (!area.Clip(bmp->Area_get()))
		return;

	//

	const int sx = (area.x2 - area.x1 + 1) * sizeof(Rgba);

	for (int py = area.y1; py <= area.y2; ++py)
	{
		const Rgba * __restrict tpix = &gTempBuf[py - y][area.x1 - x];
		Rgba * __restrict cpix = bmp->Line_get(py) + area.x1;

 		memcpy(cpix, tpix, sx);
	}

	//

	dirty.Merge(area);
}
