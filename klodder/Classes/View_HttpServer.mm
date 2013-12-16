#import "ExceptionLoggerObjC.h"
#import "HTTPServer.h"
#import "View_HttpServer.h"
#import "View_HttpServerMgr.h"

@implementation View_HttpServer

@synthesize lblAddressWifi;
@synthesize lblAddressWww;

-(void)updateUi
{
	HandleExceptionObjcBegin();
	
	if (controller.addresses == nil)
	{
		[lblAddressWifi setText:@"Searching.."];
		[lblAddressWww setText:@"Searching.."];
	}
	else
	{
		NSString* urlWifi;
		NSString* urlWww;
		
		UInt16 port = [controller.httpServer port];
		
		NSString* localIp = [controller.addresses objectForKey:@"en0"];
		
		if (!localIp)
		{
			localIp = [controller.addresses objectForKey:@"en1"];
		}

		if (!localIp)
			urlWifi = @"Wifi: No Connection!";
		else
			urlWifi = [NSString stringWithFormat:@"http://%@:%d", localIp, port];

		NSString* wwwIp = [controller.addresses objectForKey:@"www"];

		if (!wwwIp)
			urlWww = @"Web: Unable to determine external IP";
		else
			urlWww = [NSString stringWithFormat:@"Web: %@:%d", wwwIp, port];

		[lblAddressWifi setText:urlWifi];
		[lblAddressWww setText:urlWww];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
