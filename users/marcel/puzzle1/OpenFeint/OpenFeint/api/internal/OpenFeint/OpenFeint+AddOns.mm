////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2009-2010 Aurora Feint, Inc.
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

#import "OpenFeint+AddOns.h"

#define FOR_EACH_ADDON_DO(statement) \
	for (NSValue* klassVal in sAddOnClasses) \
	{  \
		Class<OpenFeintAddOn> addon = (Class<OpenFeintAddOn>)[klassVal pointerValue]; \
		statement; \
	}

static NSMutableArray* sAddOnClasses = nil;

@implementation OpenFeint (AddOns)

+ (void)registerAddOn:(Class<OpenFeintAddOn>)klass
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	
	if (sAddOnClasses == nil)
	{
		sAddOnClasses = [[NSMutableArray alloc] initWithCapacity:1];
	}
	
	[sAddOnClasses addObject:[NSValue valueWithPointer:(void const*)klass]];
	[pool release];
}		
		
+ (void)initializeAddOns:(NSDictionary*)settings
{
	FOR_EACH_ADDON_DO([addon initializeAddOn:settings])
}

+ (void)shutdownAddOns
{
	FOR_EACH_ADDON_DO([addon shutdownAddOn])
}

+ (void)setDefaultAddOnSettings:(OFSettings*) settings {
	FOR_EACH_ADDON_DO([addon defaultSettings:settings])
}

+ (void)loadAddOnSettings:(OFSettings*) settings fromReader:(OFXmlReader&) reader {
	FOR_EACH_ADDON_DO([addon loadSettings:settings fromReader:reader])
}


+ (BOOL)allowAddOnsToRespondToPushNotification:(NSDictionary*)notificationInfo duringApplicationLaunch:(BOOL)duringApplicationLaunch
{
	BOOL anyResponded = false;
	FOR_EACH_ADDON_DO(anyResponded = anyResponded || [addon respondToPushNotification:notificationInfo duringApplicationLaunch:duringApplicationLaunch])
	return anyResponded;
}

+ (void)notifyAddOnsUserLoggedIn
{
	FOR_EACH_ADDON_DO([addon userLoggedIn])
}

+ (void)notifyAddOnsUserLoggedOut
{
	FOR_EACH_ADDON_DO([addon userLoggedOut])
}

@end
