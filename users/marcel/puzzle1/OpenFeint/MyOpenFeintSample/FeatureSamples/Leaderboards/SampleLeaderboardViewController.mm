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

#import "SampleLeaderboardViewController.h"

#import "OpenFeint+Dashboard.h"
#import "OFHighScore.h"
#import "OFUser.h"
#import "OFAbridgedHighScore.h"

#import "OFLeaderboard.h"

@interface SampleLeaderboardViewController ()
- (void)loadDistributedScores;
- (void)requestViewType: (LbdViewType) requestedViewType withPageCmd: (LbdPageCmd) requestedPageCmd;
- (void)setEnumerationControlVisibility: (BOOL)wantVisible;
@property (nonatomic, readwrite, retain) OFScoreEnumerator* regularScoreEnumerator;
@end

@implementation SampleLeaderboardViewController

@synthesize scoresView, 
    activityView, 
    leaderboard, 
    activeRequest, 
    regularScoreEnumerator,
	enumerationLabel,
	enumerationSwitch,
    nextButton, 
    prevButton, 
    meButton;

- (void)_displayDownloading
{
    self.scoresView.text = @"Downloading scores";
    [self.activityView startAnimating];
}

- (void)_displayScores:(NSArray*)scores
{
	NSString* text = @"Name | Rank | Score\n";
	
	if (distributedScoreEnumerator)
	{
		text = [NSString stringWithFormat:@"Distributed Scores page %d\nPage Size: %d\nScore Delta: %d\nStart Score: %d\nCurrent score range: %d - %d\n\nName | Score\n", 
				distributedScoreEnumerator.currentPage,  
				distributedScoreEnumerator.pageSize,
				distributedScoreEnumerator.scoreDelta,
				distributedScoreEnumerator.startScore,
				distributedScoreEnumerator.startScore + (distributedScoreEnumerator.currentPage - 1) * distributedScoreEnumerator.pageSize * distributedScoreEnumerator.scoreDelta,
				distributedScoreEnumerator.startScore + distributedScoreEnumerator.currentPage * distributedScoreEnumerator.pageSize * distributedScoreEnumerator.scoreDelta];
	}
	if ([scores count] > 0)
	{
		for (id score in scores)
		{
			if ([score isKindOfClass:[OFAbridgedHighScore class]])
			{
				OFAbridgedHighScore* typedScore = (OFAbridgedHighScore*)score;
				text = [text stringByAppendingFormat:@"%@ | %@\n", typedScore.userName, typedScore.displayText]; 
			}
			else if ([score isKindOfClass:[OFHighScore class]])
			{
				OFHighScore* typedScore = (OFHighScore*)score;
				text = [text stringByAppendingFormat:@"%@ | %4d | %@\n", typedScore.user.name, typedScore.rank, typedScore.displayText]; 
			}
		}
	}
	else
	{
		text = [text stringByAppendingString:@"No scores found."]; 
	}
	
	self.scoresView.text = text;
    [self.activityView stopAnimating];
}

- (void)setActiveRequest:(OFRequestHandle*)handle
{
	if (activeRequest != nil)
	{
		[activeRequest cancel];
		OFSafeRelease(activeRequest);
	}
	
	activeRequest = [handle retain];
}

- (void)viewWillAppear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardViewController viewWillAppear");
	[super viewWillAppear:animated];

	// Get ready to work with leaderboards.
	[OFLeaderboard setDelegate:self];
	[OFScoreEnumerator setDelegate:self];
	[OFDistributedScoreEnumerator setDelegate:self];
}

- (void)viewDidAppear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardViewController viewDidAppear");
	[super viewDidAppear:animated];
	
	currentLbdViewType = LbdViewType_None;
	currentLbdPageCmd = LbdPageCmd_None;
	currentLbdIsEnumerated = NO;
	
	[self requestViewType: LbdViewType_Global withPageCmd: LbdPageCmd_Summary];
}

- (void)viewWillDisappear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardViewController viewWillDisappear");
	[super viewWillDisappear:animated];
	
	// This class must relinquish it's role as leaderboard delegate.
	[OFLeaderboard setDelegate:nil];
	[OFScoreEnumerator setDelegate:nil];
	[OFDistributedScoreEnumerator setDelegate:nil];
}

