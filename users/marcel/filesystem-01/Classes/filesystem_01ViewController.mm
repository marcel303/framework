#import "filesystem_01ViewController.h"

@implementation filesystem_01ViewController

/*

 Application placed in its own directory.
 
 Reading and writing limited to application (home) directory.
 
 Application directory contains a number of sub-directories, each with their own access restriction / pattern.
 
 <Application_Home>/AppName.app
 <Application_Home>/Documents/          writable, gets backed up
 <Application_Home>/Library/Preferences preference files
 <Application_Home>/Library/Caches      writable, no backup, persistence between launches, may get purged
 <Application_Home>/tmp/                writable, no backup, no persistence between launches
 
 Bundle file system:
 
 - NSBundle. Use NSBundle to gain access to resources embedded in bundle.
 
	NSBundle paths look as follows: /<Application_Home>/AppName.app/resource.txt (for resource.txt embedded in application bundle).
 
 Retrieving paths:
 
 - NSHomeDirectory
 - NSTemporaryDirectory
 - NSSearchPathForDirectoriesInDomain
 
 	directory:
		NSApplicationDirectory        <app>/
		NSLibraryDirectory            <app>/Library
		NSUserDirectory               /Users
		NSDocumentDirectory           <app>/Documents (?)
		NSCachesDirectory             <app>/Library/Caches
		NSApplicationSupportDirectory <app>/Library/Application
 	domainMask:
		NSUserDomainMask
 
 Retrieving properties:
 
 - NSUserDefaults. Use NSUserDefaults to load preferences from Library/Preferences and automatically keep them in sync.
 
 	The [synchronize] method save any pending changes to the storage device.
 
 	Preferences are saved as property files. Each property must be able to serialize itself to the property document format.
 
 		- NSNumber, NSString, NSData, NSArray, NSDictionary and NSDate are supported.
 
 	Preferences are saved on a per-user basis.
 
	No way of testing properties exist. Getters return API defined default value if property does not exist.
 
 	Getters:
 		[arrayForKey]
 		[floatForKey]
 		[stringForKey]
 		[dataForKey]
 		[dictionaryForKey]
 		..
 
 	Setters:
 		[setBool]
 		[setFloat]
 		[setInteger]
 		[setObject]
 
 Application settings using settings bundle:

 	Settings stored in settings bundle are automatically exposed to 'settings' system menu.
	The user may edit his/her settings from the global 'settings' menu of the device.
 	The settings application may not be the most suitable location to change settings.

 Manually managing property files:
 
	- Represent application settings as property list.
	- Serialize settings to NSData.
 	- Write NSData object to file.

 	NOTE: Property lists should be serialized using a binary formatter (consumes less space, faster load/save).
 
 Examples:
 
 	// Get documents path.
 
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
 
	NSString *documentsDirectory = [paths objectAtIndex:0];
 
	// Save property list.
 
	-(BOOL)writeApplicationPlist:(id)plist toFile:(NSString *)fileName
	{
		NSString* error;
		NSData* data = [NSPropertyListSerialization dataFromPropertyList:plist format:NSPropertyListBinaryFormat_v1_0 errorDescription:&error];
		if (!data)
		{
			NSLog(@"%@", error);
			return NO;
		}
		return [self writeApplicationData:data toFile:fileName];
	}
 
	// Load property list.
 
	- (id)applicationPlistFromFile:(NSString *)fileName
	{
		id result;
		NSData* data = [self applicationDataFromFile:fileName];
		if (!result)
		{
			NSLog(@"Data file not returned.");
			return nil;
		}
		NSPropertyListFormat format;
		NSString *error;
		result = [NSPropertyListSerialization propertyListFromData:data mutabilityOption:NSPropertyListImmutable format:&format errorDescription:&error];
		if (!result)
		{
			NSLog(@"Plist not returned, error: %@", error);
 			return nil;
		}
		return result;
	}
 
 ----
 
 File Management
 
 - NSFileHandle. Represents a file.
 - NSFileManager. Performas file system operations such as creating and deleting files.
 
 Examples:
 
*/

-(IBAction)resourceRead:(id)sender
{
	
}

-(IBAction)userDocsRead:(id)sender
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
	{
		[label setText:@"Document path not found"];
		return;
	}

	NSString* fileName = [path stringByAppendingPathComponent:@"test.dat"];
	
	NSData* data = [NSData dataWithContentsOfFile:fileName];
	
	if (!data)
	{
		[label setText:@"Unable to load data file"];
		return;
	}
	
	[label setText:@"Loaded data file"];
}

-(IBAction)userDocsWrite:(id)sender
{
	NSData* data = [[[NSData alloc] init] autorelease];

	//
	
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
	
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
	{
		[label setText:@"Document path not found"];
		return;
	}
	
	NSString* fileName = [path stringByAppendingPathComponent:@"test.dat"];
	
	bool retVal = [data writeToFile:fileName atomically:FALSE];
	
	if (!retVal)
	{
		[label setText:@"Unable to save data file"];
		return;
	}
	
	[label setText:@"Saved data file"];
}

-(IBAction)propertiesRead:(id)sender
{
	NSUserDefaults* config = [[[NSUserDefaults alloc] init] autorelease];
	
	if (!config)
	{
		[label setText:@"Unable to load config"];
		return;
	}
	
	NSString* text = [config stringForKey:@"text"];
	
	[label setText:text];
}

