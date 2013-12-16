//
//  abcAppDelegate.h
//  abc
//
//  Created by Marcel Smit on 19-12-09.
//  Copyright __MyCompanyName__ 2009. All rights reserved.
//

#import <UIKit/UIKit.h>

@class abcViewController;

@interface abcAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    abcViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet abcViewController *viewController;

@end

