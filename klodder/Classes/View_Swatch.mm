#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_Swatch.h"
#import "View_SwatchesMgr.h"

@implementation View_Swatch

-(id)initWithFrame:(CGRect)frame color:(Rgba)_color delegate:(id<SwatchDelegate>)_delegate
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		
		color = _color;
		delegate = _delegate;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	UIColor* temp = [UIColor colorWithRed:color.rgb[0] green:color.rgb[1] blue:color.rgb[2] alpha:color.rgb[3]];
												
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGContextSetStrokeColorWithColor(ctx, [UIColor blackColor].CGColor);
	CGContextSetFillColorWithColor(ctx, temp.CGColor);
	CGContextFillRect(ctx, self.bounds);
	CGContextStrokeRect(ctx, self.bounds);
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[delegate swatchColorSelect:color];
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_Swatch", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
