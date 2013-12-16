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

#import "OFService.h"
#import "OFHighScoreBatchEntry.h"
#import <CoreLocation/CoreLocation.h>

@class OFLeaderboard;
@class OFHighScore;
@class OFRequestHandle;

@interface OFHighScoreService : OFService
{
	NSMutableArray* mPendingBlobs;
}

OPENFEINT_DECLARE_AS_SERVICE(OFHighScoreService);

+ (OFRequestHandle*) getPage:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId friendsOnly:(BOOL)friendsOnly onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) getPage:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId friendsOnly:(BOOL)friendsOnly silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) getPage:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId comparedToUserId:(NSString*)comparedToUserId friendsOnly:(BOOL)friendsOnly silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) getPage:(NSInteger)pageIndex pageSize:(NSInteger)pageSize forLeaderboard:(NSString*)leaderboardId comparedToUserId:(NSString*)comparedToUserId friendsOnly:(BOOL)friendsOnly silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) getPageWithLoggedInUserForLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) getPageWithLoggedInUserWithPageSize:(NSInteger)pageSize forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) getPageWithLoggedInUserWithPageSize:(NSInteger)pageSize forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (void) getLocalHighScores:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

+ (OFRequestHandle*) setHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) setHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

// When withDisplayText is set it's shown INSTEAD of the score. If you want to show the score as well 
// as something else the score must be embedded in the display text
+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

// When withCustomData is stored on the server along with the score
+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText withCustomData:(NSString*)customData forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText withCustomData:(NSString*)customData forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently deferred:(BOOL)deferred onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;


// The blob can be up to 50k and must be downloaded separately 
+ (OFRequestHandle*) setHighScore:(int64_t)score 
	  withDisplayText:(NSString*)displayText 
	   withCustomData:(NSString*)customData 
			 withBlob:(NSData*)blob
	   forLeaderboard:(NSString*)leaderboardId 
			 silently:(BOOL)silently 
						 deferred:(BOOL)deferred
			onSuccess:(const OFDelegate&)onSuccess 
			onFailure:(const OFDelegate&)onFailure;

//+ (void) queueHighScore:(int64_t)score withDisplayText:(NSString*)displayText forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
//+ (void) sendQueuedHighScores:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

+ (OFRequestHandle*) batchSetHighScores:(OFHighScoreBatchEntrySeries&)highScoreBatchEntrySeries onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage;
+ (OFRequestHandle*) batchSetHighScores:(OFHighScoreBatchEntrySeries&)highScoreBatchEntrySeries silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage;
+ (OFRequestHandle*) batchSetHighScores:(OFHighScoreBatchEntrySeries&)highScoreBatchEntrySeries silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage fromSynch:(BOOL)fromSynch;

// High scores returned through getAllHighScoresForLoggedInUser do not have their rank set.
+ (void) getAllHighScoresForLoggedInUser:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage;

+ (void) getHighScoresFromLocation:(CLLocation*)origin radius:(int)radius pageIndex:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (void) getHighScoresFromLocation:(CLLocation*)origin radius:(int)radius pageIndex:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId userMapMode:(NSString*)userMapMode onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

+ (OFRequestHandle*) getDistributedHighScoresAtPage:(NSInteger)pageIndex 
										   pageSize:(NSInteger)pageSize 
										 scoreDelta:(NSInteger)scoreDelta
										 startScore:(NSInteger)startScore
									 forLeaderboard:(NSString*)leaderboardId 
										  onSuccess:(const OFDelegate&)onSuccess 
										  onFailure:(const OFDelegate&)onFailure;

+ (OFRequestHandle*) downloadBlobForHighScore:(OFHighScore*)highScore onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
@end
