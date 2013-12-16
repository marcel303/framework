//
//  AI_testsAppDelegate.h
//  AI tests
//
//  Created by Narf on 6/23/09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@class AI_testsViewController;

@interface AI_testsAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    AI_testsViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet AI_testsViewController *viewController;

@end

