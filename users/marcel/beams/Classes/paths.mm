#include "Exception.h"
#include "paths.h"

std::string GetResourcePath(const char* fileName)
{
	const NSString* nsFileNameFull = [NSString stringWithCString:fileName encoding:NSASCIIStringEncoding];
	const NSString* nsFileName = [nsFileNameFull stringByDeletingPathExtension];
	const NSString* nsFileExtension = [nsFileNameFull pathExtension];
	
	if (!nsFileName || !nsFileExtension)
		throw ExceptionNA();
	
	const NSBundle* bundle = [NSBundle mainBundle];
	
	const NSString* path = [bundle pathForResource:nsFileName ofType:nsFileExtension];
	
	if (!path)
		throw ExceptionVA("path does not exist: %s", fileName);
	
	return [path cStringUsingEncoding:NSASCIIStringEncoding];	
}