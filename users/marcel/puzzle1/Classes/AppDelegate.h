#import <UIKit/UIKit.h>
#import "OpenFeint.h"

@interface AppDelegate : NSObject <UIApplicationDelegate, OpenFeintDelegate>
{
    UIWindow* window;
	UIViewController* rootController;
	time_t playTime1;
	time_t playTime2;
	bool openfeintEnabled;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) UIViewController* rootController;

+(AppDelegate*)mainApp;

-(void)openfeintBegin;
-(void)openfeintEnd;
-(bool)openfeintEnabled;
+(void)playSound:(NSString*)fileName;
-(void)playTimeBegin;
-(void)playTimeEnd;
-(void)feintEnable;
-(void)feintDisable;

@end
