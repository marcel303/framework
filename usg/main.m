#import <UIKit/UIKit.h>
//#import "usgAppDelegate.h"

int main(int argc, char *argv[]) {
#if 1
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
    int retVal = UIApplicationMain(argc, argv, nil, nil);
    [pool release];
    return retVal;
#else
	usgAppDelegate* delegate = [[usgAppDelegate alloc] init];
	
	[UIApplication sharedApplication].delegate = delegate;
	
	[delegate applicationDidFinishLaunching:application];
	
	[delegate applicationDidBecomeActive:application];
	
	bool isRunning = true;
	
	while (isRunning)
	{
		NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];
		
		// process messages
		while (CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.002, FALSE) == kCFRunLoopRunHandledSource);
		
		[pool release];
	}
	
	[delegate applicationWillTerminate:application];
	
	[delegate release];
#endif
}
