//
//  OFInviteCompleteController.mm
//  OpenFeint
//
//  Created by Benjamin Morse on 2/19/10.
//  Copyright 2010 Aurora Feint. All rights reserved.
//

#import "OFInviteCompleteController.h"
#import "OFImageLoader.h"
#import "OFImageView.h"

@implementation OFInviteCompleteController

@synthesize inviteIcon, inviteIconFrame;

- (void)awakeFromNib
{
	[super awakeFromNib];
	
	self.inviteIconFrame.image = [self.inviteIconFrame.image stretchableImageWithLeftCapWidth:21.f topCapHeight:19.f];
	
	self.inviteIcon.unframed = YES;
	[self.inviteIcon setDefaultImage:[OFImageLoader loadImage:@"OFDefaultInviteIcon.png"]];
	self.inviteIcon.shouldScaleImageToFillRect = NO;
}

- (void)dealloc
{
	self.inviteIcon = nil;
	self.inviteIconFrame = nil;
	[super dealloc];
}

@end
