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

#pragma once

#import "OpenFeint.h"

@class OFUser;

/////////////////////////////////////////////////////
// Online/Offline Notification
//
// This notification is posted when the user goes online
extern NSString const* OFNSNotificationUserOnline;
// This notification is posted when the user goes offline
extern NSString const* OFNSNotificationUserOffline;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// User Changed Notification
//
// This notification is posted when the user changes
extern NSString const* OFNSNotificationUserChanged;
// These are the keys for the userInfo dictionary in the UserChanged notification
extern NSString const* OFNSNotificationInfoPreviousUser;
extern NSString const* OFNSNotificationInfoCurrentUser;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Unviewed Challenge Count Notification
//
// This notification is posted when the unviewed challenge count changes
extern NSString const* OFNSNotificationUnviewedChallengeCountChanged;
// These are the keys for the userInfo dictionary in the UnviewedChallengeCountChanged notification
extern NSString const* OFNSNotificationInfoUnviewedChallengeCount;
/////////////////////////////////////////////////////

//Presence Notifications
extern NSString const *OFNSNotificationFriendPresenceChanged;


/////////////////////////////////////////////////////
// Pending Friend Count Notification
//
// This notification is posted when the pending friend count changes
extern NSString const* OFNSNotificationPendingFriendCountChanged;
// These are the keys for the userInfo dictionary in the PendingFriendCountChanged notification
extern NSString const* OFNSNotificationInfoPendingFriendCount;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Add/Remove Friends Notification
extern NSString const* OFNSNotificationAddFriend;
extern NSString const* OFNSNotificationRemoveFriend;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Unread Announcement Notification
//
// This notification is posted when the unread announcement count changes
extern NSString const* OFNSNotificationUnreadAnnouncementCountChanged;
// These are the keys for the userInfo dictionary in the UnreadAnnouncementCountChanged notification
extern NSString const* OFNSNotificationInfoUnreadAnnouncementCount;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Unread Inbox Notification
//
// This notification is posted when the unread inbox count changes.
// NB: this will be called before the unread IM/Post/Invite notification,
// but one of them will be called as well.
extern NSString const* OFNSNotificationUnreadInboxCountChanged;
// These are the keys for the userInfo dictionary in the UnreadInboxCountChanged notification
extern NSString const* OFNSNotificationInfoUnreadInboxCount;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Unread IM Notification
//
// This notification is posted when the unread IM count changes
extern NSString const* OFNSNotificationUnreadIMCountChanged;
// These are the keys for the userInfo dictionary in the UnreadIMCountChanged notification
extern NSString const* OFNSNotificationInfoUnreadIMCount;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Unread Post Notification
//
// This notification is posted when the unread post count changes
extern NSString const* OFNSNotificationUnreadPostCountChanged;
// These are the keys for the userInfo dictionary in the UnreadPostCountChanged notification
extern NSString const* OFNSNotificationInfoUnreadPostCount;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Unread Invite Notification
//
// This notification is posted when the unread invite count changes
extern NSString const* OFNSNotificationUnreadInviteCountChanged;
// These are the keys for the userInfo dictionary in the UnreadInviteCountChanged notification
extern NSString const* OFNSNotificationInfoUnreadInviteCount;
/////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// Dashboard Orientation Changed Notification
//
// This notification is posted when the dashboard orientation changes while the dashboard is
// open.
extern NSString const* OFNSNotificationDashboardOrientationChanged;
// These are the keys for the userInfo dictionary in the DashboardOrientationChanged notification
extern NSString const* OFNSNotificationInfoOldOrientation;
extern NSString const* OFNSNotificationInfoNewOrientation;
/////////////////////////////////////////////////////

@interface OpenFeint (NSNotification)

+ (void)postUserChangedNotificationFromUser:(OFUser*)from toUser:(OFUser*)to;
+ (void)postUnviewedChallengeCountChangedTo:(NSUInteger)unviewedChallengeCount;
+ (void)postFriendPresenceChanged:(OFUser *)theUser withPresence:(NSString *)thePresence;
+ (void)postPendingFriendsCountChangedTo:(NSUInteger)pendingFriendCount;
+ (void)postAddFriend:(OFUser*)newFriend;
+ (void)postRemoveFriend:(OFUser*)oldFriend;
+ (void)postUnreadAnnouncementCountChangedTo:(NSUInteger)unreadAnnouncementCount;
+ (void)postUnreadInboxCountChangedTo:(NSUInteger)unreadInboxCount;
+ (void)postUnreadIMCountChangedTo:(NSUInteger)unreadIMCount;
+ (void)postUnreadPostCountChangedTo:(NSUInteger)unreadPostCount;
+ (void)postUnreadInviteCountChangedTo:(NSUInteger)unreadInviteCount;
+ (void)postDashboardOrientationChangedTo:(UIInterfaceOrientation)newOrientation from:(UIInterfaceOrientation)oldOrientation;

@end
