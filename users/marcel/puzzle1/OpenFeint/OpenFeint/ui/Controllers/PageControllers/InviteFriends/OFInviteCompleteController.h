//
//  OFInviteCompleteController.h
//  OpenFeint
//
//  Created by Benjamin Morse on 2/19/10.
//  Copyright 2010 Aurora Feint. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "OFShowMessageAndReturnController.h"

@class OFImageView;

@interface OFInviteCompleteController : OFShowMessageAndReturnController
{
	IBOutlet OFImageView* inviteIcon;
	IBOutlet UIImageView* inviteIconFrame;
}

@property (nonatomic, retain) OFImageView* inviteIcon;
@property (nonatomic, retain) UIImageView* inviteIconFrame;

@end
