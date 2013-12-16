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

#pragma once

#include "OFPointer.h"
#include "OFHttpRequest.h"
#include "OFHttpServiceObserver.h"

class OFHttpServiceRequestContainer : public OFSmartObject
{
public:
	OFHttpServiceRequestContainer(OFHttpRequest* request_, NSObject* userData, OFHttpServiceObserver* requestSpecificObserver_)
	: mRequest(request_)
	, mUrlPath(request_.urlPath)
	, mHttpMethod(request_.httpMethod)
	, mUserData(userData)
	, mRequestSpecificObserver(requestSpecificObserver_)
	{
	}
		
	void forceSetUrlPath(NSString* urlPath)					{ mUrlPath = urlPath; }
	void forceSetHttpMethod(NSString* httpMethod)			{ mHttpMethod = httpMethod; }
	void forceSetContentType(NSString* contentTypeToUse)	{ mContentType = contentTypeToUse; }
	void forceSetData(NSData* dataToUse)					{ mData = dataToUse; }

	NSObject* getUserData()									{ return mUserData.get(); }
	OFPointer<OFHttpServiceObserver> getObserver()			{ return mRequestSpecificObserver; }
	NSData* getData() const									{ return mData.get() ? mData.get() : mRequest.get().data; }
	NSString* getContentType() const						{ return mContentType.get() ? mContentType.get() : mRequest.get().contentType; }
	NSString* getContentDisposition() const					{ return mRequest.get().contentDisposition; }
	NSString* getUrlPath() const							{ return mUrlPath.get(); }
	NSString* getHttpMethod() const							{ return mHttpMethod.get(); }
	
	int getStatusCode() const								{ return [[mRequest.get() getResponse] statusCode]; }
	void cancelImmediately()								{ [mRequest.get() cancelImmediately]; }
	
	bool operator==(OFHttpRequest* rhv) const				{ return mRequest.get() == rhv; }

private:
	OFRetainedPtr<OFHttpRequest> mRequest;	
	OFRetainedPtr<NSString> mContentType;
	OFRetainedPtr<NSData> mData;
	OFRetainedPtr<NSString> mUrlPath;
	OFRetainedPtr<NSString> mHttpMethod;
	OFRetainedPtr<NSObject> mUserData;
	OFPointer<OFHttpServiceObserver> mRequestSpecificObserver;
};
