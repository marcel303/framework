//
//  crapbuttonAppDelegate.h
//  crapbutton
//
//  Created by Marcel Smit on 21-05-10.
//  Copyright Apple Inc 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

@class crapbuttonViewController;

@interface crapbuttonAppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    crapbuttonViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet crapbuttonViewController *viewController;

@end

