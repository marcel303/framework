#pragma once

#import <UIKit/UIKit.h>
#import "OFCallbackable.h"
#import "OFChallenge.h"
#import "OFChallengeDefinition.h"

@class OFChallengeDefinition;
@class OFDefaultTextField;

@interface SendSampleChallengeController : UIViewController<OFCallbackable, UIPickerViewDataSource, UIPickerViewDelegate, OFChallengeSendDelegate, OFChallengeDefinitionDelegate>
{
	IBOutlet UIPickerView* challengePicker;
	
	IBOutlet OFDefaultTextField* challengeText;

	IBOutlet OFDefaultTextField* challengeData;
	
	IBOutlet UILabel* newChallengesLabel;
	NSMutableArray* challengeDefs;
	OFChallengeDefinition* selectedChallenge;
}

- (IBAction)sendChallenge;

- (bool)canReceiveCallbacksNow;

@end
