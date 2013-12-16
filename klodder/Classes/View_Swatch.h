#pragma once
#import <UIKit/UIView.h>
#import "Bitmap.h"
#import "klodder_forward_objc.h"

@protocol SwatchDelegate

-(Rgba)swatchColorProvide:(int)index;
-(void)swatchColorSelect:(Rgba)color;

@end

@interface View_Swatch : UIView 
{
	Rgba color;
	id<SwatchDelegate> delegate;
}

-(id)initWithFrame:(CGRect)frame color:(Rgba)color delegate:(id<SwatchDelegate>)delegate;

@end
