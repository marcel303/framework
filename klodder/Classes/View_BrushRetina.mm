#import <QuartzCore/CALayer.h>
#import "ExceptionLoggerObjC.h"
#import "View_BrushRetina.h"

#define STROKE 1.0f

@implementation View_BrushRetina

-(id)initWithRadius:(float)radius
{
	HandleExceptionObjcBegin();
	
	const float size = 1.0f + radius * 2.0f + STROKE * 2.0f;
	
	CGRect frame;
	
	if (radius >= 20.0f && size <= 200.0f)
	{
		visible = true;
		frame = CGRectMake(0.0f, 0.0f, size, size);
	}
	else
	{
		visible = false;
		frame = CGRectMake(0.0f, 0.0f, 4.0f, 4.0f);
	}
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setClearsContextBeforeDrawing:TRUE];
		
		[self setCenter:CGPointMake(0.0f, 0.0f)];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)setLocation:(Vec2F)location
{
	[self.layer setPosition:location.ToCgPoint()];
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	if (!visible)
		return;
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGContextSetLineWidth(ctx, STROKE);
	CGContextSetStrokeColorWithColor(ctx, [UIColor grayColor].CGColor);
	CGRect circleRect = CGRectMake(1.0f + STROKE, 1.0f + STROKE, self.bounds.size.width - 2.0f - STROKE * 2.0f, self.bounds.size.height - 2.0f - STROKE * 2.0f);
	CGContextStrokeEllipseInRect(ctx, circleRect);
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
