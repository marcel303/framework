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

#import "OFCloudStorageBlob.h"
#import "OFCloudStorageService.h"
#import "OFResourceDataMap.h"

#import "OFDependencies.h"
#import "OpenFeint.h"
#import "OpenFeint+Private.h"
#import "OpenFeint+UserOptions.h"
#import "OFService+Private.h"
#import "OFActionRequestType.h"
#import "OFNotificationData.h"
#import "OFBlobDownloadObserver.h"
#import "OFProvider.h"
#import "OFDelegateChained.h"
#import "MPOAuthAPIRequestLoader.h"
#import "OFHttpService.h"
#import "OFS3UploadParameters.h"
#import "OFS3Response.h"
#import "OFHttpNestedQueryStringWriter.h"

#import "MPOAuthURLResponse.h"
#import "OFCompressableData.h"

#ifndef OF_EXCLUDE_ZLIB
#import "zlib.h"
#endif

#define kCloudStorageBlobSizeMax	(256 * 1024)

OPENFEINT_DEFINE_SERVICE_INSTANCE(OFCloudStorageService)

@implementation OFCloudStorageService

OPENFEINT_DEFINE_SERVICE(OFCloudStorageService);

- (id) init
{
	self = [super init];
	
	if (self != nil)
	{
		mS3HttpService = new OFHttpService(@"", false);
		mUseCompression = YES;
		mVerboseCompression = NO;
        mUseLegacyHeaderlessCompression = NO;
		mStatusOk					= [[OFCloudStorageStatus_Ok						alloc] init];
		mStatusNotAcceptable		= [[OFCloudStorageStatus_NotAcceptable			alloc] init];
		mStatusNotFound				= [[OFCloudStorageStatus_NotFound				alloc] init];
		mStatusGatewayTimeout		= [[OFCloudStorageStatus_GatewayTimeout			alloc] init];
		mStatusInsufficientStorage	= [[OFCloudStorageStatus_InsufficientStorage	alloc] init];
	}
	
	return self;
}


- (OFCloudStorageStatus_Object*) getStatusObject_Ok
{
	return mStatusOk;
}


- (OFCloudStorageStatus_Object*) getStatusObject_NotAcceptable
{
	return mStatusNotAcceptable;
}


- (OFCloudStorageStatus_Object*) getStatusObject_NotFound
{
	return mStatusNotFound;
}


- (OFCloudStorageStatus_Object*) getStatusObject_GatewayTimeout
{
	return mStatusGatewayTimeout;
}


- (OFCloudStorageStatus_Object*) getStatusObject_InsufficientStorage
{
	return mStatusInsufficientStorage;
}


- (void) populateKnownResources:(OFResourceNameMap*)namedResources
{
	// I don't think we need CloudStorageBlob to be a full fledged resource yet.
	// Maybe we will if this service tries to get more sophisticated.
	// In the meantime we can pacify OFResource checks by registering it anyway.
	//
	namedResources->addResource([OFCloudStorageBlob getResourceName], [OFCloudStorageBlob class]);
}


+ (OFRequestHandle*) getIndexOnSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{	
	return [[self sharedInstance] 
	 getAction:[NSString stringWithFormat:@"cloud_stores.xml"]
	 withParameters:nil
	 withSuccess:onSuccess
	 withFailure:onFailure
	 withRequestType:OFActionRequestSilent
	 withNotice:nil];
}


- (void)onUploadFailure:(MPOAuthAPIRequestLoader*)loader nextCall:(OFDelegateChained*)nextCall
{
	OFCloudStorageStatus_Object *statusObject = [self getStatusObject_GatewayTimeout];
	
	do { // once through
		MPOAuthURLResponse *oauthResponse = loader.oauthResponse;
		if (! oauthResponse){
			break;
		}
		
		NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse*)oauthResponse.urlResponse;
		if (! urlResponse){
			break;
		}
		
		NSInteger statusCode = [urlResponse statusCode];
		
		switch(statusCode){
			case CSC_Ok:
				statusObject = [self getStatusObject_Ok];
				break;
			case CSC_InsufficientStorage:
				statusObject = [self getStatusObject_InsufficientStorage];
				break;
			case CSC_NotFound:
			default:
				statusObject = [self getStatusObject_NotFound];
				break;
		}
	} while(false); // once through
	
	[nextCall invokeWith:statusObject];
}


