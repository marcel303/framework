#pragma once

#import "OFNotificationDelegate.h"

@interface SampleOFNotificationDelegate : NSObject< OFNotificationDelegate >

- (BOOL)isOpenFeintNotificationAllowed:(OFNotificationData*)notificationData;
- (void)handleDisallowedNotification:(OFNotificationData*)notificationData;
- (void)notificationWillShow:(OFNotificationData*)notificationData;

@end
