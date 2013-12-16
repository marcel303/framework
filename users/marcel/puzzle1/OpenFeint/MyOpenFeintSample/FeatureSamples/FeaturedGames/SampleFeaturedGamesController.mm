//
//  SampleFeaturedGamesController.mm
//  OpenFeint
//
//  Created by Phillip Saindon on 6/25/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "SampleFeaturedGamesController.h"
#import "OFViewHelper.h"
#import "OFRequestHandle.h"


@implementation SampleFeaturedGamesController


#pragma mark -
#pragma mark Utility

- (void)_updateUIWithGame:(OFPlayedGame*)game
{
	descriptionText.text = [NSString stringWithFormat:
							 @"Name: %@\n"
							 "Client app Id: %@\n"
							 "iTunes Url: %@",
							 game.name,
							 game.clientApplicationId,
							 game.iTunesAppStoreUrl];
	if(imageRequest)
	{
		[imageRequest cancel];
	}
	gameIcon.image = nil;
	[gameIcon setNeedsDisplay];
	imageRequest = [game getGameIcon];
}

#pragma mark -
#pragma mark OFPlayedGame Delegate Methods

- (void)didGetFeaturedGames:(NSArray*)_featuredGames
{
	OFSafeRelease(featuredGames);
	featuredGames = [[NSArray alloc] initWithArray:_featuredGames];
	
	[featuredGamePicker reloadAllComponents];
	if([featuredGames count] > 0)
	{
		OFPlayedGame* game = [featuredGames objectAtIndex:0];
		[featuredGamePicker selectRow:0 inComponent:0 animated:YES];
		[self _updateUIWithGame:game];
		imageRequest = [game getGameIcon];
	}
	else
	{
		descriptionText.text = @"No Featured Games for this app.";
	}

}

- (void)didFailGetFeaturedGames
{
	OFLog(@"Getting featured Games Failed");
}

- (void)didGetGameIcon:(UIImage*)image OFPlayedGame:(OFPlayedGame*)game
{
	imageRequest = nil;
	gameIcon.image = image;
	[gameIcon setNeedsDisplay];
}

- (void)didFailGetGameIconOFPlayedGame:(OFPlayedGame*)game
{
	imageRequest = nil;
	OFLog(@"Failed Getting game icon");
}

#pragma mark -
#pragma mark Initialization

- (void)viewWillAppear:(BOOL)animated 
{
	[super viewWillAppear:animated];
	
	UIScrollView* scrollView = OFViewHelper::findFirstScrollView(self.view);
	scrollView.contentSize = OFViewHelper::sizeThatFitsTight(scrollView);
	
	[OFPlayedGame setDelegate:self];
	[OFPlayedGame getFeaturedGames];
}

- (void)viewWillDisappear:(BOOL)animated {
	
	[OFPlayedGame setDelegate:nil];
	
	[super viewWillDisappear:animated];
}

- (void)dealloc
{
	OFSafeRelease(featuredGames);
	[super dealloc];
}

#pragma mark -
#pragma mark PickerViewDelegate

- (NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView
{
	return featuredGames ? 1 : 0;
}

- (NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component
{
	return featuredGames ? [featuredGames count] : 0;
}

- (NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component
{
	if((NSInteger)[featuredGames count] <= row)
	{
		return @"";
	}
	
	OFPlayedGame* game = [featuredGames objectAtIndex:row];
	return game.name;
}

- (void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component
{
	if(row < (NSInteger)[featuredGames count] && row >= 0)
	{
		[self _updateUIWithGame:[featuredGames objectAtIndex:row]];
	}
}


@end
