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

#import "OFDistributedScoreEnumerator.h"
#import "OFRequestHandle.h"

#import "OFHighScoreService.h"
#import "OFPaginatedSeries.h"
#import "OFLeaderboard.h"

static id sharedDelegate = nil;
static NSInteger sPreviousLeaderboardId = -1;
static NSInteger sPreviousPageSize = -1;
static NSInteger sPreviousScoreDelta = -1;
static NSInteger sPreviousStartScore = -1;

@interface OFDistributedScoreEnumerator ()

- (id)initWithLeaderboardId:(NSString*)_leaderboardId pageSize:(NSInteger)_pageSize scoreDelta:(NSInteger)_scoreDelta startScore:(NSInteger)_startScore delegate:(id<OFDistributedScoreEnumeratorDelegate>)_delegate;;
- (void)setCurrentPage:(NSInteger)newCurrentPage;
- (void)_downloadFinished:(OFPaginatedSeries*)page;
- (void)_downloadFailed;
- (void)callSuccessDelegate;

@property (nonatomic, retain) OFRequestHandle* activeRequest;

@end


@implementation OFDistributedScoreEnumerator

@synthesize pageSize, currentPage, scoreDelta, startScore, activeRequest;

+ (void)setDelegate:(id<OFDistributedScoreEnumeratorDelegate>)delegate
{
	sharedDelegate = delegate;
}

+ (OFDistributedScoreEnumerator*)scoreEnumeratorForLeaderboard:(OFLeaderboard*)leaderboard 
													  pageSize:(NSInteger)pageSize
													scoreDelta:(NSInteger)scoreDelta
													 startScore:(NSInteger)startScore
													  delegate:(id<OFDistributedScoreEnumeratorDelegate>)_delegate
{
	OFAssert(leaderboard.descendingScoreOrder, [NSString stringWithFormat:@"%@ uses ascending sort order. Only descending sort order supported for distributed scores", leaderboard.name]);
	return [[[OFDistributedScoreEnumerator alloc] initWithLeaderboardId:leaderboard.resourceId 
															   pageSize:pageSize 
															 scoreDelta:scoreDelta
															  startScore:startScore
															   delegate:_delegate] autorelease];
}

- (id)initWithLeaderboardId:(NSString*)_leaderboardId 
				   pageSize:(NSInteger)_pageSize 
				 scoreDelta:(NSInteger)_scoreDelta
				  startScore:(NSInteger)_startScore
				   delegate:(id<OFDistributedScoreEnumeratorDelegate>)_delegate;
{
    self = [super init];
    if (self != nil)
    {

		leaderboardId = [_leaderboardId retain];
		if (sPreviousLeaderboardId != [leaderboardId integerValue])
		{
			sPreviousLeaderboardId = [leaderboardId integerValue];
			sPreviousPageSize = _pageSize;
			sPreviousScoreDelta = _scoreDelta;
			sPreviousStartScore = _startScore;
		}
		else 
		{
			OFAssert(sPreviousPageSize == _pageSize, "OFDistributedScoreEnumerator must be initialized with the same page size every time for each specific leaderboard as it relies on server caching.");
			OFAssert(sPreviousScoreDelta == _scoreDelta, "OFDistributedScoreEnumerator must be initialized with the same score delta every time for each specific leaderboard as it relies on server caching.");
			OFAssert(sPreviousStartScore == _startScore, "OFDistributedScoreEnumerator must be initialized with the same start score every time for each specific leaderboard as it relies on server caching.");
		}

        lastPage = NSIntegerMax;
        pageSize = _pageSize;
		scoreDelta = _scoreDelta;
		startScore = _startScore;
		OFSafeRelease(scores);
		scores = [NSMutableDictionary new];
        self.activeRequest = nil;
		sharedDelegate = _delegate;
		[self setCurrentPage:1];
    }
    
    return self;
}

- (void)dealloc
{
	OFSafeRelease(scores);
	OFSafeRelease(leaderboardId);
    self.activeRequest = nil;
    
    [super dealloc];
}

#pragma mark Property Methods

- (void)setActiveRequest:(OFRequestHandle*)handle
{
    if (activeRequest != handle)
    {
        if (activeRequest != nil)
        {
            [activeRequest cancel];
            OFSafeRelease(activeRequest);
        }
		
        activeRequest = [handle retain];
    }
}

- (void)setCurrentPage:(NSInteger)newCurrentPage
{
	OFAssert(newCurrentPage > 0, "Page is out of range!");
    if (currentPage != newCurrentPage && 
		newCurrentPage > 0)
    {
        currentPage = newCurrentPage;
		
		if ([scores objectForKey:[NSNumber numberWithInt:currentPage]])
		{
			self.activeRequest = nil;
			[self callSuccessDelegate];
		}
		else
		{
			self.activeRequest = [OFHighScoreService 
								  getDistributedHighScoresAtPage:currentPage 
								  pageSize:pageSize 
								  scoreDelta:scoreDelta
								  startScore:startScore
								  forLeaderboard:leaderboardId 
								  onSuccess:OFDelegate(self, @selector(_downloadFinished:))
								  onFailure:OFDelegate(self, @selector(_downloadFailed))];
		}
    }
}

- (NSArray*)scores
{
	return [scores objectForKey:[NSNumber numberWithInt:currentPage]];
}

- (BOOL)hasScores
{
    return self.scores != nil;
}

#pragma mark Public Methods

- (BOOL)nextPage
{
	if (currentPage + 1 >= lastPage)
	{
		return NO;
	}
	else 
	{
		[self jumpToPage:currentPage + 1];
		return YES;
	}

}

- (BOOL)previousPage
{
    BOOL hasPreviousPage = currentPage > 1;
    [self jumpToPage:MAX(currentPage - 1, 1)];
    return hasPreviousPage;
}

- (void)jumpToPage:(NSInteger)page
{
    self.currentPage = page;
}

#pragma mark Internal Methods

- (void)callSuccessDelegate
{
	if (sharedDelegate)
    {
		NSArray* curPageScores = [scores objectForKey:[NSNumber numberWithInt:currentPage]];
		if (curPageScores && [curPageScores count] == 0)
		{
			lastPage = MIN(lastPage, currentPage);
			if ([sharedDelegate respondsToSelector:@selector(didReachLastPageOFDistributedScoreEnumerator:)])
			{
				[sharedDelegate didReachLastPageOFDistributedScoreEnumerator:self];
			}
		}
		else if ([sharedDelegate respondsToSelector:@selector(didReceiveScoresOFDistributedScoreEnumerator:)])
		{
			[sharedDelegate didReceiveScoresOFDistributedScoreEnumerator:self];
		}
    }
}

- (void)_downloadFinished:(OFPaginatedSeries*)page
{
	NSArray* objects = [page objects];
	[scores setObject:objects forKey:[NSNumber numberWithInt:currentPage]];
    self.activeRequest = nil;
    [self callSuccessDelegate];
    
}

- (void)_downloadFailed
{
    self.activeRequest = nil;
	
    if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailReceiveScoresOFDistributedScoreEnumerator:)])
    {
        [sharedDelegate didFailReceiveScoresOFDistributedScoreEnumerator:self];
    }
}

#pragma mark OFCallbackable

- (bool)canReceiveCallbacksNow
{
    return YES;
}

@end
