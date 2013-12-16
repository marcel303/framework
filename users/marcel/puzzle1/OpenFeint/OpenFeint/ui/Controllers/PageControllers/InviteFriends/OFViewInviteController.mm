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

#import "OFViewInviteController.h"
#import "OFControllerLoader.h"
#import "OpenFeint+UserOptions.h"
#import "OFWebViewController+Overridables.h"
#import "UINavigationController+OpenFeint.h"
#import "OFInvitationsController.h"

@implementation OFViewInviteController

@synthesize resourceId;

+ (OFViewInviteController*)viewInviteControllerWithInviteId:(NSString*)inviteResourceId
{
	OFViewInviteController* me = (OFViewInviteController*)OFControllerLoader::load(@"ViewInvite");
	me.resourceId = inviteResourceId;
	return me;
}

- (NSString*)getAction
{
	return [NSString stringWithFormat:@"client_webviews/invites/%@", resourceId];
}

- (BOOL)_webActionIgnoreInvite:(NSArray*)params
{
	// We get a parameter, id=<resourceId>, but we already have resourceId lying around,
	// so in lieu of parsing we'll just use that.
	UIViewController* previousViewController = [self.navigationController previousViewController:self];
	if ([previousViewController isKindOfClass:[OFInvitationsController class]])
	{
		// do something with resourceId
		[(OFInvitationsController*)previousViewController notifyInviteIgnored:self.resourceId];
	}
	[self.navigationController popViewControllerAnimated:YES];
	return YES;
}

- (NSDictionary*)getDispatchDictionary
{
	// CLOS method combinations would be super nice here
	NSDictionary* superDict = [super getDispatchDictionary];
	NSMutableDictionary* dict = [NSMutableDictionary dictionaryWithCapacity:superDict.count+1];
	[dict addEntriesFromDictionary:superDict];
	[dict setObject:@"_webActionIgnoreInvite:" forKey:@"ignoreInvite"];
	return dict;
}

- (NSString*)getTitle
{
	return @"Invitation";
}

- (void)dealloc
{	
	self.resourceId = nil;
	[super dealloc];
}

@end
