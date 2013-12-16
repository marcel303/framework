//
//  pebblesAppDelegate.h
//  pebbles
//
//  Created by Marcel Smit on 24-08-10.
//  Copyright __MyCompanyName__ 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

@class AppView;

@interface pebblesAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
	AppView* appView;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@end

