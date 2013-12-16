#include "Exception.h"
#include "ResIO.h"
#include "TextureRGBA.h"

namespace ResIO
{
	static NSString* GetPath(const char* fileName)
	{
		const NSString* nsFileNameFull = [NSString stringWithCString:fileName encoding:NSASCIIStringEncoding];
		const NSString* nsFileName = [nsFileNameFull stringByDeletingPathExtension];
		const NSString* nsFileExtension = [nsFileNameFull pathExtension];
		
		if (nsFileName == nil || nsFileExtension == nil)
			throw ExceptionNA();
		
		const NSBundle* bundle = [NSBundle mainBundle];
		
		const NSString* path = [bundle pathForResource:nsFileName ofType:nsFileExtension];
		
		if (path == nil)
			throw ExceptionNA();
		
		return path;
	}
	
	static bool ImageToBytes(UIImage* image, int* out_Sx, int* out_Sy, uint8_t** out_Bytes)
	{
		const int sx = (int)image.size.width;
		const int sy = (int)image.size.height;
		const int byteCount = sx * sy * 4;
		
		uint8_t* bytes = new uint8_t[byteCount];
		
		bzero(bytes, byteCount);
		
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		CGContextRef ctx = CGBitmapContextCreate(bytes, sx, sy, 8, 4 * sx, colorSpace, kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big);
		CGColorSpaceRelease(colorSpace);
		CGContextDrawImage(ctx, CGRectMake(0, 0, sx, sy), [image CGImage]);
		CGContextRelease(ctx);
		
		*out_Sx = sx;
		*out_Sy = sy;
		*out_Bytes = bytes;
		
		return true;
	}
	
	void* LoadTexture_RGBA_PNG(const char* fileName)
	{
		// Load image

		NSString* path = GetPath(fileName);
		
		UIImage* image = [[UIImage alloc] initWithContentsOfFile:path];

		if (image == nil)
		{
			throw ExceptionVA("unable to load image: %s", fileName);
		}
		
		int sx = 0;
		int sy = 0;
		uint8_t* bytes = 0;
		
		if (!ImageToBytes(image, &sx, &sy, &bytes))
		{
			[image release];
			
			throw ExceptionVA("failed to get image data");
		}
		
		[image release];
		
		return new TextureRGBA(sx, sy, bytes, true);
	}
}
