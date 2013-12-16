//
//  crapbuttonAppDelegate.m
//  crapbutton
//
//  Created by Marcel Smit on 21-05-10.
//  Copyright Apple Inc 2010. All rights reserved.
//

#import "crapbuttonAppDelegate.h"
#import "crapbuttonViewController.h"

@implementation crapbuttonAppDelegate

@synthesize window;
@synthesize viewController;


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
    
    // Override point for customization after app launch    
    [window addSubview:viewController.view];
    [window makeKeyAndVisible];
	
	return YES;
}


- (void)dealloc {
    [viewController release];
    [window release];
    [super dealloc];
}


@end
