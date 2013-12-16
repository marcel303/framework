//
//  toolbar_01AppDelegate.m
//  toolbar-01
//
//  Created by Marcel Smit on 01-04-10.
//  Copyright Apple Inc 2010. All rights reserved.
//

#import "toolbar_01AppDelegate.h"
#import "toolbar_01ViewController.h"

@implementation toolbar_01AppDelegate

@synthesize window;
@synthesize viewController;

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
    
    // Override point for customization after app launch    
    [window addSubview:viewController.view];
    [window makeKeyAndVisible];
	
#if 1
	viewController = [[toolbar_01ViewController alloc] init];
	viewController.view = window;
#endif
	
	return YES;
}


- (void)dealloc {
    [viewController release];
    [window release];
    [super dealloc];
}


@end
