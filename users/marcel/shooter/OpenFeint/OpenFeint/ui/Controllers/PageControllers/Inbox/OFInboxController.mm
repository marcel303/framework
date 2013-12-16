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

#import "OFInboxController.h"

#import "OFConversationService+Private.h"
#import "OFSubscriptionService+Private.h"
#import "OFSubscription.h"
#import "OFConversationController.h"
#import "OFConversation.h"

#import "OFUser.h"
#import "OFForumThread.h"
#import "OFForumTopic.h"
#import "OFForumThreadViewController.h"
#import "OFTableSequenceControllerHelper+Overridables.h"

#import "OFResourceControllerMap.h"

#import "OpenFeint+UserOptions.h"
#import "OFTableSectionDescription.h"

@interface OFInboxController (Internal)
- (void)pickerFinishedWithSelectedUser:(OFUser*)selectedUser;
- (void)_newMessagePressed;
@end

@implementation OFInboxController
@synthesize actionIndexPath;

#pragma mark Boilerplate

- (void)dealloc
{
	OFSafeRelease(pendingConversationUser);
    self.actionIndexPath = nil;
	[super dealloc];
}

#pragma mark Internal Methods

- (void)_conversationStarted:(OFPaginatedSeries*)conversationPage
{
	if (!pendingConversationUser)
		return;
		
	[self hideLoadingScreen];

	if ([conversationPage count] == 1)
	{
		OFConversation* conversation = [conversationPage objectAtIndex:0];
		OFConversationController* controller = [OFConversationController conversationWithId:conversation.resourceId withUser:conversation.otherUser];
		[self.navigationController pushViewController:controller animated:YES];
	}
	
	OFSafeRelease(pendingConversationUser);
}

- (NSString*)getNoDataFoundMessage
{
	return @"Your inbox is empty.";
}

- (void)_conversationError
{
	OFSafeRelease(pendingConversationUser);
	[self hideLoadingScreen];
	
	[[[[UIAlertView alloc] 
		initWithTitle:@"Error" 
		message:@"An error occurred. Please try again later." 
		delegate:nil 
		cancelButtonTitle:@"Ok" 
		otherButtonTitles:nil] autorelease] show];
}

- (void)pickerFinishedWithSelectedUser:(OFUser*)selectedUser
{
	[self showLoadingScreen];
	self.navigationItem.rightBarButtonItem.enabled = YES;
    
	pendingConversationUser = [selectedUser retain];
	
	[OFConversationService
		startConversationWithUser:selectedUser.resourceId
		onSuccess:OFDelegate(self, @selector(_conversationStarted:))
		onFailure:OFDelegate(self, @selector(_conversationError))];
}

- (void)pickerCancelled
{
	self.navigationItem.rightBarButtonItem.enabled = YES;
}

-(OFInboxController*)initAndBeginConversationWith:(OFUser *)theUser
{
	self = [super init];
	if (self) {
		pendingConversationUser = [theUser retain];
	}
	return self;
	
}

-(void)beginConversationWith:(OFUser *)theUser
{
	pendingConversationUser = [theUser retain];
}


- (bool)autoLoadData
{
	return pendingConversationUser == nil;
}

- (void)_newMessagePressed
{
    self.navigationItem.rightBarButtonItem.enabled = NO;
    [OFFriendPickerController launchPickerWithDelegate:self promptText:@"Select a friend to message"];
}

#pragma mark OFViewController

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];

	if (pendingConversationUser) {
		[self showLoadingScreen];
		[OFConversationService
		 startConversationWithUser:pendingConversationUser.resourceId
		 onSuccess:OFDelegate(self, @selector(_conversationStarted:))
		 onFailure:OFDelegate(self, @selector(_conversationError))];
	}
}

#pragma mark OFTableSequenceControllerHelper

- (void)populateResourceMap:(OFResourceControllerMap*)resourceMap
{
	resourceMap->addResource([OFSubscription class], @"Inbox");
}

- (OFService*)getService
{
	return [OFSubscriptionService sharedInstance];
}

- (bool)shouldAlwaysRefreshWhenShown
{
	return true;
}

- (bool)allowPagination
{
	return true;
}

