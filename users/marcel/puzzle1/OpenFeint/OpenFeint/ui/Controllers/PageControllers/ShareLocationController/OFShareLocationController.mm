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

#import "OFDependencies.h"
#import "OFShareLocationController.h"
#import "OFUserSettingService.h"
#import "OpenFeint+Private.h"
#import "OpenFeint+UserOptions.h"
#import "OFControllerLoader.h"
#import "OFPaginatedSeries.h"
#import "OFActionRequest.h"
#import "OFGameProfilePageInfo.h"
#import "OFFramedNavigationController.h"

@implementation OFShareLocationController

@synthesize descriptionLabel, shareLocationSwitch, delegate;

- (void) dealloc
{
	OFSafeRelease(descriptionLabel);
	OFSafeRelease(shareLocationSwitch);
	[super dealloc];
}

- (void)_reloadHostingView
{
	static int tries = 0;

	// attempt to wait until a location is determined, then tell our parent to reload its data.
    if ([OpenFeint userDistanceUnit] == kDistanceUnitNotDefined && tries++ < 15)
	{
		[self performSelector:@selector(_reloadHostingView) withObject:nil afterDelay:1];
	}
	else
	{
		[delegate userSharedLocation];
	}
}

- (IBAction)shareLocationChanged
{
    [(OFViewController*)delegate showLoadingScreen];
    
	if (shareLocationSwitch.on)
	{
		[OpenFeint setUserDistanceUnit:kDistanceUnitNotDefined];
		[OpenFeint startLocationManagerIfAllowed];
		
		// In three seconds, tell hosting table controller to reload data.
		[self performSelector:@selector(_reloadHostingView) withObject:nil afterDelay:3];
	}
	else
	{
		[OpenFeint setUserDistanceUnit:kDistanceUnitNotAllowed];
		[OpenFeint setUserLocation:nil];
	}
}

- (void)viewDidAppear:(BOOL)animated
{
	[super viewDidAppear:animated];
}


- (void)viewDidLoad
{
	[super viewDidLoad];
}

- (bool)canReceiveCallbacksNow
{
	return true;
}
@end
