#import <AudioToolbox/AudioToolbox.h>
#import "PlaySound.h"

void PlaySound(const char* name)
{
	NSString* path = [[NSBundle mainBundle] pathForResource:[NSString stringWithCString:name encoding:NSASCIIStringEncoding] ofType:@"wav"];

	SystemSoundID soundId;

	NSURL* url = [NSURL fileURLWithPath:path isDirectory:NO];

	AudioServicesCreateSystemSoundID((CFURLRef)url, &soundId);

	AudioServicesPlaySystemSound(soundId);
}
