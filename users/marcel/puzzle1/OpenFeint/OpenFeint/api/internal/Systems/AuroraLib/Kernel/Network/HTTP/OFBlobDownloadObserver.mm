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

#import "OFBlobDownloadObserver.h"
#import "OFS3Response.h"
#import "OFCompressableData.h"

OFBlobDownloadObserver::OFBlobDownloadObserver(OFDelegate const& onSuccess, OFDelegate const& onFailure, bool returnS3Response)
: mSuccessDelegate(onSuccess)
, mFailedDelegate(onFailure)
, mReturnS3Response(returnS3Response)
{
}


void OFBlobDownloadObserver::onFinishedDownloading(OFHttpServiceRequestContainer* info)
{
	int statusCode = info->getStatusCode();
	if (statusCode >= 200 && statusCode <= 299)
	{
		NSData* data = info->getData();
		if (mReturnS3Response)
		{
			OFS3Response * response = [[[OFS3Response alloc] initWithData:data andUserParam:info->getUserData() andStatusCode:statusCode] autorelease];
			mSuccessDelegate.invoke(response);
		}
		else
		{
			mSuccessDelegate.invoke([OFCompressableData uncompressedDataFromSerializedData:data]);
		}
	}
	else
	{
		OFLog(@"Request failed with status code: %d", info->getStatusCode());
		invokeFailure(info);
	}
}


void OFBlobDownloadObserver::onFailedDownloading(OFHttpServiceRequestContainer* info)
{
	invokeFailure(info);
}

void OFBlobDownloadObserver::invokeFailure(OFHttpServiceRequestContainer* info)
{
	if (mReturnS3Response)
	{
		int statusCode = info->getStatusCode();
		OFS3Response * response = [[[OFS3Response alloc] initWithData:nil andUserParam:info->getUserData() andStatusCode:statusCode] autorelease];
		mFailedDelegate.invoke(response);
	}
	else
	{
		mFailedDelegate.invoke();
	}
}
