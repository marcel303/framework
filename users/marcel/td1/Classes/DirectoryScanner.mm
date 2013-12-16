#import <Foundation/Foundation.h>
#import "DirectoryScanner.h"
#import "Exception.h"

std::string DirectoryScanner::GetDocumentPath()
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
	{
		throw ExceptionVA("document path not found");
	}
	
	return std::string([path cStringUsingEncoding:NSASCIIStringEncoding]) + "/";
}

std::string DirectoryScanner::GetResourcePath()
{
	NSString* path = [[NSBundle mainBundle] resourcePath];
	
	if (!path)
	{
		throw ExceptionVA("resource path not found");
	}
	
	return std::string([path cStringUsingEncoding:NSASCIIStringEncoding]) + "/";
}

std::vector<std::string> DirectoryScanner::ListDirectories(std::string _path)
{
	NSString* path = [NSString stringWithCString:_path.c_str() encoding:NSASCIIStringEncoding];
	
	std::vector<std::string> result;
	
	NSFileManager* fm = [[NSFileManager alloc] init];
	
	NSError* error = nil;
	
	NSArray* content = [fm contentsOfDirectoryAtPath:path error:&error];
	
	for (NSUInteger i = 0; i < [content count]; ++i)
	{
		NSString* fileName = [path stringByAppendingPathComponent:[content objectAtIndex:i]];
		
		BOOL isDirectory;
		
		[fm fileExistsAtPath:fileName isDirectory:&isDirectory];
		
		if (isDirectory)
			result.push_back([fileName cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	
	[fm release];
	
	return result;
}

std::vector<std::string> DirectoryScanner::ListFiles(std::string _path)
{
	NSString* path = [NSString stringWithCString:_path.c_str() encoding:NSASCIIStringEncoding];
	
	std::vector<std::string> result;
	
	NSFileManager* fm = [[NSFileManager alloc] init];
	
	NSError* error = nil;
	
	NSArray* content = [fm contentsOfDirectoryAtPath:path error:&error];
	
	for (NSUInteger i = 0; i < [content count]; ++i)
	{
		NSString* fileName = [path stringByAppendingPathComponent:[content objectAtIndex:i]];
		
		BOOL isDirectory;
		
		[fm fileExistsAtPath:fileName isDirectory:&isDirectory];
		
		if (!isDirectory)
			result.push_back([fileName cStringUsingEncoding:NSASCIIStringEncoding]);
	}
	
	[fm release];
	
	return result;
}