+ (OFRequestHandle*)uploadBlob:(NSData*) blob withKey:(NSString*) keyStr onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFRequestHandle* handle = nil;
	
	if ( blob ) {
        if ([[OFCloudStorageService sharedInstance] isCompressionEnabled])
        {
            blob = [OFCompressableData serializedDataFromData:blob];
        }
		NSUInteger blobLen = [blob length];
		
		// Enable the following line for diagnostic purposes.
		//OFLog(@"blob size: %i", (int)blobLen);
		
		if ( blobLen <= kCloudStorageBlobSizeMax ) {
			if ( [OFCloudStorageService keyIsValid: keyStr] ) {
				OFRetainedPtr<NSData> retainedData = blob;
				OFPointer<OFHttpNestedQueryStringWriter> params = new OFHttpNestedQueryStringWriter;
				
				params->io("key", keyStr);
				params->io("blob", retainedData);
				
				handle = [[self sharedInstance] 
				 postAction:@"/cloud_stores"
				 withParameters:params
				 withSuccess:onSuccess
				 withFailure:OFDelegate([self sharedInstance], @selector(onUploadFailure:nextCall:), onFailure)
				 withRequestType:OFActionRequestSilent // OFActionRequestForeground would require non-nil notice
				 withNotice:nil
				];
			}else{
				OFLog(@"Cloud storage key is not acceptable. Blob will not be uploaded. Key may only include characters, numbers, underscores and dashes.");
				onFailure.invoke([[self sharedInstance] getStatusObject_NotAcceptable]);
			}
		}else{
			OFLog(@"Cloud storage blob is too large. Blob will not be uploaded. Max size is 256k.");
			onFailure.invoke([[self sharedInstance] getStatusObject_InsufficientStorage]);
		}
	}else{
		onFailure.invoke([[self sharedInstance] getStatusObject_NotAcceptable]);
	}
	
	return handle;
}


+ (OFRequestHandle*)downloadBlobWithKey:(NSString*) keyStr onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFRequestHandle* handle = nil;
	
	if ( [OFCloudStorageService keyIsValid: keyStr] ) {
		NSString *actionStr = [NSString stringWithFormat:@"cloud_stores/%@.blob", keyStr];
		
		handle = [[OpenFeint provider] 
		 performAction:actionStr //@"cloud_stores/wa3.blob"
		 withParameters:nil
		 withHttpMethod:@"GET"
		 withSuccess:OFDelegate([self sharedInstance], @selector(onBlobDownloaded:nextCall:), onSuccess)
		 withFailure:OFDelegate([self sharedInstance], @selector(onDownloadFailure:nextCall:), onFailure)
		 withRequestType:OFActionRequestSilent
		 withNotice:nil
		 requiringAuthentication:YES
		];
	}else{
		onFailure.invoke([[self sharedInstance] getStatusObject_NotAcceptable]);
	}
	
	return handle;
}


- (void)onBlobDownloaded:(MPOAuthAPIRequestLoader*)loader nextCall:(OFDelegateChained*)nextCall
{
    if ([[OFCloudStorageService sharedInstance] isCompressionEnabled])
	{
        [nextCall invokeWith:[OFCompressableData uncompressedDataFromSerializedData:loader.data]];
    }
    else {
        [nextCall invokeWith:loader.data];
    }
}


- (void)onDownloadFailure:(MPOAuthAPIRequestLoader*)loader nextCall:(OFDelegateChained*)nextCall
{
	OFCloudStorageStatus_Object *statusObject = [self getStatusObject_GatewayTimeout];
	
	do { // once through
		MPOAuthURLResponse *oauthResponse = loader.oauthResponse;
		if (! oauthResponse){
			break;
		}
		
		NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse*)oauthResponse.urlResponse;
		if (! urlResponse){
			break;
		}
		
		NSInteger statusCode = [urlResponse statusCode];
		
		switch(statusCode){
			case CSC_Ok:
				statusObject = [self getStatusObject_Ok];
				break;
			case CSC_NotFound:
			default:
				statusObject = [self getStatusObject_NotFound];
				break;
		}
	} while(false); // once through
	
	[nextCall invokeWith:statusObject];
}


