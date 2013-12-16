#import <UIKit/UIKit.h>
#import "Application.h"
#import "UIVerticalSlider.h"

@interface ToolView : UIView
{
@public
	/*
	IBOutlet UISlider* sliderColorHue;
	IBOutlet UISlider* sliderColorSat;
	IBOutlet UISlider* sliderColorVal;
	IBOutlet UISlider* sliderSize;
	IBOutlet UISlider* sliderOpacity;*/
	
	IBOutlet UIVerticalSlider* sliderColorHue;
	IBOutlet UIVerticalSlider* sliderColorSat;
	IBOutlet UIVerticalSlider* sliderColorVal;
	IBOutlet UIVerticalSlider* sliderColorOpac;
	
	IBOutlet UIVerticalSlider* sliderBrushSize;
	IBOutlet UIVerticalSlider* sliderBrushHard;
	
	IBOutlet UIView* viewColor;
	
	Paint::ToolType toolType;
	float rgb[3];
	int size;
	float opacity;
	float hardness;
}

- (void)updateColor;
- (void)updateBrush;

/*-(IBAction) sliderColorHueChange:(id)sender;
-(IBAction) sliderColorSatChange:(id)sender;
-(IBAction) sliderColorValChange:(id)sender;
-(IBAction) sliderSizeChange:(id)sender;
-(IBAction) sliderOpacityChange:(id)sender;*/
-(IBAction) buttonToolBrushClick:(id)sender;
-(IBAction) buttonToolSmudgeClick:(id)sender;
-(IBAction) buttonToolSoftenClick:(id)sender;

@end
