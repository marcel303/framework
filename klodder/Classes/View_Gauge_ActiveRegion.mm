#import "ExceptionLoggerObjC.h"
#import "View_Gauge.h"
#import "View_Gauge_ActiveRegion.h"

#import "AppDelegate.h" // IMG()

@implementation View_Gauge_ActiveRegion

-(id)initWithFrame:(CGRect)frame gauge:(View_Gauge*)_gauge
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		gauge = _gauge;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

- (void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	UIImage* back = IMG_GAUGE_BACK;
	int y = (self.frame.size.height - IMG_GAUGE_BACK.size.height) / 2.0f;
	int sx = self.frame.size.width;
	int sy = back.size.height;
	
	// draw fill
	
	CGColorRef color;
	if (gauge.enabled)
		color = [[UIColor colorWithRed:1.0f green:1.0f blue:0.0f alpha:0.3f] CGColor];
	else
		color = [[UIColor colorWithRed:0.7 green:0.7f blue:0.7f alpha:0.3f] CGColor];
	
	CGContextSetFillColorWithColor(ctx, color);
	CGContextFillRect(ctx, CGRectMake(0.0f, y, gauge.fill * sx, sy));
								   
	// draw overlay image
	[IMG_GAUGE_BACK drawAtPoint:CGPointMake(0.0f, y)];
	
	HandleExceptionObjcEnd(false);
}

-(void)update:(CGPoint)location
{
	float t = location.x / self.frame.size.width;
	
	int value = [gauge fillToValue:t];

	[gauge setValue:value];
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	if (!gauge.enabled)
		return;
	
	[self update:[[touches anyObject] locationInView:self]];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();

	if (!gauge.enabled)
		return;
	
	[self update:[[touches anyObject] locationInView:self]];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
