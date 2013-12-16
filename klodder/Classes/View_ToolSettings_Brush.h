#pragma once

#import <UIKit/UIView.h>
#import "klodder_forward.h"
#import "klodder_forward_objc.h"
#import "View_ToolSelectMgr.h"
#import "ViewBase.h"

@interface View_ToolSettings_Brush : ViewBase <ToolView>
{
	@private
	
	AppDelegate* app;
	View_ToolSelectMgr* controller;
	
	View_ToolType* vwToolType;
	View_BrushSettingsPanel* vwBrushSettingsPanel;
	
	View_Gauge* viewOpacityGauge;
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_ToolSelectMgr*)controller;
-(void)load;
-(void)save;
-(void)brushSettingsChanged;
-(void)opacityChanged:(NSNumber*)value;
+(uint32_t)lastPatternId;

@end
