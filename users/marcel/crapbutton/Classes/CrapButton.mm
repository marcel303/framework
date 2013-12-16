#import <AudioToolbox/AudioToolbox.h>
#import "CrapButton.h"

@implementation CrapButton

#define CONFIG [NSUserDefaults standardUserDefaults]

static void SetInt(NSString* name, int value)
{
	[CONFIG setInteger:value forKey:name];
}

static int GetInt(NSString* name)
{
	return [CONFIG integerForKey:name];
}

-(id)initWithCoder:(NSCoder *)aDecoder
{
	if ((self = [super initWithCoder:aDecoder]))
	{
		[self setOpaque:TRUE];
		[self setBackgroundColor:[UIColor whiteColor]];
		
		//Get a URL for the sound file
		NSURL* url = [NSURL fileURLWithPath:[[NSBundle mainBundle] pathForResource:@"alarm" ofType:@"wav"] isDirectory:NO];

		//Use audio sevices to create the sound
		AudioServicesCreateSystemSoundID((CFURLRef)url, &soundID);
		
		SetInt(@"count_s", 0);
		
		[self show:false];
    }
	
    return self;
}

-(void)show:(bool)enabled
{
	[imageView removeFromSuperview];
	
	NSString* fileName = enabled ? @"button1" : @"button0";
	
	UIImage* image = [[[UIImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:fileName ofType:@"png"]] autorelease];
	
	imageView = [[[UIImageView alloc] initWithImage:image] autorelease];
	
	int y = (self.frame.size.height - image.size.height) / 2;
	
	imageView.frame = CGRectMake(0.0f, y, image.size.width, image.size.height);
	
	[self addSubview:imageView];
}

-(void)handleButton
{
	int prevS = GetInt(@"count_s");
	int nextS = prevS + 1;
	int prevG = GetInt(@"count_g");
	int nextG = prevG + 1;
	
	SetInt(@"count_s", nextS);
	SetInt(@"count_g", nextG);
	
	if (nextS == 20)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Congrats!" message:@"Archievement Unlocked: Slightly Bored" delegate:nil cancelButtonTitle:@"Yay!" otherButtonTitles:nil] autorelease] show];
	}
	if (nextS == 100)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Congrats!" message:@"Archievement Unlocked: Bored 100%!" delegate:nil cancelButtonTitle:@"Yay!" otherButtonTitles:nil] autorelease] show];
	}
	if (nextS == 1000)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Congrats!" message:@"Archievement Unlocked: Total Boredom!" delegate:nil cancelButtonTitle:@"Yay!" otherButtonTitles:nil] autorelease] show];
	}
	if (nextG == 1000000)
	{
		[[[[UIAlertView alloc] initWithTitle:@"Congrats!" message:@"Archievement Unlocked: Get A Life!!" delegate:nil cancelButtonTitle:@"Yay!" otherButtonTitles:nil] autorelease] show];
	}
	
	NSLog(@"count: %d, %d", nextS, nextG);
	
	[self show:true];
	
	//Use audio services to play the sound
	AudioServicesPlaySystemSound(soundID);
	
	[timer invalidate];
	[timer release];
	
	timer = [[NSTimer scheduledTimerWithTimeInterval:1.0f target:self selector:@selector(handlePop) userInfo:nil repeats:NO] retain];
}

-(void)handlePop
{
	[self show:false];
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self handleButton];
}

-(void)dealloc 
{
    [super dealloc];
}

@end
