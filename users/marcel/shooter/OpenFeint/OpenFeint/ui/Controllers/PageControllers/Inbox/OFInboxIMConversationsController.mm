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

#import "OFInboxIMConversationsController.h"
#import "OFSubscription.h"
#import "OFTableSequenceControllerHelper+Overridables.h"
#import "OpenFeint+UserOptions.h"

@implementation OFInboxIMConversationsController

- (void)loadView
{
	[super loadView];
	self.navigationItem.rightBarButtonItem = [[[UIBarButtonItem alloc]
											   initWithBarButtonSystemItem:UIBarButtonSystemItemCompose
											   target:self
											   action:@selector(_newMessagePressed)] autorelease];

}

- (void)viewWillAppear:(BOOL)animated
{
	[super viewWillAppear:animated];
	self.title = @"IM Conversations";
}

- (NSString*)getNoDataFoundMessage
{
	return @"You don't have any active IM conversations. Send a message to a friend by pressing the compose button.";
}

// @TEMP to change when we change the server action.
- (void)onBeforeResourcesProcessed:(OFPaginatedSeries *)resources
{
	[super onBeforeResourcesProcessed:resources];
	
	NSArray* enumObjects = [NSArray arrayWithArray:resources.objects];
	for (OFSubscription* sub in enumObjects)
	{
		if ([sub isForumThread])
		{
			[resources.objects removeObject:sub];
		}
	}
}

- (void)onResourcesDownloaded:(OFPaginatedSeries*)resources
{
	[super onResourcesDownloaded:resources];
	
	NSInteger numUnread = 0;
	
	for (OFSubscription* sub in resources)
	{
		numUnread += (sub.unreadCount > 0) ? 1 : 0;
	}
	
	[OpenFeint setUnreadIMCount:numUnread];
}


@end
