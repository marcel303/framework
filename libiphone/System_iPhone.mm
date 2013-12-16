#import <AudioToolbox/AudioToolbox.h>
#import <UIKit/UIKit.h>
#import <SystemConfiguration/SCNetworkReachability.h>
#import "Exception.h"
#import "Log.h"
#import "Screenshot.h"
#import "System_iPhone.h"

#define PING_ADDRESS "http://grannies-games.com/ping.php"

static bool s_ApplicationHackChecked = false;
static bool s_ApplicationIsHacked = false;
static bool s_HasConnectivityIsChecked = false;
static bool s_HasConnectivity = true;

void System_iPhone::Vibrate()
{
	AudioServicesPlayAlertSound(kSystemSoundID_Vibrate);
}

//

@interface ConnectionChecker : NSURLConnection
{
}

- (id)initWithRequest:(NSURLRequest*)request;

@end

@implementation ConnectionChecker

- (id)initWithRequest:(NSURLRequest*)request
{
	if ((self = [super initWithRequest:request delegate:self]))
	{
	}
	
	return self;
}

- (void)connection:(ConnectionChecker*)connection didReceiveResponse:(NSURLResponse*)response
{
	NSLog(@"ping: response");
}

- (void)connection:(ConnectionChecker*)connection didReceiveData:(NSData*)data
{
	NSLog(@"ping: data");
}

- (void)connection:(ConnectionChecker*)connection didFailWithError:(NSError*)error
{
	NSLog(@"ping: error");
	
	s_HasConnectivity = false;
}

- (void)connectionDidFinishLoading:(ConnectionChecker*)connection
{
	NSLog(@"ping: OK");
	
	s_HasConnectivity = true;
}

@end

//

System_iPhone::System_iPhone()
{
	mTiltReader = new TiltReader();
}

System_iPhone::~System_iPhone()
{
	delete mTiltReader;
}

void System_iPhone::CheckNetworkConnectivity()
{
	try
	{
		s_HasConnectivityIsChecked = true;
		
		NSURL* url = [NSURL URLWithString:[NSString stringWithCString:PING_ADDRESS encoding:NSASCIIStringEncoding]];
		
		NSMutableURLRequest* request = [NSMutableURLRequest requestWithURL:url];
		
		/*ConnectionChecker* checker = */[[ConnectionChecker alloc] initWithRequest:request];
		
//		s_HasConnectivity = ([NSString stringWithContentsOfURL:[NSURL URLWithString:@PING_ADDRESS] encoding:NSUTF8StringEncoding error:NULL]!=NULL) ? true : false;
	}
	catch (NSException* e)
	{
		NSLog(@"network connectivity check failed");
		
		s_HasConnectivity = false;
	}
}

bool System_iPhone::HasNetworkConnectivity_get()
{
	if (!s_HasConnectivityIsChecked)
	{
		CheckNetworkConnectivity();
	}
	
	return s_HasConnectivity;
}

void System_iPhone::HasNetworkConnectivity_set(bool value)
{
	s_HasConnectivity = value;
}

bool System_iPhone::IsHacked()
{
#if 1
#warning hack detection disabled
	return false;
#endif
	
	if (s_ApplicationHackChecked)
		return s_ApplicationIsHacked;
	
	s_ApplicationHackChecked = true;
	
	//#ifndef DEPLOYMENT
#if false
	return s_ApplicationIsHacked;
#else
	NSString* sid = @"TjhofsJefoujuz";
	NSMutableString* sid2 = [[[NSMutableString alloc] init] autorelease];
	for (size_t i = 0; i < [sid length]; ++i)
		[sid2 appendFormat:@"%c", [sid characterAtIndex:i] - 1];
	
	if ([[[NSBundle mainBundle] infoDictionary] objectForKey:sid2] != nil)
	{
		// found a signature, should not be here
		
		s_ApplicationIsHacked = true;
	}
	else
	{
		NSString* infoPath = [[NSBundle mainBundle] pathForResource:@"Info" ofType:@"plist"];
		
		NSDate* infoModifiedDate = [[[NSFileManager defaultManager] attributesOfItemAtPath:infoPath error:nil/* traverseLink:YES*/] fileModificationDate];
		NSDate* pkgInfoModifiedDate = [[[NSFileManager defaultManager] attributesOfItemAtPath:[[[NSBundle mainBundle] resourcePath] stringByAppendingPathComponent:@"PkgInfo"] error:nil/* traverseLink:YES*/] fileModificationDate];
		
		if ([infoModifiedDate timeIntervalSinceReferenceDate] > [pkgInfoModifiedDate timeIntervalSinceReferenceDate])
		{
			// weird modification dates
			
			s_ApplicationIsHacked = true;
		}
	}
	
	LOG(LogLevel_Debug, "hack detection result: %d", (int)s_ApplicationIsHacked);
	
	return s_ApplicationIsHacked;
#endif	
}