- (void)viewDidDisappear:(BOOL)animated
{
	NSLog(@"SampleLeaderboardViewController viewDidDisappear");
	[super viewDidDisappear:animated];
}

- (IBAction)openLeaderboard
{
	//TODO THIS SHOULD TAKE A LEADERBOARD, NOT A LEADERBOARD ID (NEED TO CHANGE API)
	[OpenFeint launchDashboardWithHighscorePage:[leaderboard leaderboardId]];
}

- (void)requestViewType: (LbdViewType) requestedViewType withPageCmd: (LbdPageCmd) requestedPageCmd
{
	LbdViewType	permittedViewType = LbdViewType_None;
	LbdPageCmd	permittedPageCmd = LbdPageCmd_None;
	BOOL		enabledEnumeration = NO;
	BOOL		enumerationVisibility = NO;
	BOOL		isPageChanging = NO;
	
	// Restrict options to permitted changes.
	switch (requestedViewType){
		//==========================
		case LbdViewType_Global:
		case LbdViewType_Friends:
			permittedViewType = requestedViewType;
			switch (requestedPageCmd){
				case LbdPageCmd_First:
				case LbdPageCmd_WithMe:
					permittedPageCmd = requestedPageCmd;
					enabledEnumeration = YES;
					isPageChanging = (currentLbdPageCmd != requestedPageCmd);
					break;
				case LbdPageCmd_Next:
				case LbdPageCmd_Previous:
					permittedPageCmd = requestedPageCmd;
					enabledEnumeration = YES;
					isPageChanging = YES;
					break;
				case LbdPageCmd_Summary:
				default:
					permittedPageCmd = LbdPageCmd_Summary;
					enabledEnumeration = NO;
					isPageChanging = (currentLbdPageCmd != LbdPageCmd_Summary);
					break;
			}
			enumerationVisibility = YES;
			break;
		//==========================
		case LbdViewType_Offline:
		case LbdViewType_Mine:
			permittedViewType = requestedViewType;
			permittedPageCmd = LbdPageCmd_Summary;
			enabledEnumeration = NO;
			isPageChanging = NO;
			enumerationVisibility = NO;
			break;
		//==========================
		case LbdViewType_Spaced:
			permittedViewType = requestedViewType;
			switch (requestedPageCmd){
				case LbdPageCmd_First:
					permittedPageCmd = requestedPageCmd;
					isPageChanging = (currentLbdPageCmd != requestedPageCmd);
					break;
				case LbdPageCmd_Next:
				case LbdPageCmd_Previous:
					permittedPageCmd = requestedPageCmd;
					isPageChanging = YES;
					break;
				default:
					permittedPageCmd = LbdPageCmd_First;
					isPageChanging = (currentLbdPageCmd != LbdPageCmd_Summary);
					break;
			}
			enabledEnumeration = YES;
			enumerationVisibility = YES;
			break;
		//==========================
		default:
			permittedViewType = LbdViewType_Global;
			permittedPageCmd = LbdPageCmd_Summary;
			enabledEnumeration = NO;
			isPageChanging = (currentLbdPageCmd != LbdPageCmd_Summary);
			enumerationVisibility = YES;
			break;
	}
	
	// Is an actual change being granted?
	if ((permittedViewType != currentLbdViewType) || isPageChanging){
		BOOL isViewTypeChanging = (permittedViewType != currentLbdViewType);
		
		currentLbdViewType = permittedViewType;
		currentLbdPageCmd = permittedPageCmd;
		currentLbdIsEnumerated = enabledEnumeration;
		
		// Make sure switch corresponds.
		if (enumerationSwitch.on != currentLbdIsEnumerated) {
			enumerationSwitch.on = currentLbdIsEnumerated;
		}
		
		// Clear active request.
		[self.activeRequest cancel];
		self.activeRequest = nil;
		
		// What enumerators are not to be kept?
		if (currentLbdIsEnumerated && (! isViewTypeChanging)){
			if (LbdViewType_Spaced == currentLbdViewType){
				self.regularScoreEnumerator = nil;
			}else{
				OFSafeRelease(distributedScoreEnumerator);			
			}
		}else{
			self.regularScoreEnumerator = nil;
			OFSafeRelease(distributedScoreEnumerator);			
		}

		// Start loading new data.
		switch (currentLbdViewType){
			case LbdViewType_Global:{
				if (currentLbdIsEnumerated){
					if (self.regularScoreEnumerator){
						// Changing page.
					}else{
						self.regularScoreEnumerator = [OFScoreEnumerator scoreEnumeratorForLeaderboard:leaderboard pageSize:5 filter:OFScoreFilter_Everybody];
						//[self.regularScoreEnumerator jumpToPage: 1];
						[self _displayDownloading];
					}
				}else{
					self.activeRequest = [leaderboard downloadHighScoresWithFilter:OFScoreFilter_Everybody];
					[self _displayDownloading];
				}
			}	break;
			case LbdViewType_Friends:{
				if (currentLbdIsEnumerated){
					if (self.regularScoreEnumerator){
						// Changing page.
					}else{
						self.regularScoreEnumerator = [OFScoreEnumerator scoreEnumeratorForLeaderboard:leaderboard pageSize:5 filter:OFScoreFilter_FriendsOnly];
						//[self.regularScoreEnumerator jumpToPage: 1];
						[self _displayDownloading];
					}
				}else{
					self.activeRequest = [leaderboard downloadHighScoresWithFilter:OFScoreFilter_FriendsOnly];
					[self _displayDownloading];
				}
			}	break;
			case LbdViewType_Offline:{
				NSArray* scores = [leaderboard locallySubmittedScores];
				[self _displayScores:scores];
			}	break;
			case LbdViewType_Mine:{
				OFHighScore* myScore = [leaderboard highScoreForCurrentUser];
				NSArray* scores = nil;
				
				if (myScore){
					scores = [NSArray arrayWithObject: [leaderboard highScoreForCurrentUser]];
				}else{
					scores = [NSArray array]; // empty array.
				}
				
				[self _displayScores:scores];
			}	break;
			case LbdViewType_Spaced:{
				self.meButton.hidden = YES;
				if (!leaderboard.descendingScoreOrder)
				{
					[self.activityView stopAnimating];
					self.scoresView.text = [NSString stringWithFormat:
											@"Leaderboard '%@' uses ascending sort order and does not support distributed scores.",
											leaderboard.name];
					[[[[UIAlertView alloc] 
					   initWithTitle:@"Not Allowed!"
					   message:@"Distributed scores are only supported for leaderboards with descending sort order"
					   delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] 
					  autorelease] 
					 show];
				}
				else 
				{
					[self loadDistributedScores];
				}
			}	break;
			default:
				break;
		}// switch
		
		[self setEnumerationControlVisibility: enumerationVisibility];
	}
}

