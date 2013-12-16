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

#import "OFLeaderboard.h"

#import "OFPaginatedSeries.h"
#import "OFTableSectionDescription.h"
#import "OFRequestHandle.h"
#import "OFLeaderboardService+Private.h"
#import "OFHighScoreService+Private.h"
#import "OpenFeint+UserOptions.h"
#import "OFResourceDataMap.h"

static id sharedDelegate = nil;

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFLeaderboard (Private)
- (void)_downloadedHighScores:(OFPaginatedSeries*)page;
- (void)_failedDownloadingHighScores;
- (NSString*)leaderboardId;

@end

@implementation OFLeaderboard

@synthesize filter, name, currentUserScore, descendingScoreOrder;

#pragma mark Public Methods

+ (void)setDelegate:(id<OFLeaderboardDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFLeaderboard class]];
	}
}

+ (NSArray*)leaderboards
{
	return [OFLeaderboardService getLeaderboardsLocal];
}

+ (OFLeaderboard*)leaderboard:(NSString*)leaderboardID
{
	return [OFLeaderboardService getLeaderboard:leaderboardID];
}

- (OFRequestHandle*)submitScore:(OFHighScore*)score
{
	return [OFHighScoreService 
			setHighScore:score.score 
			withDisplayText:score.displayText 
			withCustomData:score.customData 
			forLeaderboard:self.leaderboardId 
			silently:NO 
			onSuccess:OFDelegate() 
			onFailure:OFDelegate()];
}

- (OFHighScore*)highScoreForCurrentUser
{
	return [OFHighScoreService getHighScoreForUser:[OpenFeint localUser] leaderboardId:[self leaderboardId] descendingSortOrder:self.descendingScoreOrder];
}

- (NSArray*)locallySubmittedScores
{
	return [OFHighScoreService getHighScoresLocal:self.leaderboardId];
}

- (OFRequestHandle*)downloadHighScoresWithFilter:(OFScoreFilter)downloadFilter
{
	OFAssert(downloadFilter != OFScoreFilter_None, @"When Downloading High Scores you must specify a filter other than NONE");
	
	
	filter = downloadFilter;
	
	OFRequestHandle* handle =  [OFHighScoreService 
								getPage:1 
								pageSize:25 
								forLeaderboard:self.leaderboardId 
								comparedToUserId:nil
								friendsOnly:(filter == OFScoreFilter_FriendsOnly) 
								silently:YES
								onSuccess:OFDelegate(self, @selector(_downloadedHighScores:)) 
								onFailure:OFDelegate(self, @selector(_failedDownloadingHighScores))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFLeaderboard class]];
	return handle;
}

#pragma mark Internal Methods

- (id)initWithLocalSQL:(OFSqlQuery*)queryRow localUserScore:(OFHighScore*) locUserScore comparedUserScore:(OFHighScore*) compUserScore
{
	self = [super init];
	if (self != nil)
	{	
		name = [[NSString stringWithUTF8String:queryRow->getText("name")] retain];
		resourceId = [[NSString stringWithFormat:@"%s", queryRow->getText("id")] retain];
		descendingScoreOrder = queryRow->getInt("descending_sort_order") != 0;
		OFSafeRelease(currentUserScore);
		currentUserScore = [locUserScore retain];
		OFSafeRelease(comparedUserScore);
		comparedUserScore = [compUserScore retain];
	}
	return self;
}

- (bool)isComparison
{
	return comparedUserScore != nil;
}

- (OFUser*)comparedToUser
{
	return [comparedUserScore user];
}

- (void)_downloadedHighScores:(OFPaginatedSeries*)page
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadHighScores:OFLeaderboard:)])
	{
		NSArray* objects = [page objects];
		if ([objects count] > 0 && [[objects objectAtIndex:0] isKindOfClass:[OFTableSectionDescription class]])
		{
			objects = [[[objects lastObject] page] objects];
		}
		
		[sharedDelegate didDownloadHighScores:objects OFLeaderboard:self];
	}
}

- (void)_failedDownloadingHighScores
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadHighScoresOFLeaderboard:)])
	{
		[sharedDelegate didFailDownloadHighScoresOFLeaderboard:self];
	}
}

- (NSString*) leaderboardId
{
	return resourceId;
}

#pragma mark -
#pragma mark OFResource
#pragma mark -

- (void)setName:(NSString*)value
{
	OFSafeRelease(name);
	name = [value retain];
}

- (void)setDescendingScoreOrder:(NSString*)value
{
	descendingScoreOrder = [value boolValue];
}

- (void)setCurrentUsersScore:(OFHighScore*)value
{
	OFSafeRelease(currentUserScore);
	currentUserScore = [value retain];
}

- (void)setComparedToUsersScore:(OFHighScore*)value
{
	OFSafeRelease(comparedUserScore);
	comparedUserScore = [value retain];
}

+ (OFService*)getService;
{
	return [OFLeaderboardService sharedInstance];
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"name", @selector(setName:));
		dataMap->addField(@"descending_sort_order", @selector(setDescendingScoreOrder:));
		dataMap->addNestedResourceField(@"current_user_high_score", @selector(setCurrentUsersScore:), nil, [OFHighScore class]);
		dataMap->addNestedResourceField(@"compared_user_high_score", @selector(setComparedToUsersScore:), nil, [OFHighScore class]);
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"leaderboard";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"openfeint_leaderboard_discovered";
}

// Satisfy OFCallbackable protocol with canReceiveCallbacksNow.
- (bool)canReceiveCallbacksNow
{
	return YES;
}

- (void) dealloc
{
	OFSafeRelease(name);
	OFSafeRelease(currentUserScore);
	OFSafeRelease(comparedUserScore);
	[super dealloc];
}

@end









