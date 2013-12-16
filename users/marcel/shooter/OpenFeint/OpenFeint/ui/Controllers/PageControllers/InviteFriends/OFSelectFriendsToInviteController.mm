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

#import "OFDependencies.h"
#import "OFSelectFriendsToInviteController.h"
#import "OFSelectFriendsToInviteHeaderController.h"
#import "OFResourceControllerMap.h"
#import "OFFriendsService.h"
#import "OFUser.h"
#import "OFSelectableUserCell.h"
#import "OFTableSectionDescription.h"
#import "OpenFeint+UserOptions.h"
#import "OpenFeint+Settings.h"
#import "OpenFeint+Private.h"
#import "OFControllerLoader.h"
#import "OFUsersCredential.h"
#import "OFFullScreenImportFriendsMessage.h"
#import "OFChallengeDelegate.h"
#import "OFChallengeService+Private.h"
#import "OFChallenge.h"
#import "OFSelectFriendsToInviteHeaderController.h"
#import "OFDefaultTextField.h"
#import "OFReachability.h"
#import "OFTableControllerHelper+Overridables.h"
#import "OFTableSequenceControllerHelper+ViewDelegate.h"
#import "OFTableSequenceControllerHelper+Overridables.h"
#import "OFInviteService.h"
#import "OFInviteDefinition.h"
#import "OFInviteFriendsController.h"
#import "OFDeadEndErrorController.h"
#import "OFFramedNavigationController.h"
#import "UINavigationController+OpenFeint.h"
#import "NSInvocation+OpenFeint.h"

@interface OFSelectFriendsToInviteController ()

- (void) _refreshData;
- (void) _refreshDataNow;

@end

@implementation OFSelectFriendsToInviteController

@synthesize inviteIdentifier;

// Loading screen is shown when (definitionDownloadOutstanding||friendsDownloadOutstanding),
// so we need to make sure we only hide it when both are false.
- (void)hideLoadingScreen
{
	if(!definitionDownloadOutstanding && !friendsDownloadOutstanding && !anyFriendsDownloadOutstanding)
	{
		[super hideLoadingScreen];
	}
}

// Make sure we set friendsDownloadOutstanding before trying to hide the loading screen.
- (void)_onDataLoadedWrapper:(OFPaginatedSeries*)resources isIncremental:(BOOL)isIncremental
{
	friendsDownloadOutstanding = NO;
	[super _onDataLoadedWrapper:resources isIncremental:isIncremental];
}

- (void)populateResourceMap:(OFResourceControllerMap*)resourceMap
{
	resourceMap->addResource([OFUser class], @"SelectableUser");
}

- (OFService*)getService
{
	return [OFFriendsService sharedInstance];
}

- (void)doIndexActionWithPage:(NSUInteger)pageIndex onSuccess:(const OFDelegate&)success onFailure:(const OFDelegate&)failure
{
	friendsDownloadOutstanding = YES;
	[OFFriendsService getInvitableFriends:pageIndex onSuccess:success onFailure:failure];
}

- (void)doIndexActionOnSuccess:(const OFDelegate&)success onFailure:(const OFDelegate&)failure;
{
	[self doIndexActionWithPage:1 onSuccess:success onFailure:failure];
    stallNextRefresh = YES;
}

- (bool)autoLoadData
{
	return false;
}

- (UIViewController*)getNoDataFoundViewController
{
	// @DANGER : if modal then we might have two modals?
    OFFullScreenImportFriendsMessage* noDataController = (OFFullScreenImportFriendsMessage*)OFControllerLoader::load(@"HeaderedFullscreenImportFriendsMessage");
	if (followingAnyone)
	{
		noDataController.messageLabel.text = [NSString stringWithFormat:@"All of your friends have %@. You can make more friends by importing them from Facebook or Twitter or find friends by their OpenFeint name.", [OpenFeint applicationDisplayName]];
	}
	
    noDataController.owner = self;
    return noDataController;
}

- (NSString*)getNoDataFoundMessage
{
	// We don't actually use this, but if we don't override it, we'll crash.
	return @"";
}

- (NSString*)getDataNotLoadedYetMessage
{
	// This isn't used with the getNoDataFoundViewController - this is actually what shows while we're getting the invite definition.
	return @"Loading Invitation...";
}

- (void)setUIFromDefinition
{
	if (mHeader && mDefinition)
	{
		mHeader.enticeLabel.text = mDefinition.senderIncentiveText;
		[mHeader.inviteIcon setImageUrl:mDefinition.inviteIconURL];
	}
}

