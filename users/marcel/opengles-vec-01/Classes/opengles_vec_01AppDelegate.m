#import "opengles_vec_01AppDelegate.h"
#import "EAGLView.h"

@implementation opengles_vec_01AppDelegate

@synthesize window;
@synthesize glView;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
	
	[application setStatusBarHidden:TRUE];
	
	glView.animationInterval = 1.0 / 60.0;
	[glView startAnimation];
}


- (void)applicationWillResignActive:(UIApplication *)application {
	
	glView.animationInterval = 1.0 / 5.0;
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
	
	glView.animationInterval = 1.0 / 60.0;
}


- (void)dealloc {
	
	[window release];
	[glView release];
	[super dealloc];
}

@end
