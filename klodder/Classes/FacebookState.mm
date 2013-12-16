#import "Debugging.h"
#import "Deployment.h"
#import "ExceptionLoggerObjC.h"
#import "FacebookState.h"
#import "Log.h"

@implementation FacebookState

-(id)init
{
	HandleExceptionObjcBegin();
	
	if ((self = [super init]))
	{
		// initialize facebook
		facebookSession = [[FBSession sessionForApplication:NS(Deployment::FacebookKey) secret:NS(Deployment::FacebookSecret) delegate:self] retain];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	[loginDelegate release];
	loginDelegate = nil;
	
	[facebookSession release];
	
	[super dealloc];
	
	HandleExceptionObjcEnd(false);
}

-(void)resume
{
	[facebookSession resume];
}

-(bool)loggedIntoFacebook
{
	return facebookSession.isConnected;
}

-(void)logIntoFacebookWithDelegate:(id)delegate action:(SEL)action
{
	HandleExceptionObjcBegin();
	
	loginDelegate = delegate;
	[loginDelegate retain];
	loginAction = action;
	
	[[[[FBLoginDialog alloc] initWithSession:facebookSession] autorelease] show];
	
	HandleExceptionObjcEnd(false);
}

// ----------
// FBSessionDelegate
// ----------

-(void)session:(FBSession*)session didLogin:(FBUID)uid 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("FB session did login", 0);

/*	NSString* fql = [NSString stringWithFormat:@"select uid, name from user where uid == %lld", session.uid];

	NSDictionary* params = [NSDictionary dictionaryWithObject:fql forKey:@"query"];
	
	[[FBRequest requestWithDelegate:self] call:@"facebook.fql.query" params:params];*/
	
	[loginDelegate performSelector:loginAction];
	
	HandleExceptionObjcEnd(false);
}

- (void)sessionDidNotLogin:(FBSession*)session 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("FB session did not login", 0);
	
	HandleExceptionObjcEnd(false);
}

- (void)sessionDidLogout:(FBSession*)session 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("FB session did logout", 0);
	
	HandleExceptionObjcEnd(false);
}

// ----------
// FBRequestDelegate
// ----------

-(void)request:(FBRequest*)request didLoad:(id)result
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("FB request did load: %s", [request.method cStringUsingEncoding:NSASCIIStringEncoding]);
	
	if ([request.method isEqualToString:@"facebook.fql.query"])
	{
	}
	else if ([request.method isEqualToString:@"facebook.users.setStatus"])
	{
	}
	else if ([request.method isEqualToString:@"facebook.photos.upload"]) 
	{
//				NSDictionary* photoInfo = result;
//				NSString* pid = [photoInfo objectForKey:@"pid"];
		NSString* text = NS(Deployment::FacebookUploadSuccessText);
		[[[[UIAlertView alloc] initWithTitle:NS(Deployment::FacebookUploadSuccessTitle) message:text delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	}
	else
	{
		LOG_ERR("unknown request method in response: %s", [request.method cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)request:(FBRequest*)request didFailWithError:(NSError*)error
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("FB request did fail: %s", [request.method cStringUsingEncoding:NSASCIIStringEncoding]);
	
	NSString* text = [NSString stringWithFormat:@"Error: %@ (%d)", error.localizedDescription, error.code];
	
	LOG_ERR("reason: %s", [text cStringUsingEncoding:NSASCIIStringEncoding]);
	
	[[[[UIAlertView alloc] initWithTitle:@"Error" message:text delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
	
	HandleExceptionObjcEnd(false);
}

@end
