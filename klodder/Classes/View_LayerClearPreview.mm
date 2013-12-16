#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_LayerClearPreview.h"

@implementation View_LayerClearPreview

@synthesize color;

-(id)initWithCoder:(NSCoder*)aDecoder
{
	HandleExceptionObjcBegin();
	
	if ((self = [super initWithCoder:aDecoder]))
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		self.color = [UIColor blackColor];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)setColor:(UIColor*)_color
{
	[color release];
	color = _color;
	[color retain];
	
	[self setNeedsDisplay];
}

-(void)drawRect:(CGRect)rect
{
	HandleExceptionObjcBegin();
	
	UIColor* temp = color;
	
	if (temp == nil)
		temp = [UIColor blackColor];
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetFillColorWithColor(ctx, temp.CGColor);
	CGContextFillRect(ctx, self.bounds);
	
	CGContextSetStrokeColorWithColor(ctx, [UIColor blackColor].CGColor);
	CGContextStrokeRect(ctx, self.bounds);
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_LayerClearPreview", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
