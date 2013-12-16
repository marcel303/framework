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

#import "OFHighScoreService+Private.h"
#import "OFSqlQuery.h"
#import "OFReachability.h"
#import "OFActionRequestType.h"
#import "OFService+Private.h"
#import "OpenFeint+Private.h"
#import "OpenFeint+UserOptions.h"
#import <sqlite3.h>
#import "OFLeaderboardService+Private.h"
#import "OFHighScoreBatchEntry.h"
#import "OFHighScore.h"
#import "OFUser.h"
#import "OFUserService+Private.h"
#import "OFPaginatedSeries.h"
#import "OFOfflineService.h"
#import "OFLeaderboard+Sync.h";
#import "OFNotification.h";
#import "OFCloudStorageService.h"
#import "OFS3Response.h"

namespace
{
	static OFSqlQuery sSetHighScoreQuery;
	static OFSqlQuery sPendingHighScoresQuery;
	static OFSqlQuery sScoreToKeepQuery;
	static OFSqlQuery sServerSynchQuery;
	static OFSqlQuery sDeleteScoresQuery;
	static OFSqlQuery sMakeOnlyOneSynchQuery;
	static OFSqlQuery sLastSynchQuery;
	static OFSqlQuery sGetHighScoresQuery;
	static OFSqlQuery sChangeNullUserQuery;
    static OFSqlQuery sNullUserLeaderboardsQuery;
}

// A regular dictionary won't work. If you submit 2 scores in quick succession then it's important that when the first call returns it doesn't think
// the second calls blob belongs to it. We also want to clear based on only leaderboard id so using a combination of leaderboard and score as a NSDictionaty key wont work.
@interface OFPendingBlob : NSObject
{
	NSString* leaderboardId;
	int64_t score;
	NSData* blob;
}

@property (nonatomic, retain) NSString* leaderboardId;
@property (nonatomic, retain) NSData* blob;
@property (nonatomic, assign) int64_t score;

@end

@implementation OFPendingBlob

@synthesize leaderboardId, score, blob;

- (id)initWithLeaderboardId:(NSString*)_leaderboardId andScore:(int64_t)_score andBlob:(NSData*)_blob
{
	self = [super init];
	if (self)
	{
		self.leaderboardId = _leaderboardId;
		self.score = _score;
		self.blob = _blob;
	}
	return self;
}

- (void)dealloc
{
	self.leaderboardId = nil;
	self.blob = nil;
	[super dealloc];
}

@end


@implementation OFHighScoreService (Private)

- (id) init
{
	self = [super init];
	
	if (self != nil)
	{
		mPendingBlobs = [NSMutableArray new];
	}
	
	return self;
}

- (void) dealloc
{
	OFSafeRelease(mPendingBlobs);
	sSetHighScoreQuery.destroyQueryNow();
	sPendingHighScoresQuery.destroyQueryNow();
	sScoreToKeepQuery.destroyQueryNow();
	sServerSynchQuery.destroyQueryNow();
	sDeleteScoresQuery.destroyQueryNow();
	sMakeOnlyOneSynchQuery.destroyQueryNow();
	sLastSynchQuery.destroyQueryNow();
	sGetHighScoresQuery.destroyQueryNow();
	sChangeNullUserQuery.destroyQueryNow();
	sNullUserLeaderboardsQuery.destroyQueryNow();
	[super dealloc];
}

