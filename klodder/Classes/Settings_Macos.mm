#import <Foundation/Foundation.h>
#import "Settings_Macos.h"

Settings_Macos gSettings;

#define CONFIG (NSUserDefaults*)mConfig

Settings_Macos::Settings_Macos()
{
	mConfig = [[NSUserDefaults alloc] init];
}

Settings_Macos::~Settings_Macos()
{
	[CONFIG release];
}

std::string Settings_Macos::GetString(std::string name, std::string _default)
{
	NSString* nsName = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	NSString* nsResult = [CONFIG stringForKey:nsName];
	
	if (nsResult == nil)
		return _default;
	else
		return [nsResult cStringUsingEncoding:NSASCIIStringEncoding];
}

void Settings_Macos::SetString(std::string name, std::string value)
{
	NSString* nsName = [NSString stringWithCString:name.c_str() encoding:NSASCIIStringEncoding];
	NSString* nsValue = [NSString stringWithCString:value.c_str() encoding:NSASCIIStringEncoding];
	
	[CONFIG setObject:nsValue forKey:nsName];
}
