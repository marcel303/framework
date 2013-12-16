//
//  direct_01AppDelegate.h
//  direct-01
//
//  Created by Marcel Smit on 09-08-09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@class direct_01ViewController;

@interface direct_01AppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    direct_01ViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet direct_01ViewController *viewController;

@end

