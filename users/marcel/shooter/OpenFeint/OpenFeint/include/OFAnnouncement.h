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

#import "OFForumThread.h"
#import "OFCallbackable.h"

@protocol OFAnnouncementDelegate;
@class OFRequestHandle;

//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFAnnouncement allows you to get all new announcements and see
/// information about them.
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFAnnouncement : OFForumThread <OFCallbackable>
{
	NSString* body;
	BOOL isImportant;
	BOOL isUnread;
	NSString* linkedClientApplicationId;
}

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFAnnouncement related actions. Must adopt the 
/// OFAnnouncementDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFAnnouncementDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
///  Get all announcements
///  
/// @return OFRequestHandle* if a server call must be done.  If the announcements are already cached
///			on the device, this will be null.
///
/// @note Invokes	- (void)didDownloadAnnouncementsAppAnnouncements:(NSArray*)appAnnouncements devAnnouncements:(NSArray*)devAnnouncements; on success and
///					- (void)didFailDownloadAnnouncements; on failure
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)downloadAnnouncements;

//////////////////////////////////////////////////////////////////////////////////////////
/// Gets Posts for an announcement
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note Invokes		- (void)didGetPosts:(NSArray*)posts OFAnnouncement:(OFAnnouncement*)announcement; on success and
///						- (void)didFailGetPostsOFAnnouncement:(OFAnnouncement*)announcement; on failure.
//////////////////////////////////////////////////////////////////////////////////////////
- (OFRequestHandle*)getPosts;

//////////////////////////////////////////////////////////////////////////////////////////
/// Body of the announcement as seen on the developer dashboard
//////////////////////////////////////////////////////////////////////////////////////////
@property (nonatomic, readonly) NSString* body;

//////////////////////////////////////////////////////////////////////////////////////////
/// inherited public properties
///
/// The title of the announcment
/// @property NSString* title;
///
// @property
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
- (NSComparisonResult)compareByDate:(OFAnnouncement*)announcement;
@property (nonatomic, readonly) BOOL isUnread;
@property (nonatomic, readonly) BOOL isImportant;
@property (nonatomic, readonly) NSString* linkedClientApplicationId;

@end


/////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFAnnouncementDelegate Protocol to receive information regarding OFAnnouncement.
/// You must call OFAnnouncement's +(void)setDelegate: method to receive information.
/////////////////////////////////////////////////////////////////////////////////////
@protocol OFAnnouncementDelegate
@optional

/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when downloadAnnouncements successfully completes.  This is called immediately
/// if we have the announcements already cached.
///
/// @param appAnnouncements		app announcements defined on the dev dashboard
/// @param devAnnouncements		dev announcements defined on the dev dashboard
/////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadAnnouncementsAppAnnouncements:(NSArray*)appAnnouncements devAnnouncements:(NSArray*)devAnnouncements;

/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when the downloadAnnouncements fails
/////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadAnnouncements;

/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getPosts successfully completes
///
/// @param posts		An array of the OFForumPosts for a particular announcment.
///						Each element of the array is of type (OFForumPost*).
/// @param announecment	The announcement which the posts are attached to.
/////////////////////////////////////////////////////////////////////////////////////
- (void)didGetPosts:(NSArray*)posts OFAnnouncement:(OFAnnouncement*)announcement;

/////////////////////////////////////////////////////////////////////////////////////
/// Invoked when getPosts fails
///
/// @param announcment	The announcement for which the posts were requested.
/////////////////////////////////////////////////////////////////////////////////////
- (void)didFailGetPostsOFAnnouncement:(OFAnnouncement*)announcement;

@end





