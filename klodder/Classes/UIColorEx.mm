//
//  UIColor-HSVAdditions.m
//
//  Created by Matt Reagan (bravobug.com) on 12/31/09.
//  
// Released into the public domain
//Original code: http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29

#import "UIColorEx.h"

static float GetR(UIColor* color)
{
	const CGFloat* c = CGColorGetComponents(color.CGColor);
	return c[0];
}

static float GetG(UIColor* color)
{
	const CGFloat* c = CGColorGetComponents(color.CGColor);
	return c[1];
}

static float GetB(UIColor* color)
{
	const CGFloat* c = CGColorGetComponents(color.CGColor);
	return c[2];
}

@implementation UIColor (UIColor_HSVAdditions)

+(struct hsv_color)HSVfromRGB:(struct rgb_color)rgb
{
	struct hsv_color hsv;
	
	CGFloat rgb_min, rgb_max;
	rgb_min = MIN3(rgb.r, rgb.g, rgb.b);
	rgb_max = MAX3(rgb.r, rgb.g, rgb.b);
	
	hsv.val = rgb_max;
	if (hsv.val == 0) {
		hsv.hue = hsv.sat = 0;
		return hsv;
	}
	
	rgb.r /= hsv.val;
	rgb.g /= hsv.val;
	rgb.b /= hsv.val;
	rgb_min = MIN3(rgb.r, rgb.g, rgb.b);
	rgb_max = MAX3(rgb.r, rgb.g, rgb.b);
	
	hsv.sat = rgb_max - rgb_min;
	if (hsv.sat == 0) {
		hsv.hue = 0;
		return hsv;
	}
	
#if 0
	if (rgb_max == rgb.r) {
		hsv.hue = 0.0 + 60.0*(rgb.g - rgb.b);
	} else if (rgb_max == rgb.g) {
		hsv.hue = 120.0 + 60.0*(rgb.b - rgb.r);
	} else /* rgb_max == rgb.b */ {
		hsv.hue = 240.0 + 60.0*(rgb.r - rgb.g);
	}
#else
	const float dr = ((rgb_max - rgb.r) * 60.0f + (rgb_max - rgb_min) / 2.0f) / (rgb_max - rgb_min);
	const float dg = ((rgb_max - rgb.g) * 60.0f + (rgb_max - rgb_min) / 2.0f) / (rgb_max - rgb_min);
	const float db = ((rgb_max - rgb.b) * 60.0f + (rgb_max - rgb_min) / 2.0f) / (rgb_max - rgb_min);

	if (rgb_max == rgb.r)
		hsv.hue = 0.0 + db - dg;
	else if (rgb_max == rgb.g)
		hsv.hue = 120.0 + dr - db;
	else
		hsv.hue = 240.0 + dg - dr;
#endif
	
	if (hsv.hue < 0.0) {
		hsv.hue += 360.0;
	}
	
	return hsv;
}

-(CGFloat)hue
{
	struct hsv_color hsv;
	struct rgb_color rgb;
	rgb.r = GetR(self);
	rgb.g = GetG(self);
	rgb.b = GetB(self);
	hsv = [UIColor HSVfromRGB: rgb];
	return (hsv.hue / 360.0);
}

-(CGFloat)saturation
{
	struct hsv_color hsv;
	struct rgb_color rgb;
	rgb.r = GetR(self);
	rgb.g = GetG(self);
	rgb.b = GetB(self);
	hsv = [UIColor HSVfromRGB: rgb];
	return hsv.sat;
}

-(CGFloat)brightness
{
	struct hsv_color hsv;
	struct rgb_color rgb;
	rgb.r = GetR(self);
	rgb.g = GetG(self);
	rgb.b = GetB(self);
	hsv = [UIColor HSVfromRGB: rgb];
	return hsv.val;
}

-(CGFloat)value
{
	return [self brightness];
}

@end
