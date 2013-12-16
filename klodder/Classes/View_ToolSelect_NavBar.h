#import <UIKit/UIKit.h>
#import "klodder_forward_objc.h"

@interface View_ToolSelect_NavBar : UIView 
{
	View_ToolSelectMgr* controller;
}

-(id)initWithFrame:(CGRect)frame name:(NSString*)name controller:(View_ToolSelectMgr*)controller;
-(void)handleDone;

@end
