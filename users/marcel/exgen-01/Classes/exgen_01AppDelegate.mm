#import "exgen_01AppDelegate.h"
#import "exgen_01ViewController.h"
#import "SurfaceView.h"

@implementation exgen_01AppDelegate

@synthesize window;
@synthesize viewController;

- (void)applicationDidFinishLaunching:(UIApplication *)application {    
	[window setOpaque:TRUE];
	
	SurfaceView* view = [[SurfaceView alloc] initWithFrame:[window frame]];
	[window addSubview:view];
	
	[application setStatusBarHidden:TRUE];
	[application setIdleTimerDisabled:TRUE];
	
	[window makeKeyAndVisible];
}

- (void)dealloc {
    [viewController release];
    [window release];
    [super dealloc];
}

@end
