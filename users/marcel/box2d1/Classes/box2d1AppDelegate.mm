#import "box2d1AppDelegate.h"
#import "BoxView.h"
#import "Calc.h"

@implementation box2d1AppDelegate

@synthesize window;


-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    

    // Override point for customization after application launch
	
	Calc::Initialize();
	
	BoxView* view = [[[BoxView alloc] initWithFrame:window.frame] autorelease];
	
	[window addSubview:view];
	
    [window makeKeyAndVisible];
	
	return YES;
}

-(void)dealloc 
{
    [window release];
    [super dealloc];
}

@end
