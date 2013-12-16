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

#import "OFScoreEnumerator.h"
#import "OFRequestHandle.h"

#import "OFHighScoreService.h"
#import "OFPaginatedSeries.h"
#import "OFPaginatedSeriesHeader.h"
#import "OFTableSectionDescription.h"
#import "OFUser.h"
#import "OFLeaderboard.h"

static id sharedDelegate = nil;

@interface OFScoreEnumerator ()
- (id)initWithLeaderboardId:(NSString*)_leaderboardId pageSize:(NSInteger)_pageSize filter:(OFScoreFilter)_filter;
- (void)setCurrentPage:(NSInteger)newCurrentPage;
- (void)_downloadFinished:(OFPaginatedSeries*)page;
- (void)_downloadFailed;

@property (nonatomic, readwrite, retain) NSString* leaderboardId;
@property (nonatomic, readwrite, retain) OFRequestHandle* activeRequest;

@end


@implementation OFScoreEnumerator

@synthesize pageSize, currentPage, totalPages, scores, isCurrentUserPage, leaderboardId, activeRequest;

+ (void)setDelegate:(id<OFScoreEnumeratorDelegate>)delegate
{
	sharedDelegate = delegate;
}

+ (OFScoreEnumerator*)scoreEnumeratorForLeaderboard:(OFLeaderboard*)leaderboard pageSize:(NSInteger)pageSize filter:(OFScoreFilter)filter;
{
	return [[[OFScoreEnumerator alloc] initWithLeaderboardId:leaderboard.resourceId pageSize:pageSize filter:filter] autorelease];
}

- (id)initWithLeaderboardId:(NSString*)_leaderboardId pageSize:(NSInteger)_pageSize filter:(OFScoreFilter)_filter
{
    self = [super init];
    if (self != nil)
    {
        self.leaderboardId = _leaderboardId;
        
        pageSize = _pageSize;
        totalPages = 0;
        filter = _filter;        
        isCurrentUserPage = NO;
        
		OFSafeRelease(scores);
        self.activeRequest = nil;
        self.currentPage = 1;
    }
    
    return self;
}

- (void)dealloc
{
	OFSafeRelease(scores);
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
    if (currentPage != newCurrentPage)
    {
        currentPage = newCurrentPage;
        isCurrentUserPage = NO;

        OFAssert(currentPage > 0 && (currentPage <= totalPages || (currentPage == 1 && totalPages == 0)), "Page is out of range!");
        
        OFSafeRelease(scores);
        self.activeRequest = [OFHighScoreService 
            getPage:currentPage 
            pageSize:pageSize 
            forLeaderboard:leaderboardId 
            comparedToUserId:nil 
            friendsOnly:(filter == OFScoreFilter_FriendsOnly) 
            silently:YES
            onSuccess:OFDelegate(self, @selector(_downloadFinished:))
            onFailure:OFDelegate(self, @selector(_downloadFailed))];
    }
}

- (void)setPageSize:(NSInteger)newPageSize
{
    if (pageSize != newPageSize)
    {
        pageSize = newPageSize;
        totalPages = 0;
        
        self.currentPage = 1;
    }
}

- (BOOL)hasScores
{
    return scores != nil;
}

#pragma mark Public Methods

- (BOOL)nextPage
{
    BOOL hasNextPage = currentPage < totalPages;
    [self jumpToPage:MIN(currentPage + 1, totalPages)];
    return hasNextPage;
}

- (BOOL)previousPage
{
    BOOL hasPreviousPage = currentPage > 1;
    [self jumpToPage:MAX(currentPage - 1, 1)];
    return hasPreviousPage;
}

- (void)pageWithCurrentUser
{
    isCurrentUserPage = NO;
    
	OFSafeRelease(scores);
    self.activeRequest = [OFHighScoreService
        getPageWithLoggedInUserWithPageSize:pageSize 
        forLeaderboard:leaderboardId
		silently:YES
        onSuccess:OFDelegate(self, @selector(_downloadFinished:)) 
        onFailure:OFDelegate(self, @selector(_downloadFailed))];
}

- (void)jumpToPage:(NSInteger)page
{
    if (totalPages == 0)
    {
        OFLog(@"Cannot change pages until initial data is downloaded!");
    }
    else
    {
        self.currentPage = page;
    }
}

#pragma mark Internal Methods

- (void)_downloadFinished:(OFPaginatedSeries*)page
{
    OFPaginatedSeriesHeader* header = [page header];
	NSArray* objects = [page objects];
	if ([objects count] > 0 && [[objects objectAtIndex:0] isKindOfClass:[OFTableSectionDescription class]])
	{
        header = [[[objects lastObject] page] header];
		objects = [[[objects lastObject] page] objects];
	}
    
    for (OFHighScore* score in objects)
    {
        if ([score.user isLocalUser])
        {
            isCurrentUserPage = YES;
            break;
        }
    }

    currentPage = header.currentPage;
    totalPages = header.totalPages;

	OFSafeRelease(scores);
    scores = [objects retain];
    self.activeRequest = nil;
    
    if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didReceiveScoresOFScoreEnumerator:)])
    {
        [sharedDelegate didReceiveScoresOFScoreEnumerator:self];
    }
}

- (void)_downloadFailed
{
    self.activeRequest = nil;

    if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailReceiveScoresOFScoreEnumerator:)])
    {
        [sharedDelegate didFailReceiveScoresOFScoreEnumerator:self];
    }
}

#pragma mark OFCallbackable

- (bool)canReceiveCallbacksNow
{
    return YES;
}

@end
