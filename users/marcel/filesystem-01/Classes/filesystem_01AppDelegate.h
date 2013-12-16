#import <UIKit/UIKit.h>

@class filesystem_01ViewController;

@interface filesystem_01AppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
    filesystem_01ViewController *viewController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet filesystem_01ViewController *viewController;

@end