+ (void) setupOfflineSupport:(bool)recreateDB
{
	if( recreateDB )
	{
		OFSqlQuery(
			[OpenFeint getOfflineDatabaseHandle],
			"DROP TABLE IF EXISTS high_scores"
			).execute();
	}
	
	//Special PG patch
	OFSqlQuery(
		[OpenFeint getOfflineDatabaseHandle],
		"ALTER TABLE high_scores " 
		"ADD COLUMN display_text TEXT DEFAULT NULL",
		false
		).execute(false);
	//

	int highScoresVersion = [OFOfflineService getTableVersion:@"high_scores"];
	if (highScoresVersion == 1)
	{
		OFSqlQuery(
			[OpenFeint getOfflineDatabaseHandle],
			"ALTER TABLE high_scores " 
			"ADD COLUMN custom_data TEXT DEFAULT NULL"
			).execute();
		highScoresVersion = 2;
	}
	if (highScoresVersion == 2)
	{
		OFSqlQuery(
			[OpenFeint getOfflineDatabaseHandle],
			"ALTER TABLE high_scores " 
			"ADD COLUMN blob BLOB DEFAULT NULL"
			).execute(); 
	}
	else
	{
		OFSqlQuery(
			[OpenFeint getOfflineDatabaseHandle],
			"CREATE TABLE IF NOT EXISTS high_scores("
			"user_id INTEGER NOT NULL,"
			"leaderboard_id INTEGER NOT NULL,"
			"score INTEGER DEFAULT 0,"
			"display_text TEXT DEFAULT NULL,"
			"custom_data TEXT DEFAULT NULL,"
			"server_sync_at INTEGER DEFAULT NULL,"
			"blob BLOB DEFAULT NULL,"
			"UNIQUE(leaderboard_id, user_id, score))"
			).execute();
		
		OFSqlQuery(
			[OpenFeint getOfflineDatabaseHandle], 
			"CREATE INDEX IF NOT EXISTS high_scores_index "
			"ON high_scores (user_id, leaderboard_id)"
			).execute();
	}
	[OFOfflineService setTableVersion:@"high_scores" version:3];
	
	sSetHighScoreQuery.reset(
		[OpenFeint getOfflineDatabaseHandle], 
		"REPLACE INTO high_scores "
		"(user_id, leaderboard_id, score, display_text, custom_data, blob, server_sync_at) "
		"VALUES(:user_id, :leaderboard_id, :score, :display_text, :custom_data, :blob, :server_sync_at)"
		);
	
	sPendingHighScoresQuery.reset(
		[OpenFeint getOfflineDatabaseHandle],
		"SELECT leaderboard_id, score, display_text, custom_data, blob "
		"FROM high_scores "
		"WHERE user_id = :user_id AND "
		"server_sync_at IS NULL"
		);
	
	//for testing
	//OFSqlQuery([OpenFeint getOfflineDatabaseHandle], "UPDATE high_scores SET server_sync_at = NULL").execute();
	
	sScoreToKeepQuery.reset(
		[OpenFeint getOfflineDatabaseHandle],
		"SELECT min(score) AS score FROM "
		"(SELECT score FROM high_scores  "
		"WHERE user_id = :user_id AND "
		"leaderboard_id = :leaderboard_id "
		"ORDER BY score DESC LIMIT :max_scores) AS x"
		);

	sDeleteScoresQuery.reset(
		[OpenFeint getOfflineDatabaseHandle],
		"DELETE FROM high_scores  "
		"WHERE user_id = :user_id AND "
		"leaderboard_id = :leaderboard_id AND "
		"score < :score"
		);
	
	sMakeOnlyOneSynchQuery.reset(
		[OpenFeint getOfflineDatabaseHandle],
		"UPDATE high_scores "
		"SET server_sync_at = :server_sync_at "
		"WHERE user_id = :user_id AND "
		"leaderboard_id = :leaderboard_id AND "
		"score != :score AND "
		"server_sync_at IS NULL"
		);
	
	sChangeNullUserQuery.reset(
		[OpenFeint getOfflineDatabaseHandle],
		"UPDATE high_scores "
		"SET user_id = :user_id "
		"WHERE user_id IS NULL or user_id = 0"
		);
	
	sNullUserLeaderboardsQuery.reset(
		[OpenFeint getOfflineDatabaseHandle],
		"SELECT DISTINCT(leaderboard_id) FROM high_scores "
		"WHERE user_id IS NULL or user_id = 0"
		);
}

+ (bool) localSetHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId forUser:(NSString*)userId
{
	return [OFHighScoreService localSetHighScore:score forLeaderboard:leaderboardId forUser:userId displayText:nil serverDate:nil addToExisting:NO];
}

+ (bool) localSetHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId forUser:(NSString*)userId displayText:(NSString*)displayText serverDate:(NSDate*)serverDate addToExisting:(BOOL) addToExisting
{
	return [OFHighScoreService localSetHighScore:score forLeaderboard:leaderboardId forUser:userId displayText:displayText customData:nil serverDate:nil addToExisting:NO];
}

