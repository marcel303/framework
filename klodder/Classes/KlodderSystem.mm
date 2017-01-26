#import <UIKit/UIKit.h>
#import "AppDelegate.h"
#import "BlitTransform.h"
#import "Debugging.h"
#import "Exception.h"
#import "KlodderSystem.h"
#import "Log.h"
#import "MacImage.h"

System gSystem;

std::string System::GetResourcePath(const char* fileName)
{
	Assert(fileName);
	Assert(strlen(fileName) > 0);
	
	const NSString* nsFileNameFull = [NSString stringWithCString:fileName encoding:NSASCIIStringEncoding];
	const NSString* nsFileName = [nsFileNameFull stringByDeletingPathExtension];
	const NSString* nsFileExtension = [nsFileNameFull pathExtension];
	
	if (!nsFileName || !nsFileExtension)
		throw ExceptionNA();
	
	const NSBundle* bundle = [NSBundle mainBundle];
	
	const NSString* path = [bundle pathForResource:(NSString*)nsFileName ofType:(NSString*)nsFileExtension];
	
	if (!path)
		throw ExceptionVA("path does not exist: %s", fileName);
	
	return [path cStringUsingEncoding:NSASCIIStringEncoding];	
}

std::string System::GetDocumentPath(const char* fileName)
{
	Assert(fileName);
//	Assert(strlen(fileName) > 0);
	
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
	{
		throw ExceptionVA("document path not found");
	}
	
//	LOG_DBG("path: %s, fileName: %s", [path cStringUsingEncoding:NSASCIIStringEncoding], fileName);
	
	NSString* temp = [path stringByAppendingPathComponent:[NSString stringWithCString:fileName encoding:NSASCIIStringEncoding]];
	
	return [temp cStringUsingEncoding:NSASCIIStringEncoding];	
}

bool System::PathExists(const char* path)
{
	Assert(path);
	Assert(strlen(path) > 0);
	
	BOOL isDir = TRUE;
	
	return [[NSFileManager defaultManager] fileExistsAtPath:[NSString stringWithCString:path encoding:NSASCIIStringEncoding] isDirectory:&isDir];
}

void System::CreatePath(const char* path)
{
	Assert(path);
	Assert(strlen(path) > 0);
	
	NSError* error = nil;
	
    if (![[NSFileManager defaultManager] createDirectoryAtPath:[NSString stringWithCString:path encoding:NSASCIIStringEncoding] withIntermediateDirectories:YES attributes:nil error:&error])
	{
		throw ExceptionVA("unable to create path");
	}
}

void System::DeletePath(const char* path)
{
	Assert(path);
	Assert(strlen(path) > 0);
	
	NSError* error = nil;
	
	if (![[NSFileManager defaultManager] removeItemAtPath:[NSString stringWithCString:path encoding:NSASCIIStringEncoding] error:&error])
	{
		throw ExceptionVA("unable to delete directory");
	}
}

void System::SaveAsPng(const MacImage& image, const char* path)
{
	MacImage* temp = image.FlipY();
	
	UIImage* uiImage = [AppDelegate macImageToUiImage:temp size:Vec2I(image.Sx_get(), image.Sy_get())];
	
	NSData* data = UIImagePNGRepresentation(uiImage);
	
	[data writeToFile:[NSString stringWithCString:path encoding:NSASCIIStringEncoding] atomically:FALSE];
	
	delete temp;
}

void System::SaveAsJpeg(const MacImage& image, const char* path)
{
	MacImage* temp = image.FlipY();
	UIImage* uiImage = [AppDelegate macImageToUiImage:temp size:Vec2I(image.Sx_get(), image.Sy_get())];
	delete temp;
	
	NSData* data = UIImageJPEGRepresentation(uiImage, 1.0f);
	
	[data writeToFile:[NSString stringWithCString:path encoding:NSASCIIStringEncoding] atomically:FALSE];
}

MacImage* System::LoadImage(const char* path, bool flipY)
{
	UIImage* uiImage = [[UIImage alloc] initWithContentsOfFile:[NSString stringWithCString:path encoding:NSASCIIStringEncoding]];
	
	MacImage* result = 0;
	
	MacImage* temp = [AppDelegate uiImageToMacImage:uiImage size:Vec2I(uiImage.size.width, uiImage.size.height)];
	
	if (flipY)
	{
		result = temp->FlipY();
		
		delete temp;
	}
	else
	{
		result = temp;
	}
	
	[uiImage release];
	
	return result;
}

MacImage* System::Transform(MacImage& image, const BlitTransform& transform, int sx, int sy) const
{
	// horribly inefficient..
	
	UIGraphicsBeginImageContext(CGSizeMake(sx, sy));
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGContextTranslateCTM(ctx, transform.x, transform.y);
	CGContextRotateCTM(ctx, transform.angle);
	CGContextScaleCTM(ctx, transform.scale, transform.scale);
	CGContextTranslateCTM(ctx, -transform.anchorX, -transform.anchorY);
	
	MacImage* temp = image.FlipY();
	CGImageRef cgImage = temp->Image_get();
	UIImage* uiImage = [UIImage imageWithCGImage:cgImage];
	CGImageRelease(cgImage);
	
	CGContextSetInterpolationQuality(ctx, kCGInterpolationHigh);
	[uiImage drawAtPoint:CGPointMake(0.0f, 0.0f)];
	
	delete temp;
	
	UIImage* result = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	
	MacImage* result2 = [AppDelegate uiImageToMacImage:result];
	result2->FlipY_InPlace();
	
	return result2;
}

bool System::IsLiteVersion()
{
	return KLODDER_LITE;
}
