#import "SampleOFDelegate.h"
#import "OpenFeint+UserOptions.h"

@implementation SampleOFDelegate

- (void)dashboardWillAppear
{
}

- (void)dashboardDidAppear
{
}

- (void)dashboardWillDisappear
{
}

- (void)dashboardDidDisappear
{
}

- (void)offlineUserLoggedIn:(NSString*)userId
{
	NSLog(@"User logged in, but OFFLINE. UserId: %@", userId);
}

- (void)userLoggedIn:(NSString*)userId
{
	NSLog(@"User logged in. UserId: %@", userId);
}

- (BOOL)showCustomOpenFeintApprovalScreen
{
	return NO;
}

@end
