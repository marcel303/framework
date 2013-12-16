//
//  toolbar_01AppDelegate.h
//  toolbar-01
//
//  Created by Marcel Smit on 01-04-10.
//  Copyright Apple Inc 2010. All rights reserved.
//

#import <UIKit/UIKit.h>

@class toolbar_01ViewController;

@interface toolbar_01AppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    toolbar_01ViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet toolbar_01ViewController *viewController;

@end

