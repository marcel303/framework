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

#pragma once

#import "OFChallengeToUser.h"
#import "OFImageView.h"


@interface OFChallengeDetailFrame : UIView
{
@private
	BOOL						hasChallengeToUserBeenSet;
	UILabel*					descriptionLabel;
	UILabel*					resultLabel;
	UILabel*					titleLabel;
	UIImageView*				resultIcon;
	UIImageView*				speechBubble;
	UIImageView*				speechBubbleArrow;
	UIImageView*				frameArrow;
	OFImageView*				challengeIconPictureView;
	OFImageView*				challengerProfilePictureView;
	UILabel*					challengerNameLabel;
	UILabel*					challengerUserMessageLabel;
}

@property (nonatomic, retain) IBOutlet UILabel*					descriptionLabel;
@property (nonatomic, retain) IBOutlet UILabel*					resultLabel;
@property (nonatomic, retain) IBOutlet UILabel*					titleLabel;
@property (nonatomic, retain) IBOutlet UIImageView*				resultIcon;
@property (nonatomic, retain) IBOutlet UIImageView*				speechBubble;
@property (nonatomic, retain) IBOutlet UIImageView*				speechBubbleArrow;
@property (nonatomic, retain) IBOutlet OFImageView*				challengeIconPictureView;
@property (nonatomic, retain) IBOutlet OFImageView*				challengerProfilePictureView;
@property (nonatomic, retain) IBOutlet UILabel*					challengerNameLabel;
@property (nonatomic, retain) IBOutlet UILabel*					challengerUserMessageLabel;

- (void)setChallengeToUser:(OFChallengeToUser*)newChallenge;

@end
