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
#import "OFResource.h"

@class OFRequestHandle;
@protocol OFLeaderboardDelegate;


//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFLeaderboard allows you to query leaderboars and submit
/// new scores to a leaderboard.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFLeaderboard : OFResource <OFCallbackable>
{
	NSString* name;
	OFHighScore* currentUserScore;
	OFHighScore* comparedUserScore;
	BOOL descendingScoreOrder;
	OFScoreFilter filter;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFLeaderboard related actions. Must adopt the 
/// OFLeaderboardDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFLeaderboardDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieves the application's leaderboard list
///
/// @return NSArray of OFLeaderboard objects
//////////////////////////////////////////////////////////////////////////////////////////
+ (NSArray*)leaderboards;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieves a leaderboard based on the leaderboard id on the developer dashboard
///
/// @param leaderboardID	The leaderboard id
///
/// @return OFLeaderboard corrisponding to the leaderboard id
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFLeaderboard*)leaderboard:(NSString*)leaderboardID;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieves the current user's high score for a given leaderboard. 
///
/// @note The rank property on the OFHighScore object returned from this method will not
///			be set.
///
/// @return OFHighScore objects containing the local user's high score for a given
///			leaderboard.
//////////////////////////////////////////////////////////////////////////////////////////
- (OFHighScore*)highScoreForCurrentUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieves the locally submitted high score list for the given leaderboard. These scores 
/// are the 10 best scores (for leaderboards with Allow Worse Scores unchecked) or 10 most
/// recent scores (for leaderboards with Allow Worse Scores checked) for the local user.
///
/// @return NSArray of OFHighScore objects representing the local top 10 scores for the 
///			local user
//////////////////////////////////////////////////////////////////////////////////////////
- (NSArray*)locallySubmittedScores;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieves, at most, the highest scores for a given leaderboard. If you need more than 
/// the top 25 scores you should use an OFScoreEnumerator object.
///
/// @param downloadFilter	Set the filter for the leaderboard to download for.  This filter
///							type will be preserved on the leaderboard.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes	-(void)didDownloadHighScores:(NSArray*)highScores forOFLeaderboard:(OFLeaderboard*)leaderboard on success
///					-(void)didFailDownloadHighScoresForOFLeaderboard:(OFLeaderboard*)leaderboard on failure.
///
/// @see OFScoreEnumerator
/// @see OFRequestHandle
////////////////////////////////////F//////////////////////////////////////////////////////
- (OFRequestHandle*)downloadHighScoresWithFilter:(OFScoreFilter)downloadFilter;

//////////////////////////////////////////////////////////////////////////////////////////
/// Name of this leaderboard
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString*				name;

//////////////////////////////////////////////////////////////////////////////////////////
/// Leaderboard ID
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString*				leaderboardId;

//////////////////////////////////////////////////////////////////////////////////////////
/// High score of the current user on this leaderboard
///
/// @note A value of nil means the local user has not submitted a score for this 
///		  leaderboard
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) OFHighScore*			currentUserScore;

//////////////////////////////////////////////////////////////////////////////////////////
/// If @c YES then the highest score value is ranked #1, if @c NO then the lowest score
/// value is ranked #1.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)	BOOL					descendingScoreOrder;

///////////////////////////////////D///////////////////////////////////////////////////////
/// Determines which set of scores will be chosen from when downloading scores.
///
/// @note Defaults to OFScoreFilter_Everyone
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) OFScoreFilter				filter;

//////////////////////////////////////////////////////////////////////////////////////////
///@internal
//////////////////////////////////////////////////////////////////////////////////////////
- (id)initWithLocalSQL:(OFSqlQuery*)queryRow localUserScore:(OFHighScore*) locUserScore comparedUserScore:(OFHighScore*) compUserScore;
- (bool)isComparison;
- (OFUser*)comparedToUser;
+ (NSString*)getResourceName;

@end


//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFLeaderboardDelegate Protocol to receive information regarding 
/// OFLeaderboards.  You must call OFLeaderboard's +(void)setDelegate: method to receive
/// information.
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFLeaderboardDelegate
@optional

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when -(void)downloadHighScoresForLeaderboard: successfully completes.
///
/// @param leaderboard		The leaderboard which the OFHighScores belong to
/// @param highScores		An NSArray of OFHighScore objects
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadHighScores:(NSArray*)highScores OFLeaderboard:(OFLeaderboard*)leaderboard;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when -(void)downloadHighScoresForLeaderboard: fails to download.
///
/// @param leaderboard		The leaderboard in which we were trying to get high scores for
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadHighScoresOFLeaderboard:(OFLeaderboard*)leaderboard;

@end