/*std::string System_iPhone::GetDeviceId()
{
	return [[[UIDevice currentDevice] uniqueIdentifier] cStringUsingEncoding:NSASCIIStringEncoding];
}*/

std::string System_iPhone::GetResourcePath(const char* fileName)
{
	const NSString* nsFileNameFull = [NSString stringWithCString:fileName encoding:NSASCIIStringEncoding];
	NSString* nsFileName = [nsFileNameFull stringByDeletingPathExtension];
	NSString* nsFileExtension = [nsFileNameFull pathExtension];
	
	if (!nsFileName || !nsFileExtension)
		throw ExceptionNA();
	
	const NSBundle* bundle = [NSBundle mainBundle];
	
	const NSString* path = [bundle pathForResource:nsFileName ofType:nsFileExtension];
	
	if (!path)
		throw ExceptionVA("path does not exist: %s", fileName);
	
	return [path cStringUsingEncoding:NSASCIIStringEncoding];	
}

std::string System_iPhone::GetDocumentPath(const char* fileName)
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
	{
		throw ExceptionVA("document path not found");
	}
	
	NSString* temp = [path stringByAppendingPathComponent:[NSString stringWithCString:fileName encoding:NSASCIIStringEncoding]];
	
	return [temp cStringUsingEncoding:NSASCIIStringEncoding];	
}

std::string System_iPhone::GetCountryCode()
{
	NSLocale* locale = [NSLocale currentLocale];
	
	if (locale == nil)
		return "xx";
	
	NSString* countryCode = [locale objectForKey: NSLocaleCountryCode];
	
	if (countryCode == nil)
		return "yy";
	
	std::string result = [countryCode cStringUsingEncoding:NSASCIIStringEncoding];
	
	LOG(LogLevel_Debug, "country code: %s", result.c_str());
	
	return result;
}

static void FreeSS(void* info, const void* data, size_t size)
{
	Screenshot* ss = (Screenshot*)info;
	
	delete ss;
}

static UIImage* ToImage(Screenshot* ss)
{
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(ss, ss->mPixels, ss->mSx * ss->mSy * 4, FreeSS);
	CGImageRef image = CGImageCreate(
		ss->mSx,
		ss->mSy, 8, 32, ss->mSx * sizeof(ScreenshotPixel),
		colorSpace,
		0, provider, 0, FALSE, kCGRenderingIntentDefault);
	CGColorSpaceRelease(colorSpace);
	UIImage* result = [UIImage imageWithCGImage:image];
	CGImageRelease(image);
	
	return result;
}

void System_iPhone::SaveToAlbum(Screenshot* ss, const char* name)
{
	Screenshot* temp = ss->Copy();
	
	UIImage* image = ToImage(temp);
	
    UIImageWriteToSavedPhotosAlbum(image, nil, nil, nil); 
}

Vec3 System_iPhone::GetTiltVector()
{
	return mTiltReader->GravityVector_get();
}

Vec2F System_iPhone::GetTiltDirectionXY()
{
	return mTiltReader->DirectionXY_get();
}

bool System_iPhone::IsIpad()
{
	return UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad;
}

SpriteColor System_iPhone::Color_FromHSB(float h, float s, float b)
{
	UIColor* color = [UIColor colorWithHue:h saturation:s brightness:b alpha:1.0f];
	
	const CGFloat* rgba = CGColorGetComponents([color CGColor]);
	
	return SpriteColor_MakeF(rgba[0], rgba[1], rgba[2], rgba[3]);
}

int System_iPhone::GetScreenRotation()
{
	UIInterfaceOrientation orientation = [UIApplication sharedApplication].statusBarOrientation;
	
	switch (orientation)
	{
		case UIInterfaceOrientationLandscapeLeft:
			return 180;
		case UIInterfaceOrientationLandscapeRight:
			return 0;
		default:
			return 0;
	}
}
