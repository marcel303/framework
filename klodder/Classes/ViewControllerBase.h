#import <UIKit/UIView.h>

@class AppDelegate;

@interface ViewControllerBase : UIViewController 
{
	@protected
	
	AppDelegate* app;
}

-(id)initWithApp:(AppDelegate*)app;
-(void)setMenuTransparent;
-(void)setMenuOpaque;

@end
