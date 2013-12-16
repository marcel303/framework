#import "MacImage.h"
#import "TestView.h"

@implementation TestView

-(id)initWithFrame:(CGRect)frame 
{
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:TRUE];
		[self setBackgroundColor:[UIColor whiteColor]];
		[self setClearsContextBeforeDrawing:TRUE];
		
		dstImage = new MacImage();
		srcImage = new MacImage();
		
		dstImage->Size_set(320, 480, false);
		
		srcImage->Size_set(200, 200, false);
		
		MacRgba* pixels = srcImage->Data_get();
		
		for (int y = srcImage->Sy_get(); y; --y)
		{
			for (int x = srcImage->Sx_get(); x; --x)
			{				
				pixels->rgba[0] = (1.0f + sinf(x / 1.0f)) / 2.0f * 255.0f;
				pixels->rgba[1] = (1.0f + sinf(y / 1.0f)) / 2.0f * 255.0f;
				pixels->rgba[2] = ((1.0f + sinf(y / 1.0f)) + (1.0f + sinf(y / 10.0f))) / 4.0f * 255.0f;
				pixels->rgba[3] = 255;
//				pixels->rgba[3] = (2.0f + (sinf(x / 4.0f) + sinf(y / 5.0f))) / 4.0f * 255.0f;
				
				++pixels;
			}
		}
		
		[NSTimer scheduledTimerWithTimeInterval:1.0f/200.0f target:self selector:@selector(update) userInfo:nil repeats:YES];
    }
	
    return self;
}

-(void)update
{
	static float t = 0.0f;
	t += 0.01f;
#define C(T) ((sinf(t / T * M_PI * 2.0f) + 1.0f) * 0.5f)
	
	[self setBackgroundColor:[UIColor colorWithRed:C(10.0f) green:C(11.0f) blue:C(13.0f) alpha:1.0f]];
	
	[self setNeedsDisplay];
}

-(void)drawRect:(CGRect)rect 
{
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	dstImage->Clear(MacRgba_Make(0, 0, 0, 0));
	
	Resample(srcImage, dstImage);
	
	CGImageRef cgImage = dstImage->ImageWithAlpha_get();
//	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGContextSetBlendMode(ctx, kCGBlendModeNormal);
	CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, dstImage->Sx_get(), dstImage->Sy_get()), cgImage);
	CGImageRelease(cgImage);
}

-(void)dealloc 
{
	delete dstImage;
	delete srcImage;
	
    [super dealloc];
}

@end
