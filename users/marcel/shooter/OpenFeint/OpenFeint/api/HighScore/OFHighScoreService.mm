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

#import "OFDependencies.h"
#import "OFHighScoreService.h"
#import "OFHttpNestedQueryStringWriter.h"
#import "OFService+Private.h"
#import "OFHighScore.h"
#import "OFAbridgedHighScore.h"
#import "OFLeaderboard.h"
#import "OFNotificationData.h"
#import "OFHighScoreService+Private.h"
#import "OFDelegateChained.h"
#import "OpenFeint+UserOptions.h"
#import "OpenFeint+Private.h"
#import "OFReachability.h"
#import "OFUser.h"
#import "OFNotification.h"
#import "OFS3Response.h"
#import "OFS3UploadParameters.h"
#import "OFCloudStorageService.h"

#define kDefaultPageSize 25
#define kMaxHighScoreBlobSize (1024*50)

OPENFEINT_DEFINE_SERVICE_INSTANCE(OFHighScoreService);

@implementation OFHighScoreService

OPENFEINT_DEFINE_SERVICE(OFHighScoreService);

- (void) populateKnownResources:(OFResourceNameMap*)namedResources
{
	namedResources->addResource([OFHighScore getResourceName], [OFHighScore class]);
	namedResources->addResource([OFAbridgedHighScore getResourceName], [OFAbridgedHighScore class]);
	namedResources->addResource([OFS3UploadParameters getResourceName], [OFS3UploadParameters class]);
}

+ (OFRequestHandle*) getPage:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId friendsOnly:(BOOL)friendsOnly onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService getPage:pageIndex forLeaderboard:leaderboardId friendsOnly:friendsOnly silently:NO onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) getPage:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId friendsOnly:(BOOL)friendsOnly silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService getPage:pageIndex
				 forLeaderboard:leaderboardId
			   comparedToUserId:nil
					friendsOnly:friendsOnly
					   silently:silently
					  onSuccess:onSuccess
					  onFailure:onFailure];
}

+ (OFRequestHandle*) getPage:(NSInteger)pageIndex 
  forLeaderboard:(NSString*)leaderboardId 
comparedToUserId:(NSString*)comparedToUserId 
	 friendsOnly:(BOOL)friendsOnly
		silently:(BOOL)silently
	   onSuccess:(const OFDelegate&)onSuccess 
	   onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService 
		getPage:pageIndex 
		pageSize:kDefaultPageSize 
		forLeaderboard:leaderboardId 
		comparedToUserId:comparedToUserId
		friendsOnly:friendsOnly 
		silently:silently
		onSuccess:onSuccess 
		onFailure:onFailure];
}

+ (OFRequestHandle*) getPage:(NSInteger)pageIndex pageSize:(NSInteger)pageSize forLeaderboard:(NSString*)leaderboardId comparedToUserId:(NSString*)comparedToUserId friendsOnly:(BOOL)friendsOnly silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("leaderboard_id", leaderboardId);
	params->io("page", pageIndex);
	params->io("page_size", pageSize);
	
	if (friendsOnly)
	{
		bool friendsLeaderboard = true;
		OFRetainedPtr<NSString> followerId = @"me";
		params->io("friends_leaderboard", friendsLeaderboard);
		params->io("follower_id", followerId);
	}
	
	if (comparedToUserId && [comparedToUserId length] > 0)
	{
		params->io("compared_user_id", comparedToUserId);
	}
	
	OFActionRequestType requestType = silently ? OFActionRequestSilent : OFActionRequestForeground;
	
	return [[self sharedInstance] 
	 getAction:@"client_applications/@me/high_scores.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:requestType
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Downloaded High Scores"]];
}

+ (void) getLocalHighScores:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[OFHighScoreService getHighScoresLocal:leaderboardId onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) getPageWithLoggedInUserWithPageSize:(NSInteger)pageSize forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("leaderboard_id", leaderboardId);	
	params->io("near_user_id", @"me");
	params->io("page_size", pageSize);
	
	return [[self sharedInstance]
	 getAction:@"client_applications/@me/high_scores.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
     withRequestType:silently ? OFActionRequestSilent : OFActionRequestForeground
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Downloaded High Scores"]];
}

