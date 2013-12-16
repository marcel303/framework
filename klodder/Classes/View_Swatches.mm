#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_Swatch.h"
#import "View_Swatches.h"

#import "AppDelegate.h" // IMG()

#define COUNT_X 5
#define COUNT_Y 8

#define SWATCH_SX 50
#define SWATCH_SY 40
#define SWATCH_SPACING 10

#define IMG_BACK [UIImage imageNamed:@IMG("back_pattern")]

@implementation View_Swatches

-(id)initWithFrame:(CGRect)frame delegate:(id<SwatchDelegate>)_delegate
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:TRUE];
		[self setClearsContextBeforeDrawing:FALSE];
		
		delegate = _delegate;
		
		[self updateUi];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)updateUi
{
	int index = 0;
	
	const int sx = COUNT_X * SWATCH_SX + (COUNT_X - 1) * SWATCH_SPACING;
	const int sy = COUNT_Y * SWATCH_SY + (COUNT_Y - 1) * SWATCH_SPACING;
	
	const int x0 = (self.bounds.size.width - sx) / 2;
	const int y0 = (self.bounds.size.height - sy) / 2;
	
	for (int y = 0; y < COUNT_Y; ++y)
	{
		for (int x = 0; x < COUNT_X; ++x)
		{
			CGRect swatchFrame = CGRectMake(x0 + x * (SWATCH_SX + SWATCH_SPACING), y0 + y * (SWATCH_SY + SWATCH_SPACING), SWATCH_SX, SWATCH_SY);
			Rgba swatchColor = [delegate swatchColorProvide:index];
			
			View_Swatch* swatch = [[[View_Swatch alloc] initWithFrame:swatchFrame color:swatchColor delegate:delegate] autorelease];
			[self addSubview:swatch];
			
			index++;
		}
	}
}

-(void)drawRect:(CGRect)rect
{
	HandleExceptionObjcBegin();
	
	// draw background
	
	[IMG_BACK drawAsPatternInRect:CGRectMake(0.0f, 0.0f, self.frame.size.width, self.frame.size.height)];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_Swatches", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
