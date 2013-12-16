#import "Debugging.h"
#import "ExceptionLoggerObjC.h"
#import "View_ColorPicker_ColorBar.h"

#import "AppDelegate.h" // IMG()

#define STROKE 1.0f
#define BASECOLOR_SX 60.0f
#define BASECOLOR_RADIUS 10.0f

#define IMG_OLD [UIImage imageNamed:@IMG("colorpicker_old")]
#define IMG_NEW [UIImage imageNamed:@IMG("colorpicker_new")]

//static void DrawRoundedRect(CGContextRef context, float x, float y, float sx, float sy, float radius, float stroke, UIColor* fillColor, UIColor* strokeColor);
static void DrawRoundedRect(CGContextRef context, float x, float y, float sx, float sy, float radius, float stroke, bool roundLeft, bool roundRight, UIColor* fillColor, UIColor* strokeColor);
static void DrawRect(CGContextRef context, float x, float y, float sx, float sy, float stroke, UIColor* fillColor, UIColor* strokeColor);

@implementation View_ColorPicker_ColorBar

@synthesize oldColor;
@synthesize newColor;

-(id)initWithFrame:(CGRect)frame delegate:(id<ColorButtonDelegate>)_delegate;
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setMultipleTouchEnabled:FALSE];
		
		delegate = _delegate;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)setOldColor:(UIColor*)color
{
	[oldColor release];
	oldColor = color;
	[oldColor retain];
	
	[self setNeedsDisplay];
}

-(void)setNewColor:(UIColor*)color
{
	[self setNewColorDirect:color];
	
	[delegate handleColorSelect:color];
}

