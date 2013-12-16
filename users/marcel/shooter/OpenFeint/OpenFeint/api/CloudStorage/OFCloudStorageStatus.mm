////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// 
///  Copyright 2010 Aurora Feint, Inc.
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

#import "OFCloudStorageStatus.h"


@implementation OFCloudStorageStatus_Object


- (OFCloudStorageStatus_Code) getStatusCode
{
	return CSC_Unspecified;
}


- (BOOL) isAnyError
{
	return (CSC_Ok != [self getStatusCode]);
}


- (BOOL) isNetworkError
{
	return (CSC_GatewayTimeout == [self getStatusCode]);
}


@end


@implementation OFCloudStorageStatus_Ok
- (OFCloudStorageStatus_Code) getStatusCode
{
	return CSC_Ok;
}
@end


@implementation OFCloudStorageStatus_NotAcceptable
- (OFCloudStorageStatus_Code) getStatusCode
{
	return CSC_NotAcceptable;
}
@end


@implementation OFCloudStorageStatus_NotFound
- (OFCloudStorageStatus_Code) getStatusCode
{
	return CSC_NotFound;
}
@end


@implementation OFCloudStorageStatus_GatewayTimeout
- (OFCloudStorageStatus_Code) getStatusCode
{
	return CSC_GatewayTimeout;
}
@end


@implementation OFCloudStorageStatus_InsufficientStorage
- (OFCloudStorageStatus_Code) getStatusCode
{
	return CSC_InsufficientStorage;
}
@end
