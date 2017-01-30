#import "AppDelegate.h"
#import "ExceptionLoggerObjC.h"
#import "HTTPServer.h"
#import "localhostAddresses.h"
#import "Log.h"
#import "MyHTTPConnection.h"
#import "View_HttpServer.h"
#import "View_HttpServerMgr.h"

@implementation View_HttpServerMgr

@synthesize httpServer;

-(id)initWithApp:(AppDelegate*)_app;
{
	HandleExceptionObjcBegin();
	
	if (self = [super initWithNibName:@"HttpServerView_iPhone" bundle:[NSBundle mainBundle]])
	{
		[self setModalTransitionStyle:UIModalTransitionStyleCoverVertical];
		
		app = _app;
		
		[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handleAddressUpdate:) name:@"LocalhostAdressesResolved2" object:nil];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleBack:(id)sender
{
	HandleExceptionObjcBegin();
	
	[self dismissViewControllerAnimated:YES completion:NULL];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleAddressUpdate:(NSNotification*)notification
{
	HandleExceptionObjcBegin();
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();

	[self startServer];
	
	[self updateUi];
	
	[super viewWillAppear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[self stopServer];
	
	[super viewWillDisappear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleAddressUpdate
{
	HandleExceptionObjcBegin();
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)startServer
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("http server: start", 0);
	
	[errorString release];
	errorString = nil;
	
	// initialize HTTP server
	
	httpServer = [[HTTPServer alloc] init];
	[httpServer setType:@"_http._tcp."];
	[httpServer setConnectionClass:[MyHTTPConnection class]];
	NSString* root = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask,YES) objectAtIndex:0];
	[httpServer setDocumentRoot:[NSURL fileURLWithPath:root].absoluteString];
	[httpServer setPort:8080];
	[httpServer setName:@"Klodder"];
	
	// start HTTP server
	
	NSError* startError = nil;
	
	if (![httpServer start:&startError] ) 
		errorString = [[NSString stringWithFormat:@"%@", startError] retain];
	
	// update view
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)stopServer
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("http server: stop", 0);
	
	[httpServer stop];
	[httpServer release];
	httpServer = nil;
	
	HandleExceptionObjcEnd(false);
}

-(void)updateUi
{
	View_HttpServer* vw = (View_HttpServer*)self.view;
	[vw updateUi];
}

-(NSDictionary*)addresses
{
	return app.addresses;
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	
	[self stopServer];
	
	[errorString release];
	errorString = nil;
	
	[super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
