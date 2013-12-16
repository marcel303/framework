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

#import "OFForumPost.h"
#import "OFForumService.h"
#import "OFUser.h"
#import "OFResourceDataMap.h"
#import "OFStringUtility.h"

#import "NSDateFormatter+OpenFeint.h"

@implementation OFForumPost

@synthesize author, body, date, discussionId, isDiscussionConversation;

#pragma mark Boilerplate

- (void)dealloc
{
	OFSafeRelease(author);
	OFSafeRelease(body);
	OFSafeRelease(date);
	self.discussionId = nil;
	
	[super dealloc];
}

#pragma mark XML Data Field Methods

- (void)setAuthor:(OFUser *)user
{
	OFSafeRelease(author);
	author = [user retain];
}

- (void)setBody:(NSString*)value
{
	OFSafeRelease(body);
	body = [value retain];
}

- (void)setDateFromXml:(NSString*)value
{
	OFSafeRelease(date);
	date = [[[NSDateFormatter railsFormatter] dateFromString:value] retain];
}

- (void)setIsDiscussionConversation:(NSString*)value
{
	isDiscussionConversation = [value boolValue];
}

- (NSString*)dateToXml
{
	return [[NSDateFormatter railsFormatter] stringFromDate:self.date];
}

#pragma mark OFResource

+ (OFService*)getService
{
	return [OFForumService sharedInstance];
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(						@"body",			@selector(setBody:),		@selector(body));
		dataMap->addNestedResourceField(		@"user",			@selector(setAuthor:),		@selector(author),		[OFUser class]);
		dataMap->addField(						@"created_at",		@selector(setDateFromXml:),	@selector(dateToXml));
		dataMap->addField(						@"discussion_id",	@selector(setDiscussionId:),@selector(discussionId));
		dataMap->addField(@"is_discussion_conversation", @selector(setIsDiscussionConversation:));

		
		// Ignored
		//user_id
		//updated_at
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"post";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"post_discovered";
}

@end
