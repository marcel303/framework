#import <UIKit/UIKit.h>
#import "View_ToolSelectMgr.h"
#import "View_ToolType.h"
#import "ViewBase.h"

@interface View_ToolSettings_Blur : ViewBase
{
	@private
	
	View_ToolSelectMgr* controller;
	
	View_ToolType* mToolType;
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_ToolSelectMgr*)controller;

@end
