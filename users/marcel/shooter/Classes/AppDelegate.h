#import <UIKit/UIKit.h>
#import "GameCenterManager.h"
#import "OpenFeintDelegate.h"

@class AppView;
@class AppViewController;
@class HelpView;
@class PauseView;

@interface shooterAppDelegate : NSObject <UIApplicationDelegate, OpenFeintDelegate, GameCenterManagerDelegate>
{
    UIWindow* window;
	UIView* currentView;
	AppViewController* viewController;
	GameCenterManager* gameCenterMgr;
	AppView* appView;
	HelpView* helpView;
	PauseView* pauseView;
}

@property (nonatomic, retain) IBOutlet UIWindow* window;
@property (nonatomic, assign) AppViewController* viewController;
@property (nonatomic, assign) GameCenterManager* gameCenterMgr;
@property (nonatomic, assign) AppView* appView;
@property (nonatomic, assign) HelpView* helpView;
@property (nonatomic, assign) PauseView* pauseView;

+(shooterAppDelegate*)defaultDelegate;

-(void)openfeintBegin;
-(void)showView:(UIView*)view;

@end
