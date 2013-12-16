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
#import "OFChallenge.h"
#import "OFChallengeService.h"
#import "OFResourceDataMap.h"
#import "OFChallengeDefinition.h"
#import "OFUser.h"

static id sharedDelegate = nil;

@interface OFChallenge (Private)
- (void)setChallengeDefinition:(OFChallengeDefinition*)value;
- (void)setChallengeDescription:(NSString*)value;
- (void)setUserMessage:(NSString*)value;
- (void)setHiddenText:(NSString*)value;
- (void)_sendChallengeSuccess;
- (void)_sendChallengeFailure;
- (void)_downloadChallengeDataSuccess:(NSData*)data;
- (void)_downloadChallengeDataFailure;
@end

@implementation OFChallenge

@synthesize challengeDefinition, challengeDescription, challenger, challengeDataUrl, hiddenText, userMessage, challengeData;

+ (void)setDelegate:(id<OFChallengeSendDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFChallenge class]];
	}
}

- (OFChallenge*)initWithDefinition:(OFChallengeDefinition*)definition challengeDescription:(NSString*)text challengeData:(NSData*)data
{
	self = [super init];
	if(self)
	{
		challengeDefinition = [definition retain];
		challengeDescription = [text retain];
		challengeData = [data retain];
	}
	return self;
}

- (void)displayAndSendChallenge
{
	[OFChallengeService displaySendChallengeModal:self.challengeDefinition.resourceId
									challengeText:self.challengeDescription 
									challengeData:self.challengeData];
}

- (OFRequestHandle*)sendChallenge:(OFChallengeDefinition*)challengeDefinition
						  toUsers:(NSArray*)userIds 
			inResponseToChallenge:(OFChallenge*)instigatingChallenge
{
	OFRequestHandle* handle = nil;
	handle = [OFChallengeService sendChallenge:self.challengeDefinition.resourceId
								 challengeText:self.challengeDescription
								 challengeData:self.challengeData
								   userMessage:self.userMessage
									hiddenText:self.hiddenText
									   toUsers:userIds
						 inResponseToChallenge:instigatingChallenge.resourceId
									 onSuccess:OFDelegate(self, @selector(_sendChallengeSuccess))
									 onFailure:OFDelegate(self, @selector(_sendChallengeFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFChallenge class]];
	return handle;
}

- (OFRequestHandle*)downloadChallengeData
{
	OFRequestHandle* handle = nil;
	handle = [OFChallengeService downloadChallengeData:self.challengeDataUrl
											 onSuccess:OFDelegate(self, @selector(_downloadChallengeDataSuccess:))
											 onFailure:OFDelegate(self, @selector(_downloadChallengeDataFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFChallenge class]];
	return handle;
}

- (void)_sendChallengeSuccess
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didSendChallenge:)])
	{
		[sharedDelegate didSendChallenge:self];
	}
}

- (void)_sendChallengeFailure
{
	if (sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailSendChallenge:)])
	{
		[sharedDelegate didFailSendChallenge:self];
	}
}

- (void)_downloadChallengeDataSuccess:(NSData*)data
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadChallengeData:OFChallenge:)])
	{
		[sharedDelegate didDownloadChallengeData:data OFChallenge:self];
	}
}

- (void)_downloadChallengeDataFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadChallengeDataOFChallenge:)])
	{
		[sharedDelegate didFailDownloadChallengeDataOFChallenge:self];
	}
}

- (void)setChallengeDefinition:(OFChallengeDefinition*)value
{
	OFSafeRelease(challengeDefinition);
	challengeDefinition = [value retain];
}

- (void)setChallengeDescription:(NSString*)value
{
	OFSafeRelease(challengeDescription);
	challengeDescription = [value retain];
}

- (void)setUserMessage:(NSString*)value
{
	OFSafeRelease(userMessage);
	userMessage = [value retain];
}

-(void)setHiddenText:(NSString*)value
{
	OFSafeRelease(hiddenText);
	hiddenText = [value retain];
}

- (OFChallengeDefinition*)getChallengeDefinition
{
	return challengeDefinition;
}

- (void)setDataUrl:(NSString*)value
{
	OFSafeRelease(challengeDataUrl);
	challengeDataUrl = [value retain];
}


+ (OFService*)getService;
{
	return [OFChallengeService sharedInstance];
}

- (void)setChallenger:(OFUser*)value
{
	OFSafeRelease(challenger);
	challenger = [value retain];
}

- (OFUser*)getChallenger
{
	return challenger;
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"description", @selector(setChallengeDescription:), @selector(challengeDescription));
		dataMap->addField(@"hidden_text", @selector(setHiddenText:), @selector(hiddenText));
		dataMap->addField(@"user_message", @selector(setUserMessage:), @selector(userMessage));
		dataMap->addField(@"user_data_url", @selector(setDataUrl:), @selector(challengeDataUrl));
		dataMap->addNestedResourceField(@"challenge_definition", @selector(setChallengeDefinition:), @selector(getChallengeDefinition), [OFChallengeDefinition class]);
		dataMap->addNestedResourceField(@"user",@selector(setChallenger:), @selector(getChallenger), [OFUser class]);
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"challenge";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"openfeint_challenge_discovered";
}

- (BOOL)usesChallengeData
{
	return	challengeDataUrl && 
	![challengeDataUrl isEqualToString:@""] &&
	![challengeDataUrl isEqualToString:@"/empty.blob"];
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
	OFSafeRelease(challengeData);
	OFSafeRelease(challengeDefinition);
	OFSafeRelease(challenger);
	OFSafeRelease(challengeDescription);
	OFSafeRelease(userMessage);
	OFSafeRelease(challengeDataUrl);
	OFSafeRelease(hiddenText);
	[super dealloc];
}

@end
