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

#import "OFInviteFriendsController.h"
#import "OFResourceControllerMap.h"
#import "OFFriendsService.h"
#import "OFUser.h"
#import "OFUserCell.h"
#import "OFTableSectionDescription.h"
#import "OpenFeint+UserOptions.h"
#import "OpenFeint+Settings.h"
#import "OpenFeint+Private.h"
#import "OFControllerLoader.h"
#import "OFFullScreenImportFriendsMessage.h"
#import "OFImageView.h"
#import "OFInviteFriendsHeaderController.h"
#import "OFDefaultTextField.h"
#import "OFReachability.h"
#import "OFTableControllerHelper+Overridables.h"
#import "OFTableSequenceControllerHelper+ViewDelegate.h"
#import "OFTableSequenceControllerHelper+Overridables.h"
#import "OFInviteService.h"
#import "OFInvite.h"
#import "OFInviteDefinition.h"
#import "OFImageLoader.h"
#import "OFInviteCompleteController.h"
#import "UIButton+OpenFeint.h"
#import "IPhoneOSIntrospection.h"

@implementation OFInviteFriendsController

@synthesize selectedUsers, definition, closeInvocation;

- (void)populateResourceMap:(OFResourceControllerMap*)resourceMap
{
	resourceMap->addResource([OFUser class], @"User");
}

- (OFService*)getService
{
	return [OFFriendsService sharedInstance];
}

- (void)doIndexActionOnSuccess:(const OFDelegate&)success onFailure:(const OFDelegate&)failure;
{
	OFPaginatedSeries* page = [OFPaginatedSeries paginatedSeries];
	[page.objects addObjectsFromArray:selectedUsers];
	OFTableSectionDescription* section = [[[OFTableSectionDescription alloc] initWithTitle:@"Selected Friends" andPage:page] autorelease];

	success.invoke([OFPaginatedSeries paginatedSeriesWithObject:section]);
}

- (UIViewController*)getNoDataFoundViewController
{
    OFFullScreenImportFriendsMessage* noDataController = (OFFullScreenImportFriendsMessage*)OFControllerLoader::load(@"FullscreenImportFriendsMessage");
    noDataController.owner = self;
    return noDataController;
}

- (NSString*)getNoDataFoundMessage
{
	return [NSString stringWithFormat:@"All of your friends have %@.  Make some more!", [OpenFeint applicationDisplayName]];
}

- (bool)usePlainTableSectionHeaders
{
	return true;
}

- (NSString*)getTableHeaderControllerName
{
	return @"InviteFriendsHeader";
}

- (void)onTableHeaderCreated:(UIViewController*)tableHeader
{
	OFInviteFriendsHeaderController* header = (OFInviteFriendsHeaderController*)tableHeader;
	header.inviteText.text = definition.suggestedSenderMessage;
	header.titleLabel.text = [NSString stringWithFormat:@"Invite Friends to play %@", [OpenFeint applicationDisplayName]];
}

- (void)showNoFriendsSelectedMessage
{
	[[[[UIAlertView alloc] initWithTitle:@"No Friends Selected" 
								 message:@"You must select at least one friend to invite." 
								delegate:nil 
					   cancelButtonTitle:@"OK" 
					   otherButtonTitles:nil] autorelease] show];
}

- (void)showNoConnectionMessage
{
	[[[[UIAlertView alloc] initWithTitle:@"No internet connection" 
								 message:@"You must have an internet connection to invite friends." 
								delegate:nil 
					   cancelButtonTitle:@"OK" 
					   otherButtonTitles:nil] autorelease] show];
}

- (void)showSubmitFailedMessage
{
	[[[[UIAlertView alloc] initWithTitle:@"An error occurred" 
								 message:@"Your invite was not submitted. Please try again." 
								delegate:nil 
					   cancelButtonTitle:@"OK" 
					   otherButtonTitles:nil] autorelease] show];
}

- (NSString*)getUserMessage:(UITextField*)textField
{
	return (textField.text && ![textField.text isEqualToString:@""]) ? textField.text : textField.placeholder;
}

- (IBAction)inviteFriends
{
	if ([selectedUsers count] == 0)
	{
		[self showNoFriendsSelectedMessage];
	}
	else if (!OFReachability::Instance()->isGameServerReachable())
	{
		[self showNoConnectionMessage];
	}
	else
	{	
		self.navigationItem.rightBarButtonItem.enabled = NO;
		OFDelegate success(self, @selector(inviteSuccess));
		OFDelegate failure(self, @selector(inviteFailure));

		[self showLoadingScreen];
		
		[OFInviteService sendInvite:definition
						withMessage:((OFInviteFriendsHeaderController*)mTableHeaderController).inviteText.text
							toUsers:selectedUsers
						  onSuccess:OFDelegate(self, @selector(inviteSuccess))
						  onFailure:OFDelegate(self, @selector(inviteFailure))];
	}
}

- (IBAction)cancel
{
	[self.closeInvocation invoke];
}

- (void)inviteSuccess
{
	[self hideLoadingScreen];
	
	OFInviteCompleteController* completeController = (OFInviteCompleteController*)OFControllerLoader::load(@"InviteComplete");
	[completeController.inviteIcon setImageUrl:definition.inviteIconURL];

	if (selectedUsers.count > 1)
	{
		completeController.messageLabel.text = @"Your invitations were sent!";
		completeController.messageTitleLabel.text = @"Invitations Sent";
	}
	else
	{
		completeController.messageLabel.text = @"Your invitation was sent!";
		completeController.messageTitleLabel.text = @"Invitation Sent";
	}

	completeController.title = @"Sending Invite";
	[completeController.continueButton setTitleForAllStates:@"OK"];
	completeController.continueInvocation = self.closeInvocation;

	[self.navigationController pushViewController:completeController animated:YES];

}

- (void)inviteFailure
{
	self.navigationItem.rightBarButtonItem.enabled = YES;

	[self showSubmitFailedMessage];
	[self hideLoadingScreen];
}

- (bool)canReceiveCallbacksNow
{
	return true;
}

- (NSIndexPath *)tableView:(UITableView *)tableView willSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
	return nil;
}

- (void)viewDidLoad
{
	[super viewDidLoad];
	self.navigationItem.rightBarButtonItem = [[[UIBarButtonItem alloc] initWithTitle:@"Send" style:UIBarButtonItemStylePlain target:self action:@selector(inviteFriends)] autorelease];
	self.title = @"Compose Invite";

#ifdef _IPHONE_3_0
	if (!is2PointOhSystemVersion())
	{
		self.tableView.allowsSelection = NO;
	}
#endif
}

- (void)dealloc
{
	self.selectedUsers = nil;
	self.definition = nil;
	[super dealloc];
}

@end

