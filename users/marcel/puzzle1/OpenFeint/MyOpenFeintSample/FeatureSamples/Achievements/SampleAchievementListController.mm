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

#import "SampleAchievementListController.h"

#import "OFImageView.h"
#import "OFViewHelper.h"

#import "OpenFeint.h"

@interface SampleAchievementListController (Private)
- (void)_reloadAllData;
@end

@implementation SampleAchievementListController

- (void)didSubmitDeferredAchievements
{
	NSLog(@"Successfully submitted all locally unlocked achievements to the server!"); 
}

- (void)didFailSubmittingDeferredAchievements
{
	NSLog(@"Failed submitting all locally unlocked achievements to the server!"); 
}

- (void)didUnlockOFAchievement:(OFAchievement*)achievement
{
	[self _reloadAllData];
}

- (void)didFailUnlockOFAchievement:(OFAchievement*)achievement
{
	NSLog(@"Failed to unlock achievement");
}

- (void)didGetIcon:(UIImage*)image OFAchievement:(OFAchievement*)achievement
{
	imageRequest = nil;
	if(achievement == selectedAchievement)
	{
		iconView.image = image;
		[iconView setNeedsDisplay];
	}
}

- (void)didFailGetIconOFAchievement:(OFAchievement*)achievement
{
	imageRequest = nil;
	NSLog(@"Failed to get icon image");
}

- (void)_updateUIWithAchievement:(OFAchievement*)achievement
{
	titleLabel.text = achievement.title;
	descriptionLabel.text = achievement.description;
	[isUnlocked setOn:achievement.isUnlocked animated:YES];
	
	if(imageRequest)
	{
		[imageRequest cancel];
	}
	iconView.image = nil;
	[iconView setNeedsDisplay];
	imageRequest = [achievement getIcon];
}

- (void)_reloadAllData
{
	[achievements removeAllObjects];
	
	[achievements addObjectsFromArray:[OFAchievement achievements]];

	[achievementPicker reloadAllComponents];
	if ([achievements count] > 0)
	{
		selectedAchievement = [[achievements objectAtIndex:0] retain];
		[self _updateUIWithAchievement:selectedAchievement];
		[achievementPicker selectRow:0 inComponent:0 animated:YES];
	}
	
	for (OFAchievement* achievement in achievements)
	{
		OFAchievement* realAch = [OFAchievement achievement:achievement.resourceId];
		NSLog(@"Achievement: %@, %d, unlocked? %@", realAch.title, realAch.gamerscore, realAch.isUnlocked ? @"YES" : @"NO");
	}
}

- (void)awakeFromNib
{
	achievements = [[NSMutableArray arrayWithCapacity:25] retain];
	[OFAchievement setDelegate:self];
}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	
	UIScrollView* scrollView = OFViewHelper::findFirstScrollView(self.view);
	scrollView.contentSize = OFViewHelper::sizeThatFitsTight(scrollView);

	[self _reloadAllData];
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
	OFSafeRelease(iconView);
	OFSafeRelease(achievementPicker);
	OFSafeRelease(achievements);
	OFSafeRelease(selectedAchievement);
	OFSafeRelease(titleLabel);
	OFSafeRelease(descriptionLabel);
	OFSafeRelease(isUnlocked);
	OFSafeRelease(contentView);
	[OFAchievement setDelegate:nil];
	[super dealloc];
}

- (IBAction)unlockSelected
{
	if (selectedAchievement)
	{
		[OFAchievement setCustomUrlForSocialNotificaion:@"http://openfeint.com"];
		[selectedAchievement unlock];
	}
}

- (IBAction)queueUnlock
{
	if (selectedAchievement)
	{
		[selectedAchievement unlockAndDefer];
		[self _reloadAllData];
	}
}

- (IBAction)submitQueued
{
	if (selectedAchievement)
	{
		[OFAchievement submitDeferredAchievements];
	}
}

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
	return achievements ? 1 : 0;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
	return [achievements count];
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
	if ((NSInteger)[achievements count] <= row)
		return @"";
		
	OFAchievement* achievement = (OFAchievement*)[achievements objectAtIndex:row];
	return achievement.title;
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
	NSInteger numAchievements = (NSInteger)[achievements count];
	OFSafeRelease(selectedAchievement);
	if (achievements && numAchievements > row && row >= 0)
	{
		selectedAchievement = [(OFAchievement*)[achievements objectAtIndex:row] retain];
		[self _updateUIWithAchievement:selectedAchievement];
	}
}

@end
