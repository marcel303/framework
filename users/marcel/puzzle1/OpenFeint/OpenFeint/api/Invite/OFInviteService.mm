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

#import "OFInviteService.h"
#import "OFService+Private.h"
#import "OFHttpNestedQueryStringWriter.h"
#import "OFReachability.h"
#import "OFInvite.h"
#import "OFInviteDefinition.h"
#import "OFPaginatedSeries.h"
#import "OpenFeint+UserOptions.h"
#import "OFSelectFriendsToInviteController.h"
#import "OFUser.h"
#import "OpenFeint+Private.h"
#import "OFInviteNotificationController.h"
#import "OFFramedNavigationController.h"
#import "OFControllerLoader.h"
#import "UIButton+OpenFeint.h"
#import "NSInvocation+OpenFeint.h"
#import "OFInviteNotificationController.h"

OPENFEINT_DEFINE_SERVICE_INSTANCE(OFInviteService)

@implementation OFInviteService

OPENFEINT_DEFINE_SERVICE(OFInviteService);

- (void) populateKnownResources:(OFResourceNameMap*)namedResources
{
	namedResources->addResource([OFUser getResourceName], [OFUser class]);
	namedResources->addResource([OFInvite getResourceName], [OFInvite class]);
	namedResources->addResource([OFInviteDefinition getResourceName], [OFInviteDefinition class]);
}

+ (OFRequestHandle*)getDefaultInviteDefinitionForApplication:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [[self sharedInstance] 
	 getAction:@"invite_definitions/primary"
	 withParameters:nil
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];
}

+ (OFRequestHandle*)getInviteDefinition:(NSString*)inviteIdentifier onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
  OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	
	return [[self sharedInstance] 
	 getAction:[NSString stringWithFormat:@"invite_definitions/%@.xml", inviteIdentifier]
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];
}

+ (OFRequestHandle*)sendInvite:(OFInviteDefinition*)inviteDefinition withMessage:(NSString*)userMessage toUsers:(NSArray*)users onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
	params->io("key", inviteDefinition.resourceId); 
	params->io("invite[sender_message]", userMessage);
	
	for(OFResource* user in users) 
	{
		params->io("invite[receivers][]", user.resourceId); 
	}
	
	return [[self sharedInstance] 
	 postAction:@"invites.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];
}

+ (void)getInvitesForUser:(OFUser*)user pageIndex:(unsigned int)pageIndex onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter; 
	params->io("page", pageIndex); 
	NSString* markViewed = @"true";
	params->io("mark_viewed", markViewed);

	[[self sharedInstance] 
	 getAction:@"invites.xml"
	 withParameters:params
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];
}

+ (void)ignoreInvite:(NSString*)inviteResourceId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[[self sharedInstance] 
	 putAction:[NSString stringWithFormat:@"invites/%@/ignore", inviteResourceId]
	 withParameters:nil
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];	
}

+ (bool)canReceiveCallbacksNow
{
	return true;
}

+ (void)displaySendInviteModal:(NSString*)inviteIdentifier
{
	OFSelectFriendsToInviteController* controller = [OFSelectFriendsToInviteController inviteControllerWithInviteIdentifier:inviteIdentifier];
	[controller openAsModal];
}

@end
