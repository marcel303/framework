#import <UIKit/UIKit.h>
#import "ConfigState.h"

#define CONFIG [NSUserDefaults standardUserDefaults]

static bool ConfigExists(const char* name)
{
	return [CONFIG objectForKey:[NSString stringWithCString:name encoding:NSASCIIStringEncoding]] != nil;
}

void ConfigSetInt(const char* name, int value)
{
//	NSLog(@"SetInt: %@=%d", name, value);
	
	[CONFIG setInteger:value forKey:[NSString stringWithCString:name encoding:NSASCIIStringEncoding]];
}

void ConfigSetString(const char* name, const char* value)
{
	[CONFIG setObject:[NSString stringWithCString:value encoding:NSASCIIStringEncoding] forKey:[NSString stringWithCString:name encoding:NSASCIIStringEncoding]];
}

int ConfigGetInt(const char* name)
{
	return [CONFIG integerForKey:[NSString stringWithCString:name encoding:NSASCIIStringEncoding]];
}

int ConfigGetIntEx(const char* name, int _default)
{
	if (!ConfigExists(name))
		return _default;
	
	return ConfigGetInt(name);
}

FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigGetString(const char* name)
{
	return [[CONFIG stringForKey:[NSString stringWithCString:name encoding:NSASCIIStringEncoding]] cStringUsingEncoding:NSASCIIStringEncoding];
}
			
FixedSizeString<MAX_CONFIG_STRING_SIZE> ConfigGetStringEx(const char* name, const char* _default)
{
	if (!ConfigExists(name))
		return _default;
	
	return ConfigGetString(name);
}

void ConfigLoad()
{
	[CONFIG synchronize];
}

void ConfigSave(bool force)
{
	if (force)
	{
		[CONFIG synchronize];
	}
	else
	{
		// synchronize is slow and blocking, so let iOS do its periodic sync in the background
	}
}
