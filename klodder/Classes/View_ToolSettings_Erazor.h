#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "View_ToolType.h"
#import "ViewBase.h"

@interface View_ToolSettings_Eraser : ViewBase <ToolView>
{
	@private
	
	AppDelegate* app;
	View_ToolSelectMgr* controller;
	
	View_ToolType* vwToolType;
	View_BrushSettingsPanel* vwBrushSettingsPanel;
	
	View_CheckBox* mSmoothCheckbox;
	
	View_Gauge* strengthGauge;
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_ToolSelectMgr*)_controller;
-(void)load;
-(void)save;
-(void)brushSettingsChanged;
-(void)opacityChanged:(NSNumber*)value;
+(uint32_t)lastPatternId;

@end
