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

#import "OFInviteCell.h"
#import "OFInvite.h"
#import "OFInviteDefinition.h"
#import "OFImageView.h"
#import "OFImageLoader.h"
#import "OFUser.h"

@implementation OFInviteCell

@synthesize inviteIcon, gameTitle, invitedBy, userMessage, freshnessIndicator;

- (void)onResourceChanged:(OFResource*)resource
{
	OFInvite* inv = (OFInvite*)resource;
	[inviteIcon useProfilePictureFromUser:inv.senderUser];
	gameTitle.text = inv.clientApplicationName;
	invitedBy.text = [NSString stringWithFormat:@"Invited by %@", inv.senderUser.name];
	userMessage.text = inv.userMessage;
	freshnessIndicator.hidden = ![inv.state isEqualToString:@"unviewed"];
}

- (void)dealloc
{
	self.inviteIcon = nil;
	self.gameTitle = nil;
	self.invitedBy = nil;
	self.userMessage = nil;
	[super dealloc];
}

@end
