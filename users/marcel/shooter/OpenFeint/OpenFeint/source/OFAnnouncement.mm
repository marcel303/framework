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

#import "OFAnnouncement.h"
#import "OFAnnouncementService+Private.h"
#import "OFForumPost.h"
#import "OFResourceDataMap.h"
#import "OFPaginatedSeries.h"
#import "OpenFeint+UserOptions.h"
#import "OFForumService.h"
#import "OFTableSectionDescription.h"

static id sharedDelegate = nil;

@interface OFAnnouncement (Private)
	+ (void)_downloadAnnouncementsSuccess:(OFPaginatedSeries*)resources;
	+ (void)_downloadAnnouncementsFailure;
	- (void)_getPostsSuccess:(OFPaginatedSeries*)resources;
	- (void)_getPostsFailure;
	+ (void)_sendAnnouncementsSuccessFromPaginatedSeries:(OFPaginatedSeries*)resources;
@end

@implementation OFAnnouncement

@synthesize body, isImportant, isUnread, linkedClientApplicationId;

+ (void)setDelegate:(id<OFAnnouncementDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFAnnouncement class]];
	}
}

+ (OFRequestHandle*)downloadAnnouncements
{
	OFRequestHandle* handle = nil;
	
	handle = [OFAnnouncementService downloadAnnouncementsOnSuccess:OFDelegate(self, @selector(_downloadAnnouncementsSuccess:))
															 onFailure:OFDelegate(self, @selector(_downloadAnnouncementsFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFAnnouncement class]];
	return handle;
}

- (void)markAsRead
{
	NSDate* lastAnnouncementViewDate = [OpenFeint lastAnnouncementDateForLocalUser];
	if([lastAnnouncementViewDate laterDate:date] == lastAnnouncementViewDate)
	{
		[OpenFeint setLastAnnouncementDateForLocalUser:date];
	}
	isUnread = NO;
}

- (OFRequestHandle*)getPosts
{
	OFRequestHandle* handle = nil;
	handle = [OFForumService getPostsForThread:self.resourceId
										  page:1
									 onSuccess:OFDelegate(self, @selector(_getPostsSuccess:))
									 onFailure:OFDelegate(self, @selector(_getPostsFailure))];
	
	[OFRequestHandlesForModule addHandle:handle forModule:[OFAnnouncement class]];
	return handle;
}

+ (void)_downloadAnnouncementsSuccess:(OFPaginatedSeries*)resources
{
	[self _sendAnnouncementsSuccessFromPaginatedSeries:resources];
}

+ (void)_downloadAnnouncementsFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadAnnouncements)])
	{
		[sharedDelegate didFailDownloadAnnouncements];
	}
}

-(void) _getPostsSuccess:(OFPaginatedSeries*)resources
{
	NSArray* posts = [[[NSArray alloc] initWithArray:resources.objects] autorelease];
	
	if([posts count] > 0)
	{
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didGetPosts:OFAnnouncement:)])
		{
			if([posts count] > 1)
			{
				//Omit the first post, this is the "body"
				NSRange range;
				range.location = 1;
				range.length = [posts count] - 1;
				[sharedDelegate didGetPosts:[posts subarrayWithRange:range] OFAnnouncement:self];
			}
			else
			{
				//Otherwise, the replies are empty.
				[sharedDelegate didGetPosts:[[[NSArray alloc] init] autorelease] OFAnnouncement:self];
			}

		}
	}
	else
	{
		[self _getPostsFailure];
	}

}

-(void) _getPostsFailure
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailGetPostsOFAnnouncement:)])
	{
		[sharedDelegate didFailGetPostsOFAnnouncement:self];
	}
	
}

+ (void)_sendAnnouncementsSuccessFromPaginatedSeries:(OFPaginatedSeries*)resources
{
	NSArray* tableDesc = [[[NSArray alloc] initWithArray:resources.objects] autorelease];
	
	NSArray* announcements[2] = {nil, nil};
	
	for(uint i = 0; i < 2; i++)
	{
		OFTableSectionDescription* table = [tableDesc objectAtIndex:i];
		announcements[i] = [[[NSArray alloc] initWithArray:table.page.objects] autorelease];
	}
	
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadAnnouncementsAppAnnouncements:devAnnouncements:)])
	{
		[sharedDelegate didDownloadAnnouncementsAppAnnouncements:announcements[0] devAnnouncements:announcements[1]];
	}
}

#pragma mark Boilerplate

- (void)dealloc
{
	OFSafeRelease(body);
	OFSafeRelease(linkedClientApplicationId);
	[super dealloc];
}

#pragma mark Public Methods

- (NSComparisonResult)compareByDate:(OFAnnouncement*)announcement
{
	return [announcement.date compare:date];
}

#pragma mark XML Data Field Methods

- (void)setPost:(OFForumPost*)firstPost
{
	OFSafeRelease(body);
	body = [firstPost.body retain];
}

- (void)setLinkedClientApplicationId:(NSString*)value
{
	OFSafeRelease(linkedClientApplicationId);
	linkedClientApplicationId = [value retain];
}

- (void)setImportantFromString:(NSString*)value
{
	isImportant = [value boolValue];
}

- (NSString*)importantAsString
{
	return isImportant ? @"true" : @"false";
}

- (void)setUnreadFromString:(NSString*)value
{
	isUnread = [value boolValue];
}

- (NSString*)unreadAsString
{
	return isUnread ? @"true" : @"false";
}

#pragma mark OFResource

+ (OFService*)getService
{
	return [OFAnnouncementService sharedInstance];
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		(*dataMap) = (*[super getDataMap]);	// yea. that just happened.
		dataMap->addField(					@"important",						@selector(setImportantFromString:),			@selector(importantAsString));
		dataMap->addField(					@"unread",							@selector(setUnreadFromString:),			@selector(unreadAsString));
		dataMap->addNestedResourceField(	@"post",							@selector(setPost:),						nil, [OFForumPost class]);
		dataMap->addField(					@"linked_client_application_id",	@selector(setLinkedClientApplicationId:),	@selector(linkedClientApplicationId));
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"discussion";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return nil;
}

///////////////////////////////////////////////////////////////////
// OFCallbackable protocol
///////////////////////////////////////////////////////////////////

- (bool)canReceiveCallbacksNow
{
	return true;
}

+ (bool)canReceiveCallbacksNow
{
	return true;
}


@end
