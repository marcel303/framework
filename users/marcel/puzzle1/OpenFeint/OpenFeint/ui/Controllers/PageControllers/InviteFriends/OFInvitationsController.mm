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

#import "OFInvitationsController.h"
#import "OFTableSequenceControllerHelper+Overridables.h"
#import "OFInviteService.h"
#import "OpenFeint+UserOptions.h"
#import "OFInvite.h"
#import "OFInviteDefinition.h"
#import "OFViewInviteController.h"

@implementation OFInvitationsController

- (void)populateResourceMap:(OFResourceControllerMap*)resourceMap
{
	resourceMap->addResource([OFInvite class], @"Invite");
}

- (OFService*)getService
{
	return [OFInviteService sharedInstance];
}

- (void)doIndexActionWithPage:(NSUInteger)pageIndex onSuccess:(const OFDelegate&)success onFailure:(const OFDelegate&)failure
{
	[OFInviteService getInvitesForUser:[OpenFeint localUser] pageIndex:pageIndex onSuccess:success onFailure:failure];
}

- (void)doIndexActionOnSuccess:(const OFDelegate&)success onFailure:(const OFDelegate&)failure
{
	[self doIndexActionWithPage:1 onSuccess:success onFailure:failure];
}

- (void)onCellWasClicked:(OFResource*)cellResource indexPathInTable:(NSIndexPath*)indexPath
{
	if ([cellResource isKindOfClass:[OFInvite class]])
	{
		OFInvite* inv = (OFInvite*)cellResource;
		
		UIViewController* nextController = [OFViewInviteController viewInviteControllerWithInviteId:inv.resourceId];
		[self.navigationController pushViewController:nextController animated:YES];
	}
}

- (void)_updateEditButtonState
{
	if ([self isEditing])
	{
		self.navigationItem.rightBarButtonItem = [[[UIBarButtonItem alloc]
												   initWithBarButtonSystemItem:UIBarButtonSystemItemDone
												   target:self
												   action:@selector(_toggleEditing)]
												  autorelease];
	}
	else
	{
		self.navigationItem.rightBarButtonItem = [[[UIBarButtonItem alloc]
												   initWithBarButtonSystemItem:UIBarButtonSystemItemEdit
												   target:self
												   action:@selector(_toggleEditing)]
												  autorelease];
	}
}

- (void)_toggleEditing
{
	if (![self isEditing])
	{
		[self setEditing:YES];
	}
	else
	{
		[self setEditing:NO];
	}
	
	[self _updateEditButtonState];
}

- (bool)allowEditing
{
	return true;
}

- (void) onResourcesDownloaded:(OFPaginatedSeries *)resources
{
	[super onResourcesDownloaded:resources];
	
	int currentlyUnread = [OpenFeint unreadInviteCount];
	for (OFResource* res in resources)
	{
		if ([res isKindOfClass:[OFInvite class]])
		{
			OFInvite* invite = (OFInvite*)res;
			if ([invite.state isEqualToString:@"unviewed"])
			{
				--currentlyUnread;
			}
		}
	}
	
	// This shouldn't happen, but if there's an edge case where an invite comes in while we're loading the page..
	[OpenFeint setUnreadInviteCount:(currentlyUnread < 0) ? 0 : currentlyUnread];
	
	if (resources.count)
	{
		[self _updateEditButtonState];
	}
}

- (void)onResourceWasDeleted:(OFResource*)cellResource
{
	if ([cellResource isKindOfClass:[OFInvite class]])
	{
		[OFInviteService ignoreInvite:cellResource.resourceId onSuccess:OFDelegate() onFailure:OFDelegate()];
	}
	
	// @TODO if it was the last one, then stop editing and remove the button?
}

- (void)viewWillAppear:(BOOL)animated
{
	if (reloadOnWillAppear)
	{
		[self reloadDataFromServer];
	}
	[super viewWillAppear:animated];
}

- (void)notifyInviteIgnored:(NSString*)inviteResourceId
{
	// @TODO: find the cell, remove it from mSections
	reloadOnWillAppear = YES;	
}

- (NSString*)getNoDataFoundMessage
{
	return @"You have no unviewed game invitations at this time.";
}

- (bool)usePlainTableSectionHeaders
{
	return true;
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.title = @"Invitations";
}


@end
