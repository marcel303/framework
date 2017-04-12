#import "Debugging.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "Types.h"
#import "View_Eyedropper.h"

#import "AppDelegate.h" // IMG()

#define IMG_BACK [UIImage imageNamed:@IMG("eyedropper")]

@implementation View_Eyedropper

@synthesize color;

- (id)init
{
	HandleExceptionObjcBegin();
	
	UIImage* image = IMG_BACK;
	
	CGRect frame = CGRectMake(0.0f, 0.0f, image.size.width, image.size.height);
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		[self setColor:[UIColor redColor]];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)setColor:(UIColor*)_color
{
	if ([color isEqual:_color])
		return;
	
	[color release];
	color = _color;
	[color retain];
	
	[self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();

	Assert(color != nil);
	
	LOG_DBG("eyedropper: drawRect", 0);
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	// draw selected color
	
	Vec2F mid(self.frame.size.width / 2.0f, self.frame.size.height / 2.0f);
	
	float RADIUS_I = 50.0f;
	float RADIUS_O = 66.0f;
	
	CGRect rectI = CGRectMake(mid[0] - RADIUS_I, mid[1] - RADIUS_I, RADIUS_I * 2.0f, RADIUS_I * 2.0f);
	CGRect rectO = CGRectMake(mid[0] - RADIUS_O, mid[1] - RADIUS_O, RADIUS_O * 2.0f, RADIUS_O * 2.0f);
	
	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGContextSetFillColorWithColor(ctx, color.CGColor);
//	CGContextFillRect(ctx, );
	CGContextFillEllipseInRect(ctx, rectO);
	CGFloat colorI[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	CGContextSetFillColor(ctx, colorI);
	CGContextFillEllipseInRect(ctx, rectI);
	
	[IMG_BACK drawAtPoint:CGPointMake(0.0f, 0.0f)];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_Eyedropper", 0);
	
	self.color = nil;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
