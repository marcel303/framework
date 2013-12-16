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

#import "OFInviteFriendsHeaderController.h"
#import "OFInviteFriendsController.h"
#import "OFDefaultTextField.h"
#import "OFImageView.h"
#import "OpenFeint+Settings.h"
#import "OpenFeint+UserOptions.h"
#import "OFInviteDefinition.h"

@implementation OFInviteFriendsHeaderController

@synthesize inviteFriendsController,titleLabel,inviteText;

- (void)awakeFromNib
{
	[super awakeFromNib];
	self.inviteText.font = [UIFont systemFontOfSize:11.0f];
	self.inviteText.textColor = [UIColor colorWithRed:89.0/255.0 green:89.0/255.0 blue:89.0/255.0 alpha:1.0];
	self.inviteText.delegate = self;
	inviteTextHasDefaultText = YES;
}


- (BOOL)textViewShouldBeginEditing:(UITextView *)textView
{
	// maybe scroll to visible.
	return YES;
}

- (BOOL)textViewShouldEndEditing:(UITextView *)textView
{
	return YES;
}

- (void)textViewDidBeginEditing:(UITextView *)textView
{
	if (inviteTextHasDefaultText)
	{
		inviteText.text = @"";
	}
}

- (void)textViewDidEndEditing:(UITextView *)textView
{
	NSString* suggested = inviteFriendsController.definition.suggestedSenderMessage;
	if ([inviteText.text isEqualToString:@""] || [inviteText.text isEqualToString:suggested])
	{
		inviteTextHasDefaultText = YES;
		inviteText.text = inviteFriendsController.definition.suggestedSenderMessage;
	}
	else
	{
		inviteTextHasDefaultText = NO;
	}
}

- (void)resizeView:(UIView*)parentView
{
	UIView* lastElement = self.inviteText;
	CGPoint pericynthion = CGPointMake(0, lastElement.frame.origin.y + lastElement.frame.size.height);
	CGPoint perihelion = [lastElement.superview convertPoint:pericynthion toView:self.view];
	CGRect myRect = CGRectMake(0.0f, 0.0f, parentView.frame.size.width, perihelion.y + 10.f);

	self.view.frame = myRect;
	[self.view layoutSubviews];
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self.inviteText resignFirstResponder];
}

@end
