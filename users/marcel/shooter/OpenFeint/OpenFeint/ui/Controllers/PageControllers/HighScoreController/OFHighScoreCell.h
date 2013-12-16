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

#import "OFTableCellHelper.h"

@class OFImageView;
@class OFUserRelationshipIndicator;

@interface OFHighScoreCell : OFTableCellHelper 
{
	OFImageView* profilePictureView;
	UILabel* nameLabel;
	UILabel* distLabel;
	UILabel* scoreLabel;
    UIImageView *disclosureIndicator;
	UIButton* downloadBlobButton;
	OFUserRelationshipIndicator* relationshipIndicator;
}

@property (nonatomic, retain) IBOutlet UIImageView *disclosureIndicator;
@property (nonatomic, retain) IBOutlet UIButton* downloadBlobButton;
@property (nonatomic, retain) IBOutlet OFImageView* profilePictureView;
@property (nonatomic, retain) IBOutlet UILabel* nameLabel;
@property (nonatomic, retain) IBOutlet UILabel* distLabel;
@property (nonatomic, retain) IBOutlet UILabel* scoreLabel;
@property (nonatomic, retain) IBOutlet OFUserRelationshipIndicator* relationshipIndicator;

- (void)onResourceChanged:(OFResource*)resource;
- (IBAction)downloadBlobClicked;

@end
