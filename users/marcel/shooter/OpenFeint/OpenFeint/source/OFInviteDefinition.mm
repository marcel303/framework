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

#import "OFInviteDefinition.h"
#import "OFInviteService.h"
#import "OFResourceDataMap.h"
#import "OFPaginatedSeries.h"
#import "OFHttpService.h"
#import "OFImageView.h"
#import "OFImageCache.h"
#import "OpenFeint+Private.h"

//Internalize the set functions.
#define setStringFunc(func, var) - (void)func:(NSString*)value \
								{ \
									OFSafeRelease(var); \
									var = [value retain]; \
								}

static id sharedDelegate = nil;

@interface OFInviteDefinition (Private)
+ (void)_getPrimaryInviteDefinitionSuccess:(OFPaginatedSeries*)resources;
+ (void)_getPrimaryInviteDefinitionFailure;
+ (void)_getInviteDefinitionSuccess:(OFPaginatedSeries*)resources;
+ (void)_getInviteDefinitionFailure;
- (void)_getInviteIconSuccess:(NSData*)imageData;
- (void)_getInviteIconFailure;
@end

@implementation OFInviteDefinition

@synthesize clientApplicationName;
@synthesize clientApplicationID;
@synthesize inviteIdentifier;
@synthesize senderParameter;
@synthesize receiverParameter;
@synthesize inviteIconURL;
@synthesize developerMessage;
@synthesize receiverNotification;
@synthesize senderNotification;
@synthesize senderIncentiveText;
@synthesize suggestedSenderMessage;

+ (void)setDelegate:(id<OFInviteDefinitionDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFInviteDefinition cancelPreviousPerformRequestsWithTarget:[OFInviteDefinition class]];
	}
}

+ (OFRequestHandle*)getPrimaryInviteDefinition
{
	OFRequestHandle* handle = nil;
	handle = [OFInviteService getDefaultInviteDefinitionForApplication:OFDelegate(self, @selector(_getPrimaryInviteDefinitionSuccess:))
															 onFailure:OFDelegate(self, @selector(_getPrimaryInviteDefinitionFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFInviteDefinition class]];
	return handle;
}

+ (OFRequestHandle*)getInviteDefinition:(NSString*)inviteId
{
	OFRequestHandle* handle = nil;
	handle = [OFInviteService getInviteDefinition:inviteId 
										onSuccess:OFDelegate(self, @selector(_getInviteDefinitionSuccess:))
										onFailure:OFDelegate(self, @selector(_getInviteDefinitionFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFInviteDefinition class]];
	return handle;
	
	
}

- (OFRequestHandle*)getInviteIcon
{
	UIImage* image = nil;
	OFRequestHandle* handle = [OpenFeint getImageFromUrl:inviteIconURL
											 cachedImage:image
											 httpService:mHttpService
											httpObserver:mHttpServiceObserver
												  target:self
									   onDownloadSuccess:@selector(_getInviteIconSuccess:)
									   onDownloadFailure:@selector(_getInviteIconFailure)];
	
	if(image)
	{
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetInviteIcon:OFInviteDefinition:)])
		{
			[sharedDelegate didGetInviteIcon:image OFInviteDefinition:self];
		}
	}
	else
	{
		[OFRequestHandlesForModule addHandle:handle forModule:[OFInviteDefinition class]];
	}

	return handle;
}

+ (void)_getPrimaryInviteDefinitionSuccess:(OFPaginatedSeries*)resources
{
	if ([resources count] > 0)
	{
		OFInviteDefinition* inviteDef = [resources.objects objectAtIndex:0];
		if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetPrimaryInviteDefinition:)])
		{
			[sharedDelegate didGetPrimaryInviteDefinition:inviteDef];
		}
	}
	else
	{
		[self _getPrimaryInviteDefinitionFailure];
	}
}

+ (void)_getPrimaryInviteDefinitionFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailGetPrimaryInviteDefinition)])
	{
		[sharedDelegate didFailGetPrimaryInviteDefinition];
	}
}

