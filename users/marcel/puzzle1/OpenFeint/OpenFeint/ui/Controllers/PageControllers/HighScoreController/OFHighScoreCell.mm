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
#import "OFHighScoreCell.h"
#import "OFViewHelper.h"
#import "OFHighScore.h"
#import "OFUser.h"
#import "OFImageView.h"
#import "OFStringUtility.h"
#import "OpenFeint.h"
#import "OpenFeint+UserOptions.h"
#import "OFStringUtility.h"
#import "OFUserRelationshipIndicator.h"
#import "OFHighScoreController.h"
#import "OFGameProfilePageInfo.h"

const double kmsPerMile = 1.0/.621371;

@implementation OFHighScoreCell

@synthesize disclosureIndicator;
@synthesize downloadBlobButton;
@synthesize profilePictureView;
@synthesize nameLabel;
@synthesize distLabel;
@synthesize scoreLabel;
@synthesize relationshipIndicator;

- (BOOL)hasConstantHeight
{
	return YES;
}

- (void)onResourceChanged:(OFResource*)resource
{
	OFHighScore* highScore = (OFHighScore*)resource;
		
	
	[profilePictureView useProfilePictureFromUser:highScore.user];
	
	if ([OpenFeint isOnline])
	{
		NSString* scoreText = [NSString stringWithFormat:@"%@", OFStringUtility::convertFromValidParameter(highScore.displayText).get()];
		if (highScore.rank == -1)
		{
			if (highScore.score == 0)
			{
				nameLabel.text = highScore.user.name;
				scoreLabel.text = @"Not Ranked";
			}
			else 
			{
				nameLabel.text = [NSString stringWithFormat:@"%@ %@", highScore.toHighRankText, highScore.user.name];
				scoreLabel.text = scoreText;
			}
		}
		else
		{
			nameLabel.text = [NSString stringWithFormat:@"%d. %@", highScore.rank, highScore.user.name];
			scoreLabel.text = scoreText;
		}

		if (highScore.distance)
		{
			OFUserDistanceUnitType unit = [OpenFeint userDistanceUnit];
			double dist = highScore.distance * (unit == kDistanceUnitMiles ? 1 : kmsPerMile);
			NSString* unitText = (unit == kDistanceUnitMiles ? @"miles" : @"kms"); 
			distLabel.text = [NSString stringWithFormat:@"%.2f %@ away", dist, unitText];
		}
		else
		{
			distLabel.text = nil;
		}
		
		[relationshipIndicator setUser:highScore.user];
		if (disclosureIndicator)
		{
			disclosureIndicator.hidden = NO;
		}
	}
	else
	{
		NSString* userName = highScore.user ? highScore.user.name : @"Unregistered User";
		nameLabel.text = [NSString stringWithFormat:@"%d. %@", highScore.rank, userName];
		scoreLabel.text = [NSString stringWithFormat:@"%@", OFStringUtility::convertFromValidParameter(highScore.displayText).get()];
		distLabel.text = nil;
		if (disclosureIndicator)
		{
			disclosureIndicator.hidden = YES;
		}
	}
	
	// Best guess way of handling overloaded high score cells although it's not guaranteed to work.
	bool allowBlobs = disclosureIndicator && downloadBlobButton;
	if (allowBlobs && [self.owningTable isKindOfClass:[OFHighScoreController class]])
	{
		OFHighScoreController* highScoreController = (OFHighScoreController*)self.owningTable;
		if (highScoreController.gameProfileInfo && ![highScoreController.gameProfileInfo.resourceId isEqualToString:[OpenFeint localGameProfileInfo].resourceId])
		{
			allowBlobs = false;
		}
	}
	
	if (allowBlobs)
	{
		const float kLastObjectStartX = [highScore hasBlob] ? downloadBlobButton.frame.origin.x : disclosureIndicator.frame.origin.x;
		const float textEndX = kLastObjectStartX - 10.f;
		CGRect textRect = scoreLabel.frame;
		textRect.size.width = textEndX - textRect.origin.x;
		if (textRect.size.width != scoreLabel.frame.size.width)
		{
			scoreLabel.frame = textRect;
		}
	}
	
	downloadBlobButton.hidden = !allowBlobs || ![highScore hasBlob];
	
	if (nil == distLabel.text)
	{
		distLabel.hidden = YES;
		CGRect nameFrame = nameLabel.frame;
		nameFrame.origin.y = (self.frame.size.height - nameFrame.size.height) / 2.0;
		nameLabel.frame = nameFrame;
	}
	else
	{
		distLabel.hidden = NO;
		CGRect nameFrame = nameLabel.frame;
		nameFrame.origin.y = (self.frame.size.height - nameFrame.size.height) / 3.0;
		nameLabel.frame = nameFrame;
		CGRect distFrame = distLabel.frame;
		distFrame.origin.y = (self.frame.size.height - distFrame.size.height) * 2.5 / 3.0;
		distLabel.frame = distFrame;
	}
}

- (void)awakeFromNib
{
	if (![OpenFeint isOnline])
	{
		self.selectionStyle = UITableViewCellSelectionStyleNone;
	}
	[super awakeFromNib];
}

- (IBAction)downloadBlobClicked
{
	OFHighScoreController* highScoreController = (OFHighScoreController*)self.owningTable;
	if ([highScoreController respondsToSelector:@selector(downloadBlobForHighScore:)])
	{
		[highScoreController downloadBlobForHighScore:(OFHighScore*)self.resource];
	}
}

- (void)dealloc
{
    self.disclosureIndicator = nil;
	self.downloadBlobButton = nil;
	self.profilePictureView = nil;
	self.nameLabel = nil;
	self.distLabel = nil;
	self.scoreLabel = nil;
	self.relationshipIndicator = nil;
    [super dealloc];
}


@end