+ (BOOL)keyIsValid:(NSString*) keyStr{
	BOOL	validated = NO;
	int		keyLen = [keyStr length];
	int		idx;
	unichar	character;
	
	do { // once through
		if (keyLen <= 0) {
			break;
		}
		if (! [OFCloudStorageService charIsAlpha:[keyStr characterAtIndex:0]] ) {
			break;
		}
		for (idx = 1; idx < keyLen; idx++) {
			character = [keyStr characterAtIndex:idx];
			if (	! [OFCloudStorageService charIsAlpha:character]
				&&	! [OFCloudStorageService charIsNum:character]
				&&	! [OFCloudStorageService charIsPunctAllowedInKey:character]
			){
				break;
			}
		}
		if (idx < keyLen){
			break;
		}
		// Made it past all validation steps.
		validated = YES;
	} while (false); // once through
	
	return validated;
}


+ (BOOL)charIsAlpha:(unichar) character{
	return (	(0x0041 <= character)
			&&	(character <= 0x005A)
	)||(		(0x0061 <= character)
			&&	(character <= 0x007A)
	);
}


+ (BOOL)charIsNum:(unichar) character{
	return (	(0x0030 <= character)
			&&	(character <= 0x0039)
	);
}


+ (BOOL)charIsPunctAllowedInKey:(unichar) character{
	return (	(0x005F == character) // Underscore
			||	(0x002D == character) // Dash
		//	||	(0x002E == character) // Period (trouble?)
	);
}

+(OFRequestHandle*)downloadS3Blob:(NSString*)url onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	return [OFCloudStorageService downloadS3Blob:url passThroughUserData:nil onSuccess:onSuccess onFailure:onFailure];
}

+(OFRequestHandle*)downloadS3Blob:(NSString*)url passThroughUserData:(NSObject*)userData onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	OFHttpService* httpService = [[OFCloudStorageService sharedInstance] getS3HttpService];
	OFDelegate chainedSuccess([OFCloudStorageService sharedInstance], @selector(onS3BlobDownloaded:nextCall:), onSuccess);
	OFHttpRequest* httpRequest = httpService->startRequest(url, HttpMethodGet, nil, nil, nil, userData, nil, new OFBlobDownloadObserver(chainedSuccess, onFailure, true));
	return [OFRequestHandle requestHandle:httpRequest];
}

- (void)onS3BlobDownloaded:(OFS3Response*)response nextCall:(OFDelegateChained*)nextCall
{
	if ([[OFCloudStorageService sharedInstance] isCompressionEnabled])
	{
		response.data = [OFCloudStorageService uncompressBlob:response.data];
	}
	[nextCall invokeWith:response];
}


+(void)uploadS3Blob:(NSData*)blob withParameters:(OFS3UploadParameters*)parameters onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	[OFCloudStorageService uploadS3Blob:blob withParameters:parameters passThroughUserData:nil onSuccess:onSuccess onFailure:onFailure];
}

+(void)uploadS3Blob:(NSData*)blob withParameters:(OFS3UploadParameters*)parameters passThroughUserData:(NSObject*)userData onSuccess:(const OFDelegate&)onSuccess onFailure:(const OFDelegate&)onFailure
{
	NSString* blobUrl = parameters.action;
	if (!blobUrl || [blobUrl length] == 0)
	{
		OFLog(@"Trying to upload a blob without a upload url");
		return;
	}
	if (!blob)
	{
		OFLog(@"Trying to upload a nil blob");
		return;
	}
	
	if ([[OFCloudStorageService sharedInstance] isCompressionEnabled])
	{
		blob = [OFCloudStorageService compressBlob:blob];
	}
	OFHttpService* httpService = [[OFCloudStorageService sharedInstance] getS3HttpService];
	httpService->startRequest(blobUrl, HttpMethodPost, [parameters createS3HttpBodyForBlob:blob], nil, nil, userData, [OFS3UploadParameters getMultiPartBoundary], new OFBlobDownloadObserver(onSuccess, onFailure, true));
}

- (OFHttpService*)getS3HttpService
{
	return mS3HttpService.get();
}

