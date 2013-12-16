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

#import "OFResource.h"
#import "OFSqlQuery.h"
#import "OFCallbackable.h"

@class OFService;
@class OFUser;
@class OFS3UploadParameters;
@class OFLeaderboard;
@class OFRequestHandle;
@protocol OFHighScoreDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// OFScoreFilter allows you to control which set of scores are being operated on.
//////////////////////////////////////////////////////////////////////////////////////////
typedef enum
{
	// The Filter has not been set yet because nothing has been downloaded for this leaderboard
	OFScoreFilter_None = 0,
	/// Only consider the scores of users who are friends with the local user
	OFScoreFilter_FriendsOnly,
	/// Consider the scores of all users
	OFScoreFilter_Everybody
} OFScoreFilter;

//////////////////////////////////////////////////////////////////////////////////////////
/// @category Public
/// The public interface for OFHighScore exposes information for a single OpenFeint high 
/// score.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFHighScore : OFResource<OFCallbackable>
{
@private
	OFUser* user;
	int64_t score;
	NSInteger rank;
	NSString* leaderboardId;
	NSString* displayText;
	NSString* customData;
	NSData* blob;
	NSString* blobUrl;
	NSString* toHighRankText;
	OFS3UploadParameters* blobUploadParameters;
	double latitude;
	double longitude;
	double distance;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFHighScore related actions. Must adopt the 
/// OFHighScoreDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFHighScoreDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Retrieves the current user's high scores for all application leaderboards. 
///
/// @note The rank property on the OFHighScore objects returned from this method will not
///			be set.
///
/// @return NSArray of OFHighScore objects; one for each leaderboard.
//////////////////////////////////////////////////////////////////////////////////////////
+ (NSArray*)allHighScoresForCurrentUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Initializes a HighScore for sumbittion.  Use OFLeaderboard's submitScore: to submit 
/// this score to a leaderboard.
///
/// @param submitScore		score to submit
///
/// @note	You may optionally also fillout the 
///			dispalyText
///			customData 
///			blob
///			properties before submission and after initialization.
///
/// @return an initialized OFHighScore
//////////////////////////////////////////////////////////////////////////////////////////
- (OFHighScore*)initForSubmissionWithScore:(int64_t)submitScore;

//////////////////////////////////////////////////////////////////////////////////////////
/// Submit score to a leaderboard
///
/// @param leaderbard			The leaderboard to submit to.
///
/// @note	To create a high score to submit see OFHighScore's initForSubmissionWithScore:
///			method.
///
/// @see OFHighScore
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)submitTo:(OFLeaderboard*)leaderboard;

//////////////////////////////////////////////////////////////////////////////////////////
/// The blob needs to be explicity downloaded.  After calling this the blob property will
/// be filled out if we have data attached to this high score.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didDownloadBlob:(OFHighScore*)score; on success and
///						- (void)didFailDownloadBlob:(OFHighScore*)score; on failure
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)downloadBlob;

//////////////////////////////////////////////////////////////////////////////////////////
/// Raw integral score value
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, assign)			int64_t		score;

//////////////////////////////////////////////////////////////////////////////////////////
/// Formatted string representation of score suitable for display
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, retain)			NSString*	displayText;

//////////////////////////////////////////////////////////////////////////////////////////
/// Arbitrary data attached to this score submission
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, retain)			NSString*	customData;

//////////////////////////////////////////////////////////////////////////////////////////
/// The data uploaded with this high score.
///
/// @note You must explicitly call downloadBlob on a highscore to have this data filled out.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, retain)			NSData* blob;

//////////////////////////////////////////////////////////////////////////////////////////
/// Position on the leaderboard for this score. The best score has a rank of 1.
///
/// @note This field is only valid for scores that have been retrieved from the OpenFeint
///		  servers.
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)			NSInteger	rank;

//////////////////////////////////////////////////////////////////////////////////////////
/// OpenFeint User who submitted this score
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly)			OFUser*		user;

//////////////////////////////////////////////////////////////////////////////////////////
///@internal
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly, retain)	NSString*	leaderboardId;
@property (nonatomic, readonly, retain)	NSString* toHighRankText;
@property (nonatomic, readonly, retain)	NSString* blobUrl;
@property (nonatomic, readonly, retain)	OFS3UploadParameters* blobUploadParameters;
@property (nonatomic, readonly)			double latitude;
@property (nonatomic, readonly)			double longitude;
@property (nonatomic, readonly)			double distance;

- (id)initWithLocalSQL:(OFSqlQuery*)queryRow forUser:(OFUser*)hsUser rank:(NSUInteger)scoreRank;
- (BOOL)hasBlob;
- (void)_setBlob:(NSData*)_blob;
+ (NSString*)getResourceName;

@end

//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFHighScoreDelegate Protocol to receive information regarding 
/// OFHighScore.  You must call OFHighScore's +(void)setDelegate: method to receive
/// information.
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFHighScoreDelegate
@optional

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when submitTo successfully completes for the given score
///
/// @param score			The score that successfully was submitted
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didSubmit:(OFHighScore*)score;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when submitTo fails for the given score
///
/// @param score			The score that failed to submit.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailSubmit:(OFHighScore*)score;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when downloadBlob successfully completes
///
/// @param score	The OFHighScore which now has the blob property filled out.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadBlob:(OFHighScore*)score;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked when downloadBlob fails
///
/// @param score	The score which failed to download the blob data.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadBlob:(OFHighScore*)score;

@end