+ (bool) localSetHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId forUser:(NSString*)userId displayText:(NSString*)displayText customData:(NSString*)customData serverDate:(NSDate*)serverDate addToExisting:(BOOL) addToExisting
{
	return [OFHighScoreService localSetHighScore:score forLeaderboard:leaderboardId forUser:userId displayText:displayText customData:customData serverDate:serverDate addToExisting:addToExisting shouldSubmit:nil];
}

+ (bool) localSetHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId forUser:(NSString*)userId displayText:(NSString*)displayText customData:(NSString*)customData serverDate:(NSDate*)serverDate addToExisting:(BOOL)addToExisting shouldSubmit:(BOOL*)outShouldSubmit
{
	return [OFHighScoreService localSetHighScore:score forLeaderboard:leaderboardId forUser:userId displayText:displayText customData:customData blob:nil serverDate:serverDate addToExisting:addToExisting shouldSubmit:nil overrideExisting:YES];
}

+ (bool) localSetHighScore:(int64_t)score 
			forLeaderboard:(NSString*)leaderboardId 
				   forUser:(NSString*)userId 
			   displayText:(NSString*)displayText 
				customData:(NSString*)customData 
					  blob:(NSData*)blob
				serverDate:(NSDate*)serverDate 
			 addToExisting:(BOOL) addToExisting 
			  shouldSubmit:(BOOL*)outShouldSubmit
		  overrideExisting:(BOOL)overrideExisting
{
	BOOL success = NO;
	BOOL shouldSubmitToServer = YES;
	OFLeaderboard_Sync* leaderboard = [OFLeaderboardService getLeaderboardDetails:leaderboardId];
	
	if (leaderboard && (!leaderboard.isAggregate || addToExisting))
	{
		NSString* serverSynch = nil;
		if( serverDate )
		{
			serverSynch = [NSString stringWithFormat:@"%d", (long)[serverDate timeIntervalSince1970]];
		}
		NSString*lastSyncDate = [OFLeaderboardService getLastSyncDateUnixForUserId:userId];
		int64_t previousScore = 0;
		BOOL hasPreviousScore = [OFHighScoreService getPreviousHighScoreLocal:&previousScore forLeaderboard:leaderboardId];
		if (addToExisting && hasPreviousScore)
		{
			score =  previousScore + score;
		}
		
		//@note allowPostingLowerScores actually means allow posting WORSE scores
		if (!leaderboard.allowPostingLowerScores && hasPreviousScore)
		{
			if ((leaderboard.descendingSortOrder && score <= previousScore) ||	// if higher is better and this new score is lower
				(!leaderboard.descendingSortOrder && score >= previousScore))	// or lower is better and this new score is higher
			{
				if (blob == nil || score != previousScore)
				{
					shouldSubmitToServer = NO;										// don't submit it to the server
				}
			}
		}
		
		
		[OFHighScoreService buildSetHighScoreQuery:overrideExisting];
		
		NSString* sScore = [NSString stringWithFormat:@"%qi", score];
		sSetHighScoreQuery.bind("user_id", userId);		
		sSetHighScoreQuery.bind("leaderboard_id", leaderboardId);
		sSetHighScoreQuery.bind("score", sScore);
		sSetHighScoreQuery.bind("display_text", displayText);
		sSetHighScoreQuery.bind("custom_data", customData);
		NSString* newScoresSynchTime = serverSynch;
		if (!newScoresSynchTime && !shouldSubmitToServer)
		{
			// If it shouldn't be submitted then mark it as synched right away
			newScoresSynchTime = [NSString stringWithFormat:@"%d", (long)[[NSDate date] timeIntervalSince1970]];
		}
		sSetHighScoreQuery.bind("server_sync_at", newScoresSynchTime);
		if (blob)
		{
			sSetHighScoreQuery.bind("blob", blob.bytes, blob.length);
		}
		
		sSetHighScoreQuery.execute();
		success = (sSetHighScoreQuery.getLastStepResult() == SQLITE_OK);
		sSetHighScoreQuery.resetQuery();
		
		
		[self buildScoreToKeepQuery:leaderboard.descendingSortOrder];
		sScoreToKeepQuery.bind("leaderboard_id", leaderboardId);
		sScoreToKeepQuery.bind("user_id", userId);		
		sScoreToKeepQuery.execute();
		if( sScoreToKeepQuery.getLastStepResult() == SQLITE_ROW )
		{
			[self buildDeleteScoresQuery:leaderboard.descendingSortOrder];
			NSString* scoreToKeep = [NSString stringWithFormat:@"%qi", sScoreToKeepQuery.getInt64("keep_score")];
			sDeleteScoresQuery.bind("leaderboard_id", leaderboardId);
			sDeleteScoresQuery.bind("user_id", userId);		
			sDeleteScoresQuery.bind("score", scoreToKeep);		
			sDeleteScoresQuery.execute();
			sDeleteScoresQuery.resetQuery();
		}
		NSString* synchScore = leaderboard.allowPostingLowerScores ? sScore : [NSString stringWithFormat:@"%qi", sScoreToKeepQuery.getInt64("high_score")];
		sScoreToKeepQuery.resetQuery();
		
		// [adill note] this normally sets server_sync_at for all scores other
		// than this one. we want to avoid that if the leaderboard allows worse
		// scores and this score is sourced from a server sync (during bootstrap)
		// because it will mark the latest offline score as un-sync'd -- which is bad.
		if (!(leaderboard.allowPostingLowerScores && serverSynch))
		{
			//want only one pending score, but keep history of other scores
			sMakeOnlyOneSynchQuery.bind("leaderboard_id", leaderboardId);
			sMakeOnlyOneSynchQuery.bind("user_id", userId);		
			sMakeOnlyOneSynchQuery.bind("score", synchScore);
			sMakeOnlyOneSynchQuery.bind("server_sync_at", lastSyncDate);
			sMakeOnlyOneSynchQuery.execute();
			sMakeOnlyOneSynchQuery.resetQuery();
		}
		
		//Is leaderboard part of an aggregate
		NSMutableArray* aggregateLeaderboards = [OFLeaderboardService getAggregateParents:leaderboardId];
		for (unsigned int i = 0; i < [aggregateLeaderboards count]; i++)
		{
			OFLeaderboard_Sync* parentLeaderboard = (OFLeaderboard_Sync*)[aggregateLeaderboards objectAtIndex:i];
			[OFHighScoreService localSetHighScore:(score - previousScore)
								   forLeaderboard:parentLeaderboard.resourceId
										  forUser:userId 
									  displayText:nil
									  customData:nil
									   serverDate:[NSDate date]
									addToExisting:YES];
			//[parentLeaderboard release];
		}
	}

	if (outShouldSubmit != nil)
	{
		(*outShouldSubmit) = shouldSubmitToServer;
	}

	return success;
}

