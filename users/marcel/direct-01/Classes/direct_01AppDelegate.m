//
//  direct_01AppDelegate.m
//  direct-01
//
//  Created by Marcel Smit on 09-08-09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import "direct_01AppDelegate.h"
#import "direct_01ViewController.h"

@implementation direct_01AppDelegate

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