+ (OFRequestHandle*) getPageWithLoggedInUserWithPageSize:(NSInteger)pageSize forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
    return [OFHighScoreService getPageWithLoggedInUserWithPageSize:pageSize forLeaderboard:leaderboardId silently:NO onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) getPageWithLoggedInUserForLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService getPageWithLoggedInUserWithPageSize:kDefaultPageSize forLeaderboard:leaderboardId onSuccess:onSuccess onFailure:onFailure];
}

- (void)_onSetHighScore:(NSArray*)resources nextCall:(OFDelegateChained*)nextCall
{
	unsigned int highScoreCnt = [resources count];
	for (unsigned int i = 0; i < highScoreCnt; i++ )
	{
		OFHighScore* highScore = [resources objectAtIndex:i];
		NSData* blob = [OFHighScoreService getPendingBlobForLeaderboard:highScore.leaderboardId andScore:highScore.score];
				
		// When there is a blob to upload we don't store the score locally until the blob is done uploading. This means it doesn't get marked as synced and if 
		// something goes wrong or the game closes before uploading the blob then next time the entire highscore will get synced again and if it's still the best
		// the blob will get uploaded again.
		if (blob && highScore.blobUploadParameters)
		{
			[OFHighScoreService uploadBlob:blob forHighScore:highScore];
		}
		else
		{
			if (blob)
			{
				OFLog(@"Failed to upload blob for high score");
			}
			[OFHighScoreService 
			 localSetHighScore:highScore.score
			 forLeaderboard:highScore.leaderboardId
			 forUser:highScore.user.resourceId
			 displayText:highScore.displayText
			 customData:highScore.customData
			 blob:blob
			 serverDate:[NSDate date]
			 addToExisting:NO
			 shouldSubmit:nil
			 overrideExisting:YES];
		}
		
		[OFHighScoreService removePendingBlobForLeaderboard:highScore.leaderboardId];
	}
	[nextCall invoke];
}

+ (OFRequestHandle*) setHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService setHighScore:score withDisplayText:nil forLeaderboard:leaderboardId silently:NO onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) setHighScore:(int64_t)score forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{	
	return [OFHighScoreService setHighScore:score withDisplayText:nil forLeaderboard:leaderboardId silently:silently onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService setHighScore:score withDisplayText:displayText forLeaderboard:leaderboardId silently:NO onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService setHighScore:score withDisplayText:displayText withCustomData:nil forLeaderboard:leaderboardId silently:silently onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText withCustomData:(NSString*)customData forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService setHighScore:score withDisplayText:displayText withCustomData:customData withBlob:nil forLeaderboard:leaderboardId silently:silently deferred:NO onSuccess:onSuccess onFailure:onFailure];	
}