+ (bool) synchHighScore:(NSString*)userId
{
	sServerSynchQuery.bind("user_id", userId);	
	sServerSynchQuery.execute();
	bool success = (sServerSynchQuery.getLastStepResult() == SQLITE_OK);
	sServerSynchQuery.resetQuery();
	return success;
}

+ (OFRequestHandle*) sendPendingHighScores:(NSString*)userId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFRequestHandle* handle = nil;
	
	if ([OpenFeint isOnline] && userId != @"Invalid" && [userId longLongValue] > 0)
	{
		int64_t leaderboardBestScore = 0;
		NSString* leaderboardId = nil;

		NSString*lastSyncDate = [OFLeaderboardService getLastSyncDateUnixForUserId:userId];
		
		//Get leaderboards with no user_id
		sNullUserLeaderboardsQuery.execute();
		
		//associate any offline high scores to user
		sChangeNullUserQuery.bind("user_id", userId);
		sChangeNullUserQuery.execute();
		sChangeNullUserQuery.resetQuery();
		
		for (; !sNullUserLeaderboardsQuery.hasReachedEnd(); sNullUserLeaderboardsQuery.step())
		{
			leaderboardId = [NSString stringWithFormat:@"%d", sNullUserLeaderboardsQuery.getInt("leaderboard_id")];
			[OFHighScoreService getPreviousHighScoreLocal:&leaderboardBestScore forLeaderboard:leaderboardId];
			sMakeOnlyOneSynchQuery.bind("score", [NSString stringWithFormat:@"%qi", leaderboardBestScore]);
			sMakeOnlyOneSynchQuery.bind("leaderboard_id", leaderboardId);
			sMakeOnlyOneSynchQuery.bind("user_id", userId);
			sMakeOnlyOneSynchQuery.bind("server_sync_at", lastSyncDate);
			sMakeOnlyOneSynchQuery.execute();
			sMakeOnlyOneSynchQuery.resetQuery();
		}

		sNullUserLeaderboardsQuery.resetQuery();
		
		OFHighScoreBatchEntrySeries pendingHighScores;
		sPendingHighScoresQuery.bind("user_id", userId);
		for (sPendingHighScoresQuery.execute(); !sPendingHighScoresQuery.hasReachedEnd(); sPendingHighScoresQuery.step())
		{
			OFHighScoreBatchEntry *highScore = new OFHighScoreBatchEntry();
			highScore->leaderboardId = [NSString stringWithFormat:@"%d", sPendingHighScoresQuery.getInt("leaderboard_id")];
			highScore->score = sPendingHighScoresQuery.getInt64("score");
			const char* cDisplayText = sPendingHighScoresQuery.getText("display_text");
			if( cDisplayText != nil )
			{
				highScore->displayText = [NSString stringWithUTF8String:cDisplayText];
			}
			const char* cCustomData = sPendingHighScoresQuery.getText("custom_data");
			if( cCustomData != nil )
			{
				highScore->customData = [NSString stringWithUTF8String:cCustomData];
			}
			const char* bytes = NULL;
			unsigned int blobLength = 0;
			sPendingHighScoresQuery.getBlob("blob", bytes, blobLength);
			if (blobLength > 0)
			{
				highScore->blob = [NSData dataWithBytes:bytes length:blobLength];
			}

			pendingHighScores.push_back(highScore);
		}
		sPendingHighScoresQuery.resetQuery();
		
		if (pendingHighScores.size() > 0)
		{
			OFDelegate chainedSuccessDelegate([OFHighScoreService sharedInstance], @selector(_onSetHighScore:nextCall:), onSuccess);

			if (false) //(!silently)
			{
				OFNotificationData* notice = [OFNotificationData dataWithText:[NSString stringWithFormat:@"Submitted %i Score%s", pendingHighScores.size(), pendingHighScores.size() > 1 ? "" : "s"] andCategory:kNotificationCategoryLeaderboard andType:kNotificationTypeSuccess];
				[[OFNotification sharedInstance] showBackgroundNotice:notice andStatus:OFNotificationStatusSuccess andInputResponse:nil];
			}
			
			handle = [OFHighScoreService 
			     batchSetHighScores:pendingHighScores
						   silently:YES
				          onSuccess:chainedSuccessDelegate
						  onFailure:onFailure
					optionalMessage: nil
						  fromSynch:YES
			 ];
		}
	}
	
	return handle;
}

