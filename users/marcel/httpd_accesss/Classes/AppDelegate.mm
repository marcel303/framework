#import <map>
#import "AppDelegate.h"
#import "Log.h"
#import "FileStream.h"
#import "StringEx.h"
#import "StreamReader.h"

@implementation httpd_accesssAppDelegate

@synthesize window;


#pragma mark -
#pragma mark Application lifecycle

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {    
    
    // Override point for customization after application launch.
	
    [window makeKeyAndVisible];
	
	std::string fileName = [[[NSBundle mainBundle] pathForResource:@"access_log" ofType:nil] cStringUsingEncoding:NSASCIIStringEncoding];
	
	FileStream stream;
	stream.Open(fileName.c_str(), OpenMode_Read);
	StreamReader reader(&stream, false);
	
	std::map<std::string, int> map;
	std::map<std::string, std::map<std::string, int> > concurrent;
	
	int lineCount = 0;
	
	while (!stream.EOF_get())
	{
		if ((lineCount % 10000) == 0)
			LOG_INF("scanning line %d (%d/%d bytes, %ld%%)", lineCount, stream.Position_get(), stream.Length_get(), 100L * stream.Position_get() / stream.Length_get());
		
		lineCount++;
		
		std::string line = reader.ReadLine();
		
		int pos = String::Find(line, '-');
		
		if (pos < 0)
			continue;
		
		std::string ip = String::SubString(line, 0, pos);
		
		ip = String::Trim(ip);
		
		map[ip] = 1;
		
		int pos1 = String::Find(line, '[');
		int pos2 = String::Find(line, ']');
		
		if (pos1 < 0 || pos2 < 0)
			continue;
		
		int length = pos2 - pos1 - 2;
		
		if (length < 0)
			continue;
		
		std::string date = String::SubString(line, pos1 + 1, length);
		
		int pos3 = String::Find(date, ' ');
		
		if (pos3 < 0)
			continue;
		
		date = String::SubString(date, 0, pos3);
		
		std::vector<std::string> components = String::Split(date, "/:");
		
		if (components.size() != 6)
			continue;
		
		components.pop_back();
		
		std::string slot = String::Join(components, ".");
		
		std::map<std::string, std::map<std::string, int> >::iterator concurrentPtr = concurrent.find(slot);
		
		if (concurrentPtr == concurrent.end())
		{
			concurrent[slot] = std::map<std::string, int>();
			concurrentPtr = concurrent.find(slot);
		}

		std::map<std::string, int>& c2 = concurrentPtr->second;
		
		std::map<std::string, int>::iterator c2Ptr = c2.find(ip);
		
		if (c2Ptr == c2.end())
		{
			c2[ip] = 0;
			c2Ptr = c2.find(ip);
		}
		
		c2Ptr->second++;
		
		//LOG_INF("slot: %s", slot.c_str());
	}
	
	for (std::map<std::string, int>::iterator i = map.begin(); i != map.end(); ++i)
		LOG_INF("item: %s", i->first.c_str());
	
	LOG_INF("count: %lu", map.size());
	
	int min = -1;
	int max = -1;
	
	for (std::map<std::string, std::map<std::string, int> >::iterator i = concurrent.begin(); i != concurrent.end(); ++i)
	{
		int count = i->second.size();
		
		LOG_INF("count: %s: %d", i->first.c_str(), count);
		
		if (count < min || min == -1)
			min = count;
		if (count > max || max == -1)
			max = count;
	}
	
	LOG_INF("concurrency: min: %d", min);
	LOG_INF("concurrency: max: %d", max);
	
	return YES;
}


- (void)applicationWillResignActive:(UIApplication *)application {
    /*
     Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
     Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
     */
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
    /*
     Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later. 
     If your application supports background execution, called instead of applicationWillTerminate: when the user quits.
     */
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
    /*
     Called as part of  transition from the background to the inactive state: here you can undo many of the changes made on entering the background.
     */
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
    /*
     Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
     */
}


- (void)applicationWillTerminate:(UIApplication *)application {
    /*
     Called when the application is about to terminate.
     See also applicationDidEnterBackground:.
     */
}


#pragma mark -
#pragma mark Memory management

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application {
    /*
     Free up as much memory as possible by purging cached data objects that can be recreated (or reloaded from disk) later.
     */
}


- (void)dealloc {
    [window release];
    [super dealloc];
}


@end
