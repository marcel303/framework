#import "Application.h"
#import "GCDAsyncSocket.h"
#import "Exception.h"
#import "ExceptionLoggerObjC.h"
#import "FileArchive.h"
#import "FileStream.h"
#import "HtmlTemplateEngine.h"
#import "HTTPDataResponse.h"
#import "HTTPFileResponse.h"
#import "HTTPServer.h"
#import "HTTPResponse.h"
#import "ImageId.h"
#import "KlodderSystem.h"
#import "MemoryStream.h"
#import "MyHTTPConnection.h"
#import "Path.h"

static NSString* GetCachesPath()
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
		throw ExceptionVA("caches directory unavailable");
	
	return path;
}

static void MakeSureCachesDirectoryExists()
{
	NSString* path = GetCachesPath();
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	
	if (![fileManager fileExistsAtPath:path])
	{
		LOG_INF("caches directory does not exist. creating path", 0);
		[fileManager createDirectoryAtPath:path attributes:nil];
	}
}

static void RenderHtml(HtmlTemplateEngine& engine, std::string func, std::vector<std::string> args)
{
	if (func == "render")
	{
		if (args.size() != 1)
			throw ExceptionNA();
		
		if (args[0] == "content")
		{
			NSString* path = [NSString stringWithCString:gSystem.GetDocumentPath("").c_str() encoding:NSASCIIStringEncoding];
			
		    NSArray* fileList = [[NSFileManager defaultManager] directoryContentsAtPath:path];
			
			for (NSString* fileName in fileList)
			{
				NSString* extension = [fileName pathExtension];
				
				if ([extension isEqualToString:@"xml"])
				{
					NSString* baseName = [fileName stringByDeletingPathExtension];
					NSString* fileNameThumb = [[baseName stringByAppendingString:@"_thumbnail"] stringByAppendingPathExtension:@"png"];
					
					NSDictionary* attributes = [[NSFileManager defaultManager] fileAttributesAtPath:[path stringByAppendingPathComponent:fileName] traverseLink:NO];
					NSString* modicationDate = [[attributes objectForKey:NSFileModificationDate] description];
					
					engine.SetKey("thumbnail", [fileNameThumb cStringUsingEncoding:NSASCIIStringEncoding]);
					engine.SetKey("link", [baseName cStringUsingEncoding:NSASCIIStringEncoding]);
					engine.SetKey("name", [baseName cStringUsingEncoding:NSASCIIStringEncoding]);
					engine.SetKey("date", [modicationDate cStringUsingEncoding:NSASCIIStringEncoding]);
					
					engine.IncludeResource("browser_picture.txt");
				}
			}
		}
		else
			throw ExceptionNA();
	}
	else
		throw ExceptionNA();
}

@implementation MyHTTPConnection

- (BOOL)isBrowseable:(NSString *)path
{
	// Override me to provide custom configuration...
	// You can configure it for the entire server, or based on the current request
	
	return YES;
}

-(NSString*)createBrowseableIndex:(NSString *)path
{
	HandleExceptionObjcBegin();
	
	HtmlTemplateEngine engine(RenderHtml);
	
	engine.Begin();
	engine.IncludeResource("browser_page.txt");
	
	std::string text = engine.ToString();
	
	return [NSString stringWithCString:text.c_str() encoding:NSASCIIStringEncoding];
	
	HandleExceptionObjcEnd(false);
	
	return @"";
}

-(NSObject<HTTPResponse>*)httpResponseForMethod:(NSString *)method URI:(NSString *)path
{
	HandleExceptionObjcBegin();
	
	NSString* nsBaseNameDoc = [self filePathForURI:path allowDirectory:NO];
	NSString* nsBaseNameRes = nil;
	
	try
	{
		NSString* relativePath = path;
		while([relativePath hasPrefix:@"/"] && [relativePath length] > 1)
		{
			relativePath = [relativePath substringFromIndex:1];
		}
		nsBaseNameRes = [NSString stringWithCString:gSystem.GetResourcePath([relativePath cStringUsingEncoding:NSASCIIStringEncoding]).c_str() encoding:NSASCIIStringEncoding];
	}
	catch (std::exception& e)
	{
		// nop
	}
	
	std::string baseName;
	
	if (nsBaseNameDoc != nil)
		baseName = [nsBaseNameDoc cStringUsingEncoding:NSASCIIStringEncoding];
	
	NSString* nsFileNameXml = [nsBaseNameDoc stringByAppendingString:@".xml"];
	
	if (nsBaseNameDoc != nil)
	{
		HTTPFileResponse* response = [[[HTTPFileResponse alloc] initWithFilePath:nsBaseNameRes forConnection: self] autorelease];
		
		return response;
	}
	else if ([[NSFileManager defaultManager] fileExistsAtPath:nsBaseNameDoc])
	{
		HTTPFileResponse* response = [[[HTTPFileResponse alloc] initWithFilePath:nsBaseNameDoc forConnection: self] autorelease];
		
		return response;
	}
	else if ([[NSFileManager defaultManager] fileExistsAtPath:nsFileNameXml])
	{
		MakeSureCachesDirectoryExists();
		
		NSString* nsFileNameKlodder = [[nsBaseNameDoc stringByAppendingString:@".klodder"] lastPathComponent];
		
		NSString* nsCacheName = nsFileNameKlodder;
		NSString* nsCacheFileName = [GetCachesPath() stringByAppendingPathComponent:nsCacheName];
		std::string cacheFileName = [nsCacheFileName cStringUsingEncoding:NSASCIIStringEncoding];
		
		//
		
		ImageId imageId(Path::StripExtension(Path::GetFileName([nsFileNameXml cStringUsingEncoding:NSASCIIStringEncoding])).c_str());
		
		FileStream stream;
		stream.Open(cacheFileName.c_str(), OpenMode_Write);
		
		Application::SaveImageArchive(imageId, &stream);
		
		stream.Close();
		
		//
		
		HTTPFileResponse* response = [[[HTTPFileResponse alloc] initWithFilePath:nsCacheFileName forConnection:self] autorelease];

		NSString* key = @"Content-Disposition";
		NSString* value = [NSString stringWithFormat:@"attachment; filename=\"%@\"", nsFileNameKlodder];
		
		response.httpHeaders = [NSDictionary dictionaryWithObjectsAndKeys:value, key, nil];
		
		return response;
	}
	else
	{
#if 1
		NSString* folder = [path isEqualToString:@"/"] ? config.documentRoot : [NSString stringWithFormat: @"%@%@", config.documentRoot, path];

		if ([self isBrowseable:folder])
		{
			NSData* browseData = [[self createBrowseableIndex:folder] dataUsingEncoding:NSUTF8StringEncoding];
			
			return [[[HTTPDataResponse alloc] initWithData:browseData] autorelease];
		}
#else
		return nil;
#endif
	}
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

@end
