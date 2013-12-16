#import "AppDelegate.h"
#import "AppView.h"
#import "AppViewController.h"
#import "HelpView.h"
#import "Log.h"
#import "OpenFeint.h"
#import "PauseView.h"
#import "render_iphone.h"

shooterAppDelegate* gDefaultDelegate = 0;

@implementation shooterAppDelegate

@synthesize window;
@synthesize viewController;
@synthesize gameCenterMgr;
@synthesize appView;
@synthesize helpView;
@synthesize pauseView;

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary*)launchOptions
{
	gDefaultDelegate = self;
	
	[application setStatusBarOrientation:UIInterfaceOrientationLandscapeRight animated:NO];
	[application setIdleTimerDisabled:TRUE];
	
	viewController = [[AppViewController alloc] initWithNibName:nil bundle:nil];
	window.rootViewController = viewController;
	[window addSubview:viewController.view];
	
	gameCenterMgr = [[GameCenterManager alloc] init];
	gameCenterMgr.delegate = self;
	
	appView = [[AppView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame];
	helpView = [[HelpView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame];
	pauseView = [[PauseView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame];

	[self showView:appView];
	
    [window makeKeyAndVisible];
	
	[self openfeintBegin];
	
	gTiltSensitivity = GetInt(@"tilt_sensitivity") / 100.0f;
	if (gTiltSensitivity <= 0.0f)
		gTiltSensitivity = 1.0f;
	
	return YES;
}

+(shooterAppDelegate*)defaultDelegate
{
	return gDefaultDelegate;
}

-(void)openfeintBegin
{
	[OpenFeint initializeWithProductKey:@"rqzVjpa77XKyRPJA63pIPQ"
		andSecret:@"RevJehm9qXL0FP9dy9bnoW3rOi5viTvU2A2YgpGlc"
		andDisplayName:@"shooter"
							andSettings:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:UIInterfaceOrientationLandscapeRight], OpenFeintSettingDashboardOrientation, nil] //see OpenFeintSettings.h
		andDelegates:[OFDelegatesContainer containerWithOpenFeintDelegate:self]];              // see OFDelegatesContainer.h
}

-(void)showView:(UIView*)view
{
//	[viewController.view removeFromSuperview];
	viewController.view = view;
//	[window addSubview:viewController.view];
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	[OpenFeint applicationDidBecomeActive];
	
	[appView resume];
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	[OpenFeint applicationWillResignActive];
	
	[appView pause];
}

- (void)applicationDidEnterBackground:(UIApplication *)application 
{
	[OpenFeint applicationDidEnterBackground];
	
	[appView pause];
}

- (void)applicationWillEnterForeground:(UIApplication *)application 
{
	[OpenFeint applicationWillEnterForeground];
	
	[appView resume];
}

-(void)dashboardWillAppear
{
	LOG_DBG("dashboardWillAppear", 0);
	
	[appView pause];
}

-(void)dashboardWillDisappear
{
	LOG_DBG("dashboardWillDisappear", 0);
	
	[appView resume];
}

- (void) processGameCenterAuth: (NSError*) error
{
}

- (void) scoreReported: (NSError*) error
{
	
}

- (void) reloadScoresComplete: (GKLeaderboard*) leaderBoard error: (NSError*) error
{
	for (GKScore* score in leaderBoard.scores)
	{
		LOG_INF("score: %u", (uint32_t)score.value);
	}
}

- (void) achievementSubmitted: (GKAchievement*) ach error:(NSError*) error
{
	
}

- (void) achievementResetResult: (NSError*) error
{
	
}

- (void) mappedPlayerIDToPlayer: (GKPlayer*) player error: (NSError*) error
{
	
}

-(void)dealloc
{
	[appView release];
	[helpView release];
	[pauseView release];
    [window release];
	[gameCenterMgr release];
	[viewController release];
    [super dealloc];
}

@end
