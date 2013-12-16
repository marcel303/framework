////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2009 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import "OFTableSequenceControllerHelper.h"
#import "OFFramedNavigationControllerBehavior.h"

@class OFUser, OFUserCell, OFInviteDefinition, OFSelectFriendsToInviteHeaderController;

@interface OFSelectFriendsToInviteController : OFTableSequenceControllerHelper<OFCallbackable, OFFramedNavigationControllerBehavior>
{
	NSString* inviteIdentifier;
	NSMutableArray* mSelectedUsers;
	OFInviteDefinition* mDefinition;
	OFSelectFriendsToInviteHeaderController* mHeader;
	BOOL stallNextRefresh;
	
	BOOL definitionDownloadOutstanding;
	BOOL friendsDownloadOutstanding;
	BOOL anyFriendsDownloadOutstanding;
	BOOL followingAnyone;
}

@property (nonatomic, retain) NSString* inviteIdentifier;

- (IBAction)advance;
- (IBAction)cancel;
- (void)toggleSelectionOfUser:(OFUser*)user;

+ (OFSelectFriendsToInviteController*)inviteControllerWithInviteIdentifier:(NSString*)_inviteIdentifier;
+ (OFSelectFriendsToInviteController*)inviteController;

- (void)openAsModal;
- (void)openAsModalInDashboard;

@end