- (void)setEnumerationControlVisibility: (BOOL)wantVisible
{
	self.enumerationLabel.hidden = (! wantVisible) || (currentLbdViewType == LbdViewType_Spaced);
	self.enumerationSwitch.hidden = (! wantVisible) || (currentLbdViewType == LbdViewType_Spaced);
	
	if (wantVisible){
		self.nextButton.hidden = (! currentLbdIsEnumerated);
		self.prevButton.hidden = (! currentLbdIsEnumerated);
		self.meButton.hidden = (! currentLbdIsEnumerated) || (currentLbdViewType == LbdViewType_Spaced);		
	}else{
		self.nextButton.hidden = YES;
		self.prevButton.hidden = YES;
		self.meButton.hidden = YES;
	}
}

- (IBAction)changeViewType:(id)sender
{
	UISegmentedControl* control = (UISegmentedControl*)sender;
	[self requestViewType: (LbdViewType) control.selectedSegmentIndex withPageCmd:currentLbdPageCmd];
}

- (IBAction)prev
{
	if (distributedScoreEnumerator)
	{
		if ([distributedScoreEnumerator previousPage] && ![distributedScoreEnumerator hasScores])
		{
			[self _displayDownloading];
		}
	}
	else 
	{
		if ([self.regularScoreEnumerator previousPage])
		{
			[self _displayDownloading];
		}
	}
}

