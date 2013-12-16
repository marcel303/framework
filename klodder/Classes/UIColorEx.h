#import <UIKit/UIColor.h>

//#import "UIColor-Expanded.h"
//  get it here:
//  http://github.com/ars/uicolor-utilities

#define MIN3(x,y,z)  ((y) <= (z) ? \
((x) <= (y) ? (x) : (y)) \
: \
((x) <= (z) ? (x) : (z)))

#define MAX3(x,y,z)  ((y) >= (z) ? \
((x) >= (y) ? (x) : (y)) \
: \
((x) >= (z) ? (x) : (z)))


struct rgb_color {
    CGFloat r, g, b;
};

struct hsv_color {
    CGFloat hue;        
    CGFloat sat;        
    CGFloat val;        
};

@interface UIColor (UIColor_HSVAdditions)

@property (nonatomic, readonly) CGFloat hue; // Only valid if canProvideRGBComponents is YES
@property (nonatomic, readonly) CGFloat saturation; // Only valid if canProvideRGBComponents is YES
@property (nonatomic, readonly) CGFloat brightness; // Only valid if canProvideRGBComponents is YES
@property (nonatomic, readonly) CGFloat value; // (same as brightness, added for naming consistency)


//workhorse method, does conversion:
+(struct hsv_color)HSVfromRGB:(struct rgb_color)rgb;
//individual value accessors:
-(CGFloat)hue;
-(CGFloat)saturation;
-(CGFloat)brightness;
-(CGFloat)value;
@end
