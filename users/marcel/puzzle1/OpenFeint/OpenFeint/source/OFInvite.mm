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

#import "OFInvite.h"
#import "OFInviteDefinition.h"
#import "OFUser.h"
#import "OFInviteService.h"
#import "OFResourceDataMap.h"
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

@interface OFInvite (Private)
- (void)_sendInviteSuccess;
- (void)_sendInviteFailure;
- (void)_getInviteIconSuccess:(NSData*)imageData;
- (void)_getInviteIconFailure;
@end

@implementation OFInvite

@synthesize senderUser;
@synthesize receiverUser;
@synthesize clientApplicationName;
@synthesize clientApplicationID;
@synthesize inviteIdentifier;
@synthesize senderParameter;
@synthesize receiverParameter;
@synthesize inviteIconURL;
@synthesize developerMessage;
@synthesize receiverNotification;
@synthesize senderNotification;
@synthesize userMessage;
@synthesize state;

+ (void)setDelegate:(id<OFInviteSendDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFInvite class]];
	}
}

- (OFInvite*)initWithInviteDefinition:(OFInviteDefinition*)inviteDefinitionIn
{
	self = [super init];
	if(self)
	{
		inviteDefinition = [inviteDefinitionIn retain];
		userMessage = [inviteDefinitionIn.suggestedSenderMessage retain];
	}
	return self;
}

- (OFRequestHandle*)sendInviteToUsers:(NSArray*)users
{
	if(inviteDefinition == nil)
	{
		OFLog(@"You must initialize this invitiation via initWithInviteDefinition to send it");
		return nil;
	}
	
	OFRequestHandle* handle = nil;
	handle = [OFInviteService sendInvite:inviteDefinition
							 withMessage:userMessage
								 toUsers:users
							   onSuccess:OFDelegate(self, @selector(_sendInviteSuccess))
							   onFailure:OFDelegate(self, @selector(_sendInviteFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFInvite class]];
	return handle;
}

- (void)displayAndSendInviteScreen
{
	if(inviteDefinition == nil)
	{
		OFLog(@"You must initialize this invitiation via initWithInviteDefinition to send it");
		return;
	}
	
	[OFInviteService displaySendInviteModal:inviteDefinition.inviteIdentifier];
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
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetInviteIcon:OFInvite:)])
		{
			[sharedDelegate didGetInviteIcon:image OFInvite:self];
		}
	}
	else
	{
		[OFRequestHandlesForModule addHandle:handle forModule:[OFInvite class]];
	}

	return handle;
}

- (void)_sendInviteSuccess
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didSendInvite:)])
	{
		[sharedDelegate didSendInvite:self];
	}
}

- (void)_sendInviteFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailSendInvite:)])
	{
		[sharedDelegate didFailSendInvite:self];
	}
}

- (void)_getInviteIconSuccess:(NSData*)imageData
{
	UIImage* image = [UIImage imageWithData:imageData];
	if (image)
	{
		[[OFImageCache sharedInstance] store:image withIdentifier:inviteIconURL];
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetInviteIcon:OFInvite:)])
		{
			[sharedDelegate didGetInviteIcon:image OFInvite:self];
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
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailGetInviteIconOFInvite:)])
	{
		[sharedDelegate didFailGetInviteIconOFInvite:self];
	}
	mHttpServiceObserver.reset(NULL);
}

+ (OFService*)getService;
{
	return [OFInviteService sharedInstance];
}

- (void)setSenderUser:(OFUser*)user
{
	OFSafeRelease(senderUser);
	senderUser = [user retain];
}

- (void)setReceiverUser:(OFUser*)user
{
	OFSafeRelease(receiverUser);
	receiverUser = [user retain];
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
setStringFunc(setUserMessage, userMessage)
setStringFunc(setState, state)


+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addNestedResourceField(@"sender", @selector(setSenderUser:), @selector(senderUser), [OFUser class]);
		dataMap->addNestedResourceField(@"receiver", @selector(setReceiverUser:), @selector(receiverUser), [OFUser class]);
		dataMap->addField(@"client_application_name", @selector(setClientApplicationName:), @selector(clientApplicationName));
		dataMap->addField(@"client_application_id", @selector(setClientApplicationID:), @selector(clientApplicationID));
		dataMap->addField(@"invite_identifier", @selector(setInviteIdentifier:), @selector(inviteIdentifier));
		dataMap->addField(@"sender_parameter", @selector(setSenderParameter:), @selector(senderParameter));
		dataMap->addField(@"receiver_parameter", @selector(setReceiverParameter:), @selector(receiverParameter));
		dataMap->addField(@"invite_icon_url", @selector(setInviteIconURL:), @selector(inviteIconURL));
		dataMap->addField(@"developer_message", @selector(setDeveloperMessage:), @selector(developerMessage));
		dataMap->addField(@"receiver_notification", @selector(setReceiverNotification:), @selector(receiverNotification));
		dataMap->addField(@"sender_notification", @selector(setSenderNotification:), @selector(senderNotification));
		dataMap->addField(@"user_message", @selector(setUserMessage:), @selector(userMessage));
		dataMap->addField(@"state", @selector(setState:), @selector(state));
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"invite";
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
	OFSafeRelease(senderUser);
	OFSafeRelease(receiverUser);
	OFSafeRelease(clientApplicationName);
	OFSafeRelease(clientApplicationID);
	OFSafeRelease(inviteIdentifier);
	OFSafeRelease(senderParameter);
	OFSafeRelease(receiverParameter);
	OFSafeRelease(inviteIconURL);
	OFSafeRelease(developerMessage);
	OFSafeRelease(receiverNotification);
	OFSafeRelease(senderNotification);
	OFSafeRelease(userMessage);
	OFSafeRelease(state);
	OFSafeRelease(inviteDefinition);
	[super dealloc];
}


@end
