#import "Events.h"

@implementation Events

+(void)post:(NSString*)name
{
	[[NSNotificationCenter defaultCenter] postNotificationName:name object:nil];
}

@end
