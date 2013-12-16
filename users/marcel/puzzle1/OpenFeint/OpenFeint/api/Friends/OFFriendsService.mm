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
#import "OFFriendsService.h"
#import "OFHttpNestedQueryStringWriter.h"
#import "OFService+Private.h"
#import "OFUser.h"
#import "OFGamePlayer.h"
#import "OFRetainedPtr.h"
#import "OFUsersCredential.h"
#import "OFDelegateChained.h"
#import "OFPaginatedSeries.h"
#import "OpenFeint+UserOptions.h"

OPENFEINT_DEFINE_SERVICE_INSTANCE(OFFriendsService)

@implementation OFFriendsService

OPENFEINT_DEFINE_SERVICE(OFFriendsService);

- (void)populateKnownResources:(OFResourceNameMap*)namedResources
{
	namedResources->addResource([OFUser getResourceName], [OFUser class]);
	namedResources->addResource([OFGamePlayer getResourceName], [OFGamePlayer class]);
	namedResources->addResource([OFUsersCredential getResourceName], [OFUsersCredential class]);
	
}

+ (OFRequestHandle*)getUsersFollowedByUser:(NSString*)userId 
			  params:(OFPointer<OFHttpNestedQueryStringWriter>)params 
					 onSuccess:(const OFDelegate&)onSuccess 
					 onFailure:(const OFDelegate&)onFailure
{
	
	params->io("user_id", userId ? userId : @"me");
	params->io("scope", @"people-of-interest");
	
	return [[self sharedInstance] 
	 getAction:@"users.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Downloaded Friends"]];
}

+ (void)getUsersFollowedByLocalUser:(NSInteger)pageIndex onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[OFFriendsService getUsersFollowedByUser:nil pageIndex:pageIndex onSuccess:onSuccess onFailure:onFailure];
}

// Gets the list of friends that are invitable to this app.  Currently: friends who don't have this device credential and who don't have the app already.
+ (void)getInvitableFriends:(NSInteger)pageIndex onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("page", pageIndex);

	[[self sharedInstance]
	 getAction:@"invites/new/invitable_friends"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];
}

+ (OFRequestHandle*)getAllUsersFollowedByUserAlphabetical:(NSString*)userId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	bool alphabetical = true;
	params->io("full_alphabetical_list", alphabetical);
	return [OFFriendsService getUsersFollowedByUser:userId params:params onSuccess:onSuccess onFailure:onFailure];
}

+ (void)getAllUsersFollowedByLocalUserAlphabetical:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[OFFriendsService getAllUsersFollowedByUserAlphabetical:nil onSuccess:onSuccess onFailure:onFailure];
}

+ (OFRequestHandle*)getAllUsersWithApp:(NSString*)applicationId followedByUser:(NSString*)userId alphabeticalOnSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	bool alphabetical = true;
	params->io("full_alphabetical_list", alphabetical);
	params->io("with_client_application", ([applicationId length] > 0) ? applicationId : [OpenFeint clientApplicationId]);
	params->io("not_sectioned", @"yes");
	return [OFFriendsService getUsersFollowedByUser:userId params:params onSuccess:onSuccess onFailure:onFailure];
}

+ (void)getUsersFollowedByUser:(NSString*)userId pageIndex:(NSInteger)pageIndex onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("page", pageIndex);
	[OFFriendsService getUsersFollowedByUser:userId params:params onSuccess:onSuccess onFailure:onFailure];
}

+ (void)getUsersWithAppFollowedByLocalUser:(NSString*)applicationId pageIndex:(NSInteger)pageIndex onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[OFFriendsService getUsersWithAppFollowedByUser:applicationId followedByUser:nil pageIndex:pageIndex onSuccess:onSuccess onFailure:onFailure];
}

+ (void)getUsersWithAppFollowedByUser:(NSString*)applicationId followedByUser:(NSString*)userId pageIndex:(NSInteger)pageIndex onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("page", pageIndex);
	params->io("user_id", userId ? userId : @"me");
	params->io("scope", @"people-of-interest");

	params->io("with_client_application", applicationId ? applicationId : [OpenFeint clientApplicationId]);
	
	
	[[self sharedInstance] 
	 getAction:@"users.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Downloaded Friends"]];
}

+ (void)getUsersFollowingLocalUser:(NSInteger)pageIndex 
						 excludeUsersFollowedByTarget:(BOOL)excludeUsersFollowedByTarget
						 onSuccess:(const OFDelegate&)onSuccess 
						 onFailure:(const OFDelegate&)onFailure
{
	[OFFriendsService getUsersFollowingUser:nil 
			   excludeUsersFollowedByTarget:excludeUsersFollowedByTarget 
								  pageIndex:pageIndex 
								  onSuccess:onSuccess 
								  onFailure:onFailure];
}

+ (void)getUsersFollowingUser:(NSString*)userId 
 excludeUsersFollowedByTarget:(BOOL)excludeUsersFollowedByTarget
					pageIndex:(NSInteger)pageIndex 
					onSuccess:(const OFDelegate&)onSuccess 
					onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("page", pageIndex);
	if (userId)
	{
		params->io("user_id", userId);
	}
	params->io("scope", @"followers");
	if (excludeUsersFollowedByTarget)
	{
		bool exclude = true;
		params->io("im-not-following", exclude);
	}
	
	[[self sharedInstance] 
	 getAction:@"users.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Downloaded Followers"]];
}
	
+ (OFRequestHandle*)makeLocalUserFollow:(NSString*)userId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{	
	return [[self sharedInstance] 
	 postAction:[NSString stringWithFormat:@"users/%@/following.xml", userId]
	 withParameters:nil
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Followed"]];
}

+ (OFRequestHandle*)makeLocalUserStopFollowing:(NSString*)userId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{	
	return [[self sharedInstance] 
	 deleteAction:[NSString stringWithFormat:@"users/%@/following.xml", userId]
	 withParameters:nil
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Stopped Following"]];
}

+ (void)removeLocalUsersFollower:(NSString*)userId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("follower", @"yes");
	
	[[self sharedInstance] 
	 deleteAction:[NSString stringWithFormat:@"users/%@/following.xml", userId]
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestForeground
	 withNotice:[OFNotificationData foreGroundDataWithText:@"Removed Follower"]];
}

+ (void)isLocalUserFollowingAnyone:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	int pageIndex = 1;
	int perPage = 1;
	OFDelegate onSuccessChained([self sharedInstance], @selector(onIsLocalUserFollowingAnyoneSuccess:nextCall:), onSuccess);
	
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("page", pageIndex);
	params->io("per_page", perPage);
	params->io("user_id", @"me");
	params->io("scope", @"people-of-interest");
	
	[[self sharedInstance] 
	 getAction:@"users.xml"
	 withParameters:params
	 withSuccess:onSuccessChained
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];
}

- (void)onIsLocalUserFollowingAnyoneSuccess:(OFPaginatedSeries*)page nextCall:(OFDelegateChained*)nextCall
{
	BOOL isFollowingAnyone = [page.objects count] > 0;
	[nextCall invokeWith:[NSNumber numberWithBool:isFollowingAnyone]];
}

@end
