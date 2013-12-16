#import <UIKit/UIView.h>
#import "Bitmap.h"
#import "ColorPickerState.h"
#import "klodder_forward_objc.h"
#import "Types.h"
#import "View_ColorPicker_ColorBar.h"
#import "ViewBase.h"

#include <vector> // exclusion zone
class ExclusionZone
{
public:
	void Add(RectF rect);
	
	bool IsInside(Vec2F location);
	
	void RenderUi();
	
private:
	std::vector<RectF> mRectList;
};

@interface View_ColorPicker : ViewBase <ColorButtonDelegate>
{
	@private
	
	AppDelegate* app;
	UIViewController* controller;
	id<PickerDelegate> delegate;
	ExclusionZone exclusionZone;
	
	View_ColorPicker_ColorBar* colorBar;
	View_ColorPicker_BasicIndicator* indicator;
	View_ColorPicker_BrightnessGauge* brightnessGauge;
	View_ColorPicker_OpacityGauge* opacityGauge;
	
	Rgba oldColor;
	
	UIImageView* darkOverlay;
}

@property (readonly, assign) Rgba color;

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app controller:(UIViewController*)controller delegate:(id<PickerDelegate>)delegate;
-(void)sampleAtPoint:(Vec2F)location;
-(void)handleTouch:(Vec2F)location;
-(Rgba)color;
-(void)updateUi;

// ColorButtonDelegate
-(void)handleColorSelect:(UIColor*)color;
-(void)handleColorHistorySelect;

@end
