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

#import "OFCloudStorage.h"
#import "OFCloudStorageBlob.h"
#import "OFCloudStorageService.h"
#import "OFPaginatedSeries.h"

static id sharedDelegate = nil;

//////////////////////////////////////////////////////////////////////////////////////////
/// @internal
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFCloudStorage (Private)
+ (void)_uploadSuccess;
+ (void)_uploadFail:(id)status;
+ (void)_downloadSuccess:(NSData*)blob;
+ (void)_downloadFail:(id)status;
+ (void)_downloadKeysForCurrentUserSuccess:(OFPaginatedSeries*)loadedBlobs;
+ (void)_downloadKeysForCurrentUserFail;
@end

@implementation OFCloudStorage

+ (void)setDelegate:(id<OFCloudStorageDelegate>)delegate
{
	sharedDelegate = delegate;
	
	if(sharedDelegate == nil)
	{
		[OFRequestHandlesForModule cancelAllRequestsForModule:[OFCloudStorage class]];
	}
}

+ (OFRequestHandle*)upload:(NSData*)data withKey:(NSString*)key
{
	OFRequestHandle* handle = [OFCloudStorageService uploadBlob:data
														withKey:key
													  onSuccess:OFDelegate(self, @selector(_uploadSuccess))
													  onFailure:OFDelegate(self, @selector(_uploadFail:))];
	[OFRequestHandlesForModule addHandle:handle forModule:[OFCloudStorage class]];
	return handle;
}

+ (OFRequestHandle*)downloadKeysForCurrentUser
{
	OFRequestHandle* handle = [OFCloudStorageService getIndexOnSuccess:OFDelegate(self, @selector(_downloadKeysForCurrentUserSuccess:))
															 onFailure:OFDelegate(self, @selector(_downloadKeysForCurrentUserFail))];
	[OFRequestHandlesForModule addHandle:handle forModule:[OFCloudStorage class]];
	return handle;
}

+ (OFRequestHandle*)downloadDataWithKey:(NSString*)key
{
	OFRequestHandle* handle = [OFCloudStorageService downloadBlobWithKey:key
															   onSuccess:OFDelegate(self, @selector(_downloadSuccess:))
															   onFailure:OFDelegate(self, @selector(_downloadFail:))];
	[OFRequestHandlesForModule addHandle:handle forModule:[OFCloudStorage class]];
	return handle;
}

+ (void)_uploadSuccess
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didUpload)])
	{
		[sharedDelegate didUpload];
	}
}

+ (void)_uploadFail:(id)status
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailUpload:)])
	{
		OFCloudStorageStatus_Code statusCode = [status getStatusCode];
		[sharedDelegate didFailUpload:statusCode];
	}
}

+ (void)_downloadSuccess:(NSData*)blob
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadData:)])
	{
		[sharedDelegate didDownloadData:blob];
	}
}

+ (void)_downloadFail:(id)status
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadData:)])
	{
		OFCloudStorageStatus_Code statusCode = [status getStatusCode];
		[sharedDelegate didFailDownloadData:statusCode];
	}
}

+ (void)_downloadKeysForCurrentUserSuccess:(OFPaginatedSeries*)loadedBlobs
{
	NSArray* currentUsersDataBlobs = [[[NSArray alloc] initWithArray:loadedBlobs.objects] autorelease];
	
	if([currentUsersDataBlobs count] > 0)
	{
		if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didDownloadKeysForCurrentUser:)])
		{
			NSMutableArray* mutableKeys = [[[NSMutableArray alloc] initWithCapacity:[currentUsersDataBlobs count]] autorelease];
			for(uint i = 0; i < [currentUsersDataBlobs count]; i++)
			{
				OFCloudStorageBlob* blob = (OFCloudStorageBlob*)[currentUsersDataBlobs objectAtIndex:i];
				[mutableKeys addObject:blob.keyStr];
			}
			
			NSArray* keys = [[[NSArray alloc] initWithArray:mutableKeys] autorelease];
			[sharedDelegate didDownloadKeysForCurrentUser:keys];
		}
	}
	else
	{
		[self _downloadKeysForCurrentUserFail];
	}

}

+ (void)_downloadKeysForCurrentUserFail
{
	if(sharedDelegate && [sharedDelegate respondsToSelector:@selector(didFailDownloadKeysForCurrentUser)])
	{
		[sharedDelegate didFailDownloadKeysForCurrentUser];
	}
}

- (bool)canReceiveCallbacksNow
{
	return YES;
}

+ (bool)canReceiveCallbacksNow
{
	return YES;
}

@end