+ (BOOL) getPreviousHighScoreLocal:(int64_t*)score forLeaderboard:(NSString*)leaderboardId
{
	OFLeaderboard_Sync* leaderboard = [OFLeaderboardService getLeaderboardDetails:leaderboardId];
	[self buildGetHighScoresQuery:leaderboard.descendingSortOrder limit:1];
	sGetHighScoresQuery.bind("leaderboard_id", leaderboardId);
	sGetHighScoresQuery.bind("user_id", [OpenFeint localUser].resourceId);
	sGetHighScoresQuery.execute(); 
	
	BOOL foundScore = NO;
	int64_t scoreToReturn = 0;	// for historical reasons we're going to set 'score' to 0 even if we don't have a score
	if (!sGetHighScoresQuery.hasReachedEnd())
	{
		foundScore = YES;
		scoreToReturn = sGetHighScoresQuery.getInt64("score");
	}
	sGetHighScoresQuery.resetQuery();
	
	if (score != nil)
	{
		(*score) = scoreToReturn;
	}

	return foundScore;
}

+ (void) getHighScoresLocal:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPaginatedSeries* page = [OFPaginatedSeries paginatedSeriesFromArray:[self getHighScoresLocal:leaderboardId]];
	onSuccess.invoke(page);
}

+ (NSArray*) getHighScoresLocal:(NSString*)leaderboardId
{
	NSMutableArray* highScores = [NSMutableArray arrayWithCapacity:10];
	
	OFLeaderboard_Sync* leaderboard = [OFLeaderboardService getLeaderboardDetails:leaderboardId];
	[self buildGetHighScoresQuery:leaderboard.descendingSortOrder limit:10];
	sGetHighScoresQuery.bind("leaderboard_id", leaderboardId);
	sGetHighScoresQuery.bind("user_id", [OpenFeint localUser].resourceId);
	NSUInteger rank = 0;
	for (sGetHighScoresQuery.execute(); !sGetHighScoresQuery.hasReachedEnd(); sGetHighScoresQuery.step())
	{
		OFUser* user = [OFUserService getLocalUser:[NSString stringWithFormat:@"%s", sGetHighScoresQuery.getText("user_id")]];
		[highScores addObject:[[[OFHighScore alloc] initWithLocalSQL:&sGetHighScoresQuery forUser:user rank:++rank] autorelease]];
	}
	sGetHighScoresQuery.resetQuery();
	
	return highScores;
}

