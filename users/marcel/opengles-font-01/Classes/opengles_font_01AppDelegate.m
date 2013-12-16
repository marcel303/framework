//
//  opengles_font_01AppDelegate.m
//  opengles-font-01
//
//  Created by Marcel Smit on 10-04-09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "opengles_font_01AppDelegate.h"
#import "EAGLView.h"

@implementation opengles_font_01AppDelegate

@synthesize window;
@synthesize glView;

- (void)applicationDidFinishLaunching:(UIApplication *)application {
    
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