+ (OFRequestHandle*) setHighScore:(int64_t)score withDisplayText:(NSString*)displayText withCustomData:(NSString*)customData forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently deferred:(BOOL)deferred onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFHighScoreService setHighScore:score withDisplayText:displayText withCustomData:customData withBlob:nil forLeaderboard:leaderboardId silently:silently deferred:deferred onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*) setHighScore:(int64_t)score 
	  withDisplayText:(NSString*)displayText 
	   withCustomData:(NSString*)customData 
			 withBlob:(NSData*)blob
	   forLeaderboard:(NSString*)leaderboardId 
			 silently:(BOOL)silently
			deferred:(BOOL)deferred
			onSuccess:(const OFDelegate&)onSuccess 
			onFailure:(const OFDelegate&)onFailure
{
	OFRequestHandle* handle = nil;
	
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	OFRetainedPtr<NSString> leaderboardIdString = leaderboardId;
	params->io("leaderboard_id", leaderboardIdString);
	bool hasBlob = (blob && ([blob length] <= kMaxHighScoreBlobSize));
	
	{
		OFISerializer::Scope high_score(params, "high_score");
		params->io("score", score);
		if (displayText)
		{
			params->io("display_text", displayText);
		}
		if (customData)
		{
			params->io("custom_data", customData);
		}
		
		CLLocation* location = [OpenFeint getUserLocation];
		if (location)
		{
			double lat = location.coordinate.latitude;
			double lng = location.coordinate.longitude;
			params->io("lat", lat);
			params->io("lng", lng);
		}
		params->io("has_blob", hasBlob);
	}
	
	NSString* notificationText = nil;
	
    NSString* lastLoggedInUser = [OpenFeint lastLoggedInUserId];
	BOOL shouldSubmit = YES;
	BOOL succeeded = [OFHighScoreService localSetHighScore:score forLeaderboard:leaderboardId forUser:lastLoggedInUser displayText:displayText customData:customData blob:blob serverDate:nil addToExisting:NO shouldSubmit:&shouldSubmit overrideExisting:YES];
	if (shouldSubmit)
	{
		if (!deferred && [OpenFeint isOnline])
		{
			OFDelegate submitSuccessDelegate([self sharedInstance], @selector(_onSetHighScore:nextCall:));
			if (hasBlob)
			{
				
				[OFHighScoreService setPendingBlob:blob forLeaderboard:leaderboardId andScore:score];
			}
			else
			{
				[OFHighScoreService removePendingBlobForLeaderboard:leaderboardId];
				if (blob)
				{
					OFLog(@"High score blob is to big and will not be uploaded. Maximum size is 50k.");
				}
			}
			
			 handle = [[self sharedInstance]
					   postAction:@"client_applications/@me/high_scores.xml"
					   withParameters:params
					   withSuccess:submitSuccessDelegate
					   withFailure:OFDelegate()
					   withRequestType:OFActionRequestSilent
					   withNotice:nil];
			
			notificationText = @"New high score!";
		}
		else
		{
			notificationText = @"New high score! Saving locally.";
		}
		
		if (!silently)
		{
			OFNotificationData* notice = [OFNotificationData dataWithText:notificationText andCategory:kNotificationCategoryHighScore andType:kNotificationTypeSuccess];
			if([OpenFeint isOnline])
			{
				[[OFNotification sharedInstance] setDefaultBackgroundNotice:notice andStatus:OFNotificationStatusSuccess andInputResponse:nil];
			}
			else 
			{
				[[OFNotification sharedInstance] showBackgroundNotice:notice andStatus:OFNotificationStatusSuccess andInputResponse:nil];
			}
		}
	}
	
	if (succeeded)
		onSuccess.invoke();
	else
		onFailure.invoke();
		
	return handle;
}

/*
+ (void) queueHighScore:(int64_t)score withDisplayText:(NSString*)displayText forLeaderboard:(NSString*)leaderboardId silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
    NSString* lastLoggedInUser = [OpenFeint lastLoggedInUserId];
	[OFHighScoreService localSetHighScore:score forLeaderboard:leaderboardId forUser:lastLoggedInUser displayText:displayText serverDate:nil addToExisting:NO];

	if (!silently)
	{
		OFNotificationData* notice = [OFNotificationData dataWithText:@"Saved High Score" andCategory:kNotificationCategoryLeaderboard andType:kNotificationTypeSuccess];
		[[OFNotification sharedInstance] showBackgroundNotice:notice andStatus:OFNotificationStatusSuccess andInputResponse:nil];
	}
	
}

+ (void) sendQueuedHighScores:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	NSString* lastLoggedInUser = [OpenFeint lastLoggedInUserId];
	[OFHighScoreService sendPendingHighScores:lastLoggedInUser silently:silently onSuccess:onSuccess onFailure:onFailure];
}

*/

+ (OFRequestHandle*) batchSetHighScores:(OFHighScoreBatchEntrySeries&)highScoreBatchEntrySeries onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage
{
	return [OFHighScoreService batchSetHighScores:highScoreBatchEntrySeries silently:NO onSuccess:onSuccess onFailure:onFailure optionalMessage:submissionMessage];
}