+ (OFHighScore*)getHighScoreForUser:(OFUser*)user leaderboardId:(NSString*)leaderboardId descendingSortOrder:(bool)descendingSortOrder
{
	[self buildGetHighScoresQuery:descendingSortOrder limit:1];
	sGetHighScoresQuery.bind("leaderboard_id", leaderboardId);
	sGetHighScoresQuery.bind("user_id", user.resourceId);
	sGetHighScoresQuery.execute(); 
	OFHighScore* highScore = !sGetHighScoresQuery.hasReachedEnd() ? [[[OFHighScore alloc] initWithLocalSQL:&sGetHighScoresQuery forUser:user rank:1] autorelease] : nil;
	sGetHighScoresQuery.resetQuery();
	return highScore;
}

+ (void) uploadBlob:(NSData*)blob forHighScore:(OFHighScore*)highScore
{
	if (!highScore.blobUploadParameters)
	{
		OFLog(@"Trying to upload a blob for a high score that doesn't have any upload parameters");
		return;
	}
	if (!blob)
	{
		OFLog(@"Trying to upload a nil high score blob");
		return;
	}
	[highScore _setBlob:blob];
	OFDelegate success([OFHighScoreService sharedInstance], @selector(onBlobUploaded:));
	OFDelegate failure([OFHighScoreService sharedInstance], @selector(onBlobUploadFailed));
	[OFCloudStorageService uploadS3Blob:blob withParameters:highScore.blobUploadParameters passThroughUserData:highScore onSuccess:success onFailure:failure];
}

- (void)onBlobUploaded:(OFS3Response*)response
{
	OFHighScore* highScore = (OFHighScore*)response.userParam;
	[OFHighScoreService 
		localSetHighScore:highScore.score
		forLeaderboard:[NSString stringWithFormat:@"%d", highScore.leaderboardId]
		forUser:highScore.user.resourceId
		displayText:highScore.displayText
		customData:highScore.customData
		blob:highScore.blob
		serverDate:[NSDate date]
		addToExisting:NO
		shouldSubmit:nil
	overrideExisting:YES];
}

- (void)onBlobUploadFailed
{
	OFLog(@"Failed to upload high score blob");
}

+ (void) buildGetHighScoresQuery:(bool)descendingOrder limit:(int)limit
{
	NSMutableString* query = [[[NSMutableString alloc] initWithString:@"SELECT * FROM high_scores WHERE leaderboard_id = :leaderboard_id AND user_id = :user_id ORDER BY score "] autorelease];
	NSString* orderClause = (descendingOrder ? @"DESC" : @"ASC");
	[query appendString:orderClause];
	[query appendString:[NSString stringWithFormat:@" LIMIT %i", limit]];
	sGetHighScoresQuery.reset( [OpenFeint getOfflineDatabaseHandle], [query UTF8String] );
}

