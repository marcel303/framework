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
#import "OFAbridgedHighScore.h"
#import "OFLeaderboard.h"
#import "OFResourceDataMap.h"
#import "OFHttpService.h"
#import "OFImageView.h"
#import "OFImageCache.h"
#import "OpenFeint+Private.h"

static id sharedDelegate = nil;

@interface OFAbridgedHighScore (Private)
	- (void)_getProfilePictureSuccess:(NSData*)imageData;
	- (void)_getProfilePictureFailure;
@end

@implementation OFAbridgedHighScore

@synthesize score;
@synthesize displayText;
@synthesize userId;
@synthesize userName;
@synthesize userProfilePictureUrl;
@synthesize userGamerScore;

+ (void)setDelegate:(id<OFAbridgedHighScoreDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFAbridgedHighScore class]];
	}
}

- (OFRequestHandle*)getProfilePicture
{
	UIImage* image = nil;
	OFRequestHandle* handle = [OpenFeint getImageFromUrl:userProfilePictureUrl
											 cachedImage:image
											 httpService:mHttpService 
											httpObserver:mHttpServiceObserver 
												  target:self
									   onDownloadSuccess:@selector(_getProfilePictureSuccess:)
									   onDownloadFailure:@selector(_getProfilePictureFailure)];
	
	if(image)
	{
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetProfilePicture:OFAbridgedHighScore:)])
		{
			[sharedDelegate didGetProfilePicture:image OFAbridgedHighScore:self];
		}
	}
	else
	{
		[OFRequestHandlesForModule addHandle:handle forModule:[OFAbridgedHighScore class]];
	}
	
	return handle;
}

- (void)_getProfilePictureSuccess:(NSData*)imageData
{
	UIImage* image = [UIImage imageWithData:imageData];
	if (image)
	{
		[[OFImageCache sharedInstance] store:image withIdentifier:userProfilePictureUrl];
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetProfilePicture:OFAbridgedHighScore:)])
		{
			[sharedDelegate didGetProfilePicture:image OFAbridgedHighScore:self];
		}
	}
	else
	{
		[self _getProfilePictureFailure];
	}
	mHttpServiceObserver.reset(NULL);
}

- (void)_getProfilePictureFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailGetProfilePictureOFAbridgedHighScore:)])
	{
		[sharedDelegate didFailGetProfilePictureOFAbridgedHighScore:self];
	}
	mHttpServiceObserver.reset(NULL);
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

- (void)setScore:(NSString*)value
{
	score = [value longLongValue];
}

- (void)setUserGamerScore:(NSString*)value
{
	userGamerScore = [value integerValue];
}

- (void)setLeaderboardId:(NSString*)value
{
	OFSafeRelease(leaderboardId);
	leaderboardId = [value retain];
}

- (void)setDisplayText:(NSString*)value
{
	OFSafeRelease(displayText);
	if (value && ![value isEqualToString:@""])
	{
		displayText = [value retain];
	}
}

- (void)setUserName:(NSString*)value
{
	OFSafeRelease(userName);
	if (value && ![value isEqualToString:@""])
	{
		userName = [value retain];
	}
}

- (void)setUserId:(NSString*)value
{
	OFSafeRelease(userId);
	if (value && ![value isEqualToString:@""])
	{
		userId = [value retain];
	}
}

- (void)setUserProfilePictureUrl:(NSString*)value
{
	OFSafeRelease(userProfilePictureUrl);
	if (value && ![value isEqualToString:@""])
	{
		userProfilePictureUrl = [value retain];
	}
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
		dataMap->addField(@"score",						@selector(setScore:));
		dataMap->addField(@"leaderboard_id",			@selector(setLeaderboardId:));
		dataMap->addField(@"display_text",				@selector(setDisplayText:));
		dataMap->addField(@"user_id",					@selector(setUserId:));
		dataMap->addField(@"user_name",					@selector(setUserName:));
		dataMap->addField(@"user_profile_picture_url",	@selector(setUserProfilePictureUrl:));
		dataMap->addField(@"user_gamer_score",			@selector(setUserGamerScore:));
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"abridged_score";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"openfeint_abridged_high_score_discovered";
}

- (bool)canReceiveCallbacksNow
{
	return true;
}

- (void) dealloc
{
	[OpenFeint destroyHttpService:mHttpService];
	OFSafeRelease(displayText);
	OFSafeRelease(leaderboardId);
	OFSafeRelease(userId);
	OFSafeRelease(userName);
	OFSafeRelease(userProfilePictureUrl);
	[super dealloc];
}

@end