+ (OFRequestHandle*) batchSetHighScores:(OFHighScoreBatchEntrySeries&)highScoreBatchEntrySeries silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage
{
	return [OFHighScoreService batchSetHighScores:highScoreBatchEntrySeries silently:silently onSuccess:onSuccess onFailure:onFailure optionalMessage:submissionMessage fromSynch:NO];
}

+ (OFRequestHandle*) batchSetHighScores:(OFHighScoreBatchEntrySeries&)highScoreBatchEntrySeries silently:(BOOL)silently onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage fromSynch:(BOOL)fromSynch
{
	OFRequestHandle* requestHandle = nil;
	BOOL succeeded = YES;

	if (!fromSynch)
	{
		NSString* lastLoggedInUser = [OpenFeint lastLoggedInUserId];
		int seriesSize = highScoreBatchEntrySeries.size();
		OFHighScoreBatchEntrySeries::iterator it = highScoreBatchEntrySeries.end() - 1;
		for(int i = seriesSize; i > 0 ; i--)
		{
			OFHighScoreBatchEntrySeries::iterator it1 = it;
			it--;
			OFHighScoreBatchEntry* highScore = (*it1);
			BOOL shouldSubmit = YES;
			succeeded = [OFHighScoreService localSetHighScore:highScore->score forLeaderboard:highScore->leaderboardId forUser:lastLoggedInUser displayText:highScore->displayText customData:highScore->customData serverDate:nil addToExisting:NO shouldSubmit:&shouldSubmit];
			if (!shouldSubmit)
			{
				highScoreBatchEntrySeries.erase(it1);
			}
		}
	}
	
	OFHighScoreBatchEntrySeries::const_iterator it = highScoreBatchEntrySeries.begin();
	OFHighScoreBatchEntrySeries::const_iterator itEnd = highScoreBatchEntrySeries.end();
	for (; it != itEnd; ++it)
	{
		if ((*it)->blob)
		{
			[OFHighScoreService setPendingBlob:(*it)->blob.get() forLeaderboard:(*it)->leaderboardId.get() andScore:(*it)->score];
		}
		else
		{
			[OFHighScoreService removePendingBlobForLeaderboard:(*it)->leaderboardId.get()];
		}
	}
	
	if (highScoreBatchEntrySeries.size() > 0) 
	{
		OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
		params->serialize("high_scores", "entry", highScoreBatchEntrySeries);
		
		CLLocation* location = [OpenFeint getUserLocation];
		if (location)
		{
			double lat = location.coordinate.latitude;
			double lng = location.coordinate.longitude;
			params->io("lat", lat);
			params->io("lng", lng);
		}
		
		OFNotificationData* notice = [OFNotificationData dataWithText:submissionMessage ? submissionMessage : @"Submitted High Scores" 
														  andCategory:kNotificationCategoryHighScore
															  andType:kNotificationTypeSubmitting];
		requestHandle = [[self sharedInstance]
		 postAction:@"client_applications/@me/high_scores.xml"
		 withParameters:params
		 withSuccess:onSuccess
		 withFailure:onFailure
		 withRequestType:(silently ? OFActionRequestSilent : OFActionRequestBackground)
		 withNotice:notice];
	} 
	else if (succeeded)
	{
		onSuccess.invoke();
		if (!silently && submissionMessage)
		{
			OFNotificationData* notice = [OFNotificationData dataWithText:submissionMessage andCategory:kNotificationCategoryHighScore andType:kNotificationTypeSuccess];
			[[OFNotification sharedInstance] showBackgroundNotice:notice andStatus:OFNotificationStatusSuccess andInputResponse:nil];
		}
	}
	else
	{
		onFailure.invoke();
	}
	
	return requestHandle;
}

