//
//  SampleInviteNotificationController.mm
//  OpenFeint
//
//  Created by Benjamin Morse on 2/5/10.
//  Copyright 2010 Aurora Feint. All rights reserved.
//

#import "SampleInviteNotificationController.h"

@implementation SampleInviteNotificationController

@synthesize message;

- (void)dealloc
{
	self.message = nil;
	[super dealloc];
}

@end
