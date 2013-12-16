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

#import "OFService.h"
#import "OFHttpService.h"
#import "OFCloudStorageStatus.h"


@class OFS3UploadParameters;
@class OFRequestHandle;

@interface OFCloudStorageService : OFService
{
@private
	OFPointer<OFHttpService> mS3HttpService;
	BOOL mUseCompression;
	BOOL mVerboseCompression;
    BOOL mUseLegacyHeaderlessCompression;
	OFCloudStorageStatus_Ok						*mStatusOk;
	OFCloudStorageStatus_NotAcceptable			*mStatusNotAcceptable;
	OFCloudStorageStatus_NotFound				*mStatusNotFound;
	OFCloudStorageStatus_GatewayTimeout			*mStatusGatewayTimeout;
	OFCloudStorageStatus_InsufficientStorage	*mStatusInsufficientStorage;	
}

OPENFEINT_DECLARE_AS_SERVICE(OFCloudStorageService);

// Returns a list of all the OFCloudStorageBlobs for the local user and application. These contain the keys and url's for the blobs but the actual blobs must be downloaded separately
+ (OFRequestHandle*) getIndexOnSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

+ (OFRequestHandle*)uploadBlob:(NSData*) blob withKey:(NSString*) keyStr onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (OFRequestHandle*)downloadBlobWithKey:(NSString*) keyStr onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
+ (BOOL)keyIsValid:(NSString*) keyStr;
+ (BOOL)charIsAlpha:(unichar) character;
+ (BOOL)charIsNum:(unichar) character;
+ (BOOL)charIsPunctAllowedInKey:(unichar) character;

// The NSData will be passed directly to the success delegate
+(OFRequestHandle*)downloadS3Blob:(NSString*)url onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
// A OFS3Response will be passed to the success delegate
+(OFRequestHandle*)downloadS3Blob:(NSString*)url passThroughUserData:(NSObject*)userData onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

// The success delegate gets called without parameters
+(void)uploadS3Blob:(NSData*)blob withParameters:(OFS3UploadParameters*)parameters onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;
// The success delegate gets called with the passThroughUserData
+(void)uploadS3Blob:(NSData*)blob withParameters:(OFS3UploadParameters*)parameters passThroughUserData:(NSObject*)userData onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure;

- (OFHttpService*)getS3HttpService;

+ (NSData*)compressBlob:(NSData*)blob;
+ (NSData*)uncompressBlob:(NSData*)compressedBlob;

- (void)disableCompression;
- (BOOL)isCompressionEnabled;
- (void)enableVeboseCompression;
- (BOOL)isVerboseCompressionEnabled;
- (void)useLegacyHeaderlessCompression;
- (BOOL)isUsingLegacyHeaderlessCompression;

@end
