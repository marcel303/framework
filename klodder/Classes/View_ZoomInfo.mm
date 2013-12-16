#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ZoomInfo.h"

@implementation View_ZoomInfo

@synthesize zoom;

-(id)initWithFrame:(CGRect)frame 
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setUserInteractionEnabled:FALSE];
		
		label = [[UILabel alloc] initWithFrame:frame];
		[label setOpaque:FALSE];
		[label setBackgroundColor:[UIColor clearColor]];
		[label setTextColor:[UIColor blueColor]];
		[label setTextAlignment:UITextAlignmentCenter];
		[label setFont:[UIFont fontWithName:@"Helvetica" size:14.0f]];
		[self addSubview:label];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)setZoom:(float)_zoom
{
	zoom = _zoom;
	
	[label setText:[NSString stringWithFormat:@"Zoom: %d%%", (int)(zoom * 100.0f)]];
}

static void DrawRoundedRect(CGContextRef context, float x, float y, float sx, float sy, float radius, float stroke, UIColor* fillColor, UIColor* strokeColor)
{	
    CGContextSetLineWidth(context, stroke);
    CGContextSetFillColorWithColor(context, fillColor.CGColor);
    CGContextSetStrokeColorWithColor(context, strokeColor.CGColor);
//	CGContextSetAlpha(context, 0.5f);
    
	// clamp radius to size
	
    if (radius > sx * 0.5f)
        radius = sx * 0.5f;
    if (radius > sy * 0.5f)
        radius = sy * 0.5f;
    
    CGFloat minx = x + stroke;
    CGFloat midx = x + sx * 0.5f;
    CGFloat maxx = x + sx - stroke;
    CGFloat miny = y + stroke;
    CGFloat midy = y + sy * 0.5f;
    CGFloat maxy = y + sy - stroke;
	
    CGContextMoveToPoint(context, minx, midy);
    CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
    CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
    CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
    CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
    CGContextClosePath(context);
	
    CGContextDrawPath(context, kCGPathFillStroke);
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	CGContextRef context = UIGraphicsGetCurrentContext();
	
	DrawRoundedRect(context, 0.0f, 0.0f, self.frame.size.width, self.frame.size.height, 10.0f, 1.5f, [UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:0.7f], [UIColor redColor]);
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ZoomInfo", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
