//
//  opengles_ext_01AppDelegate.m
//  opengles-ext-01
//
//  Created by Marcel Smit on 12-05-09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "opengles_ext_01AppDelegate.h"
#import "OpenGLView.h"

@implementation opengles_ext_01AppDelegate

@synthesize window;



- (void)applicationDidFinishLaunching:(UIApplication *)application {    

	OpenGLView* view = [[OpenGLView alloc] initWithFrame:[window frame]];
	[window addSubview:view];

//	[NSThread sleepForTimeInterval:5.0];
	
    [window makeKeyAndVisible];
}


- (void)dealloc {
    [window release];
    [super dealloc];
}


@end
