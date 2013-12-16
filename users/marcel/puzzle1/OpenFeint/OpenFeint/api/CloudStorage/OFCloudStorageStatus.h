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

#import <Foundation/NSObject.h>
#import "OFCloudStorage.h"

@interface OFCloudStorageStatus_Object : NSObject
{
}
- (OFCloudStorageStatus_Code) getStatusCode;
- (BOOL) isAnyError;
- (BOOL) isNetworkError;
@end


@interface OFCloudStorageStatus_Ok : OFCloudStorageStatus_Object
{
}
- (OFCloudStorageStatus_Code) getStatusCode;
@end


@interface OFCloudStorageStatus_NotAcceptable : OFCloudStorageStatus_Object
{
}
- (OFCloudStorageStatus_Code) getStatusCode;
@end


@interface OFCloudStorageStatus_NotFound : OFCloudStorageStatus_Object
{
}
- (OFCloudStorageStatus_Code) getStatusCode;
@end


@interface OFCloudStorageStatus_GatewayTimeout : OFCloudStorageStatus_Object
{
}
- (OFCloudStorageStatus_Code) getStatusCode;
@end


@interface OFCloudStorageStatus_InsufficientStorage : OFCloudStorageStatus_Object
{
}
- (OFCloudStorageStatus_Code) getStatusCode;
@end

