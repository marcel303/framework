#import <AudioToolbox/AudioToolbox.h>
#include "Sound.h"

void PlaySound(const char* name)
{
	NSString* fileName = [NSString stringWithCString:name encoding:NSASCIIStringEncoding];
	NSString* path = [[NSBundle mainBundle] pathForResource:fileName ofType:@"wav"];

	SystemSoundID soundId;

	NSURL* url = [NSURL fileURLWithPath:path isDirectory:NO];

	AudioServicesCreateSystemSoundID((CFURLRef)url, &soundId);

	AudioServicesPlaySystemSound(soundId);
}
