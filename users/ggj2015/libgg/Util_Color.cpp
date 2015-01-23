#include "Calc.h"
#include "Util_Color.h"

#if 0

// note: HSL <-> RGB conversion routines taken from CxImage and converted to floating point

#define max Calc::Max
#define min Calc::Min
#define HSLMAX   1.0f
#define RGBMAX   1.0f
#define HSLUNDEFINED (HSLMAX*2.0f/3.0f)

class RGBQUAD
{
public:
	float rgbRed;
	float rgbGreen;
	float rgbBlue;
	float alpha;
};

static inline float MOD(float v, float m)
{
	while (v > m)
		v -= m;
	while (v < 0.0f)
		v += m;
	
	return v;
}

static inline float CLAMP(float v, float m)
{
	if (v < 0.0f)
		v = 0.0f;
	else if (v > m)
		v = m;
	
	return v;
}

RGBQUAD RGBtoHSL(RGBQUAD rgb)
{
	float H,L,S;					/* output HSL values */

	const float R = rgb.rgbRed;	/* get R, G, and B out of DWORD */
	const float G = rgb.rgbGreen;
	const float B = rgb.rgbBlue;

	const float cMax = Calc::Max(Calc::Max(R, G), B);	/* calculate lightness */
	const float cMin = Calc::Min(Calc::Min(R, G), B);
	
	L = (((cMax+cMin)*HSLMAX)+RGBMAX)/(2.0f*RGBMAX);

	if (cMax==cMin)
	{			/* r=g=b --> achromatic case */
		S = 0.0f;					/* saturation */
		H = HSLUNDEFINED;		/* hue */
	}
	else
	{		
		/* chromatic case */
		if (L <= (HSLMAX/2.0f))	/* saturation */
			S = CLAMP(((((cMax-cMin)*HSLMAX)+((cMax+cMin)/2.0f))/(cMax+cMin)), HSLMAX);
		else
			S = CLAMP(((((cMax-cMin)*HSLMAX)+((2.0f*RGBMAX-cMax-cMin)/2.0f))/(2.0f*RGBMAX-cMax-cMin)), HSLMAX);
		
		/* hue */
		const float Rdelta = ((((cMax-R)*(HSLMAX/6.0f)) + ((cMax-cMin)/2.0f) ) / (cMax-cMin));
		const float Gdelta = ((((cMax-G)*(HSLMAX/6.0f)) + ((cMax-cMin)/2.0f) ) / (cMax-cMin));
		const float Bdelta = ((((cMax-B)*(HSLMAX/6.0f)) + ((cMax-cMin)/2.0f) ) / (cMax-cMin));

		if (R == cMax)
			H = MOD((Bdelta - Gdelta), HSLMAX);
		else if (G == cMax)
			H = MOD(((HSLMAX/3.0f) + Rdelta - Bdelta), HSLMAX);
		else /* B == cMax */
			H = MOD((((2.0f*HSLMAX)/3.0f) + Gdelta - Rdelta), HSLMAX);

//		if (H < 0) H += HSLMAX;     //always false
		if (H > HSLMAX) H -= HSLMAX;
	}
	
	RGBQUAD hsl={H,S,L,0};
	return hsl;
}
////////////////////////////////////////////////////////////////////////////////
float HueToRGB(float n1,float n2, float hue)
{
	//<F. Livraghi> fixed implementation for HSL2RGB routine
	float rValue;

	if (hue > 360.0f)
		hue = hue - 360.0f;
	else if (hue < 0.0f)
		hue = hue + 360.0f;

	if (hue < 60.0f)
		rValue = n1 + (n2-n1)*hue/60.0f;
	else if (hue < 180.0f)
		rValue = n2;
	else if (hue < 240.0f)
		rValue = n1+(n2-n1)*(240.0f-hue)/60.0f;
	else
		rValue = n1;

	return rValue;
}

RGBQUAD HSLtoRGB(RGBQUAD hsl)
{ 
	float m1,m2;
	float r,g,b;

	const float h = hsl.rgbRed * 360.0f / HSLMAX;
	const float s = hsl.rgbGreen / HSLMAX;
	const float l = hsl.rgbBlue / HSLMAX;

	if (l <= 0.5)	
		m2 = l * (1.0f + s);
	else
		m2 = l + s - l * s;

	m1 = 2.0f * l - m2;

	if (s == 0) 
	{
		r = g = b = l * RGBMAX;
	}
	else 
	{
		r = CLAMP((HueToRGB(m1, m2, h + 120.0f) * RGBMAX), RGBMAX);
		g = CLAMP((HueToRGB(m1, m2, h         ) * RGBMAX), RGBMAX);
		b = CLAMP((HueToRGB(m1, m2, h - 120.0f) * RGBMAX), RGBMAX);
	}

	RGBQUAD rgb = {r,g,b,0};
	return rgb;
}

#define SCALE 1.0f

void RgbToHsl(float* _rgb, float* hsl)
{
	RGBQUAD rgb;
	rgb.rgbRed = _rgb[0] * SCALE;
	rgb.rgbGreen = _rgb[1] * SCALE;
	rgb.rgbBlue = _rgb[2] * SCALE;
	
	rgb = RGBtoHSL(rgb);
	
	hsl[0] = rgb.rgbRed / SCALE;
	hsl[1] = rgb.rgbGreen / SCALE;
	hsl[2] = rgb.rgbBlue / SCALE;
}

void HslToRgb(float* _hsl, float* rgb)
{ 
	RGBQUAD hsl;
	hsl.rgbRed = _hsl[0] * SCALE;
	hsl.rgbGreen = _hsl[1] * SCALE;
	hsl.rgbBlue = _hsl[2] * SCALE;
	
	hsl = HSLtoRGB(hsl);
	
	rgb[0] = hsl.rgbRed / SCALE;
	rgb[1] = hsl.rgbGreen / SCALE;
	rgb[2] = hsl.rgbBlue / SCALE;
}

#endif
