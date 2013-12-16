#import <UIKit/UIKit.h>

@interface AppDelegate : NSObject <UIApplicationDelegate> 
{
    UIWindow *window;
	UIViewController* rootController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;

@end