- (void)failedDownloadingDefinition
{
	definitionDownloadOutstanding = NO;
	[self hideLoadingScreen];
	
//	[self.navigationController pushViewController:[OFDeadEndErrorController mustBeOnlineErrorWithMessage:@"Failed downloading invite data.  Try again later."] animated:YES];
}

- (void)downloadedDefinition:(OFPaginatedSeries*)series
{
	if (series.objects.count)
	{
		definitionDownloadOutstanding = NO;
		[self hideLoadingScreen];
		
		OFInviteDefinition* definition = [series.objects objectAtIndex:0];
		
		mDefinition = [definition retain];
		[self setUIFromDefinition];		
	}
	else
	{
		[self failedDownloadingDefinition];
	}
}

- (void)localUserFollowingAnyoneFail
{
	anyFriendsDownloadOutstanding = NO;
	[self hideLoadingScreen];
}

- (void)localUserFollowingAnyone:(NSNumber*)_followingAnyone
{
	anyFriendsDownloadOutstanding = NO;
	[self hideLoadingScreen];
	followingAnyone = [_followingAnyone boolValue];
	[self _refreshData];
}

-(void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
	[self showLoadingScreen];
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
	
	// @TODO: we don't want to reload friends if we're just coming back from the send invite controller, but since we are,
	// let's not have our list of friends be inconsistent with the selected set.  To be fixed.
	OFSafeRelease(mSelectedUsers);
	
	if (inviteIdentifier)
	{
		[OFInviteService getInviteDefinition:inviteIdentifier
								   onSuccess:OFDelegate(self, @selector(downloadedDefinition:))
								   onFailure:OFDelegate(self, @selector(failedDownloadingDefinition))];
	}
	else
	{
		[OFInviteService getDefaultInviteDefinitionForApplication:OFDelegate(self, @selector(downloadedDefinition:))
														onFailure:OFDelegate(self, @selector(failedDownloadingDefinition))];
	}
	definitionDownloadOutstanding = YES;
	
	// Also, does user have any friends?
	[OFFriendsService isLocalUserFollowingAnyone:OFDelegate(self, @selector(localUserFollowingAnyone:)) onFailure:OFDelegate(self, @selector(localUserFollowingAnyoneFail))];
	anyFriendsDownloadOutstanding = YES;
	
}

- (bool)usePlainTableSectionHeaders
{
	return true;
}

- (NSString*)getTableHeaderControllerName
{
	return @"SelectFriendsToInviteHeader";
}

- (void)onTableHeaderCreated:(UIViewController*)tableHeader
{
	mHeader = (OFSelectFriendsToInviteHeaderController*)tableHeader;
	
	// clear out definition-dependent UI in case we haven't downloaded the definition yet
	mHeader.enticeLabel.text = @"";
	
	[self setUIFromDefinition];
}

- (void)onSectionsCreated:(NSMutableArray*)sections
{
	OFTableSectionDescription* firstSection = [sections objectAtIndex:0];
	firstSection.title = @"Friends To Invite";
}

- (void)toggleSelectionOfUser:(OFUser*)user
{
	if (mSelectedUsers == nil)
	{
		mSelectedUsers = [NSMutableArray new];
	}
	
	NSIndexPath* indexPath = [self getFirstIndexPathForResource:user];
	if (!indexPath)
	{
		return;
	}
	OFSelectableUserCell* userCell = (OFSelectableUserCell*)[self.tableView cellForRowAtIndexPath:indexPath];
	OFUser* userInTable = (OFUser*)[self getResourceAtIndexPath:indexPath]; 
	if (userInTable)
	{
		if ([mSelectedUsers containsObject:userInTable])
		{
			userCell.checked = NO;
			[mSelectedUsers removeObject:userInTable];
		}
		else
		{
			userCell.checked = YES;
			[mSelectedUsers addObject:userInTable];
		}
	}
	
	[userCell setSelected:NO animated:YES];
}

- (void)onCell:(OFTableCellHelper*)cell resourceChanged:(OFResource*)user
{
	if ([cell isKindOfClass:[OFSelectableUserCell class]])
	{
		((OFSelectableUserCell*)cell).checked = [mSelectedUsers containsObject:user];
	}
}

- (void)onCellWasClicked:(OFResource*)cellResource indexPathInTable:(NSIndexPath*)indexPath
{
	if ([cellResource isKindOfClass:[OFUser class]])
	{
		UITableViewCell* cell = [self.tableView cellForRowAtIndexPath:indexPath];
		if ([cell isKindOfClass:[OFSelectableUserCell class]])
		{	
			[self toggleSelectionOfUser:(OFUser*)cellResource];
		}
	}
}

