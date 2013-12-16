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

#pragma once

#import "OFHighScore.h"

@class OFRequestHandle;
@class OFLeaderboard;
@protocol OFDistributedScoreEnumeratorDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// OFDistributedScoreEnumerator objects provide a convenient way to enumerate large sets of scores
/// for a particular leaderboard that are evenly distributed at a given delta. This is useful if you
/// want to show scores during gameplay to indicate the players progress. OFDistributedScoreEnumerator allows 
/// you to show interesting scores without having to download a prohibitively large score set. It relies 
/// heavily on server side caching so there are certain restrictions that must be followed.
/// 
/// Distributed score enumeration works by downloading one 'page' at a time. A page is simply a fixed-size
/// array of scores. When initializing the enumerator you must in addition to the page size also supply
/// the desired score delta. With a page size of 50 and a score delta of 1000 the first page would
/// contain 50 scores as evenly distributed between 0 and 50000 as possible. Since you may not 
/// always want to start at 0 you may also specify a start score. The score delta and start score MUST be
/// constant across game sessions to ensure efficient server caching. Distributed scores are only available
/// for descending leaderboards and do not work for friends leaderboards.
///
/// For performance reasons distributed scores rely heavily on caching. It is important that you always use the same score delta and 
/// start score or this will break. The returned objects are OFAbridgedHighScores which is a smaller version of OFHighScores.
///
/// @see OFDistributedScoreEnumeratorDelegate
/// @see OFLeaderboard
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFDistributedScoreEnumerator : NSObject<OFCallbackable>
{
	NSInteger pageSize;
	NSInteger currentPage;
	NSInteger scoreDelta;
	NSInteger startScore;
	NSInteger lastPage;
	NSMutableDictionary* scores;
    NSString* leaderboardId;
    OFRequestHandle* activeRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFDistributedScoreEnumerator related actions. Must adopt the 
/// OFDistributedScoreEnumeratorDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFDistributedScoreEnumeratorDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Returns an initialized enumerator which will enumerate a set of scores 
/// of a given leaderboard trying to find scores evenly distributed with the given delta.
/// The enumerator object will immediately fetch the first set of scores, 
/// but they will not be available when this method returns. Distributed scores are cached
/// so the local players score will no be in any result sets for a while after it has been submitted.
///
/// Example: An enumerator with pageSize:5, scoreDelta: 1000, startScore: 5000 will try to find
///			 scores near the values 5000, 6000, 7000, 8000, 9000, 10000 for the first page and 
///			 scores near the values 11000, 12000, 13000, 14000, 15000 for the second page.
///
/// IMPORTANT: pageSize and scoreDelta MUST be constant across all game sessions. Without this caching
///			   will fail and the server requests will start getting rejected.
///
/// @param leaderboard			The leaderboard to enumerator over
/// @param pageSize				Number of scores contained in each page. Max 100. The returned page may
///								contain less than 100 scores if not enough scores in the valid range were found.
/// @param scoreDelta			The ideal difference in value between each score. ScoreDelta  
///								must be constant so that the server requests can be cached. 
///								Never initialize this value from the players score or any other dynamic source.
///								Example: If passing in a scoreDelta of 50 the returned scores will have as close
///								values as possible to the following (startScore + 50, startScore + 100, startScore + 150, ...)
/// @param startScore			The desired value of the first score on the first page. This value must be constant
///								so that the server requests can be cached.
///
/// @return An initialized OFDistributedScoreEnumerator object.
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFDistributedScoreEnumerator*)scoreEnumeratorForLeaderboard:(OFLeaderboard*)leaderboard 
													  pageSize:(NSInteger)pageSize
													scoreDelta:(NSInteger)scoreDelta
													startScore:(NSInteger)startScore
													  delegate:(id<OFDistributedScoreEnumeratorDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieve the next page of scores.
///
/// @return @c YES if the next page is being downloaded or already exist in memory, @c NO if the current page is
///         already the last page. If the next page is already in memory this will return YES but the success delegate
///			will first be called.
///			If you show a loading screen depending on the result of this call, also check that
///			hasScores returns false before showing the loading screen since the success delegate may already have been called.
///
/// @note Invokes -(void)ofDistributedScoreEnumeratorReceivedNewScores: on success,
///			-(void)ofDistributedScoreEnumeratorFailedReceivingNewScores: on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)nextPage;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieve the previous page of scores.
///
/// @return @c YES if the previous page is being downloaded, @c NO if the current page is
///         already the first page. If the previous page is already in memory this will return YES but the success delegate
///			will first be called.
///			If you show a loading screen depending on the result of this call, also check that
///			hasScores returns false before showing the loading screen since the success delegate may already have been called
///
/// @note Invokes -(void)ofScoreDistributedEnumeratorReceivedNewScores: on success,
///			-(void)ofScoreDistributedEnumeratorFailedReceivingNewScores: on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)previousPage;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieve a given page of scores.
///
/// @note Invokes -(void)ofDistributedScoreEnumeratorReceivedNewScores: on success,
///			-(void)ofDistributedScoreEnumeratorFailedReceivingNewScores: on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)jumpToPage:(NSInteger)page;

//////////////////////////////////////////////////////////////////////////////////////////
/// The number of scores requested at one time.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSInteger pageSize;

//////////////////////////////////////////////////////////////////////////////////////////
/// The current page of scores contained in the scores property.
///
/// @note Page numbers are 1-based.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSInteger currentPage;

//////////////////////////////////////////////////////////////////////////////////////////
/// The ideal delta between each score
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSInteger scoreDelta;

//////////////////////////////////////////////////////////////////////////////////////////
/// The desired value of the first score on the first page.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSInteger startScore;

//////////////////////////////////////////////////////////////////////////////////////////
/// @c YES if the enumerator has a valid set of scores, @c NO otherwise
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) BOOL hasScores;

//////////////////////////////////////////////////////////////////////////////////////////
/// The current page of scores. The score objects contained in the array are instances of
/// the OFAbridgedHighScores class.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSArray* scores;

@end

@protocol OFDistributedScoreEnumeratorDelegate
@optional
//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFDistributedScoreEnumerator object whenever a new set of scores has been retrieved.
/// Scores are downloaded as OFAbridgedHighScore objects.
///
/// @param scoreEnumerator	The OFDistributedScoreEnumerator object which requested the new scores
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didReceiveScoresOFDistributedScoreEnumerator:(OFDistributedScoreEnumerator*)scoreEnumerator;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFDistributedScoreEnumerator object whenever a request fails
///
/// @param scoreEnumerator	The OFDistributedScoreEnumerator object which requested the new scores
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailReceiveScoresOFDistributedScoreEnumerator:(OFDistributedScoreEnumerator*)scoreEnumerator;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFDistributedScoreEnumerator object whenever there are no more scores to load
///
/// @param scoreEnumerator	The OFDistributedScoreEnumerator object which requested the new scores
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didReachLastPageOFDistributedScoreEnumerator:(OFDistributedScoreEnumerator*)scoreEnumerator;

@end
