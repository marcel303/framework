#import "Deployment.h"
#import "ExceptionLoggerObjC.h"
#import "FlickrState.h"
#import "Log.h"

static NSString* kStoredAuthTokenKeyName = @"FlickrAuthToken";
static NSString* kGetAuthTokenStep = @"kGetAuthTokenStep";
static NSString* kCheckTokenStep = @"kCheckTokenStep";
//static NSString* kGetUserInfoStep = @"kGetUserInfoStep";
static NSString* kSetImagePropertiesStep = @"kSetImagePropertiesStep";
static NSString* kUploadImageStep = @"kUploadImageStep";

@implementation FlickrState

@synthesize flickrRequest;
@synthesize flickrContext;

-(id)init
{
	HandleExceptionObjcBegin();
	
	if ((self = [super init]))
	{
		// initialize flickr
		flickrContext = [[OFFlickrAPIContext alloc] initWithAPIKey:NS(Deployment::FlickrKey) sharedSecret:NS(Deployment::FlickrSecret)];
		flickrRequest = [[OFFlickrAPIRequest alloc] initWithAPIContext:flickrContext];
		[flickrRequest setDelegate:self];
		flickrIsConnected = false;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	[flickrRequest release];
	flickrRequest = nil;
    [flickrContext release];
	flickrContext = nil;
	
	[super dealloc];
	
	HandleExceptionObjcEnd(false);
}

-(void)resume
{
	[self flickrResume];
	[self flickrCheckLogin];
}

-(bool)loggedIntoFlickr
{
	return flickrIsConnected;
}

-(void)logIntoFlickr
{
	NSURL* loginUrl = [flickrContext loginURLFromFrobDictionary:nil requestedPermission:OFFlickrWritePermission];
	
	[[UIApplication sharedApplication] openURL:loginUrl];
}

// ObjectiveFlickr support

- (void)setAndSaveFlickrAuthToken:(NSString*)authToken
{
	LOG_DBG("Flickr: set and save auth", 0);
	
	if (![authToken length]) 
	{
		flickrContext.authToken = nil;
		[[NSUserDefaults standardUserDefaults] removeObjectForKey:kStoredAuthTokenKeyName];
	}
	else 
	{
		flickrContext.authToken = authToken;
		[[NSUserDefaults standardUserDefaults] setObject:authToken forKey:kStoredAuthTokenKeyName];
	}
}

-(void)handleOpenUrl:(NSURL*)url
{
	HandleExceptionObjcBegin();
	
	// query has the form of "&frob=", the rest is the frob
	
	NSString* frob = [[url query] substringFromIndex:6];
	
	LOG_DBG("frob: %s", [frob cStringUsingEncoding:NSASCIIStringEncoding]);
	
	flickrRequest.sessionInfo = kGetAuthTokenStep;
	[flickrRequest callAPIMethodWithGET:@"flickr.auth.getToken" arguments:[NSDictionary dictionaryWithObjectsAndKeys:frob, @"frob", nil]];
	
	HandleExceptionObjcEnd(false);
}

-(void)flickrResume
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("Flickr: resume", 0);
	
	NSString* authToken;
	
	if (authToken = [[NSUserDefaults standardUserDefaults] objectForKey:kStoredAuthTokenKeyName]) 
	{
		LOG_DBG("Flickr: found stored auth token", 0);
		
		flickrContext.authToken = authToken;
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)flickrCheckLogin
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("Flickr: check login", 0);
	
	if (flickrRequest.sessionInfo == nil)
	{
		LOG_DBG("Flickr: sessionInfo is nil", 0);
		
		if ([flickrContext.authToken length]) 
		{
			LOG_DBG("Flickr: checkToken", 0);
			
			flickrRequest.sessionInfo = kCheckTokenStep;
			[flickrRequest callAPIMethodWithGET:@"flickr.auth.checkToken" arguments:nil];
		}
		else
		{
			LOG_DBG("Flickr: authToken is empty", 0);
		}
	}
	
	HandleExceptionObjcEnd(false);
}

