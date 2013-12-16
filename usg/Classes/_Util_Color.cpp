#include "Util_Color.h"
#ifdef IPHONEOS
#include "System.h"
#endif

#ifdef WIN32
static float HueToRGB(float n1, float n2, float hue)
{
	if (hue < 0.0f)
		hue += 1.0f;
	else if (hue > 1.0f)
		hue -= 1.0f;

	if (hue < 1.0f / 6.0f)
		return n1 + (((n2 - n1) * hue + (1.0f / 12.0f)) * 2.0f);
	else if (hue < 1.0f / 2.0f)
		return n2;
	else if (hue < 2.0f / 3.0f)
		return n1 + (((n2 - n1) * ((2.0f / 3.0f) - hue) + (1.0f / 12.0f)) * 2.0f);
	else
		return n1;
}

SpriteColor HLStoRGB(float hue, float lum, float sat)
{
	if (sat == 0.0f)
	{
		return SpriteColor_MakeF(lum, lum, lum, 1.0f);
	}
	else
	{
		float Magic2;

		if (lum <= 0.5f)
			Magic2 = lum * (1.0f + sat) + 0.5f;
		else
			Magic2 = lum + sat - ((lum * sat) + 0.5f);

		const float Magic1 = 2.0f * lum - Magic2;

		const float R = HueToRGB(Magic1, Magic2, hue + (1.0f / 3.0f)) + 0.5f;
		const float G = HueToRGB(Magic1, Magic2, hue) + 0.5f;
		const float B = HueToRGB(Magic1, Magic2, hue - (1.0f / 3.0f)) + 0.5f;

		return SpriteColor_MakeF(R, G, B, 1.0f);
	}
}

static inline SpriteColor Color(float h, float _s, float _b)
{
	h = fmodf(h, 1.0f);

	_b = 0.5f;

	return HLStoRGB(h, _b, _s);
}
#endif

#ifdef IPHONEOS
	#define Color(h, s, b) g_System.Color_FromHSB(h, s, b)
#endif

#define HUE_LUT_SIZE 128

namespace Calc
{
	static SpriteColor g_HueLUT[HUE_LUT_SIZE];
	
	static inline void HueLUT_GetIndices(float hue, int& out_Index1, int& out_Index2, float& out_T)
	{
		out_Index1 = (int)(hue * HUE_LUT_SIZE) & (HUE_LUT_SIZE - 1);
		out_Index2 = (out_Index1 + 1) & (HUE_LUT_SIZE - 1);
		out_T = hue - floorf(hue);
	}
	
	static SpriteColor HueLUT_Sample(float hue)
	{
		int index1;
		int index2;
		float t;
		
		HueLUT_GetIndices(hue, index1, index2, t);
		
		const SpriteColor& color1 = g_HueLUT[index1];
		const SpriteColor& color2 = g_HueLUT[index2];
		
//		return color1;
		
		return SpriteColor_BlendF(color1, color2, t);
	}
	
	void Initialize_Color()
	{
		for (int i = 0; i < HUE_LUT_SIZE; ++i)
		{
			const float hue = i / (float)HUE_LUT_SIZE;
			
			const float saturation = 1.0f;
			const float brightness = 1.0f;
			
			g_HueLUT[i] = Color(hue, saturation, brightness);
		}
	}
	
	SpriteColor Color_FromHue(float hue)
	{
		//return Color_FromHSB_Ex(hue, saturation, brightness);
		
		return HueLUT_Sample(hue);
	}
	
	SpriteColor Color_FromHue_NoLUT(float hue)
	{
		return Color(hue, 1.0f, 1.0f);
	}
};
