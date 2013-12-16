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

#import <UIKit/UIKit.h>
#import "OFCallbackable.h"
#import "OFDefaultTextField.h"
#import "OFLeaderboard.h"

@interface SampleLeaderboardListController : UIViewController<OFCallbackable, UIPickerViewDataSource, UIPickerViewDelegate, OFLeaderboardDelegate, OFHighScoreDelegate>
{
	UIPickerView* leaderboardPicker;
	OFDefaultTextField* highScoreTextField;
	OFDefaultTextField* displayTextTextField;
	OFDefaultTextField* customDataTextField;
	OFDefaultTextField* blobTextField;
	UIButton* submitButton;
	UIButton* batchButton;
	NSMutableArray* leaderboards;
	OFLeaderboard* selectedLeaderboard;
}

@property (nonatomic, retain) IBOutlet UIPickerView* leaderboardPicker;
@property (nonatomic, retain) IBOutlet OFDefaultTextField* highScoreTextField;
@property (nonatomic, retain) IBOutlet OFDefaultTextField* displayTextTextField;
@property (nonatomic, retain) IBOutlet OFDefaultTextField* customDataTextField;
@property (nonatomic, retain) IBOutlet OFDefaultTextField* blobTextField;
@property (nonatomic, retain) IBOutlet UIButton* submitButton;
@property (nonatomic, retain) IBOutlet UIButton* batchButton;

- (IBAction)setHighScore;
- (IBAction)openLeaderboard;
- (bool)canReceiveCallbacksNow;

- (IBAction)closeKeyboard;
- (IBAction)enableCloseKbdButton;
- (IBAction)disableCloseKbdButton;

@end
