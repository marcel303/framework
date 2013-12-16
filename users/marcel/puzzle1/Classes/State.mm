#import <UIKit/UIKit.h>
#import "State.h"

#define CONFIG [NSUserDefaults standardUserDefaults]

void SetInt(NSString* name, int value)
{
	NSLog(@"SetInt: %@=%d", name, value);
	
	[CONFIG setInteger:value forKey:name];
	
	[CONFIG synchronize];
}

int GetInt(NSString* name)
{
	return [CONFIG integerForKey:name];
}
