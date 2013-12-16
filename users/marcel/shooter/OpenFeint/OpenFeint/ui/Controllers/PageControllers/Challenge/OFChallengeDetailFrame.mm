//  Copyright 2009-2010 Aurora Feint, Inc.
// 
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//  
//  	http://www.apache.org/licenses/LICENSE-2.0
//  	
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.

#import "OFDependencies.h"
#import "OFChallengeDetailFrame.h"
#import "OFChallenge.h"
#import	"OFChallengeDefinition.h"
#import "OFUser.h"
#import "OFChallengeToUser.h"
#import "OFImageLoader.h"

@implementation OFChallengeDetailFrame

@synthesize descriptionLabel, resultLabel, titleLabel, resultIcon, speechBubble, speechBubbleArrow;
@synthesize challengeIconPictureView, challengerProfilePictureView, challengerNameLabel, challengerUserMessageLabel;

- (void)awakeFromNib
{
	[super awakeFromNib];
	
	self.speechBubble.image = [speechBubble.image stretchableImageWithLeftCapWidth:8 topCapHeight:16];
	CGRect arrowFrame = self.speechBubbleArrow.frame;
	self.speechBubbleArrow.frame = arrowFrame;
	self.speechBubbleArrow.transform = CGAffineTransformMake(-1.f, 0.f, 0.f, -1.f, 0.f, 0.f);
	
	if (!hasChallengeToUserBeenSet)
	{
		self.descriptionLabel.text = @"";
		self.resultLabel.text = @"";
		self.titleLabel.text = @"";
		self.challengerNameLabel.text = @"From ";
		self.challengerUserMessageLabel.text = @"";
	}
}

- (void)dealloc
{
	self.descriptionLabel = nil;
	self.resultLabel = nil;
	self.titleLabel = nil;
	self.resultIcon = nil;
	self.challengeIconPictureView = nil;
	self.challengerProfilePictureView = nil;
	self.challengerNameLabel = nil;
	self.challengerUserMessageLabel = nil;
	self.speechBubble = nil;
	self.speechBubbleArrow = nil;
	[super dealloc];
}

- (void)setChallengeToUser:(OFChallengeToUser*)newChallenge
{
	hasChallengeToUserBeenSet = YES;
	
	[self.challengeIconPictureView setDefaultImage:[OFImageLoader loadImage:@"OFDefaultChallengeIcon.png"]];
	self.challengeIconPictureView.imageUrl = newChallenge.challenge.challengeDefinition.iconUrl;
	
	self.titleLabel.text = newChallenge.challenge.challengeDefinition.title;
	
	self.descriptionLabel.text = newChallenge.challenge.challengeDescription;
	
	self.resultLabel.text = newChallenge.formattedResultDescription;
	self.resultLabel.hidden = NO;
	self.resultIcon.hidden = NO;

	NSString* challengeResultIconName = [OFChallengeToUser getChallengeResultIconName:newChallenge.result];
	self.resultIcon.image = challengeResultIconName ? [OFImageLoader loadImage:challengeResultIconName] : nil;
	
	self.challengerNameLabel.text = newChallenge.challenge.challenger.name;
	
	self.challengerUserMessageLabel.text = newChallenge.challenge.userMessage;
	
	[self.challengerProfilePictureView useProfilePictureFromUser:newChallenge.challenge.challenger];
}

@end
