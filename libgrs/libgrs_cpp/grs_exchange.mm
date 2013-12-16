#include "grs_exchange.h"
#include "Log.h"

#ifdef IPHONEOS

@interface GrsConnection : NSURLConnection
{
	NSMutableData* data;
}

- (id)initWithRequest:(NSURLRequest*)request delegate:(id)target;

@property (assign) NSMutableData* data;

@end

@implementation GrsConnection

@synthesize data;

- (id)initWithRequest:(NSURLRequest*)request delegate:(id)target
{
	if ((self = [super initWithRequest:request delegate:target]))
	{
		data = [[NSMutableData alloc] init];
	}
	
	return self;
}

@end

@interface RequestHelper : NSObject
{
	CallBack callBack;
}

@property (assign) CallBack callBack;

- (id)initWithCallback:(CallBack)callBack;
- (void)performRequest:(NSURLRequest*)request;

@end

@implementation RequestHelper

@synthesize callBack;

- (id)initWithCallback:(CallBack)_callBack
{
	if ((self = [super init]))
	{
		callBack = _callBack;
	}
	
	return self;
}

- (void)performRequest:(NSURLRequest*)request
{
	[[GrsConnection alloc] initWithRequest:request delegate:self];
}

- (void)connection:(GrsConnection*)connection didReceiveResponse:(NSURLResponse*)response
{
	[connection.data setLength:0];
}

- (void)connection:(GrsConnection*)connection didReceiveData:(NSData*)data
{
	[connection.data appendData:data];
}

- (void)connection:(GrsConnection*)connection didFailWithError:(NSError*)error

{
	NSLog(@"Connection failed! Error - %@ %@",
		  [error localizedDescription],
		  [[error userInfo] objectForKey:NSURLErrorFailingURLStringErrorKey]);
	
	[connection release];
}

- (void)connectionDidFinishLoading:(GrsConnection*)connection

{
	LOG(LogLevel_Debug, "Succeeded! Received %d bytes of data", [connection.data length]);

	if (callBack.IsSet())
		callBack.Invoke(connection.data);
	
	LOG(LogLevel_Debug, "[DONE] Succeeded! Received %d bytes of data", [connection.data length]);
	
	[connection release];
}

@end

#endif

namespace GRS
{
	Exchange::Exchange()
	{
#ifdef IPHONEOS
		requestHelper = [[RequestHelper alloc] initWithCallback:CallBack(this, HandleResponse)];
#endif
	}
	
	Exchange::~Exchange()
	{
#ifdef IPHONEOS
		RequestHelper* helper = (RequestHelper*)requestHelper;
		
		[helper release];
#endif
	}
	
#ifdef IPHONEOS
	static NSMutableURLRequest* CreateRequest(const std::string& _url, const std::string& httpBody)
	{
		NSURL* url = [NSURL URLWithString:[NSString stringWithCString:_url.c_str() encoding:NSASCIIStringEncoding]];
		
		NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
		
		NSString* content = [NSString stringWithCString:httpBody.c_str() encoding:NSASCIIStringEncoding];
		
		[request setHTTPMethod:@"POST"];
		[request setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
		[request setValue:[NSString stringWithFormat:@"%d", [content length]] forHTTPHeaderField:@"Content-Length"];
		[request setHTTPBody:[content dataUsingEncoding:NSASCIIStringEncoding]];

		return request;
	}
#endif
	
	void Exchange::Post(const std::string& _url, const std::string& httpBody)
	{
#ifdef IPHONEOS
		NSURLRequest* request = CreateRequest(_url, httpBody);
		
		RequestHelper* helper = (RequestHelper*)requestHelper;
		
		[helper performRequest:request];
#endif
	}
	
	void Exchange::Post_Synchronized(const std::string& _url, const std::string& httpBody)
	{
#ifdef IPHONEOS
		NSURLRequest* request = CreateRequest(_url, httpBody);
		
		NSData* response = [NSURLConnection sendSynchronousRequest:request returningResponse:nil error:nil];
		
		HandleResponse(this, response);
		
//		RequestHelper* helper = (RequestHelper*)requestHelper;
		
//		[helper performRequest:request];
#endif
	}
	
	void Exchange::HandleResponse(void* obj, void* arg)
	{
#ifdef IPHONEOS
		Exchange* self = (Exchange*)obj;

//		RequestHelper* helper = (RequestHelper*)self->requestHelper;
		
		NSData* data = (NSData*)arg;
		
		if ([data length] == 0)
			return;
		
		NSString* responseText = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];

		if (responseText != nil)
		{
#ifdef DEBUG
			NSLog(@"%@", [@"responseText: " stringByAppendingString:responseText]);
#endif
			
			std::string responseText2 = [responseText cStringUsingEncoding:NSASCIIStringEncoding];
			
			[responseText release];
			
			if (self->OnResult.IsSet())
				self->OnResult.Invoke(&responseText2);
		}
#endif
	}
}
