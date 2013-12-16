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

#import "SampleLeaderboardListController.h"
#import "OpenFeint.h"
#import "OFHighScoreService.h"
#import "OFLeaderboardService.h"
#import "OFLeaderboard.h"
#import "OFViewHelper.h"
#import "OpenFeint+Dashboard.h"
#import "OFControllerLoader.h"
#import "SampleLeaderboardViewController.h"
#import "OFLeaderboard.h"


@implementation SampleLeaderboardListController

@synthesize leaderboardPicker, highScoreTextField, displayTextTextField, customDataTextField, blobTextField, submitButton, batchButton;

- (void)closeKeyboard
{
	[self.highScoreTextField resignFirstResponder];
	[self.displayTextTextField resignFirstResponder];
	[self.customDataTextField resignFirstResponder];
	[self.blobTextField resignFirstResponder];
}

- (void)disableCloseKbdButton
{
	[self.navigationItem.rightBarButtonItem setEnabled:false];
}

- (void)enableCloseKbdButton
{
	[self.navigationItem.rightBarButtonItem setEnabled:true];
}

- (void)awakeFromNib
{
	self.highScoreTextField.manageScrollViewOnFocus = NO;
	self.highScoreTextField.closeKeyboardOnReturn = YES;
	
	self.displayTextTextField.manageScrollViewOnFocus = NO;
	self.displayTextTextField.closeKeyboardOnReturn = YES;

	self.customDataTextField.manageScrollViewOnFocus = NO;
	self.customDataTextField.closeKeyboardOnReturn = YES;
	
	self.blobTextField.manageScrollViewOnFocus = NO;
	self.blobTextField.closeKeyboardOnReturn = YES;
	
	self.leaderboardPicker.delegate = self;
	self.leaderboardPicker.dataSource = self;
	
	[self disableCloseKbdButton];
}

- (void)viewWillAppear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardListController viewWillAppear");
	[super viewWillAppear:animated];
	
	UIScrollView* scrollView = OFViewHelper::findFirstScrollView(self.view);
	scrollView.contentSize = OFViewHelper::sizeThatFitsTight(scrollView);
	
	// Get ready to work with leaderboards.
	leaderboards = [[OFLeaderboard leaderboards] retain];
	[self.leaderboardPicker reloadAllComponents];
	[self pickerView:self.leaderboardPicker  didSelectRow:[self.leaderboardPicker selectedRowInComponent:0] inComponent:0];
}

- (void)viewDidAppear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardListController viewDidAppear");
	[OFHighScore setDelegate:self];
	[OFLeaderboard setDelegate:self];
	[super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardListController viewWillDisappear");
	[super viewWillDisappear:animated];
	
	// This class must relinquish it's role as leaderboard delegate.
	[OFLeaderboard setDelegate:nil];
	[OFHighScore setDelegate:nil];
}

- (void)viewDidDisappear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardListController viewDidDisappear");
	[super viewDidDisappear:animated];
}

- (IBAction)setHighScore
{
	if (selectedLeaderboard)
	{
		//create a new high score to submit.
		OFHighScore* newHighScore = [[[OFHighScore alloc] initForSubmissionWithScore:[self.highScoreTextField.text longLongValue]] autorelease];
		newHighScore.displayText = self.displayTextTextField.text;
		newHighScore.customData = nil;
		
		//Submit the high score to the leaderboard.
		[newHighScore submitTo:selectedLeaderboard];
	}
	[self closeKeyboard];
}

- (IBAction)openLeaderboard
{
	if (selectedLeaderboard)
	{
		[self closeKeyboard];

		SampleLeaderboardViewController* next = (SampleLeaderboardViewController*)OFControllerLoader::load(@"SampleLeaderboardView");
		next.leaderboard = selectedLeaderboard;
		[[self navigationController] pushViewController:next animated:YES];
	}
}

- (bool)canReceiveCallbacksNow
{
	return YES;
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
	const unsigned int numOrientations = 4;
	UIInterfaceOrientation myOrientations[numOrientations] = { UIInterfaceOrientationPortrait, UIInterfaceOrientationLandscapeLeft, UIInterfaceOrientationLandscapeRight, UIInterfaceOrientationPortraitUpsideDown };
	return [OpenFeint shouldAutorotateToInterfaceOrientation:interfaceOrientation withSupportedOrientations:myOrientations andCount:numOrientations];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	[OpenFeint setDashboardOrientation:self.interfaceOrientation];
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
}

- (void)dealloc
{
	self.leaderboardPicker = nil;
	self.highScoreTextField = nil;
	self.displayTextTextField = nil;
	self.customDataTextField = nil;
	self.blobTextField = nil;
	self.submitButton = nil;
	self.batchButton = nil;
	OFSafeRelease(leaderboards);
	[super dealloc];
}

#pragma mark OFLeaderboardAPIDelegate



#pragma mark UIPickerViewDataSource / UIPickerViewDelegate

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
	if (leaderboards)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
	if (leaderboards)
	{
		return [leaderboards count];
	}
	else
	{
		return 0;
	}
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
	if (leaderboards)
	{
		OFLeaderboard* curLeaderboard = (OFLeaderboard*)[leaderboards objectAtIndex:row];
		return curLeaderboard.name;
	}
	else
	{
		return @"";
	}
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
	NSInteger numLeaderboards = (NSInteger)[leaderboards count];
	if (leaderboards && numLeaderboards > row && row >= 0)
	{
		OFLeaderboard* leaderboard = (OFLeaderboard*)[leaderboards objectAtIndex:row];
		selectedLeaderboard = [leaderboard retain];
	}
	else
	{
		OFSafeRelease(selectedLeaderboard);
	}
	[self closeKeyboard];
}

@end
