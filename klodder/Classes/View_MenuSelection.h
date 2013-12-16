#import <UIKit/UIView.h>
#import "klodder_forward_objc.h"

@interface View_MenuSelection : UIView 
{
	@private
	
	int index;
	AppDelegate* app;
	UIViewController* controller;
}

-(id)initWithIndex:(int)index frame:(CGRect)frame app:(AppDelegate*)app controller:(UIViewController*)controller;

@end
