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
@protocol OFScoreEnumeratorDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// OFScoreEnumerator objects provide a convenient way to enumerate the scores for a
/// particular leaderboard. 
/// 
/// Enumeration works by downloading one 'page' at a time. A page is simply a fixed-size
/// array of scores. 
///
/// @see OFScoreEnumeratorDelegate
/// @see OFLeaderboard
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFScoreEnumerator : NSObject<OFCallbackable>
{
	BOOL isCurrentUserPage;
	NSInteger pageSize;
	NSInteger currentPage;
	NSInteger totalPages;
	NSArray* scores;
	OFScoreFilter filter;
    NSString* leaderboardId;
    OFRequestHandle* activeRequest;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFScoreEnumerator related actions. Must adopt the 
/// OFScoreEnumeratorDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFScoreEnumeratorDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Returns an initialized score enumerator which will enumerate a set of scores 
/// of a given leaderboard based on a filter. The enumerator object will immediately fetch 
/// the first page of scores, but they will not be available when this method returns.
/// One of the following delegate methods will be called to when the asynchronous
/// server response has been received.  The first page of scores is passed as a parameter
/// to the success delegate.
///
/// @note Invokes -(void)didReceiveScoresOFScoreEnumerator: on success,
///			-(void)didFailReceiveScoresOFScoreEnumerator: on failure.
///
/// @param leaderboard		The leaderboard to enumerator over
/// @param pageSize			Number of scores contained in each page
/// @param filter			The filter to use when getting the scores
///
/// @return An initialized OFScoreEnumerator object.
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFScoreEnumerator*)scoreEnumeratorForLeaderboard:(OFLeaderboard*)leaderboard pageSize:(NSInteger)pageSize filter:(OFScoreFilter)filter;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieve the next page of scores.
///
/// @return @c YES if the next page is being downloaded, @c NO if the current page is
///         already the last page.
///
/// @note Invokes -(void)didReceiveScoresOFScoreEnumerator: on success,
///			-(void)didFailReceiveScoresOFScoreEnumerator: on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)nextPage;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieve the previous page of scores.
///
/// @return @c YES if the previous page is being downloaded, @c NO if the current page is
///         already the first page.
///
/// @note Invokes -(void)didReceiveScoresOFScoreEnumerator: on success,
///			-(void)didFailReceiveScoresOFScoreEnumerator: on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (BOOL)previousPage;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieve the page of scores that contains the current user's score.
///
/// @note Invokes -(void)didReceiveScoresOFScoreEnumerator: on success,
///			-(void)didFailReceiveScoresOFScoreEnumerator: on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)pageWithCurrentUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieve a given page of scores.
/// The page index is a one-based value such that page == 1 refers to the first page.
///
/// @note Invokes -(void)didReceiveScoresOFScoreEnumerator: on success,
///			-(void)didFailReceiveScoresOFScoreEnumerator: on failure.
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
/// The total number of pages of scores that are enumerable.
///
/// @note Page numbers are 1-based.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSInteger totalPages;

//////////////////////////////////////////////////////////////////////////////////////////
/// @c YES if the enumerator has a valid set of scores, @c NO otherwise
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) BOOL hasScores;

//////////////////////////////////////////////////////////////////////////////////////////
/// The current page of scores. The score objects contained in the array are instances of
/// the OFHighScore class.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSArray* scores;

//////////////////////////////////////////////////////////////////////////////////////////
/// @c YES if the current page of scores contains the current user's score, @c NO otherwise
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) BOOL isCurrentUserPage;

@end

@protocol OFScoreEnumeratorDelegate
@optional
//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFScoreEnumerator object whenever a new set of scores has been retrieved
///
/// @param scoreEnumerator	The OFScoreEnumerator object which requested the new scores
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didReceiveScoresOFScoreEnumerator:(OFScoreEnumerator*)scoreEnumerator;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by an OFScoreEnumerator object whenever 
///
/// @param scoreEnumerator	The OFScoreEnumerator object which requested the new scores
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailReceiveScoresOFScoreEnumerator:(OFScoreEnumerator*)scoreEnumerator;
@end
