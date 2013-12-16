#import "audio_02AppDelegate.h"
#import "MainController.h"

@implementation audio_02AppDelegate

@synthesize window;
@synthesize controller;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
	[window addSubview:controller.view];
    [window makeKeyAndVisible];
}

- (void)dealloc {
    [window release];
    [super dealloc];
}

@end
