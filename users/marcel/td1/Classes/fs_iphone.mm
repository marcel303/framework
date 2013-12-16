#import <UIKit/UIKit.h>
#include "fs_iphone.h"

std::string iphone_resource_path(const char* fileName)
{
	const NSString* nsFileNameFull = [NSString stringWithCString:fileName encoding:NSASCIIStringEncoding];
	const NSString* nsFileName = [nsFileNameFull stringByDeletingPathExtension];
	const NSString* nsFileExtension = [nsFileNameFull pathExtension];
	
	if (!nsFileName || !nsFileExtension)
		throw std::exception();
	
	const NSBundle* bundle = [NSBundle mainBundle];
	
	const NSString* path = [bundle pathForResource:nsFileName ofType:nsFileExtension];
	
	if (!path)
		throw std::exception();
	
	return [path cStringUsingEncoding:NSASCIIStringEncoding];	
}
