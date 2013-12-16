////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2009 Aurora Feint, Inc.
/// 
///  Licensed under the Apache License, Version 2.0 (the "License");
///  you may not use this file except in compliance with the License.
///  You may obtain a copy of the License at
///  
///  	http://www.apache.org/licenses/LICENSE-2.0
///  	
///  Unless required by applicable law or agreed to in writing, software
///  distributed under the License is distributed on an "AS IS" BASIS,
///  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
///  See the License for the specific language governing permissions and
///  limitations under the License.
/// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#import "OFDependencies.h"
#import "OFS3UploadParameters.h"
#import "OFCloudStorageService.h"
#import "OFResourceDataMap.h"

@implementation OFS3UploadParameters

@synthesize action;

- (void)setKey:(NSString*)value
{
	if (key != value)
	{
		OFSafeRelease(key);
		key = [value retain];
	}
}

- (void)setAction:(NSString*)value
{
	if (action != value)
	{
		OFSafeRelease(action);
		action = [value retain];
	}
}

- (void)setAWSAccessKeyId:(NSString*)value
{
	if (AWSAccessKeyId != value)
	{
		OFSafeRelease(AWSAccessKeyId);
		AWSAccessKeyId = [value retain];
	}
}

- (void)setAcl:(NSString*)value
{
	if (acl != value)
	{
		OFSafeRelease(acl);
		acl = [value retain];
	}
}

- (void)setPolicy:(NSString*)value
{
	if (policy != value)
	{
		OFSafeRelease(policy);
		policy = [value retain];
	}
}

- (void)setSignature:(NSString*)value
{
	if (signature != value)
	{
		OFSafeRelease(signature);
		signature = [value retain];
	}
}

+ (OFService*)getService;
{
	return [OFCloudStorageService sharedInstance];
}

+ (OFResourceDataMap*)getDataMap
{
	static OFPointer<OFResourceDataMap> dataMap;
	
	if(dataMap.get() == NULL)
	{
		dataMap = new OFResourceDataMap;
		dataMap->addField(@"action", @selector(setAction:));
		dataMap->addField(@"key", @selector(setKey:));
		dataMap->addField(@"AWSAccessKeyId", @selector(setAWSAccessKeyId:));
		dataMap->addField(@"acl", @selector(setAcl:));
		dataMap->addField(@"policy", @selector(setPolicy:));
		dataMap->addField(@"signature", @selector(setSignature:));
	}
	
	return dataMap.get();
}

+ (NSString*)getResourceName
{
	return @"blob_upload_parameters";
}

+ (NSString*)getResourceDiscoveredNotification
{
	return @"s3_upload_parameters_discovered";
}

+ (NSString*)getMultiPartBoundary
{
	return @"8b1b0390bf21a96d7f9b6d9ef5d3438ae3556eff";
}

- (NSData*)getBoundaryMarkerStart
{
	return [[NSString stringWithFormat:@"--%@\r\n", [OFS3UploadParameters getMultiPartBoundary]] dataUsingEncoding:NSUTF8StringEncoding];
}

- (NSData*)getBoundaryMarkerEnd
{
	return [[NSString stringWithFormat:@"--%@--", [OFS3UploadParameters getMultiPartBoundary]] dataUsingEncoding:NSUTF8StringEncoding];
}

- (void) dealloc
{
	OFSafeRelease(action);
	OFSafeRelease(key);
	OFSafeRelease(AWSAccessKeyId);
	OFSafeRelease(acl);
	OFSafeRelease(policy);
	OFSafeRelease(signature);
	
	[super dealloc];
}

- (NSString*)getHeaderSpacer
{
	return @"\r\n";
}

- (void) addParamToData:(NSMutableData*)multipartData boundary:(NSData*)boundary paramName:(NSString*)paramName paramValue:(NSString*)paramValue
{
	const NSString* headerContentDisposition = [NSString stringWithFormat:@"Content-Disposition: form-data; name=\"%@\"\r\n", paramName];
	const NSString* headerSpacer = [self getHeaderSpacer];
	
	[multipartData appendData:boundary];
	[multipartData appendData:[headerContentDisposition dataUsingEncoding:NSUTF8StringEncoding]];
	[multipartData appendData:[headerSpacer dataUsingEncoding:NSUTF8StringEncoding]];	
	[multipartData appendData:[paramValue dataUsingEncoding:NSUTF8StringEncoding]];	
	[multipartData appendData:[headerSpacer dataUsingEncoding:NSUTF8StringEncoding]];	
}


- (NSData*)createS3HttpBodyForBlob:(NSData*)blob
{
	NSMutableData* multipartData = [[NSMutableData new] autorelease];
	const NSData* boundary = [self getBoundaryMarkerStart];
	const NSData* boundaryEnd = [self getBoundaryMarkerEnd];
	
	[self addParamToData:multipartData boundary:boundary paramName:@"key" paramValue:key];
	[self addParamToData:multipartData boundary:boundary paramName:@"AWSAccessKeyId" paramValue:AWSAccessKeyId];
	[self addParamToData:multipartData boundary:boundary paramName:@"acl" paramValue:acl];
	[self addParamToData:multipartData boundary:boundary paramName:@"policy" paramValue:policy];
	[self addParamToData:multipartData boundary:boundary paramName:@"signature" paramValue:signature];
	
	const NSString* headerContentDisposition = [NSString stringWithFormat:@"Content-Disposition: form-data; name=\"file\"; filename=\"test.blob\"\r\n"];
	const NSString* headerContentType = @"Content-Type: \"application/octet-stream\"\r\n";
	const NSString* headerSpacer = [self getHeaderSpacer];
	
	// The actual blob must come last
	[multipartData appendData:boundary];
	[multipartData appendData:[headerContentDisposition dataUsingEncoding:NSUTF8StringEncoding]];
	[multipartData appendData:[headerContentType dataUsingEncoding:NSUTF8StringEncoding]];
	[multipartData appendData:[headerSpacer dataUsingEncoding:NSUTF8StringEncoding]];
	[multipartData appendBytes:blob.bytes length:blob.length];
	[multipartData appendData:[headerSpacer dataUsingEncoding:NSUTF8StringEncoding]];	
	
	
	[multipartData appendData:boundaryEnd];

	return multipartData;
}

@end
