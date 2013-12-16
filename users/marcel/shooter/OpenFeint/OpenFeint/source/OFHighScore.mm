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
#import "OFHighScore.h"
#import "OFHighScoreService.h"
#import "OFLeaderboard.h"
#import "OFResourceDataMap.h"
#import "OFUser.h"
#import "OFS3UploadParameters.h"

#import "OFUser.h"

static id sharedDelegate = nil;

@interface OFHighScore (Private)
- (void)_submitToSuccess;
- (void)_submitToFailure;
- (void)_downloadBlobSuccess:(OFHighScore*)score;
- (void)_downloadBlobFailure;
@end

@implementation OFHighScore

@synthesize latitude;
@synthesize longitude;
@synthesize distance;
@synthesize blob;
@synthesize blobUrl;
@synthesize user;
@synthesize rank;
@synthesize leaderboardId;

//Dev can set during submission of a high score.
@synthesize score;
@synthesize displayText;
@synthesize customData;

+ (void)setDelegate:(id<OFHighScoreDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFHighScore class]];
	}
}

+ (NSArray*)allHighScoresForCurrentUser
{
	NSArray* leaderboards = [OFLeaderboard leaderboards];
	NSMutableArray* highScores = [NSMutableArray arrayWithCapacity:[leaderboards count]];
	
	for (OFLeaderboard* leaderboard in leaderboards)
	{
		if (leaderboard.currentUserScore)
			[highScores addObject:leaderboard.currentUserScore];
	}
	
	return highScores;
}

- (OFHighScore*)initForSubmissionWithScore:(int64_t)submitScore
{
	self = [super init];
	if(self)
	{
		self.score = submitScore;
	}
	return self;
}

- (OFRequestHandle*)submitTo:(OFLeaderboard*)leaderboard
{
	OFRequestHandle* handle = nil;
	handle = [OFHighScoreService setHighScore:self.score 
							  withDisplayText:self.displayText 
							   withCustomData:self.customData 
									 withBlob:self.blob
							   forLeaderboard:leaderboard.leaderboardId 
									 silently:NO 
									 deferred:NO
									onSuccess:OFDelegate(self, @selector(_submitToSuccess)) 
									onFailure:OFDelegate(self, @selector(_submitToFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFHighScore class]];
	return handle;
}

- (OFRequestHandle*)downloadBlob
{
	OFRequestHandle* handle = nil;
	if(![self hasBlob])
	{
		[self _downloadBlobFailure];
	}
	else if(blob)
	{
		[self _downloadBlobSuccess:self];
	}
	else
	{
		handle = [OFHighScoreService downloadBlobForHighScore:self
													onSuccess:OFDelegate(self, @selector(_downloadBlobSuccess:))
													onFailure:OFDelegate(self, @selector(_downlaodBlobFailure))];
		[OFRequestHandlesForModule addHandle:handle forModule:[OFHighScore class]];
	}
	return handle;
}

- (void)_submitToSuccess
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didSubmit:)])
	{
		[sharedDelegate didSubmit:self];
	}
}

- (void)_submitToFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailSubmit:)])
	{
		[sharedDelegate didFailSubmit:self];
	}
}

- (void)_downloadBlobSuccess:(OFHighScore*)score
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadBlob:)])
	{
		[sharedDelegate didDownloadBlob:self];
	}
}

- (void)_downloadBlobFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadBlob:)])
	{
		[sharedDelegate didFailDownloadBlob:self];
	}
}

- (NSString*)displayText
{
	if (displayText)
	{
		return displayText;
	}
	else
	{
		return [NSString stringWithFormat:@"%qi", score];
	}
}

- (id)initWithLocalSQL:(OFSqlQuery*)queryRow forUser:(OFUser*)hsUser rank:(NSUInteger)scoreRank
{
	self = [super init];
	if (self != nil)
	{	
		OFSafeRelease(user);
		OFSafeRelease(blob);
		OFSafeRelease(customData);
		OFSafeRelease(displayText);
		OFSafeRelease(toHighRankText);
		OFSafeRelease(blobUploadParameters);
		user = [hsUser retain];
		leaderboardId = [[NSString stringWithFormat:@"%d", queryRow->getInt("leaderboard_id")] retain];
		score = queryRow->getInt64("score");
		const char* cDisplayText = queryRow->getText("display_text");
		if( cDisplayText != nil && strlen(cDisplayText) > 0)
		{
			displayText = [[NSString stringWithUTF8String:cDisplayText] retain];
		}
		
		const char* bytes = NULL;
		unsigned int blobSize = 0;
		queryRow->getBlob("blob", bytes, blobSize);
		if (blobSize > 0)
		{
			blob = [[NSData dataWithBytes:bytes length:blobSize] retain];
		}
		
		const char* cCustomData = queryRow->getText("custom_data");
		if( cCustomData != nil )
		{
			customData = [[NSString stringWithUTF8String:cCustomData] retain];
		}
		
		rank = scoreRank;
		
		OFSafeRelease(resourceId);
		resourceId = @"1";	// arbitrary non-zero id
	}
	return self;
}

