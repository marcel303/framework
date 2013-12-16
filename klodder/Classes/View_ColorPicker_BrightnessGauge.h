#pragma once
#import <UIKit/UIView.h>
#import "ColorPickerState.h"
#import "klodder_forward_objc.h"

@interface View_ColorPicker_BrightnessGauge : UIView 
{
	@private
	
	float height;
	id<PickerDelegate> delegate;
	View_ColorPicker_BasicIndicator* indicator;
	
	@public
	
	float value;
}

@property (assign) float value;

-(id)initWithFrame:(CGRect)frame height:(float)height delegate:(id<PickerDelegate>)delegate;
-(void)sample:(Vec2F)location;
-(void)setValueDirect:(float)value;

@end