// ----------
// OFFlickrAPIRequestDelegate
// ----------

-(void)flickrAPIRequest:(OFFlickrAPIRequest *)inRequest didCompleteWithResponse:(NSDictionary *)inResponseDictionary
{
	HandleExceptionObjcBegin();
	
	NSLog(@"Flickr: did complete: %s %@ %@", __PRETTY_FUNCTION__, inRequest.sessionInfo, inResponseDictionary);
		
	if (inRequest.sessionInfo == kGetAuthTokenStep) 
	{
		flickrIsConnected = true;
		[self setAndSaveFlickrAuthToken:[[inResponseDictionary valueForKeyPath:@"auth.token"] textContent]];
		//self.flickrUserName = [inResponseDictionary valueForKeyPath:@"auth.user.username"];
	}
	else if (inRequest.sessionInfo == kCheckTokenStep) 
	{
		flickrIsConnected = true;
		//self.flickrUserName = [inResponseDictionary valueForKeyPath:@"auth.user.username"];
	}
	else if (inRequest.sessionInfo == kUploadImageStep) 
	{
#ifdef DEBUG
		NSLog(@"%@", inResponseDictionary);
#endif
		
		NSString* photoID = [[inResponseDictionary valueForKeyPath:@"photoid"] textContent];

		flickrRequest.sessionInfo = kSetImagePropertiesStep;
		[flickrRequest callAPIMethodWithPOST:@"flickr.photos.setMeta" arguments:[NSDictionary dictionaryWithObjectsAndKeys:photoID, @"photo_id", @"Snap and Run", @"title", @"Uploaded from my iPhone/iPod Touch", @"description", nil]];        		        
	}
	else if (inRequest.sessionInfo == kSetImagePropertiesStep) 
	{
		[UIApplication sharedApplication].idleTimerDisabled = NO;		
	}
	else
	{
		LOG_WRN("unknown flickr session state", 0);
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)flickrAPIRequest:(OFFlickrAPIRequest *)inRequest didFailWithError:(NSError *)inError
{
	HandleExceptionObjcBegin();
	
	NSLog(@"Flickr did fail: %s %@ %@", __PRETTY_FUNCTION__, inRequest.sessionInfo, inError);
	
	if (inRequest.sessionInfo == kGetAuthTokenStep) 
	{
		flickrIsConnected = false;
	}
	else if (inRequest.sessionInfo == kCheckTokenStep) 
	{
		flickrIsConnected = false;
		[self setAndSaveFlickrAuthToken:nil];
	}
	else if (inRequest.sessionInfo == kUploadImageStep) 
	{
		[UIApplication sharedApplication].idleTimerDisabled = NO;

		[[[[UIAlertView alloc] initWithTitle:@"API Failed" message:[inError description] delegate:nil cancelButtonTitle:@"Dismiss" otherButtonTitles:nil] autorelease] show];
	}
	else
	{
		LOG_WRN("unknown flickr session state", 0);
		
		[[[[UIAlertView alloc] initWithTitle:@"API Failed" message:[inError description] delegate:nil cancelButtonTitle:@"Dismiss" otherButtonTitles:nil] autorelease] show];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)flickrAPIRequest:(OFFlickrAPIRequest *)inRequest imageUploadSentBytes:(NSUInteger)inSentBytes totalBytes:(NSUInteger)inTotalBytes
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("Flickr: sent bytes", 0);
	
	if (inSentBytes == inTotalBytes) 
	{
//		snapPictureDescriptionLabel.text = @"Waiting for Flickr...";
	}
	else 
	{
//		snapPictureDescriptionLabel.text = [NSString stringWithFormat:@"%lu/%lu (KB)", inSentBytes / 1024, inTotalBytes / 1024];
	}
	
	HandleExceptionObjcEnd(false);
}

@end
