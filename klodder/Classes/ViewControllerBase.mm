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
	[self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [self.navigationController.navigationBar setTranslucent:YES];
    
	[self.navigationController.toolbar setBarStyle:UIBarStyleDefault];
    [self.navigationController.toolbar setTranslucent:YES];
}

-(void)setMenuOpaque
{
	[self.navigationController.navigationBar setBarStyle:UIBarStyleDefault];
    [self.navigationController.navigationBar setTranslucent:NO];
    
	[self.navigationController.toolbar setBarStyle:UIBarStyleDefault];
    [self.navigationController.toolbar setTranslucent:NO];
}

-(void)setFullScreenLayout
{
    [self setEdgesForExtendedLayout:UIRectEdgeAll];
    [self setExtendedLayoutIncludesOpaqueBars:YES];
}

@end
