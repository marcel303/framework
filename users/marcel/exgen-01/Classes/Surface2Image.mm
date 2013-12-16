#include "Surface2Image.h"

CGImageRef Surface2Image(Surface* surface)
{
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	
	CGContextRef context = CGBitmapContextCreate(surface->Buffer_get(), surface->Sx_get(), surface->Sy_get(), 8, surface->Sx_get() * 4, colorSpace, kCGImageAlphaPremultipliedLast);
	
	CGColorSpaceRelease(colorSpace);
	
	CGImageRef image = CGBitmapContextCreateImage(context);
	
	CGContextRelease(context);
	
	return image;
}
