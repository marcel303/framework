#pragma once

#import "OFChallengeDelegate.h"

@interface SampleOFChallengeDelegate : NSObject< OFChallengeDelegate >

- (void)userLaunchedChallenge:(OFChallengeToUser*)challengeToLaunch withChallengeData:(NSData*)challengeData;
- (void)userRestartedChallenge;

@end
