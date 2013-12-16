#import "AppDelegate.h"
#import "AppView.h"
#import "AppViewMgr.h"
#import "Calc.h"
#include "DirectoryScanner.h"
#include "FileStream.h"
#import "MainViewMgr.h"
#import "sounds.h"
#include "StringEx.h"

#ifdef DEBUG
#include "level_pack.h"
static void DBG_SyncLevelPackFromResources(std::string src)
{
	std::string name = [[[NSString stringWithCString:src.c_str() encoding:NSASCIIStringEncoding] lastPathComponent] cStringUsingEncoding:NSASCIIStringEncoding];
	std::string dst = DirectoryScanner::GetDocumentPath() + name;
	std::vector<std::string> srcFiles = DirectoryScanner::ListFiles(src);
	std::vector<std::string> dstFiles;
	for (size_t i = 0; i < srcFiles.size(); ++i)
	{
		std::string fileName = [[[NSString stringWithCString:srcFiles[i].c_str() encoding:NSASCIIStringEncoding] lastPathComponent] cStringUsingEncoding:NSASCIIStringEncoding];
		std::string dst = DirectoryScanner::GetDocumentPath() + name + "/" + fileName;
		dstFiles.push_back(dst);
	}
	
	NSFileManager* fm = [[NSFileManager alloc] init];
	[fm createDirectoryAtPath:[NSString stringWithCString:dst.c_str() encoding:NSASCIIStringEncoding] withIntermediateDirectories:NO attributes:nil error:nil];
	for (size_t i = 0; i < srcFiles.size(); ++i)
	{
		std::string src = srcFiles[i];
		std::string dst = dstFiles[i];
		FileStream srcStream;
		FileStream dstStream;
		srcStream.Open(src.c_str(), OpenMode_Read);
		dstStream.Open(dst.c_str(), OpenMode_Write);
		LOG_DBG("copying %s to %s", src.c_str(), dst.c_str());
		StreamExtensions::StreamTo(&srcStream, &dstStream, 4096);
	}
	[fm release];
}
static void DBG_SyncLevelPacksFromResources()
{	
	std::vector<std::string> directoryList = DirectoryScanner::ListDirectories(DirectoryScanner::GetResourcePath());
	
	for (size_t i = 0; i < directoryList.size(); ++i)
	{
		std::string directory = directoryList[i];
		
		std::string name = [[[NSString stringWithCString:directory.c_str() encoding:NSASCIIStringEncoding] lastPathComponent] cStringUsingEncoding:NSASCIIStringEncoding];
		
		if (!String::StartsWith(name, "Pack"))
			continue;
		
		DBG_SyncLevelPackFromResources(directory);
	}
}
static void DBG_TestLevelPack()
{
	std::vector<std::string> directoryList = DirectoryScanner::ListDirectories(DirectoryScanner::GetDocumentPath());
	
	for (size_t i = 0; i < directoryList.size(); ++i)
	{
		std::string directory = directoryList[i];
		
		std::string name = [[[NSString stringWithCString:directory.c_str() encoding:NSASCIIStringEncoding] lastPathComponent] cStringUsingEncoding:NSASCIIStringEncoding];
		
		if (!String::StartsWith(name, "Pack"))
			continue;
		
		directory = directory + "/";
		
		std::string descriptionPath = directory + "pack.txt";
		
		FileStream stream;
		stream.Open(descriptionPath.c_str(), OpenMode_Read);
		LevelPackDescription desc(directory);
		desc.Load(&stream);
	}
}
#endif

@implementation td1AppDelegate

@synthesize window;
@synthesize mgr;

#pragma mark -
#pragma mark Application lifecycle

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
	Calc::Initialize();
	
#ifdef DEBUG
	DBG_SyncLevelPacksFromResources();
	DBG_TestLevelPack();
#endif
	
	SoundInit();
	
    [window makeKeyAndVisible];
	
//	self.mgr = [[[AppViewMgr alloc] initWithNibName:@"AppViewMgr" bundle:nil] autorelease];
	self.mgr = [[[MainViewMgr alloc] initWithNibName:@"MainViewMgr" bundle:nil] autorelease];
	[window addSubview:mgr.view];
	
	return YES;
}

+(std::vector<LevelPackDescription>) scanLevelPacks
{
	std::vector<LevelPackDescription> result;
	
	std::vector<std::string> directoryList = DirectoryScanner::ListDirectories(DirectoryScanner::GetDocumentPath());
	
	for (size_t i = 0; i < directoryList.size(); ++i)
	{
		std::string directory = directoryList[i];
		
		std::string name = [[[NSString stringWithCString:directory.c_str() encoding:NSASCIIStringEncoding] lastPathComponent] cStringUsingEncoding:NSASCIIStringEncoding];
		
		if (!String::StartsWith(name, "Pack"))
			continue;
		
		directory = directory + "/";
		
		std::string descriptionPath = directory + "pack.txt";
		
		FileStream stream;
		stream.Open(descriptionPath.c_str(), OpenMode_Read);
		LevelPackDescription desc(directory);
		desc.Load(&stream);
		
		result.push_back(desc);
	}
	
	return result;
}

+(std::vector<LevelDescription>)scanLevels:(NSString*)path
{
	std::vector<LevelDescription> result;
	
	std::vector<std::string> fileList = DirectoryScanner::ListFiles([path cStringUsingEncoding:NSASCIIStringEncoding]);

	for (size_t i = 0; i < fileList.size(); ++i)
	{
		std::string file = fileList[i];
		
		std::string name = [[[NSString stringWithCString:file.c_str() encoding:NSASCIIStringEncoding] lastPathComponent] cStringUsingEncoding:NSASCIIStringEncoding];
		std::string base = [[[NSString stringWithCString:file.c_str() encoding:NSASCIIStringEncoding] stringByDeletingPathExtension] cStringUsingEncoding:NSASCIIStringEncoding];
		
		if (!String::StartsWith(name, "level"))
			continue;
		if (String::Contains(name, '_'))
			continue;

		FileStream stream;
		stream.Open(file.c_str(), OpenMode_Read);
		LevelDescription desc(base);
		desc.Load(&stream);
		
		result.push_back(desc);
	}
	
	return result;
}

-(void)applicationWillResignActive:(UIApplication *)application 
{
}

-(void)applicationDidEnterBackground:(UIApplication *)application 
{
}

-(void)applicationWillEnterForeground:(UIApplication *)application 
{
}

-(void)applicationDidBecomeActive:(UIApplication *)application 
{
}

-(void)applicationWillTerminate:(UIApplication *)application 
{
}

#pragma mark -
#pragma mark Memory management

-(void)applicationDidReceiveMemoryWarning:(UIApplication *)application 
{
}

-(void)dealloc 
{
	SoundShutdown();
	self.mgr = nil;
    [window release];
    [super dealloc];
}

@end
