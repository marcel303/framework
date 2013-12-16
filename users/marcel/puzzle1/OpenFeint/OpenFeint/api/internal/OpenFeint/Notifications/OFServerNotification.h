//
//  OFServerNotification.h
//  OpenFeint
//
//  Created by Ron Midthun on 3/1/10.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "OFNotification.h"
#import "OFResource.h"

//Note: If Text and Detail are filled out we will use a 2 line format in the notificaton
//      If Text is filled out and Detail is not, then we'll use a 1 line format in the notification

@interface OFServerNotification : OFResource {
	NSString *showDefaultNotification;
	NSString *text;
	NSString *detail;
    NSString *backgroundDefaultImage;
    NSString *backgroundImage;
    NSString *iconDefaultImage;
    NSString *iconImage;
    NSString *statusDefaultImage;
    NSString *statusImage;
	NSString *inputTab;
	NSString *inputControllerName;
    NSString *score;
}
@property (nonatomic, readonly) NSString *showDefaultNotification; //set to yes if you don't want to override what is the hard coded notification.
@property (nonatomic, readonly) NSString *text;
@property (nonatomic, readonly) NSString *detail;
@property (nonatomic, readonly) NSString *backgroundDefaultImage;
@property (nonatomic, readonly) NSString *backgroundImage;
@property (nonatomic, readonly) NSString *iconDefaultImage;
@property (nonatomic, readonly) NSString *iconImage;
@property (nonatomic, readonly) NSString *statusDefaultImage;
@property (nonatomic, readonly) NSString *statusImage;
@property (nonatomic, readonly) NSString *inputTab;
@property (nonatomic, readonly) NSString *inputControllerName;
@property (nonatomic, readonly) NSString *score;

+ (OFResourceDataMap*)getDataMap;
+ (NSString*)getResourceName;
+ (NSString*)getResourceDiscoveredNotification;
@end
