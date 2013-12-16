#import <UIKit/UIView.h>
#import "klodder_forward.h"
#import "klodder_forward_objc.h"
#import "View_ToolType.h"

@interface View_BrushPreview : UIView 
{
	AppDelegate* app;
	BrushSettings* settings;
	Rgba color;
	ToolViewType type;
	MacImage* brushPreview;
	uint32_t patternId;
}

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)app settings:(BrushSettings*)settings type:(ToolViewType)type color:(Rgba)color;
-(void)redraw;

@end
