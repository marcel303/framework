#import <UIKit/UIKit.h>

@class exgen_01ViewController;

@interface exgen_01AppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    exgen_01ViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet exgen_01ViewController *viewController;

@end

