#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "ViewBase.h"

@interface View_MiniBrushSize : UIView
{
	AppDelegate* app;
	View_MiniBrushSizeContainer* parent;
	View_BrushPreview* preview;
	UISlider* slider;
	View_MiniToolSelect* toolSelect;
}

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app parent:(View_MiniBrushSizeContainer*)parent;
-(void)show;
-(void)hide;
-(void)updateUi;

-(void)handleSliderUp;
-(void)handleSliderChanged;
-(void)handleToolSelect:(NSNumber*)type;

@end
