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

#import "OFAchievement.h"
#import "OFPaginatedSeries.h"
#import "OFTableSectionDescription.h"
#import "OFRequestHandle.h"
#import "OFAchievementService.h"
#import "OFAchievementService+Private.h"
#import "OFDependencies.h"
#import "OFResourceDataMap.h"
#import "OFSqlQuery.h" 
#import "OFHttpService.h"
#import "OFImageView.h"
#import "OFImageCache.h"
#import "OpenFeint+Private.h"

static id sharedDelegate = nil; 

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFAchievement (Private)
- (void)_submitDeferredSuccess;
- (void)_submitDeferredFailure;
- (void)_unlockSuccess;
- (void)_unlockFailure;
- (void)_getIconSuccess:(NSData*)imageData;
- (void)_getIconFailure;
@end

@implementation OFAchievement

@synthesize title, description, gamerscore, iconUrl, isSecret, isUnlocked, unlockDate;
@synthesize isUnlockedByComparedToUser, comparedToUserId, endVersion, startVersion, position;

+ (void)setDelegate:(id<OFAchievementDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFAchievement class]];
	}
}

+ (NSArray*)achievements
{
	return [OFAchievementService getAchievementsLocal];
}

+ (OFAchievement*)achievement:(NSString*)achievementId
{
	return [OFAchievementService getAchievementLocalWithUnlockInfo:achievementId];
}

+ (OFRequestHandle*)submitDeferredAchievements
{
	OFRequestHandle* handle = nil;
	handle = [OFAchievementService 
			  submitQueuedUnlockedAchievements:OFDelegate(self, @selector(_submitDeferredSuccess)) 
			  onFailure:OFDelegate(self, @selector(_submitDeferredFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFAchievement class]];
	return handle;
}

+ (void)setCustomUrlForSocialNotificaion:(NSString*)url
{
	[OFAchievementService sharedInstance].mCustomUrlWithSocialNotification = url;
}

- (OFRequestHandle*)unlock
{
	OFRequestHandle* handle = nil;
	handle = [OFAchievementService unlockAchievement:self.resourceId
										   onSuccess:OFDelegate(self, @selector(_unlockSuccess))
										   onFailure:OFDelegate(self, @selector(_unlockFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFAchievement class]];
	return handle;
}

- (OFRequestHandle*)getIcon
{
	UIImage* image = nil;
	OFRequestHandle* handle = [OpenFeint getImageFromUrl:iconUrl
											 cachedImage:image
											 httpService:mHttpService
											httpObserver:mHttpServiceObserver
												  target:self
									   onDownloadSuccess:@selector(_getIconSuccess:)
									   onDownloadFailure:@selector(_getIconFailure)];
	
	if(image)
	{
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetIcon:OFAchievement:)])
		{
			[sharedDelegate didGetIcon:image OFAchievement:self];
		}
	}
	else
	{
		[OFRequestHandlesForModule addHandle:handle forModule:[OFAchievement class]];
	}
	
	return handle;
}

- (void)unlockAndDefer
{
	[OFAchievementService queueUnlockedAchievement:self.resourceId];
}

+ (void)_submitDeferredSuccess
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didSubmitDeferredAchievements)])
	{
		[sharedDelegate didSubmitDeferredAchievements];
	}
}

+ (void)_submitDeferredFailure
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailSubmittingDeferredAchievements)])
	{
		[sharedDelegate didFailSubmittingDeferredAchievements];
	}
}

- (void)_getIconSuccess:(NSData*)imageData
{
	UIImage* image = [UIImage imageWithData:imageData];
	if (image)
	{
		[[OFImageCache sharedInstance] store:image withIdentifier:iconUrl];
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetIcon:OFAchievement:)])
		{
			[sharedDelegate didGetIcon:image OFAchievement:self];
		}
	}
	else
	{
		[self _getIconFailure];
	}
	mHttpServiceObserver.reset(NULL);
}

- (void)_getIconFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailGetIconOFAchievement:)])
	{
		[sharedDelegate didFailGetIconOFAchievement:self];
	}
	mHttpServiceObserver.reset(NULL);
}

- (void)_unlockSuccess
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didUnlockOFAchievement:)])
	{
		[sharedDelegate didUnlockOFAchievement:self];
	}
}

- (void)_unlockFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailUnlockOFAchievement:)])
	{
		[sharedDelegate didFailUnlockOFAchievement:self];
	}
}

