#import <AudioToolbox/AudioToolbox.h>
#import "AppDelegate.h"
#import "Calc.h"
#import "vwMain.h"
#import "OpenFeintHelper.h"
#import "State.h"

static AppDelegate* gMainApp = 0;

@implementation AppDelegate

@synthesize window;
@synthesize rootController;

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
#ifdef DEBUG
#if 0
	SetInt(@"app_count", 0);
	SetInt(@"feint_enabled", 0);
	SetInt(@"play_time", 0);
	SetInt(@"share_disabled", 0);
	SetInt(@"play_count", 0);
	SetInt(@"rating_disabled", 0);
#endif
#endif
	
	gMainApp = self;
	
	Calc::Initialize();
	
	int playCount = GetInt(@"app_count");
	SetInt(@"app_count", playCount + 1);
	
	//[application setStatusBarStyle:UIStatusBarStyleBlackOpaque];
	
	rootController = [[vwMain alloc] initWithNibName:@"vwMain" bundle:nil];
	
	[window addSubview:rootController.view];
    [window makeKeyAndVisible];
	
	openfeintEnabled = false;
	
	//
	
	if (GetInt(@"feint_enabled"))
	{
		[self openfeintBegin];
	}
	
	return YES;
}

+(AppDelegate*)mainApp
{
	return gMainApp;
}

-(void)openfeintBegin
{
	[OpenFeint initializeWithProductKey:@"4ktY3w0bsKB6CE6PbYmM1w"
		andSecret:@"Utb3UIb1rtQxnGF99UH5nnnIRKewU6CWgdb9rFZBo5E"
		andDisplayName:@"puzzle"
		andSettings:[NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithInt:UIInterfaceOrientationPortrait], OpenFeintSettingDashboardOrientation, [NSNumber numberWithBool:YES], OpenFeintSettingInvertNotifications, nil] //see OpenFeintSettings.h
		andDelegates:[OFDelegatesContainer containerWithOpenFeintDelegate:self]]; // see OFDelegatesContainer.h
	
	openfeintEnabled = true;
}

-(void)openfeintEnd
{
	[OpenFeint shutdown];
	
	openfeintEnabled = false;
}

-(bool)openfeintEnabled
{
	return openfeintEnabled;
}

-(void)applicationDidBecomeActive:(UIApplication *)application
{
	[OpenFeint applicationDidBecomeActive];
	
	[self playTimeBegin];
}

-(void)applicationWillResignActive:(UIApplication *)application
{
	[OpenFeint applicationWillResignActive];
	
	[self playTimeEnd];
}

-(void)applicationWillEnterForeground:(UIApplication *)application 
{
	[OpenFeint applicationWillEnterForeground];
	
	[self playTimeBegin];
}

-(void)applicationDidEnterBackground:(UIApplication *)application 
{
	[OpenFeint applicationDidEnterBackground];
	
	[self playTimeEnd];
}

-(void)playTimeBegin
{
	LOG_DBG("playTime: begin", 0);
	
	playTime1 = time(&playTime1);
	
	int playTime = GetInt(@"play_time");
	[OpenFeintHelper achievePlayTime:playTime];
}

-(void)playTimeEnd
{
	LOG_DBG("playTime: end", 0);
	
	playTime2 = time(&playTime2);
	
	double diff = difftime(playTime2, playTime1);
	
	if (diff > 0)
	{
		int delta = (int)diff;
		
		int playTime = GetInt(@"play_time");
		playTime += delta;
		SetInt(@"play_time", playTime);
		
		LOG_DBG("playTime: update: %d", playTime);
	}
	
	playTime1 = playTime2;
}

+(void)playSound:(NSString*)fileName
{
	NSString* path = [[NSBundle mainBundle] pathForResource:fileName ofType:@"wav"];

	SystemSoundID soundId;

	NSURL* url = [NSURL fileURLWithPath:path isDirectory:NO];

	AudioServicesCreateSystemSoundID((CFURLRef)url, &soundId);

	AudioServicesPlaySystemSound(soundId);
}

-(void)feintEnable
{
	SetInt(@"feint_enabled", 1);
	
	[self openfeintBegin];
	
}

-(void)feintDisable
{
	SetInt(@"feint_enabled", 0);
	
	[self openfeintEnd];
}

-(void)dealloc 
{
    [window release];
    [super dealloc];
}

@end
