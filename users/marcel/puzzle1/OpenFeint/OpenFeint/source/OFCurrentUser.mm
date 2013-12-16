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

#import "OFCurrentUser.h"
#import "OFUser.h"
#import "OpenFeint+UserOptions.h"
#import "OFFriendsService.h"
#import "OFClientApplicationService+Private.h"
#import "OFPaginatedSeries.h"
#import "OFTableSectionDescription.h"
#import "OFUsersCredentialService.h"
#import "OFUsersCredential.h"

static id sharedDelegate = nil;

@interface OFCurrentUser (Private)
+ (void)_sendFriendRequestSuccess;
+ (void)_sendFriendRequestFailure;
+ (void)_unfriendSuccess;
+ (void)_unfriendFailure;
+ (void)_favoriteCurrentGameSuccess;
+ (void)_favoriteCurrentGameFailure;
+ (void)_checkConnectedToSocialNetworkSuccess:(OFPaginatedSeries*)credentialResources;
+ (void)_checkConnectedToSocialNetworkFailure;
@end

@implementation OFCurrentUser

+ (void)setDelegate:(id<OFCurrentUserDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFCurrentUser class]];
	}
}

+ (OFUser*)currentUser
{
	return [OpenFeint localUser];
}

+ (OFRequestHandle*)favoriteCurrentGame:(NSString*)reviewText
{
	OFRequestHandle* handle = nil;
	handle = [OFClientApplicationService makeGameFavorite:[OpenFeint clientApplicationId] 
											   reviewText:reviewText 
												onSuccess:OFDelegate(self, @selector(_favoriteCurrentGameSuccess)) 
												onFailure:OFDelegate(self, @selector(_favoriteCurrentGameFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFCurrentUser class]];
	return handle;
}

+ (OFRequestHandle*)sendFriendRequest:(OFUser*)user
{
	OFRequestHandle* handle = nil;
	handle = [OFFriendsService makeLocalUserFollow:user.resourceId 
										 onSuccess:OFDelegate(self, @selector(_currentUserSendFriendRequestSuccess))
										 onFailure:OFDelegate(self, @selector(_currentUserSendFriendRequestFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFCurrentUser class]];
	return handle;
}

+ (OFRequestHandle*)unfriend:(OFUser*)user
{
	OFRequestHandle* handle = nil;
	handle = [OFFriendsService makeLocalUserStopFollowing:user.resourceId
												onSuccess:OFDelegate(self, @selector(_currentUserUnfriendSuccess))
												onFailure:OFDelegate(self, @selector(_currentUserUnfriendFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFCurrentUser class]];
	return handle;
}

+ (OFRequestHandle*)checkConnectedToSocialNetwork
{
	OFRequestHandle* handle = nil;
	handle = [OFUsersCredentialService getIndexOnSuccess:OFDelegate(self, @selector(_checkConnectedToSocialNetworkSuccess:))
											  onFailure:OFDelegate(self, @selector(_checkConnectedToSocialNetworkFailure))
						   onlyIncludeLinkedCredentials:YES];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFCurrentUser class]];
	return handle;
}

+ (BOOL)hasFriends
{
	return [OpenFeint lastLoggedInUserHadFriendsOnBootup];
}

+ (NSInteger)unviewedChallengesCount
{
	return [OpenFeint unviewedChallengesCount];
}

+ (BOOL)allowsAutoSocialNotifications
{
	return [OpenFeint userAllowsNotifications];
}

+ (NSInteger)OpenFeintBadgeCount
{
	return [self unviewedChallengesCount] + [OpenFeint pendingFriendsCount] + [OpenFeint unreadInboxTotal];
}

+ (void)_favoriteCurrentGameSuccess
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFavoriteCurrentGame)])
	{
		[sharedDelegate didFavoriteCurrentGame];
	}
}

+ (void)_favoriteCurrentGameFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailFavoriteCurrentGame)])
	{
		[sharedDelegate didFailFavoriteCurrentGame];
	}
}

+ (void)_checkConnectedToSocialNetworkSuccess:(OFPaginatedSeries*)credentialResources
{
	BOOL connected = NO;
	NSMutableArray* credentials = [[(OFTableSectionDescription*)[[credentialResources objects] objectAtIndex:0] page] objects];
	for (OFUsersCredential* credential in credentials)
	{
		if ([credential isTwitter] || [credential isFacebook])
		{
			connected = YES;
		}
	}
	
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didCheckConnectedToSocialNetwork:)])
	{
		[sharedDelegate didCheckConnectedToSocialNetwork:connected];
	}
}

+ (void)_checkConnectedToSocialNetworkFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailCheckConnectedToSocialNetwork)])
	{
		[sharedDelegate didFailCheckConnectedToSocialNetwork];
	}
}

+ (void)_currentUserSendFriendRequestSuccess
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didSendFriendRequest)])
	{
		[sharedDelegate didSendFriendRequest];
	}
}

+ (void)_currentUserSendFriendRequestFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailSendFriendRequest)])
	{
		[sharedDelegate didFailSendFriendRequest];
	}
}

+ (void)_currentUserUnfriendSuccess
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didUnfriend)])
	{
		[sharedDelegate didUnfriend];
	}
}

+ (void)_currentUserUnfriendFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailUnfriend)])
	{
		[sharedDelegate didFailUnfriend];
	}
}

+ (bool)canReceiveCallbacksNow
{
	return YES;
}

- (bool)canReceiveCallbacksNow
{
	return YES;
}


@end