-(IBAction)propertiesWrite:(id)sender
{
	NSUserDefaults* config = [[[NSUserDefaults alloc] init] autorelease];
	
	if (!config)
	{
		[label setText:@"Unable to load config"];
		return;
	}
	
	NSString* text = [NSString stringWithFormat:@"Hello World %d", (int)rand()];
	
	[config setValue:text forKey:@"text"];
	
	bool retVal = [config synchronize];
	
	if (!retVal)
	{
		[label setText:@"Failed to save config"];
		return;
	}
	
	[label setText:@"Set config string"];
}

static void EnumerateDirectories()
{
	NSString* homePath = NSHomeDirectory();
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	
	NSArray* homeContents = [fileManager directoryContentsAtPath:homePath];
	NSArray* libraryContents = [fileManager directoryContentsAtPath:[homePath stringByAppendingPathComponent:@"Library"]];
	
	NSLog(@"Home Contents:");
	for (int i = 0; i < [homeContents count]; ++i)
	{
		NSLog([homeContents objectAtIndex:i]);
	}
	
	NSLog(@"Library Contents:");
	for (int i = 0; i < [libraryContents count]; ++i)
	{
		NSLog([libraryContents objectAtIndex:i]);
	}
}

static void MakeSureCachesDirectoryExists()
{
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
		return;
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	
	if (![fileManager fileExistsAtPath:path])
	{
		NSLog(@"Caches directory does not exist. Creating path.");
		[fileManager createDirectoryAtPath:path attributes:nil];
	}
}

-(IBAction)cacheRead:(id)sender
{
	// Make sure cache directory exists.
	
	MakeSureCachesDirectoryExists();
	
	// Enumerate directories to console.
	
	EnumerateDirectories();
	
	// Get path to caches directory.
	
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString* path = [paths objectAtIndex:0];
	
	NSString* fileName = [path stringByAppendingPathComponent:@"test.dat"];
	
	// Open file.
	
	NSFileHandle* file = [NSFileHandle fileHandleForReadingAtPath:fileName];
	
	if (!file)
	{
		[label setText:@"Unable to open file for reading."];
		return;
	}
	
	// Read from file.
	
	NSData* data = [file readDataToEndOfFile];

	[label setText:[NSString stringWithFormat:@"Data read from cache %d", [data length]]];
}

-(IBAction)cacheWrite:(id)sender
{
	// Make sure cache directory exists.
	
	MakeSureCachesDirectoryExists();
	
	// Get path to caches directory.
	
	NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
	NSString* path = [paths objectAtIndex:0];
	
	if (!path)
	{
		[label setText:@"Caches path not found"];
		return;
	}
	
	//
	
	NSString* fileName = [path stringByAppendingPathComponent:@"test.dat"];
	
#if 1
	// Create file.
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	
	bool retVal = [fileManager createFileAtPath:fileName contents:nil attributes:nil];
	
	if (!retVal)
	{
		[label setText:@"Failed to create cache file"];
		return;
	}
#endif
	
	// Open file.
	
	NSFileHandle* file = [NSFileHandle fileHandleForWritingAtPath:fileName];
	
	if (!file)
	{
		[label setText:@"Unable to open file for writing"];
		return;
	}
	
	// Write to file.
	
	char bufferData[4] = { 0, 1, 2, 3 };
	int bufferSize = sizeof(bufferData);
	
	NSData* data = [NSData dataWithBytes:bufferData length:bufferSize];
	
	[file writeData:data];
	
	[label setText:@"Data written to cache"];
}

-(IBAction)tempRead:(id)sender
{

}

-(IBAction)tempWrite:(id)sender
{
	NSString* path = NSTemporaryDirectory();
	NSString* fileName = [path stringByAppendingPathComponent:@"test.dat"];
	
#if 1
	// Create file.
	
	NSFileManager* fileManager = [NSFileManager defaultManager];
	
	bool retVal = [fileManager createFileAtPath:fileName contents:nil attributes:nil];
	
	if (!retVal)
	{
		[label setText:@"Failed to create cache file"];
		return;
	}
#endif
	
	// Open file.
	
	NSFileHandle* file = [NSFileHandle fileHandleForWritingAtPath:fileName];
	
	if (!file)
	{
		[label setText:@"Unable to open file for writing"];
		return;
	}
	
	// Write to file.
	
	char bufferData[4] = { 0, 1, 2, 3 };
	int bufferSize = sizeof(bufferData);
	
	NSData* data = [NSData dataWithBytes:bufferData length:bufferSize];
	
	[file writeData:data];
	
	[label setText:@"Data written to temp"];
}

-(IBAction)bundleRead:(id)sender
{
	NSBundle* bundle = [NSBundle mainBundle];
	NSString* path = [bundle pathForResource:@"resource" ofType:@"txt"];

	NSLog([NSString stringWithFormat:@"Resource path: %@", path]);
	
	NSString* text = [NSString stringWithContentsOfFile:path];
	
	if (!text)
	{
		[label setText:@"Failed to load contents of resource"];
		return;
	}
	
	[label setText:[NSString stringWithFormat:@"Resource contents: %@", text]];
}

-(IBAction)bundleWrite:(id)sender
{
	[label setText:@"Cannot write resources to bundles, you dummy ;)"];
}

//

-(void)touch:(CGPoint)location
{
	[label setText:[[[NSString alloc] initWithFormat:@"touch: %d, %d", (int)location.x, (int)location.y] autorelease]];
	
	if (location.y > 240.0f)
	{
		UIAlertView* alert = [[UIAlertView alloc] initWithTitle:@"Hello World" message:@"This Is A Test" delegate:self cancelButtonTitle:@"CANCEL" otherButtonTitles:@"OK", @"Wha?!", nil];
		[alert show];
		[alert release];
	}
}

-(void)dealloc
{
    [super dealloc];
}

@end
