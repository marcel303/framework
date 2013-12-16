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

#import "OFSelectFriendsToInviteHeaderController.h"
#import "OFSelectFriendsToInviteController.h"
#import "OFDefaultTextField.h"
#import "OFImageView.h"
#import "OFImageLoader.h"
#import "OpenFeint+Settings.h"
#import "OpenFeint+UserOptions.h"

@implementation OFSelectFriendsToInviteHeaderController

@synthesize selectFriendsToInviteController, enticeLabel, inviteIcon, inviteIconFrame;

- (void)awakeFromNib
{
	[super awakeFromNib];

	self.inviteIconFrame.image = [self.inviteIconFrame.image stretchableImageWithLeftCapWidth:21.f topCapHeight:19.f];

	self.inviteIcon.unframed = YES;
	[self.inviteIcon setDefaultImage:[OFImageLoader loadImage:@"OFDefaultInviteIcon.png"]];
	self.inviteIcon.shouldScaleImageToFillRect = NO;
}

- (void)resizeView:(UIView*)parentView
{
#if 1
	UIView* lastElement = self.enticeLabel;
	CGPoint pericynthion = CGPointMake(0, lastElement.frame.origin.y + lastElement.frame.size.height);
	CGPoint perihelion = [lastElement.superview convertPoint:pericynthion toView:self.view];
	CGRect myRect = CGRectMake(0.0f, 0.0f, parentView.frame.size.width, perihelion.y + 10.f);
#else
	CGRect myRect = CGRectMake(0.0f, 0.0f, parentView.frame.size.width, self.view.frame.size.height);
#endif
	self.view.frame = myRect;
	[self.view layoutSubviews];
}

@end
