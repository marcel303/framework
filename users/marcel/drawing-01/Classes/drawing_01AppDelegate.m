#import "drawing_01AppDelegate.h"
#import "DrawingView.h"

@implementation drawing_01AppDelegate

@synthesize window;

- (void)applicationDidFinishLaunching:(UIApplication *)application {    
	[window setOpaque:TRUE];
	
	DrawingView* view = [[DrawingView alloc] initWithFrame:[window frame]];
	[window addSubview:view];
	
	[application setStatusBarHidden:TRUE];
	[application setIdleTimerDisabled:TRUE];
	
	[window makeKeyAndVisible];
}

- (void)dealloc {
    [window release];
    [super dealloc];
}

@end
