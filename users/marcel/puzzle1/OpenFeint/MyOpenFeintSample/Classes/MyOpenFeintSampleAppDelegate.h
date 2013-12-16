#import <UIKit/UIKit.h>

@class SampleOFDelegate;
@class SampleOFNotificationDelegate;
@class SampleOFChallengeDelegate;

@interface MyOpenFeintSampleAppDelegate : NSObject <UIApplicationDelegate> 
{
    UIWindow *window;
    UINavigationController *rootController;
	SampleOFDelegate *ofDelegate;
	SampleOFNotificationDelegate *ofNotificationDelegate;
	SampleOFChallengeDelegate *ofChallengeDelegate;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, retain) IBOutlet UINavigationController *rootController;

@end