+ (void)_getInviteDefinitionSuccess:(OFPaginatedSeries*)resources
{
	if ([resources count] > 0)
	{
		OFInviteDefinition* inviteDef = [resources.objects objectAtIndex:0];
		if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetInviteDefinition:)])
		{
			[sharedDelegate didGetInviteDefinition:inviteDef];
		}
	}
	else
	{
		[self _getInviteDefinitionFailure];
	}
}

+ (void)_getInviteDefinitionFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailGetInviteDefinition)])
	{
		[sharedDelegate didFailGetInviteDefinition];
	}
}

- (void)_getInviteIconSuccess:(NSData*)imageData
{
	UIImage* image = [UIImage imageWithData:imageData];
	if (image)
	{
		[[OFImageCache sharedInstance] store:image withIdentifier:inviteIconURL];
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetInviteIcon:OFInviteDefinition:)])
		{
			[sharedDelegate didGetInviteIcon:image OFInviteDefinition:self];
		}
	}
	else
	{
		[self _getInviteIconFailure];
	}
	mHttpServiceObserver.reset(NULL);
}

- (void)_getInviteIconFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailGetInviteIconOFInviteDefinition:)])
	{
		[sharedDelegate didFailGetInviteIconOFInviteDefinition:self];
	}
	mHttpServiceObserver.reset(NULL);
}

+ (OFService*)getService;
{
	return [OFInviteService sharedInstance];
}

setStringFunc(setClientApplicationName, clientApplicationName)
setStringFunc(setClientApplicationID, clientApplicationID)
setStringFunc(setInviteIdentifier, inviteIdentifier)
setStringFunc(setSenderParameter, senderParameter)
setStringFunc(setReceiverParameter, receiverParameter)
setStringFunc(setInviteIconURL, inviteIconURL)
setStringFunc(setDeveloperMessage, developerMessage)
setStringFunc(setReceiverNotification, receiverNotification)
setStringFunc(setSenderNotification, senderNotification)
setStringFunc(setSenderIncentiveText, senderIncentiveText);
setStringFunc(setSuggestedSenderMessage, suggestedSenderMessage);

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"client_application_name", @selector(setClientApplicationName:), @selector(clientApplicationName));
		dataMap->addField(@"client_application_id", @selector(setClientApplicationID:), @selector(clientApplicationID));
		dataMap->addField(@"invite_identifier", @selector(setInviteIdentifier:), @selector(inviteIdentifier));
		dataMap->addField(@"sender_parameter", @selector(setSenderParameter:), @selector(senderParameter));
		dataMap->addField(@"receiver_parameter", @selector(setReceiverParameter:), @selector(receiverParameter));
		dataMap->addField(@"invite_icon_url", @selector(setInviteIconURL:), @selector(inviteIconURL));
		dataMap->addField(@"developer_message", @selector(setDeveloperMessage:), @selector(developerMessage));
		dataMap->addField(@"receiver_notification", @selector(setReceiverNotification:), @selector(receiverNotification));
		dataMap->addField(@"sender_notification", @selector(setSenderNotification:), @selector(senderNotification));
		dataMap->addField(@"sender_incentive_text", @selector(setSenderIncentiveText:), @selector(senderIncentiveText));
		dataMap->addField(@"suggested_sender_message", @selector(setSuggestedSenderMessage:), @selector(suggestedSenderMessage));
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"invite_definition";
}

- (bool)canReceiveCallbacksNow
{
	return true;
}

+ (bool)canReceiveCallbacksNow
{
	return true;
}

- (void) dealloc
{
	OFSafeRelease(clientApplicationName);
	OFSafeRelease(clientApplicationID);
	OFSafeRelease(inviteIdentifier);
	OFSafeRelease(senderParameter);
	OFSafeRelease(receiverParameter);
	OFSafeRelease(inviteIconURL);
	OFSafeRelease(developerMessage);
	OFSafeRelease(receiverNotification);
	OFSafeRelease(senderNotification);
	OFSafeRelease(senderIncentiveText);
	OFSafeRelease(suggestedSenderMessage);
	[super dealloc];
}

@end
