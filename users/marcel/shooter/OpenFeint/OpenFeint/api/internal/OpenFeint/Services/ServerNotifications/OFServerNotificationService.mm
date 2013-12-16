//
//  OFServerNotificationService.mm
//  OpenFeint
//
//  Created by Ron Midthun on 3/1/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import "OFServerNotificationService.h"
#import "OFServerNotification.h"
#import "OFDependencies.h"

OPENFEINT_DEFINE_SERVICE_INSTANCE(OFServerNotificationService);

@implementation OFServerNotificationService

OPENFEINT_DEFINE_SERVICE(OFServerNotificationService);

- (id) init
{
	self = [super init];
	if (self != nil)
	{
	}
	return self;
}

- (void)dealloc
{
	[super dealloc];
}


- (void) populateKnownResources:(OFResourceNameMap*)namedResources
{
	namedResources->addResource([OFServerNotification getResourceName], [OFServerNotification class]);
}

@end
