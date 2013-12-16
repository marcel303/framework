#import "Calc.h"
#import "ColorPickerState.h"

PickerState::PickerState()
{
	isActive = false;
	mR = mG = mB = 1.0f;
	mBrightness = 0.5f;
	mOpacity = 1.0f;
}

Rgba PickerState::Color_get() const
{	
	return Rgba_Make(mR * mBrightness, mG * mBrightness, mB * mBrightness);
}

void PickerState::Color_set(Rgba color)
{
	float brightness = Calc::Max(color.rgb[0], Calc::Max(color.rgb[1], color.rgb[2]));
	
	BaseColor_set(color.rgb[0], color.rgb[1], color.rgb[2]);
	Brightness_set(brightness);
}

Rgba PickerState::BaseColor_get() const
{
	return Rgba_Make(mR, mG, mB, 1.0f);
}

void PickerState::BaseColor_set(float r, float g, float b)
{
	// normalize color
	
	float l = Calc::Max(r, Calc::Max(g, b));
	
	if (l == 0.0f)
	{
		mR = mG = mB = 1.0f;
	}
	else
	{
		mR = r / l;
		mG = g / l;
		mB = b / l;
	}
	
	LOG_DBG("base color r/g/b: %f, %f, %f", mR, mG, mB);
}

float PickerState::Brightness_get() const
{
	return mBrightness;
}

void PickerState::Brightness_set(float brightness)
{
	mBrightness = brightness;
}

float PickerState::Opacity_get() const
{
	return mOpacity;
}

void PickerState::Opacity_set(float opacity)
{
	mOpacity = opacity;
}
