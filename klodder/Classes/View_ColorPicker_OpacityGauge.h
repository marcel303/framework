#pragma once

#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "ColorPickerState.h"

@interface View_ColorPicker_OpacityGauge : UIView 
{
	@private
	
	float height;
	id<PickerDelegate> delegate;
	
	@public
	
	float value;
	View_ColorPicker_OpacityIndicator* indicator;
}

@property (assign) float value;

-(id)initWithFrame:(CGRect)frame height:(float)height delegate:(id<PickerDelegate>)delegate;
-(void)sample:(Vec2F)location;
-(void)setValueDirect:(float)value;

@end
