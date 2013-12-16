#import "Calc.h"
#import "ConfigState.h"
#import "GameView.h"
#import "Log.h"
#import "PerfCount.h"
#import "Random.h"
#import "usgAppDelegate.h"
#import "Util_ColorEx.h"

@implementation usgAppDelegate

@synthesize window;
@synthesize controller;

- (void)applicationDidFinishLaunching:(UIApplication*)application
{    
	[NSThread setThreadPriority:1.0];
	
	//
	
	srand(time(NULL));
	
	Calc::Initialize();
	Calc::Initialize_Color();
	
	g_PerfCount.Create(PC_RENDER, "render", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_MAKECURRENT, "render.makecurrent", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_SETUP, "render.setup", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_PRESENT, "render.present", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_BACKGROUND, "render.bg", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_INTERFACE, "render.interface", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_PRIMARY, "render.primary", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_PARTICLES, "render.particles", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_SHADOWS, "render.shadows", PerfType_Time);
	g_PerfCount.Create(PC_RENDER_FLUSH, "render.flush", PerfType_Count);
	g_PerfCount.Create(PC_UPDATE, "update", PerfType_Time);
	g_PerfCount.Create(PC_UPDATE_SB_CLEAR, "update.sb.clear", PerfType_Time);
	g_PerfCount.Create(PC_UPDATE_SB_DRAW, "update.sb.draw", PerfType_Time);
	g_PerfCount.Create(PC_UPDATE_SB_DRAW_SCAN, "update.sb.draw.scan", PerfType_Accum);
	g_PerfCount.Create(PC_UPDATE_SB_DRAW_FILL, "update.sb.draw.fill", PerfType_Accum);
	g_PerfCount.Create(PC_UPDATE_LOGIC, "update.logic", PerfType_Time);
	g_PerfCount.Create(PC_UPDATE_HITTEST, "update.hittest", PerfType_Time);
	g_PerfCount.Create(PC_UPDATE_POST, "update.post", PerfType_Time);
	
	//
	
	[application setStatusBarHidden:TRUE];
	[application setIdleTimerDisabled:TRUE];
	
	//
	
	GameView* gameView = [[GameView alloc] initWithFrame:window.frame];
	
	self.controller = [[GameViewController alloc] initWithGameView:gameView];

	[gameView release];
	
	[self.window setRootViewController:controller];
	
	//[gameView initForReal];
	
	//

	[self.window makeKeyAndVisible];
	
	//
	
	[[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeRight animated:NO];
//	[[UIApplication sharedApplication] setStatusBarOrientation:UIInterfaceOrientationLandscapeLeft animated:NO];
}

- (void)dealloc
{
	LOG(LogLevel_Info, "application dealloc");
	
	[super dealloc];
}

- (void)freeResources
{
	LOG(LogLevel_Info, "application free resources");
	
	[controller release];
	controller = nil;
	
	[window release];
	window = nil;
}

- (void)applicationDidBecomeActive:(UIApplication*)application
{
	if (!g_GameView)
		return;
	
	LOG(LogLevel_Info, "application did become active");
	
	[g_GameView resumeApp];
}

- (void)applicationWillResignActive:(UIApplication*)application
{
	if (!g_GameView)
		return;
	
	LOG(LogLevel_Info, "application will resign active");
	
	[g_GameView pauseApp];
}

- (void)applicationDidEnterBackground:(UIApplication *)application 
{
	if (!g_GameView)
		return;
	
	LOG(LogLevel_Info, "application did enter background");
	
	[g_GameView pauseApp];

	// "In iOS 4.0 and later, this method is called instead of the applicationWillTerminate: method
	// when the user quits an application that supports background execution."
	// - Which means we should save here.. because we may as well get killed..
	ConfigSave(true);
}

- (void)applicationWillEnterForeground:(UIApplication *)application 
{
	if (!g_GameView)
		return;
	
	LOG(LogLevel_Info, "application did enter foreground");
	
	//[g_GameView resumeApp];
}

- (void)applicationWillTerminate:(UIApplication*)application
{
	LOG(LogLevel_Info, "application will terminate");
	
	ConfigSave(true);
	
	NSLog(@"controller.retainCount: %d", [controller retainCount]);
	NSLog(@"window.retainCount: %d", [window retainCount]);
	NSLog(@"gameView.retainCount: %d", [controller.gameView retainCount]);
	
	[self freeResources];
	
	NSLog(@"controller.retainCount: %d", [controller retainCount]);
	NSLog(@"window.retainCount: %d", [window retainCount]);
	NSLog(@"gameView.retainCount: %d", [controller.gameView retainCount]);
}

@end
