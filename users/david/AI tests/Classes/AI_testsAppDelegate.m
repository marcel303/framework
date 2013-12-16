//
//  AI_testsAppDelegate.m
//  AI tests
//
//  Created by Narf on 6/23/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "AI_testsAppDelegate.h"
#import "AI_testsViewController.h"

@implementation AI_testsAppDelegate

@synthesize window;
@synthesize viewController;


- (void)applicationDidFinishLaunching:(UIApplication *)application {    
    
    // Override point for customization after app launch
	[application setStatusBarHidden:TRUE];
	[application setIdleTimerDisabled:FALSE];
	
    [window addSubview:viewController.view];
    [window makeKeyAndVisible];
}


- (void)dealloc {
    [viewController release];
    [window release];
    [super dealloc];
}


@end
