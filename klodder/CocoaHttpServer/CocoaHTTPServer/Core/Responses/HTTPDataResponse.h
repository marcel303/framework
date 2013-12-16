#import <Foundation/Foundation.h>
#import "HTTPResponse.h"


@interface HTTPDataResponse : NSObject <HTTPResponse>
{
	NSUInteger offset;
	NSData *data;
	
	@public
	
	NSDictionary* httpHeaders;
}

@property (nonatomic, retain) NSDictionary* httpHeaders;

- (id)initWithData:(NSData *)data;

@end