- (IBAction)next
{
	if (distributedScoreEnumerator)
	{
		if ([distributedScoreEnumerator nextPage] && ![distributedScoreEnumerator hasScores])
		{
			[self _displayDownloading];
		}
	}
	else 
	{
		if ([self.regularScoreEnumerator nextPage])
		{
			[self _displayDownloading];
		}
	}
}

- (IBAction)me
{
    [self.regularScoreEnumerator pageWithCurrentUser];
    [self _displayDownloading];
}

- (IBAction)switchToggled:(id)sender
{
    UISwitch* theSwitch = (UISwitch*)sender;
	LbdPageCmd requestedLbdPageCmd = (theSwitch.on ? LbdPageCmd_First : LbdPageCmd_Summary);
	
	[self requestViewType: currentLbdViewType withPageCmd: requestedLbdPageCmd];
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
	OFSafeRelease(distributedScoreEnumerator);
	self.scoresView = nil;
	self.activityView = nil;
	self.leaderboard = nil;
    self.regularScoreEnumerator = nil;
	self.enumerationLabel = nil;
	self.enumerationSwitch = nil;
    self.nextButton = nil;
    self.prevButton = nil;
    self.meButton = nil;
	[super dealloc];
}

#pragma mark Download Scores without enumerator

- (void)didDownloadHighScores:(NSArray*)highScores OFLeaderboard:(OFLeaderboard*)leaderboard;
{
	[self _displayScores:highScores];
	self.activeRequest = nil;
}

- (void)didFailDownloadHighScoresOFLeaderboard:(OFLeaderboard*)leaderboard;
{
	self.scoresView.text = @"Failed to download high scores";
	self.activeRequest = nil;
	[self.activityView stopAnimating];
}

#pragma mark Download Enumerated Scores

- (void)didReceiveScoresOFScoreEnumerator:(OFScoreEnumerator*)_scoreEnumerator
{
    [self _displayScores:regularScoreEnumerator.scores];

    self.prevButton.enabled = _scoreEnumerator.currentPage > 1;
    self.nextButton.enabled = _scoreEnumerator.currentPage < _scoreEnumerator.totalPages;
}

- (void)didFailReceiveScoresOFScoreEnumerator:(OFScoreEnumerator*)_scoreEnumerator
{
	self.scoresView.text = @"Failed to download high scores";
}

#pragma mark Download Distributed Scores

- (void)loadDistributedScores
{
	self.regularScoreEnumerator = nil;

	if (distributedScoreEnumerator == nil)
	{
		distributedScoreEnumerator = [[OFDistributedScoreEnumerator scoreEnumeratorForLeaderboard:leaderboard
			pageSize:10
			scoreDelta:100
			startScore:0
			delegate:self] retain];
		[[[[UIAlertView alloc]
		   initWithTitle:@"Distributed Scores"
		   message:@"pageSize, scoreDelta and startScore must be hard coded for distributed scores. To change the values in the sample application edit the code in loadDistributedScores."
		   delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] 
		  autorelease] 
		 show];
		
		[self _displayDownloading];
	}
	else
	{
		[self _displayScores:distributedScoreEnumerator.scores];
	}
}

- (void)didReachLastPageOFDistributedScoreEnumerator:(OFDistributedScoreEnumerator*)_scoreEnumerator
{
	self.scoresView.text = @"Reached End Of List";
	[self.activityView stopAnimating];
	self.activeRequest = nil;
}

- (void)didReceiveScoresOFDistributedScoreEnumerator:(OFDistributedScoreEnumerator*)_scoreEnumerator
{
	[self _displayScores:_scoreEnumerator.scores];
	self.activeRequest = nil;
}

- (void)didFailReceiveScoresOFDistributedScoreEnumerator:(OFDistributedScoreEnumerator*)_scoreEnumerator
{
	self.activeRequest = nil;
	self.scoresView.text = @"Failed to download high scores";
	[self.activityView stopAnimating];
}

@end
