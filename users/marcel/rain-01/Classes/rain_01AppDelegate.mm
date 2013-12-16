//
//  rain_01AppDelegate.m
//  rain-01
//
//  Created by user on 3/26/10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import "rain_01AppDelegate.h"
#import "EAGLView.h"

@implementation rain_01AppDelegate

@synthesize window;
@synthesize glView;

//- (void)applicationDidFinishLaunching:(UIApplication *)application
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	[application setNetworkActivityIndicatorVisible:FALSE];
	[application setStatusBarHidden:TRUE animated:FALSE];
	[application setIdleTimerDisabled:TRUE];
	
	[window setOpaque:TRUE];
	
	UIImage* back = [UIImage imageNamed:@"back.png"];
		
	UIImageView* imageView = [[UIImageView alloc] initWithImage:back];
//	[window addSubview:imageView];
	
	glView = [[EAGLView alloc] initWithFrame:[window frame]];
	[window addSubview:glView];
	
	[glView startAnimation];
	
	[window makeKeyAndVisible];
	
	return YES;
}

- (void) applicationWillResignActive:(UIApplication *)application
{
	[glView stopAnimation];
}

- (void) applicationDidBecomeActive:(UIApplication *)application
{
	[glView startAnimation];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	[glView stopAnimation];
}

- (void) dealloc
{
	[window release];
	[glView release];
	
	[super dealloc];
}

@end
