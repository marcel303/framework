#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"
#import "View_BrushSelect.h"
#import "View_ToolType.h"
#import "ViewBase.h"

@interface View_BrushSettingsPanel : ViewBase <BrushSelectDelegate>
{
	AppDelegate* app;
	
	View_ToolSelectMgr* controller;
	
	View_BrushSelect* viewPatternSelect;
	View_BrushSettings* viewSettings;
	View_BrushPreview* viewPreview;
	
	//
	
	View_Gauge* spacingGauge;
}

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app controller:(View_ToolSelectMgr*)controller type:(ToolViewType)type;

//

-(void)brushSettingsChanged;
-(void)spacingChanged:(NSNumber*)value;

// ====================
// BrushSelectDelegate implementation
// ====================
-(std::vector<Brush_Pattern*>)getPatternList;
-(void)handleBrushPatternSelect:(int)patternId;
-(void)handleBrushSoftSelect;

@end
