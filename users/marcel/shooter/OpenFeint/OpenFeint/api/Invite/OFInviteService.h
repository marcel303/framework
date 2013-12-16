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

#import "OFService.h"

@class OFInvite, OFInviteDefinition, OFUser;

@interface OFInviteService : OFService

OPENFEINT_DECLARE_AS_SERVICE(OFInviteService);

+ (OFRequestHandle*)getDefaultInviteDefinitionForApplication:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*)getInviteDefinition:(NSString*)inviteIdentifier onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*)sendInvite:(OFInviteDefinition*)inviteDefinition withMessage:(NSString*)userMessage toUsers:(NSArray*)users onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (void)getInvitesForUser:(OFUser*)user pageIndex:(unsigned int)pageIndex onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (void)ignoreInvite:(NSString*)inviteResourceId onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
////////////////////////////////////////////////////////////
///
/// displaySendInviteModal
/// 
/// @param inviteIdentifier			The invite identifier of the invite definition as seen in the developer dashboard
///									You may use nil for the default invite definition.
///
////////////////////////////////////////////////////////////
+ (void)displaySendInviteModal:(NSString*)inviteIdentifier;

@end
