#pragma once
#import <UIKit/UIView.h>

@protocol ColorButtonDelegate

-(void)handleColorSelect:(UIColor*)color;
-(void)handleColorHistorySelect;

@end

@interface View_ColorPicker_ColorBar : UIView 
{
	id<ColorButtonDelegate> delegate;
	
	UIColor* oldColor;
	UIColor* newColor;
}

@property (nonatomic, retain) UIColor* oldColor;
@property (nonatomic, retain) UIColor* newColor;

-(id)initWithFrame:(CGRect)frame delegate:(id<ColorButtonDelegate>)delegate;
-(void)setOldColor:(UIColor*)color;
-(void)setNewColor:(UIColor*)color;
-(void)setNewColorDirect:(UIColor*)color;

@end