- (id)initWithLocalSQL:(OFSqlQuery*)queryRow
{
	self = [super init];
	if (self != nil)
	{	
		title = [[NSString stringWithUTF8String:queryRow->getText("title")] retain];
		description = [[NSString stringWithUTF8String:queryRow->getText("description")] retain];
		gamerscore = queryRow->getInt("gamerscore");
		iconUrl = [[NSString stringWithFormat:@"%s", queryRow->getText("icon_file_name")] retain];
		isSecret = queryRow->getBool("is_secret");
		position = queryRow->getInt("position");
		startVersion = [[NSString stringWithUTF8String:queryRow->getText("start_version")] retain];
		endVersion = [[NSString stringWithUTF8String:queryRow->getText("end_version")] retain];
		
		OFSafeRelease(unlockDate);
		int64_t secondsSince1970 = queryRow->getInt64("unlocked_date");
		if (secondsSince1970 > 0)
		{
			unlockDate = [[NSDate dateWithTimeIntervalSince1970:secondsSince1970] retain];
		}
		isUnlocked = unlockDate != nil;
		
		OFSafeRelease(resourceId);
		resourceId = [[NSString stringWithFormat:@"%s", queryRow->getText("id")] retain];
	}
	return self;
}

- (void)setTitle:(NSString*)value
{
	OFSafeRelease(title);
	title = [value retain];
}

- (void)setDescription:(NSString*)value
{
	OFSafeRelease(description);
	description = [value retain];
}

- (void)setGamerscoreFromString:(NSString*)value
{
	gamerscore = [value intValue];
}

- (void)setIconUrl:(NSString*)value
{
	OFSafeRelease(iconUrl);
	iconUrl = [value retain];
}

- (void)setIsSecretFromString:(NSString*)value
{
	isSecret = [value boolValue];
}

- (void)setIsUnlockedFromString:(NSString*)value
{
	isUnlocked = [value boolValue];
}

- (void)setIsUnlockedByComparedToUserFromString:(NSString*)value
{
	isUnlockedByComparedToUser = [value boolValue];
}

- (void)setComparedToUserId:(NSString*)value
{
	OFSafeRelease(comparedToUserId);
	comparedToUserId = [value retain];
}

- (void)setUnlockDateFromString:(NSString*)value
{
	OFSafeRelease(unlockDate);
	
	if (value != nil)
	{
		NSDateFormatter* dateFormatter = [[[NSDateFormatter alloc] init]  autorelease];
		
		[dateFormatter setDateFormat:@"yyy-MM-dd HH:mm:ss zzz"];
		NSMutableString* tmpDate = [[[NSMutableString alloc] initWithString:value] autorelease]; 
		if( [value length] == 19 )
		{
			[tmpDate appendString: @" GMT"];
		}
		unlockDate = [[dateFormatter dateFromString:tmpDate] retain];
	}
}

- (void)setPositionFromString:(NSString*)value
{
	position = [value intValue];
}

- (void)setEndVersion:(NSString*)value
{
	OFSafeRelease(endVersion);
	endVersion = [value retain];
}

- (void)setStartVersion:(NSString*)value
{
	OFSafeRelease(startVersion);
	startVersion = [value retain];
}

+ (OFService*)getService;
{
	return [OFAchievementService sharedInstance];
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"title", @selector(setTitle:));
		dataMap->addField(@"description", @selector(setDescription:));
		dataMap->addField(@"gamerscore", @selector(setGamerscoreFromString:));
		dataMap->addField(@"icon_url", @selector(setIconUrl:));
		dataMap->addField(@"is_secret", @selector(setIsSecretFromString:));
		dataMap->addField(@"is_unlocked", @selector(setIsUnlockedFromString:));
		dataMap->addField(@"is_unlocked_by_compared_to_user", @selector(setIsUnlockedByComparedToUserFromString:));
		dataMap->addField(@"compared_to_user_id", @selector(setComparedToUserId:));
		dataMap->addField(@"unlock_date", @selector(setUnlockDateFromString:));
		dataMap->addField(@"position", @selector(setPositionFromString:));
		dataMap->addField(@"end_version", @selector(setEndVersion:));
		dataMap->addField(@"start_version", @selector(setStartVersion:));
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"achievement";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"openfeint_achievement_discovered";
}

- (bool)canReceiveCallbacksNow
{
	return YES;
}

+ (bool)canReceiveCallbacksNow
{
	return YES;
}

- (void) dealloc
{
	OFSafeRelease(title);
	OFSafeRelease(description);
	OFSafeRelease(iconUrl);
	OFSafeRelease(unlockDate);
	OFSafeRelease(comparedToUserId);
	OFSafeRelease(endVersion);
	OFSafeRelease(startVersion);

	[super dealloc];
}

@end
