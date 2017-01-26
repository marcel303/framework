#if BUILD_FACEBOOK

#import <Foundation/Foundation.h>
#import "FBConnect/FBConnect.h"

@interface FacebookState : NSObject <FBSessionDelegate, FBRequestDelegate>
{
	FBSession* facebookSession;
	id loginDelegate;
	SEL loginAction;
}

-(void)resume;
-(bool)loggedIntoFacebook;
-(void)logIntoFacebookWithDelegate:(id)delegate action:(SEL)action;

// FBRequestDelegate
-(void)request:(FBRequest*)request didLoad:(id)result;
-(void)request:(FBRequest*)request didFailWithError:(NSError*)error;

@end

#endif