- (void)onCellWasClicked:(OFResource*)cellResource indexPathInTable:(NSIndexPath*)indexPath
{
    if([self cellTypeAtIndexPath:indexPath] == OFTableSectionCellTypes::Action) return;
	if ([cellResource isKindOfClass:[OFSubscription class]])
	{
		UIViewController* pushController = nil;
		
		OFSubscription* subscription = (OFSubscription*)cellResource;
		
		if ([subscription isForumThread])
		{
			OFForumTopic* topic = [[[OFForumTopic alloc] initWithId:subscription.topicId] autorelease];
			OFForumThread* thread = subscription.discussion;
            thread.isSubscribed = YES;
			pushController = [OFForumThreadViewController threadView:thread topic:topic];
		}
		else if ([subscription isConversation])
		{
			pushController = [OFConversationController conversationWithId:subscription.discussionId withUser:subscription.otherUser];
		}

		[self.navigationController pushViewController:pushController animated:YES];
#ifdef _DEBUG
        id check = [pushController parentViewController];
        OFLog(@"Nav: %@", check);
#else
		[pushController parentViewController];
#endif
	}
}

- (NSString*)getTableHeaderViewName
{
	return nil;
}

- (void)onTableHeaderCreated:(UIViewController*)tableHeader
{
}

- (void)doIndexActionWithPage:(unsigned int)oneBasedPageNumber onSuccess:(const OFDelegate&)success onFailure:(const OFDelegate&)failure
{
	[OFSubscriptionService
		getSubscriptionsPage:oneBasedPageNumber 
		onSuccess:success 
		onFailure:failure];
}

- (void)doIndexActionOnSuccess:(const OFDelegate&)success onFailure:(const OFDelegate&)failure
{
	[OFSubscriptionService
		getSubscriptionsPage:1 
		onSuccess:success 
		onFailure:failure];
}

-(NSString*)actionCellClassName {
    return @"InboxAction";
}


-(void) deletionWasSuccessful {    
    //fix up the table
    [self reloadDataFromServer];  //BUG1685 - QA would rather take the hit than empty the list
    return;
    
    /*
     //this code would just remove the item itself, the problem is that when you deleted a large
     //number of IMs, it wouldn't add the new ones to the end of the list.
     
    OFTableSectionDescription* section = [self getSectionForIndexPath:self.actionIndexPath];
    if([section.page.objects count] == 1) {
        [self reloadDataFromServer];  //last item, so force a reload
    }
    else {        
        //try to just remove the appropriate items
        NSUInteger dataRow, cellType;
        [section gatherDataForRow:self.actionIndexPath.row dataRow:dataRow cellType:cellType];
        [section.page.objects removeObjectAtIndex:dataRow];
        [section.expandedViews removeObjectAtIndex:dataRow];
        NSIndexPath *firstRow = [NSIndexPath indexPathForRow:self.actionIndexPath.row-1 inSection:self.actionIndexPath.section];
        
        [self.tableView deleteRowsAtIndexPaths:[NSArray arrayWithObjects:self.actionIndexPath, firstRow, nil] 
                              withRowAnimation:UITableViewRowAnimationFade];
        
    }
     */
}

-(void)actionSheet:(UIActionSheet*)sheet clickedButtonAtIndex:(NSInteger)buttonIndex {
    if(buttonIndex != [sheet cancelButtonIndex]) {
        OFSubscription* sub = (OFSubscription*) [self getCellAtIndexPath:self.actionIndexPath];
        [OFConversationService deleteConversation:sub.discussionId 
                                        onSuccess:OFDelegate(self, @selector(deletionWasSuccessful))
                                        onFailure:OFDelegate()];
    }
}

- (void) actionPressedForTag:(NSUInteger) tag indexPath:(NSIndexPath*) indexPath {
    //tag0 is Block User, tag1 is Delete, these are defined in the nib    
    if(tag == 1) {        
        self.actionIndexPath = indexPath;
        UIActionSheet* sheet = [[UIActionSheet alloc] initWithTitle:@"Deleting IM" 
                                                           delegate:self 
                                                  cancelButtonTitle:@"Cancel" 
                                             destructiveButtonTitle:@"Delete" 
                                                  otherButtonTitles:nil];
        [sheet showInView:self.tableView];
    }
}

@end
