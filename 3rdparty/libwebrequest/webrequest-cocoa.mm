#import "webrequest.h"
#import <atomic>
#import <Cocoa/Cocoa.h>
#import <Foundation/Foundation.h>

struct WebRequestCocoa : WebRequest
{
	NSURLSessionDataTask * task = nullptr;
	
	std::atomic<bool> failure;
	std::atomic<bool> done;
	
	NSData * resultData = nullptr;
	
	WebRequestCocoa(const char * in_url)
		: failure(false)
		, done(false)
	{
		NSURL * url = [NSURL URLWithString:[NSString stringWithCString:in_url encoding:NSASCIIStringEncoding]];
 		NSURLRequest * request = [NSURLRequest requestWithURL:url];

		NSURLSession * session = [NSURLSession sharedSession];

		task = [session dataTaskWithRequest:request completionHandler:
			^(NSData * data, NSURLResponse * response, NSError * error)
			{
				if (error != nullptr)
				{
					failure = true;
				}
				else
				{
					if ([response isKindOfClass:[NSHTTPURLResponse class]] == false)
						failure = true;
					else
					{
						NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse*)response;
						
						if (httpResponse.statusCode < 200 || httpResponse.statusCode >= 300)
							failure = true;
						else
						{
							resultData = data;
							[resultData retain];
						}
					}
				}
				
				done = true;
     		}];

		[task retain];
		[task resume];
	}

	virtual ~WebRequestCocoa() override
	{
		if (resultData != nullptr)
		{
			[resultData release];
			resultData = nullptr;
		}
		
		if (task != nullptr)
		{
			[task release];
			task = nullptr;
		}
	}

	float getProgress() override
	{
		return task.progress.fractionCompleted;
	}

    virtual bool isDone() override
    {
    	return done;
    }

    virtual bool isSuccess() override
    {
    	return failure == false;
    }

    virtual bool getResultAsData(uint8_t *& bytes, size_t & numBytes) override
    {
    	bytes = nullptr;
    	numBytes = 0;
		
		if (done == false || failure)
    		return false;
		else
		{
			numBytes = [resultData length];
			bytes = new uint8_t[numBytes];
			
			[resultData getBytes:bytes length:numBytes];
			
			return true;
		}
    }

    virtual bool getResultAsCString(char *& result) override
    {
    	if (done == false || failure)
    		return false;
		else
    	{
    		NSString * resultString = [[NSString alloc] initWithData:resultData encoding:NSUTF8StringEncoding];

			const char * str = [resultString cStringUsingEncoding:NSUTF8StringEncoding];
			
			if (str == nullptr)
				return false;
			else
			{
				const size_t length = strlen(str);
				
				result = new char[length + 1];
				
				memcpy(result, str, length + 1);
				
				return true;
			}
		}
    }
};

WebRequest * createWebRequest(const char * url)
{
	return new WebRequestCocoa(url);
}