+ (void) getAllHighScoresForLoggedInUser:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure optionalMessage:(NSString*)submissionMessage
{
	OFNotificationData* notice = [OFNotificationData dataWithText:submissionMessage ? submissionMessage : @"Downloaded High Scores" 
													  andCategory:kNotificationCategoryHighScore
														  andType:kNotificationTypeDownloading];
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	OFRetainedPtr<NSString> me = @"me";
	bool acrossLeaderboards = true;
	params->io("across_leaderboards", acrossLeaderboards);
	params->io("user_id", me);
	
	[[self sharedInstance] 
	 getAction:@"client_applications/@me/high_scores.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:notice];
}

+ (void) getHighScoresFromLocation:(CLLocation*)origin radius:(int)radius pageIndex:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[OFHighScoreService getHighScoresFromLocation:origin radius:radius pageIndex:pageIndex forLeaderboard:leaderboardId userMapMode:nil onSuccess:onSuccess onFailure:onFailure];
}

+ (void) getHighScoresFromLocation:(CLLocation*)origin radius:(int)radius pageIndex:(NSInteger)pageIndex forLeaderboard:(NSString*)leaderboardId userMapMode:(NSString*)userMapMode onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	
	bool geolocation = true;
	params->io("geolocation", geolocation);

	params->io("page", pageIndex);
	
	params->io("leaderboard_id", leaderboardId);
	if (radius != 0)
		params->io("radius", radius);
	
	if (origin)
	{
		CLLocationCoordinate2D coord = origin.coordinate;
		params->io("lat", coord.latitude);
		params->io("lng", coord.longitude);
	}
	
	if (userMapMode)
	{
		params->io("map_me", userMapMode);
	}

	[[self sharedInstance] 
	 getAction:@"client_applications/@me/high_scores.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:nil];	
}

+ (OFRequestHandle*) getDistributedHighScoresAtPage:(NSInteger)pageIndex 
										   pageSize:(NSInteger)pageSize 
										 scoreDelta:(NSInteger)scoreDelta
										 startScore:(NSInteger)startScore
									 forLeaderboard:(NSString*)leaderboardId 
										  onSuccess:(const OFDelegate&)onSuccess 
										  onFailure:(const OFDelegate&)onFailure
{	
	return [[self sharedInstance] 
			getAction:[NSString stringWithFormat:@"leaderboards/%@/high_scores/range/%d/%d/%d/%d.xml", leaderboardId, startScore, scoreDelta, pageIndex, pageSize]
			withParameters:nil
			withSuccess:onSuccess
			withFailure:onFailure
			withRequestType:OFActionRequestSilent
			withNotice:nil];
}


+ (OFRequestHandle*) downloadBlobForHighScore:(OFHighScore*)highScore onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFRequestHandle* request = nil;
	if (![highScore hasBlob])
	{
		OFLog(@"Trying to download the blob for a high score that doesn't have a blob attached to it.");
		onFailure.invoke();
	}
	
	if (highScore.blob)
	{
		onSuccess.invoke(highScore);
	}
	else
	{
		OFDelegate chainedSuccess([OFHighScoreService sharedInstance], @selector(onBlobDownloaded:nextCall:), onSuccess);
		OFDelegate chainedFailure([OFHighScoreService sharedInstance], @selector(onBlobFailedDownloading:nextCall:), onFailure);
		request = [OFCloudStorageService downloadS3Blob:highScore.blobUrl passThroughUserData:highScore onSuccess:chainedSuccess onFailure:chainedFailure];
	}
	
	return request;
}

- (void) onBlobDownloaded:(OFS3Response*)response nextCall:(OFDelegateChained*)nextCall
{
	OFHighScore* highScore = (OFHighScore*)response.userParam;
	if (highScore)
	{
		[highScore _setBlob:response.data];
	}
	[nextCall invokeWith:highScore];
}

- (void) onBlobFailedDownloading:(OFS3Response*)response nextCall:(OFDelegateChained*)nextCall
{
	if (response && response.statusCode == 404)
	{
		[OFHighScoreService reportMissingBlobForHighScore:(OFHighScore*)response.userParam];
	}
	[nextCall invoke];
}

@end