- (BOOL)hasBlob
{
	return blob || (blobUrl && [blobUrl length] > 0);
}

- (void)setUser:(OFUser*)value
{
	OFSafeRelease(user);
	user = [value retain];
}

- (void)_setScore:(NSString*)value
{
	score = [value longLongValue];
}

- (void)setRank:(NSString*)value
{
	rank = [value integerValue];
}

- (void)setLeaderboardId:(NSString*)value
{
	OFSafeRelease(leaderboardId);
	leaderboardId = [value retain];
}

- (void)_setDisplayText:(NSString*)value
{
	OFSafeRelease(displayText);
	if (value && ![value isEqualToString:@""])
	{
		displayText = [value retain];
	}
}

- (NSString*)toHighRankText
{
	return toHighRankText ? toHighRankText : @"99.999+";
}

- (void)setToHighRankText:(NSString*)value
{
	OFSafeRelease(toHighRankText);
	if (value && ![value isEqualToString:@""])
	{
		toHighRankText = [value retain];
	}
}

- (void)_setCustomData:(NSString*)value
{
	OFSafeRelease(customData);
	if (value && ![value isEqualToString:@""])
	{
		customData = [value retain];
	}
}

- (NSString*)customData
{
	return customData;
}

- (void)setBlobUrl:(NSString*)value
{
	if (value != blobUrl)
	{
		OFSafeRelease(blobUrl);
		if ([value length] > 0)
		{
			blobUrl = [value retain];
		}
	}
}

- (NSString*) blobUrl
{
	return blobUrl;
}

- (void)setBlobUploadParameters:(OFS3UploadParameters*)value
{
	if (value != blobUploadParameters)
	{
		OFSafeRelease(blobUploadParameters);
		blobUploadParameters = [value retain];
	}
}

- (OFS3UploadParameters*)blobUploadParameters
{
	return blobUploadParameters;
}

- (void)_setBlob:(NSData*)_blob
{
	if (blob != _blob)
	{
		OFSafeRelease(blob);
		blob = [_blob retain];
	}
}

- (void) setLatitude:(NSString*)value
{
	latitude = [value doubleValue];
}

- (void) setLongitude:(NSString*)value
{
	longitude = [value doubleValue];
}

- (void) setDistance:(NSString*)value
{
	distance = [value doubleValue];
}

+ (OFService*)getService;
{
	return [OFHighScoreService sharedInstance];
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"score",				@selector(_setScore:));
		dataMap->addField(@"rank",				@selector(setRank:));
		dataMap->addField(@"leaderboard_id",	@selector(setLeaderboardId:));
		dataMap->addField(@"display_text",		@selector(_setDisplayText:));
		dataMap->addField(@"to_high_rank_text",	@selector(setToHighRankText:));
		dataMap->addField(@"custom_data",		@selector(_setCustomData:));
		dataMap->addField(@"blob_url",			@selector(setBlobUrl:));
		dataMap->addField(@"lat",				@selector(setLatitude:));
		dataMap->addField(@"lng",				@selector(setLongitude:));
		dataMap->addField(@"distance",			@selector(setDistance:));
		dataMap->addNestedResourceField(@"user", @selector(setUser:), nil, [OFUser class]);
		dataMap->addNestedResourceField(@"blob_upload_parameters", @selector(setBlobUploadParameters:), nil, [OFS3UploadParameters class]);
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"high_score";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"openfeint_high_score_discovered";
}

- (bool)canReceiveCallbacksNow
{
	return true;
}

- (void) dealloc
{
	OFSafeRelease(user);
	OFSafeRelease(displayText);
	OFSafeRelease(toHighRankText);
	OFSafeRelease(customData);
	OFSafeRelease(blob);
	OFSafeRelease(blobUrl);
	OFSafeRelease(blobUploadParameters);
	[super dealloc];
}

@end
