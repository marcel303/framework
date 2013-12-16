//
//  abcAppDelegate.m
//  abc
//
//  Created by Marcel Smit on 19-12-09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "abcAppDelegate.h"
#import "abcViewController.h"

@implementation abcAppDelegate

@synthesize window;
@synthesize viewController;


- (void)applicationDidFinishLaunching:(UIApplication *)application {    
    
    // Override point for customization after app launch    
    [window addSubview:viewController.view];
    [window makeKeyAndVisible];
}


- (void)dealloc {
    [viewController release];
    [window release];
    [super dealloc];
}


@end
