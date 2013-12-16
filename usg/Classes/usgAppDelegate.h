#import <UIKit/UIKit.h>
#import "GameViewController.h"

@interface usgAppDelegate : NSObject <UIApplicationDelegate>
{
	UIWindow* window;
	GameViewController* controller;
}

@property (nonatomic, retain) IBOutlet UIWindow* window;
@property (nonatomic, retain) IBOutlet GameViewController* controller;

- (void)freeResources;

@end
