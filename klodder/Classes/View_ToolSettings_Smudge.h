#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "View_ToolType.h"
#import "ViewBase.h"

@interface View_ToolSettings_Smudge : ViewBase <ToolView> 
{
	@private
	
	AppDelegate* app;
	View_ToolSelectMgr* controller;
	
	View_ToolType* viewToolType;
	View_BrushSettingsPanel* viewBrushSettings;
	
	View_CheckBox* viewSmoothCheckbox;
	View_Gauge* viewStrengthGauge;
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_ToolSelectMgr*)controller;
-(void)load;
-(void)save;
-(void)brushSettingsChanged;
-(void)strengthChanged:(NSNumber*)value;
+(uint32_t)lastPatternId;

@end
