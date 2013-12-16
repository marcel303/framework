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

#import "OFCallbackable.h"

@class OFRequestHandle;

@protocol OFCloudStorageDelegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// OFCloudStorageStatus_Code allows you to check codes passed back in failure delegates 
/// for OFCloudStorage.  These codes are loosely based on HTTP status codes 
/// [http://en.wikipedia.org/wiki/List_of_HTTP_status_codes].
//////////////////////////////////////////////////////////////////////////////////////////
enum OFCloudStorageStatus_Code 
{
	CSC_Unspecified			= 0, 	/// Status is not specified; This code is never expected in a reply.
	CSC_Ok					= 200,	/// Status is normal; no error.
	CSC_NotAcceptable		= 406,	/// Not acceptable; Request parameters may be may be malformed.
	CSC_NotFound			= 404,	/// Specified blob does not exist on server.
	CSC_GatewayTimeout		= 504,	/// Expected reply not received; network connection may have gone offline.
	CSC_InsufficientStorage	= 507,	/// Server unable to store given blob due to server capacity contstraints.
	
	// The following codes may be supported in the future.
	// CSC_BadRequest		= 400,	// The request is invalid.
	// CSC_Unauthorized		= 401,	//
	// CSC_RequestTimeout	= 408,	//
	
};

//////////////////////////////////////////////////////////////////////////////////////////
/// The public interface for OFCloudStorage allows you to upload and download data from
/// the Open Feint server.  Data is associated uniquely in 3 feilds, client application id,
/// current user id, and a key passed in by the developer.  This allows you to have static
/// keys for your data in your application across all users 
///	(ex. static const NSString* saveGameDataKey = @"SaveGameData")
//////////////////////////////////////////////////////////////////////////////////////////
@interface OFCloudStorage : NSObject<OFCallbackable>

//////////////////////////////////////////////////////////////////////////////////////////
/// Set a delegate for all OFCloudStorage related actions. Must adopt the 
/// OFCloudStorageDelegate protocol.
///
/// @note Defaults to nil. Weak reference
//////////////////////////////////////////////////////////////////////////////////////////
+ (void)setDelegate:(id<OFCloudStorageDelegate>)delegate;

//////////////////////////////////////////////////////////////////////////////////////////
/// Uploads the data to the OpenFeint Server and associates it with the key.
///
/// @param data		The data to upload to the server
/// @param key		The key to associate with the data
///
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note			Invokes - (void)didUpload on success and
///					- (void)didFailUpload on failure.
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)upload:(NSData*)data withKey:(NSString*)key;

//////////////////////////////////////////////////////////////////////////////////////////
/// Gets all cloud storage keys on the Open Feint server for this user.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note			Invokes - (void)didDownloadKeysForCurrentUser:(NSArray*)keys on success and
///					- (void)didFailDownloadForCurrentUser on failure
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)downloadKeysForCurrentUser;

//////////////////////////////////////////////////////////////////////////////////////////
/// Downloads the data associated with the key stored on the server and returns it to the
/// OFCloudStorageDelegate that is set.
///
/// @return OFRequestHandle for the server request.  Use this to cancel the request
///
/// @note			Invokes - (void)didDownloadData on success and
///					- (void)didFailDownloadData on failure.
//////////////////////////////////////////////////////////////////////////////////////////
+ (OFRequestHandle*)downloadDataWithKey:(NSString*)key;

@end


//////////////////////////////////////////////////////////////////////////////////////////
/// Adopt the OFCloudStorageDelegate Protocol to receive information regarding 
/// OFCloudStorage.  You must call OFCloudStorage's +(void)setDelegate: method to receive
/// information.
//////////////////////////////////////////////////////////////////////////////////////////
@protocol OFCloudStorageDelegate
@optional

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by OFCloudStorage when + (void)upload:(NSData*)data withKey:(NSString*)key
/// completes successfully.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didUpload;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by OFCloudStorage when + (void)upload:(NSData*)data withKey:(NSString*)key
/// fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailUpload:(OFCloudStorageStatus_Code)statusCode;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by OFCloudStorage when + (void)downloadDataWithKey:(NSString*)key
/// completes successfully.
///
/// @param data			The data retrieved from the key passed into downloadDataWithKey:
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadData:(NSData*)data;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by OFCloudStorage when + (void)downloadDataWithKey:(NSString*)key
/// fails.
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadData:(OFCloudStorageStatus_Code)statusCode;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by OFCloudStorage when + (void)downloadKeysForCurrentUser completes successfully
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didDownloadKeysForCurrentUser:(NSArray*)keys;

//////////////////////////////////////////////////////////////////////////////////////////
/// Invoked by OFCloudStorage when + (void)downloadKeysForCurrentUser fails
//////////////////////////////////////////////////////////////////////////////////////////
- (void)didFailDownloadKeysForCurrentUser;

@end
