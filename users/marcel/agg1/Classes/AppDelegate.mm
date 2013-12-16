#import "AppDelegate.h"
#import "TestView.h"

@implementation agg1AppDelegate

@synthesize window;

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Override point for customization after application launch
	
    [window makeKeyAndVisible];
	
	TestView* view = [[[TestView alloc] initWithFrame:[UIScreen mainScreen].applicationFrame] autorelease];
	
	[window addSubview:view];
	
	return YES;
}

-(void)dealloc 
{
    [window release];
    [super dealloc];
}

@end