+ (NSData*)compressBlob:(NSData*)blob
{
#ifdef OF_EXCLUDE_ZLIB
	return blob;
#else
    NSData *compressedBlob = nil;
    unsigned int originalBlobSize = [blob length];
    if (blob && originalBlobSize > 0) 
	{
        if(![[OFCloudStorageService sharedInstance] isUsingLegacyHeaderlessCompression]) {
            return [OFCompressableData serializedDataFromData:blob];
        }
		// bufferSize is pretty arbitrary. It really shouldn't double in size when compressing but if you try to compress 4 bytes then who knows
		const unsigned long bufferSize = originalBlobSize * 2 + 128; 
		unsigned char* destBuffer = new unsigned char[bufferSize];
		unsigned long destSize = bufferSize;
        int error = compress(destBuffer, &destSize, (const Bytef*)blob.bytes, originalBlobSize);
		if (error == Z_OK)
		{
			compressedBlob = [NSData dataWithBytes:destBuffer length:destSize];
		}
		else
		{
			OFLog(@"Failed to compress blob.");
		}
        delete destBuffer;
    }
	else
	{
		OFLog(@"Trying to compress a nil or empty blob");
	}
	
	if (compressedBlob && [[OFCloudStorageService sharedInstance] isVerboseCompressionEnabled])
	{
		const float ratio = (float)[compressedBlob length] / (float)originalBlobSize;
		const int percent = (int)(ratio * 100.f);
		//NSLog([NSString stringWithFormat:@"OpenFeint: BLOB compressed. Compressed size is %d percent of original size.", percent]);
		NSLog(@"OpenFeint: BLOB compressed. Compressed size is %d percent of original size.", percent);
	}
	
    return compressedBlob;
#endif
}

+ (NSData*)uncompressBlob:(NSData*)compressedBlob
{
#ifdef OF_EXCLUDE_ZLIB
	return compressedBlob;
#else
	if (!compressedBlob || [compressedBlob length] == 0)
	{
		OFLog(@"Trying to decompress a nil or empty blob");
		return nil;
	}
    if(![[OFCloudStorageService sharedInstance] isUsingLegacyHeaderlessCompression]) {
        return [OFCompressableData uncompressedDataFromSerializedData:compressedBlob];
    }
    
    z_stream zStream;
    zStream.next_in = (Bytef *)[compressedBlob bytes];
    zStream.avail_in = [compressedBlob length];
    zStream.total_out = 0;
    zStream.zalloc = Z_NULL;
    zStream.zfree = Z_NULL;
    zStream.opaque = Z_NULL;
	
    if (inflateInit(&zStream) != Z_OK) 
	{
		OFLog(@"Error decompressing blob");
        return nil;
    }
	
	const unsigned int compressedSize = [compressedBlob length];
	
	// Assuming 50% compression. Overestimating will decrease the likelyhood of having to increase the buffer size which is rather costly
    NSMutableData *decompressedBlob = [NSMutableData dataWithLength:compressedSize * 2];
	
	BOOL success = NO;
    while (!success) 
	{
        if (zStream.total_out >= [decompressedBlob length]) 
		{
            [decompressedBlob increaseLengthBy:compressedSize];
        }
		
		const char* startAddress = (const char*)decompressedBlob.bytes;
        zStream.next_out = (Bytef*)&startAddress[zStream.total_out];
        zStream.avail_out = [decompressedBlob length] - zStream.total_out;
		
        const int status = inflate(&zStream, Z_SYNC_FLUSH);
        if (status == Z_STREAM_END) 
		{
            success = YES;
        } 
		else if (status != Z_OK) 
		{
            break;
        }
    }
    
	if (inflateEnd (&zStream) != Z_OK) 
	{
		OFLog(@"Error decompressing blob");
        return nil;
    }
	
    if (success) 
	{
        [decompressedBlob setLength:zStream.total_out];
        return decompressedBlob;
    } 
	else 
	{
        return nil;
    }
#endif
}

- (void)disableCompression
{
	mUseCompression = NO;
}

- (BOOL)isCompressionEnabled
{
#ifdef OF_EXCLUDE_ZLIB
	return NO;
#else
	return mUseCompression;
#endif
}

- (void)enableVeboseCompression
{
	mVerboseCompression = YES;
}

- (BOOL)isVerboseCompressionEnabled
{
	return mVerboseCompression;
}

-(void)useLegacyHeaderlessCompression
{
    mUseLegacyHeaderlessCompression = YES;
}

-(BOOL)isUsingLegacyHeaderlessCompression
{
    return mUseLegacyHeaderlessCompression;
}

- (void)dealloc
{
	mS3HttpService->cancelAllRequests();
	
	[mStatusOk					release];
	[mStatusNotAcceptable		release];
	[mStatusNotFound			release];
	[mStatusGatewayTimeout		release];
	[mStatusInsufficientStorage	release];
	
	[super dealloc];
}

@end
