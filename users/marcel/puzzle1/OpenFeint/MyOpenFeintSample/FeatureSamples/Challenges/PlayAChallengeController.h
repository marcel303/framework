#pragma once

#import <UIKit/UIKit.h>
#import "OFCallbackable.h"
#import "OFChallengeToUser.h"

@class OFChallengeToUser;
@class OFDefaultTextField;

@interface PlayAChallengeController : UIViewController<OFChallengeToUserDelegate>
{
	IBOutlet UILabel* challengeTextLabel;
	IBOutlet UILabel* userMessageLabel;
	IBOutlet UISegmentedControl* result;
	IBOutlet OFDefaultTextField* resultDescription;
    
    IBOutlet UILabel* challengeDataLabel;

	OFChallengeToUser* challenge;
	NSData* data;
}

- (void)setChallenge:(OFChallengeToUser*)_challenge;
- (void)setData:(NSData*)_data;

- (IBAction)completeChallenge;
- (IBAction)rejectChallenge;
- (IBAction) writeToFile;

@end