+ (void) buildScoreToKeepQuery:(bool)descendingOrder
{
	//Book keeping save top 10 scores
	NSMutableString* query = [[[NSMutableString alloc] initWithString:@"SELECT "] autorelease];
	NSString* scoreClause = (descendingOrder ? @"min" : @"max");
	[query appendString:scoreClause];
	[query appendString:@"(x.score) AS keep_score, "];
	scoreClause = (descendingOrder ? @"max" : @"min");
	[query appendString:scoreClause];
	[query appendString:@"(x.score) AS high_score FROM (SELECT score FROM high_scores WHERE user_id = :user_id AND leaderboard_id = :leaderboard_id ORDER BY score "];
	NSString* orderClause = (descendingOrder ? @"DESC" : @"ASC");
	[query appendString:orderClause];
	[query appendString:@" LIMIT 10) AS x"];
	sScoreToKeepQuery.reset( [OpenFeint getOfflineDatabaseHandle], [query UTF8String]);
}

+ (void) buildDeleteScoresQuery:(bool)descendingOrder
{
	NSMutableString* query =[[[NSMutableString alloc] initWithString:@"DELETE FROM high_scores WHERE user_id = :user_id AND leaderboard_id = :leaderboard_id AND score "] autorelease];
	NSString* comparison = (descendingOrder ? @"<" : @">");
	[query appendString:comparison];
	[query appendString:@" :score"];
	sDeleteScoresQuery.reset([OpenFeint getOfflineDatabaseHandle], [query UTF8String]);
}

+ (void) buildSetHighScoreQuery:(bool)replaceExisting
{
	NSMutableString* query =[[[NSMutableString alloc] initWithString:replaceExisting ? @"REPLACE " : @"INSERT OR IGNORE "] autorelease];
	[query appendString:@"INTO high_scores (user_id, leaderboard_id, score, display_text, custom_data, blob, server_sync_at) "
						"VALUES(:user_id, :leaderboard_id, :score, :display_text, :custom_data, :blob, :server_sync_at)"];
	sSetHighScoreQuery.reset([OpenFeint getOfflineDatabaseHandle], [query UTF8String]);
}

- (NSData*)_getPendingBlobForLeaderboard:(NSString*)leaderboardId andScore:(int64_t)score
{
	for (OFPendingBlob* pendingBlob in mPendingBlobs)
	{
		if (pendingBlob.score == score && [pendingBlob.leaderboardId isEqualToString:leaderboardId])
		{
			return pendingBlob.blob;
		}
	}
	return nil;
}

+ (NSData*)getPendingBlobForLeaderboard:(NSString*)leaderboardId andScore:(int64_t)score
{
	return [[OFHighScoreService sharedInstance] _getPendingBlobForLeaderboard:leaderboardId andScore:score];
}

- (void)_setPendingBlob:(NSData*)blob forLeaderboard:(NSString*)leaderboardId andScore:(int64_t)score
{
	[OFHighScoreService removePendingBlobForLeaderboard:leaderboardId];
	OFPendingBlob* pendingBlob = [[[OFPendingBlob alloc] initWithLeaderboardId:leaderboardId andScore:score andBlob:blob] autorelease];
	[mPendingBlobs addObject:pendingBlob];
}

+ (void)setPendingBlob:(NSData*)blob forLeaderboard:(NSString*)leaderboardId andScore:(int64_t)score
{
	[[OFHighScoreService sharedInstance] _setPendingBlob:blob forLeaderboard:leaderboardId andScore:score];
}

- (void)_removePendingBlobForLeaderboard:(NSString*)leaderboardId
{
	for (OFPendingBlob* pendingBlob in mPendingBlobs)
	{
		if ([pendingBlob.leaderboardId isEqualToString:leaderboardId])
		{
			[mPendingBlobs removeObject:pendingBlob];
			return;
		}
	}
}

+ (void)removePendingBlobForLeaderboard:(NSString*)leaderboardId
{
	[[OFHighScoreService sharedInstance] _removePendingBlobForLeaderboard:leaderboardId];
}

+ (void)reportMissingBlobForHighScore:(OFHighScore*)highScore
{			
	if (!highScore)
	{
		OFLog(@"Reporting missing blob for nil high score");
		return;
	}
	
	[[self sharedInstance]
			postAction:[NSString stringWithFormat:@"high_scores/%@/invalidate_blob.xml", highScore.resourceId]
			 withParameters:NULL
			 withSuccess:OFDelegate()
			 withFailure:OFDelegate()
			 withRequestType:OFActionRequestSilent
			 withNotice:nil];
			
}

@end
