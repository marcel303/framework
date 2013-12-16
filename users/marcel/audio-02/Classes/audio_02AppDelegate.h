#import <UIKit/UIKit.h>
#import "MainController.h"

@interface audio_02AppDelegate : NSObject <UIApplicationDelegate> {
    UIWindow *window;
	MainController* controller;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet MainController *controller;

@end

