#import "AppDelegate.h"
#import "ViewControllerBase.h"

@implementation ViewControllerBase

-(id)initWithApp:(AppDelegate*)_app
{
	if (self = [super init])
	{
		app = _app;
	}
	
	return self;
}

-(void)setMenuTransparent
{
	[self.navigationController.navigationBar setBarStyle:UIBarStyleBlackTranslucent];
//	[self.navigationController.toolbar setTintColor:nil];
	[self.navigationController.toolbar setBarStyle:UIBarStyleBlackTranslucent];
}

-(void)setMenuOpaque
{
	[self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
//	[self.navigationController.toolbar setTintColor:nil];
//	[self.navigationController.toolbar setTintColor:[UIColor colorWithRed:1.0f green:0.5f blue:1.0f alpha:1.0f]];
	[self.navigationController.toolbar setBarStyle:UIBarStyleDefault];
}

@end