-(void)setNewColorDirect:(UIColor*)color
{
	[newColor release];
	newColor = color;
	[newColor retain];
	
	[self setNeedsDisplayInRect:CGRectMake(BASECOLOR_SX, 0.0f, self.bounds.size.width - BASECOLOR_SX * 2.0f, self.bounds.size.height)];
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	Assert(oldColor != nil);
	Assert(newColor != nil);
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	float sx = self.bounds.size.width - STROKE * 2.0f;
	float sy = self.bounds.size.height - STROKE * 2.0f;
	float colorSx = (sx - BASECOLOR_SX * 2.0f) * 0.5f;
	
	float x0 = STROKE;
	float x = x0;
	float y = STROKE;
	
	DrawRoundedRect(ctx, x, y, BASECOLOR_SX, sy, BASECOLOR_RADIUS, STROKE, true, false, [UIColor whiteColor], nil);
	x += BASECOLOR_SX;
	DrawRect(ctx, x, y, colorSx, sy, STROKE, oldColor, [UIColor blackColor]);
	[IMG_OLD drawAtPoint:CGPointMake(x + colorSx - IMG_OLD.size.width - 2.0f, y + sy - IMG_OLD.size.height - 2.0f)];
	x += colorSx;
	DrawRect(ctx, x, y, colorSx, sy, STROKE, newColor, [UIColor blackColor]);
	[IMG_NEW drawAtPoint:CGPointMake(x + 2.0f, y + sy - IMG_NEW.size.height - 2.0f)];
	x += colorSx;
	DrawRoundedRect(ctx, x, y, BASECOLOR_SX, sy, BASECOLOR_RADIUS, STROKE, false, true, [UIColor blackColor], nil);
	x += BASECOLOR_SX;
	
	DrawRoundedRect(ctx, x0, y, sx, sy, BASECOLOR_RADIUS, STROKE, true, true, nil, [UIColor blackColor]);
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch* touch = [touches anyObject];
	
	CGPoint location = [touch locationInView:self];
	
	if (location.x < BASECOLOR_SX)
	{
		LOG_DBG("select white color", 0);
		
		[self setNewColor:[UIColor colorWithRed:1.0f green:1.0f blue:1.0f alpha:1.0f]];
	}
	else if (location.x > self.bounds.size.width - BASECOLOR_SX)
	{
		LOG_DBG("select black color", 0);
		
		[self setNewColor:[UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:1.0f]];
	}
	else
	{
		LOG_DBG("select color history", 0);
		
		[delegate handleColorHistorySelect];
	}
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	[oldColor release];
	oldColor = nil;
	[newColor release];
	newColor = nil;
	
	[super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end

#if 0
static void DrawRoundedRect(CGContextRef context, float x, float y, float sx, float sy, float radius, float stroke, UIColor* fillColor, UIColor* strokeColor)
{	
    CGContextSetLineWidth(context, stroke);
	if (fillColor != nil)
	    CGContextSetFillColorWithColor(context, fillColor.CGColor);
	if (strokeColor != nil)
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
	
	if (fillColor != nil)
		CGContextFillPath(context);
	if (strokeColor != nil);
		CGContextStrokePath(context);
	
    //CGContextDrawPath(context, kCGPathFillStroke);
}
#else
static void DrawRoundedRect(CGContextRef context, float x, float y, float sx, float sy, float radius, float stroke, bool roundLeft, bool roundRight, UIColor* fillColor, UIColor* strokeColor)
{
	CGContextSetAllowsAntialiasing(context, true);
	CGContextSetInterpolationQuality(context, kCGInterpolationHigh);
    CGContextSetLineWidth(context, stroke);
	if (fillColor != nil)
		CGContextSetFillColorWithColor(context, fillColor.CGColor);
	if (strokeColor != nil)
		CGContextSetStrokeColorWithColor(context, strokeColor.CGColor);
//	CGContextSetAlpha(context, 0.5f);
    
	// clamp radius to size
	
    if (radius > sx * 0.5f)
        radius = sx * 0.5f;
    if (radius > sy * 0.5f)
        radius = sy * 0.5f;

#if 0
    CGFloat minx = x + stroke;
    CGFloat midx = x + sx * 0.5f;
    CGFloat maxx = x + sx - stroke;
    CGFloat miny = y + stroke;
    CGFloat midy = y + sy * 0.5f;
    CGFloat maxy = y + sy - stroke;
#else
    CGFloat minx = x;
    CGFloat midx = x + sx * 0.5f;
    CGFloat maxx = x + sx;
    CGFloat miny = y;
    CGFloat midy = y + sy * 0.5f;
    CGFloat maxy = y + sy;
	
	LOG_DBG("path: (%f, %f) - (%f, %f)", minx, miny, maxx, maxy);
#endif
	
    CGContextMoveToPoint(context, minx, midy);
	
	if (roundLeft)
	    CGContextAddArcToPoint(context, minx, miny, midx, miny, radius);
	else
		CGContextAddLineToPoint(context, minx, miny);
	
	if (roundRight)
	    CGContextAddArcToPoint(context, maxx, miny, maxx, midy, radius);
	else
		CGContextAddLineToPoint(context, maxx, miny);
	
	if (roundRight)
	    CGContextAddArcToPoint(context, maxx, maxy, midx, maxy, radius);
	else
		CGContextAddLineToPoint(context, maxx, maxy);
	
	if (roundLeft)
	    CGContextAddArcToPoint(context, minx, maxy, minx, midy, radius);
	else
		CGContextAddLineToPoint(context, minx, maxy);
	
    CGContextClosePath(context);
	
	if (fillColor != nil)
		CGContextFillPath(context);
	if (strokeColor != nil);
		CGContextStrokePath(context);
	
    //CGContextDrawPath(context, kCGPathFillStroke);
}
#endif

static void DrawRect(CGContextRef context, float x, float y, float sx, float sy, float stroke, UIColor* fillColor, UIColor* strokeColor)
{
	CGContextSetLineWidth(context, 1.0f);
	CGContextSetBlendMode(context, kCGBlendModeNormal);
	
#if 0
//	x += stroke;
	y += stroke;
//	sx -= stroke * 2.0f;
	sy -= stroke * 2.0f;
#endif
	
	if (strokeColor != nil)
		CGContextSetStrokeColorWithColor(context, strokeColor.CGColor);
	if (fillColor != nil)
		CGContextSetFillColorWithColor(context, fillColor.CGColor);

	if (fillColor != nil)
		CGContextFillRect(context, CGRectMake(x, y, sx, sy));
	if (strokeColor != nil)
		CGContextStrokeRect(context, CGRectMake(x, y, sx, sy));
}
