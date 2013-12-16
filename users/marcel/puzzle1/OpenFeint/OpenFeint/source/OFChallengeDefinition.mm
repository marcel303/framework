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
#import "OFChallengeDefinition.h"
#import "OFChallengeDefinitionService.h"
#import "OFResourceDataMap.h"
#import "OFPaginatedSeries.h"
#import "OFHttpService.h"
#import "OFImageView.h"
#import "OFImageCache.h"
#import "OpenFeint+Private.h"

static id sharedDelegate = nil;

@interface OFChallengeDefinition (Private)
+ (void)_downloadAllChallengeDefinitionsSuccess:(OFPaginatedSeries*)loadedChallenges;
+ (void)_downloadAllChallengeDefinitionsFail;
+ (void)_downloadChallengeDefinitionWithIdSuccess:(OFPaginatedSeries*)resources;
+ (void)_downloadChallengeDefinitionWithIdFail;
- (void)_getIconSuccess:(NSData*)imageData;
- (void)_getIconFailure;
@end

@implementation OFChallengeDefinition

@synthesize title, iconUrl, multiAttempt, clientApplicationId;

+ (void)setDelegate:(id<OFChallengeDefinitionDelegate>)delegate;
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFChallengeDefinition class]];
	}
}

+ (OFRequestHandle*)downloadAllChallengeDefinitions
{
	OFRequestHandle* handle = nil;
	handle = [OFChallengeDefinitionService getIndexOnSuccess:OFDelegate(self, @selector(_downloadAllChallengeDefinitionsSuccess:))
												   onFailure:OFDelegate(self, @selector(_downloadAllChallengeDefinitionsFail))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFChallengeDefinition class]];
	return handle;
	
}

+ (OFRequestHandle*)downloadChallengeDefinitionWithId:(NSString*)challengeDefinitionId
{
	OFRequestHandle* handle = nil;
	handle = [OFChallengeDefinitionService getChallengeDefinitionWithId:challengeDefinitionId
															  onSuccess:OFDelegate(self, @selector(_downloadChallengeDefinitionWithIdSuccess:))
															  onFailure:OFDelegate(self, @selector(_downloadChallengeDefinitionWithIdFail))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFChallengeDefinition class]];
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
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetIcon:OFChallengeDefintion:)])
		{
			[sharedDelegate didGetIcon:image OFChallengeDefintion:self];
		}
	}
	else
	{
		[OFRequestHandlesForModule addHandle:handle forModule:[OFChallengeDefinition class]];
	}

	return handle;
}

+ (void)_downloadAllChallengeDefinitionsSuccess:(OFPaginatedSeries*)loadedChallenges
{
	NSArray* challengeDefinitions = [[[NSArray alloc] initWithArray:loadedChallenges.objects] autorelease];

	if ([challengeDefinitions count] > 0)
	{
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadAllChallengeDefinitions:)])
		{
			[sharedDelegate didDownloadAllChallengeDefinitions:challengeDefinitions];
		}
	}
	else
	{
		[self _downloadAllChallengeDefinitionsFail];
	}

}

+ (void)_downloadAllChallengeDefinitionsFail
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadChallengeDefinitions)])
	{
		[sharedDelegate didFailDownloadChallengeDefinitions];
	}
}

+ (void)_downloadChallengeDefinitionWithIdSuccess:(OFPaginatedSeries*)resources
{
	if ([resources count] > 0)
	{
		OFChallengeDefinition* challengeDefinition = [resources.objects objectAtIndex:0];
		if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadChallengeDefinition:)])
		{
			[sharedDelegate didDownloadChallengeDefinition:challengeDefinition];
		}
	}
	else
	{
		[self _downloadChallengeDefinitionWithIdFail];
	}
}

+ (void)_downloadChallengeDefinitionWithIdFail
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadChallengeDefinition)])
	{
		[sharedDelegate didFailDownloadChallengeDefinition];
	}
}

- (void)_getIconSuccess:(NSData*)imageData
{
	UIImage* image = [UIImage imageWithData:imageData];
	if (image)
	{
		[[OFImageCache sharedInstance] store:image withIdentifier:iconUrl];
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetIcon:OFChallengeDefintion:)])
		{
			[sharedDelegate didGetIcon:image OFChallengeDefintion:self];
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
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(getFailGetIconOFChallengeDefinition:)])
	{
		[sharedDelegate getFailGetIconOFChallengeDefinition:self];
	}
	mHttpServiceObserver.reset(NULL);
}

- (void)setTitle:(NSString*)value
{
	OFSafeRelease(title);
	title = [value retain];
}

- (void)setClientApplicationId:(NSString*)value
{
	if (clientApplicationId != value)
	{
		OFSafeRelease(clientApplicationId);
		clientApplicationId = [value retain];
	}
}

- (void)setIconUrl:(NSString*)value
{
	OFSafeRelease(iconUrl);
	iconUrl = [value retain];
}

- (void)setMultiAttempt:(NSString*)value
{
	multiAttempt = [value boolValue];
}

- (NSString*)getMultiAttemptAsString
{
	return [NSString stringWithFormat:@"%u", (uint)multiAttempt];
}

+ (OFService*)getService;
{
	return [OFChallengeDefinitionService sharedInstance];
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"name", @selector(setTitle:), @selector(title));
		dataMap->addField(@"client_application_id", @selector(setClientApplicationId:), @selector(clientApplicationId));
		dataMap->addField(@"image_url", @selector(setIconUrl:), @selector(iconUrl));
		dataMap->addField(@"multi_attempt", @selector(setMultiAttempt:), @selector(getMultiAttemptAsString));
		
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"challenge_definition";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"openfeint_challenge_definition_discovered";
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
	OFSafeRelease(clientApplicationId);
	OFSafeRelease(iconUrl);
	[super dealloc];
}

@end
