#import "OpenFeint.h"

#import "MyOpenFeintSampleAppDelegate.h"

#import "SampleOFDelegate.h"
#import "SampleOFNotificationDelegate.h"
#import "SampleOFChallengeDelegate.h"

#import "SampleRootController.h"

@implementation MyOpenFeintSampleAppDelegate

@synthesize window;
@synthesize rootController;

- (void)performApplicationStartupLogic
{
	SampleRootController* rootViewController = nil;
	NSArray* nibObjects = [[NSBundle mainBundle] loadNibNamed:@"SampleRootController" owner:nil options:nil];
	for (NSObject* obj in nibObjects)
	{
		if ([obj isKindOfClass:[SampleRootController class]])
		{
			rootViewController = (SampleRootController*)obj;
			break;
		}
	}
	
	[rootController pushViewController:rootViewController animated:NO];
	
    // Override point for customization after app launch    
    window.frame = [UIScreen mainScreen].bounds;
    [window addSubview:rootController.view];
    [window makeKeyAndVisible];
	
	NSDictionary* settings = [NSDictionary dictionaryWithObjectsAndKeys:
							  [NSNumber numberWithInt:UIInterfaceOrientationPortrait], OpenFeintSettingDashboardOrientation,
							  @"SampleApp", OpenFeintSettingShortDisplayName,
							  [NSNumber numberWithBool:YES], OpenFeintSettingEnablePushNotifications,
							  [NSNumber numberWithBool:NO], OpenFeintSettingDisableUserGeneratedContent,
  							  [NSNumber numberWithBool:NO], OpenFeintSettingAlwaysAskForApprovalInDebug,
							  window, OpenFeintSettingPresentationWindow,
							  nil
							  ];
							  
	ofDelegate = [SampleOFDelegate new];
	ofNotificationDelegate = [SampleOFNotificationDelegate new];
	ofChallengeDelegate = [SampleOFChallengeDelegate new];

	OFDelegatesContainer* delegates = [OFDelegatesContainer containerWithOpenFeintDelegate:ofDelegate
																	  andChallengeDelegate:ofChallengeDelegate
																   andNotificationDelegate:ofNotificationDelegate];

	[OpenFeint initializeWithProductKey:your_product_key_here 
							  andSecret:your_product_secret_here
						 andDisplayName:@"LongSampleAppName"
							andSettings:settings
						   andDelegates:delegates];
}

- (void)applicationDidFinishLaunching:(UIApplication *)application
{
	[self performApplicationStartupLogic];
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	[self performApplicationStartupLogic];
	[OpenFeint respondToApplicationLaunchOptions:launchOptions];
	return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
	[OpenFeint applicationDidBecomeActive];
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	[OpenFeint applicationWillResignActive];
}

- (void)applicationWillTerminate:(UIApplication*)application
{
	[OpenFeint shutdown];
}

- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken
{
	[OpenFeint applicationDidRegisterForRemoteNotificationsWithDeviceToken:deviceToken];
}

- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error
{
	[OpenFeint applicationDidFailToRegisterForRemoteNotifications];
}

- (void)application:(UIApplication *)application didReceiveRemoteNotification:(NSDictionary *)userInfo
{
	[OpenFeint applicationDidReceiveRemoteNotification:userInfo];
}

- (void)dealloc 
{
	[ofDelegate release];
	[ofNotificationDelegate release];
	[ofChallengeDelegate release];
    [rootController release];
    [window release];
    [super dealloc];
}


@end

