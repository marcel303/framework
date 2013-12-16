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

#import "OFInboxTopLevelController.h"
#import "OFFanClubCell.h"
#import "OFControllerLoader.h"
#import "OFImageLoader.h"
#import "OFTableSectionDescription.h"
#import "OFInboxController.h"
#import "OpenFeint+UserOptions.h"
#import "OpenFeint+NSNotification.h"
#import "OFBadgeView.h"

enum
{
	kCellIndex_Invitations,
	kCellIndex_IMConversations,
	kCellIndex_ForumPosts,
};

@implementation OFInboxTopLevelController

- (void) createCell:(NSMutableArray*)cellArray withTitle:(NSString*)cellTitle andIconName:(NSString*)iconName andBadgeValue:(NSUInteger)badgeValue andCreateSelector:(SEL)createSelector
{
	// @TODO don't misappropriate OFFanClubCell
	OFFanClubCell* curCell = (OFFanClubCell*)OFControllerLoader::loadCell(@"FanClub");
	curCell.titleLabel.text = cellTitle;
	curCell.createControllerSelector = createSelector;
	curCell.iconView.image = iconName ? [OFImageLoader loadImage:iconName] : nil;
	[curCell.badgeView setValue:badgeValue];
	[cellArray addObject:curCell];
}

- (NSMutableArray*) buildTableSectionDescriptions
{
	NSMutableArray* staticCells = [NSMutableArray arrayWithCapacity:3];
	
	[self createCell:staticCells withTitle:@"Invitations" andIconName:@"OFInboxAcceptInviteIcon.png" andBadgeValue:[OpenFeint unreadInviteCount] andCreateSelector:@selector(onInvitations)];
	[self createCell:staticCells withTitle:@"IM Conversations" andIconName:@"OFInboxIMIcon.png" andBadgeValue:[OpenFeint unreadIMCount] andCreateSelector:@selector(onIMConversations)];
	[self createCell:staticCells withTitle:@"Starred Forum Posts" andIconName:@"OFInboxForumsIcon.png" andBadgeValue:[OpenFeint unreadPostCount] andCreateSelector:@selector(onForumPosts)];
	
	return [NSMutableArray arrayWithObject:[OFTableSectionDescription sectionWithTitle:nil andStaticCells:staticCells]];
}


- (void)onCellWasClicked:(OFResource*)cellResource indexPathInTable:(NSIndexPath*)indexPath
{
	if ([mSections count] > 0)
	{
		NSMutableArray* staticCells = [[mSections objectAtIndex:0] staticCells];
		if ([staticCells count] > indexPath.row)
		{
			OFFanClubCell* cell = [staticCells objectAtIndex:indexPath.row];
			[self.navigationController pushViewController:[self performSelector:cell.createControllerSelector] animated:YES];
			[cell setSelected:NO animated:YES];
		}
	}
}

- (UIViewController*)onInvitations
{
	return OFControllerLoader::load(@"Invitations");
}

- (UIViewController*)onIMConversations
{
	return OFControllerLoader::load(@"InboxIMConversations");
}

- (UIViewController*)onForumPosts
{
	return OFControllerLoader::load(@"InboxSubscribedForumThreads");
}

- (void)_somethingHappened:(NSNotification*)notification
{
	NSArray* staticCells = [[mSections objectAtIndex:0] staticCells];
	OFFanClubCell* cell = nil;
	NSNumber* value = nil;
	
	if ([notification.name isEqualToString:OFNSNotificationUnreadIMCountChanged])
	{
		cell = [staticCells objectAtIndex:kCellIndex_IMConversations];
		value = [notification.userInfo objectForKey:OFNSNotificationInfoUnreadIMCount];
	}
	else if ([notification.name isEqualToString:OFNSNotificationUnreadPostCountChanged])
	{
		cell = [staticCells objectAtIndex:kCellIndex_ForumPosts];
		value = [notification.userInfo objectForKey:OFNSNotificationInfoUnreadPostCount];
	}
	else if ([notification.name isEqualToString:OFNSNotificationUnreadInviteCountChanged])
	{
		cell = [staticCells objectAtIndex:kCellIndex_Invitations];
		value = [notification.userInfo objectForKey:OFNSNotificationInfoUnreadInviteCount];
	}

	if (cell && value)
	{
		// check out all the values, it's like walmart up in here
		cell.badgeView.value = [value intValue];
	}
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.title = @"My Inbox";
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_somethingHappened:) name:OFNSNotificationUnreadIMCountChanged object:nil];	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_somethingHappened:) name:OFNSNotificationUnreadPostCountChanged object:nil];	
	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(_somethingHappened:) name:OFNSNotificationUnreadInviteCountChanged object:nil];	
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self name:OFNSNotificationUnreadIMCountChanged object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:OFNSNotificationUnreadPostCountChanged object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self name:OFNSNotificationUnreadInviteCountChanged object:nil];
	[super dealloc];
}

@end