- (NSString*)getUserMessage:(UITextField*)textField
{
	return (textField.text && ![textField.text isEqualToString:@""]) ? textField.text : textField.placeholder;
}

- (IBAction)cancel
{
	[[OpenFeint getRootController] dismissModalViewControllerAnimated:YES];
}

- (IBAction)advance
{
	if (0 == [mSelectedUsers count])
	{
		[[[[UIAlertView alloc] initWithTitle:@"No Friends Selected" 
									 message:@"You must select at least one friend to invite." 
									delegate:nil 
						   cancelButtonTitle:@"OK" 
						   otherButtonTitles:nil] autorelease] show];
		
	}
	else
	{
		OFInviteFriendsController* controller = (OFInviteFriendsController*)OFControllerLoader::load(@"InviteFriends");
		controller.selectedUsers = mSelectedUsers;
		controller.definition = mDefinition;
		
		// This is kind of crappy.  Copy our close button's behavior (which is set by how we're opened)
		// and let the new controller know how to close.
		if (self.navigationItem.leftBarButtonItem)
		{
			SEL selector = self.navigationItem.leftBarButtonItem.action;
			id target = self.navigationItem.leftBarButtonItem.target;
			
			controller.closeInvocation = [NSInvocation invocationWithTarget:target andSelector:selector];
		}
		else
		{
			NSInvocation* invocation = nil;

			// do we have a parent?
			UIViewController* parent = [self.navigationController previousViewController:self];
			BOOL animated = YES;

			if (parent)
			{
				invocation = [NSInvocation invocationWithTarget:self.navigationController
													andSelector:@selector(popToViewController:animated:)
												   andArguments:&parent, &animated];
			}
			else
			{
				// If we have no parent, we should pop to root.
				invocation = [NSInvocation invocationWithTarget:self.navigationController
													andSelector:@selector(popToRootViewControllerAnimated:)
												   andArguments:&animated];
			}
			
			controller.closeInvocation = invocation;			
		}

		[self.navigationController pushViewController:controller animated:YES];
	}
}

- (void)viewWillDisappear:(BOOL)animated
{
	// Gotta make sure the loading screen clears
	definitionDownloadOutstanding = NO;
	friendsDownloadOutstanding = NO;
	anyFriendsDownloadOutstanding = NO;
	[super viewWillDisappear:animated];	
}

- (bool)canReceiveCallbacksNow
{
	return true;
}

- (BOOL)shouldShowNavBar
{
	return YES;
}

- (void) _refreshData
{
    if (stallNextRefresh)
    {
        [self showLoadingScreen];
        [NSTimer scheduledTimerWithTimeInterval:5.0 target:self selector:@selector(_refreshDataNow) userInfo:nil repeats:NO];
    }
    else
    {
        [self _refreshDataNow];
    }
}

- (void) _refreshDataNow
{
    [super _refreshData];
}

- (void)dealloc
{
	OFSafeRelease(mSelectedUsers);
	OFSafeRelease(mDefinition);
	[super dealloc];
}

+ (OFSelectFriendsToInviteController*)inviteControllerWithInviteIdentifier:(NSString*)_inviteIdentifier
{
	OFSelectFriendsToInviteController* controller = (OFSelectFriendsToInviteController*)OFControllerLoader::load(@"SelectFriendsToInvite");
	controller.title = @"Invite Friends";
	controller.inviteIdentifier = _inviteIdentifier;
	
	UIBarButtonItem* right = [[UIBarButtonItem alloc] initWithTitle:@"Invite" style:UIBarButtonItemStylePlain target:controller action:@selector(advance)];
	controller.navigationItem.rightBarButtonItem = right;
	[right release];
	
	return controller;
}

+ (OFSelectFriendsToInviteController*)inviteController
{
	return [self inviteControllerWithInviteIdentifier:nil];
}

- (void)openAsModal
{
	UIBarButtonItem* left = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:[OpenFeint class] action:@selector(dismissDashboard)];
	self.navigationItem.leftBarButtonItem = left;
	[left release];
	OFFramedNavigationController* navController = [[[OFFramedNavigationController alloc] initWithRootViewController:self] autorelease];

	[OpenFeint presentRootControllerWithModal:navController];
}

- (void)openAsModalInDashboard
{
	UIBarButtonItem* left = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemCancel target:self action:@selector(cancel)];
	self.navigationItem.leftBarButtonItem = left;
	[left release];
	OFFramedNavigationController* navController = [[[OFFramedNavigationController alloc] initWithRootViewController:self] autorelease];
	
	[[OpenFeint getRootController] presentModalViewController:navController animated:YES];
}

@end

