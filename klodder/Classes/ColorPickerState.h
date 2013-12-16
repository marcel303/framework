#pragma once

#include "Bitmap.h"

class PickerState
{
public:
	PickerState();
	
	Rgba Color_get() const;
	void Color_set(Rgba color);
	Rgba BaseColor_get() const;
	void BaseColor_set(float r, float g, float b);
	float Brightness_get() const;
	void Brightness_set(float brightness);
	float Opacity_get() const;
	void Opacity_set(float opacity);
	
	bool isActive;
	
private:
	float mR, mG, mB;
	float mBrightness;
	float mOpacity;
};

@protocol PickerDelegate

-(PickerState*)colorPickerState;
-(void)colorChanged;
-(void)openingSwatches;

@end
