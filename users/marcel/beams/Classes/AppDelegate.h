#import <UIKit/UIKit.h>
#import "beams_forward_objc.h"
#import "AdMobDelegateProtocol.h"

@interface AppDelegate : NSObject <UIApplicationDelegate, AdMobDelegate> 
{
    UIWindow *window;
	UIViewController* rootController;
}

@property (nonatomic, retain) IBOutlet UIWindow *window;
@property (nonatomic, assign) UIViewController* rootController;

// AdMobDelegate
- (NSString *)publisherIdForAd:(AdMobView *)adView;
- (UIViewController *)currentViewControllerForAd:(AdMobView *)adView;

@end

