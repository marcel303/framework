#import <QuartzCore/QuartzCore.h>
#import "Debugging.h"
#import "Log.h"
#import "SolutionView.h"

#define ANIM_DURATION 3.0f
#define ANIM_INTERVAL (1.0f / 60.0f)

@implementation SolutionView

-(id)initWithFrame:(CGRect)frame image:(UIImage*)_image
{
    if ((self = [super initWithFrame:frame])) 
	{
//		[self setBackgroundColor:[UIColor blackColor]];
//		[self setClearsContextBeforeDrawing:YES];
		[self setOpaque:FALSE];
		image = _image;
		[self animationBegin];
    }
	
    return self;
}

-(void)animationBegin
{
	animationTimer = [NSTimer scheduledTimerWithTimeInterval:ANIM_INTERVAL target:self selector:@selector(animationUpdate) userInfo:nil repeats:YES];
	animationCounter = ANIM_DURATION;
}

-(void)animationEnd
{
	if (animationTimer == nil)
		return;
	
	[animationTimer invalidate];
	animationTimer = nil;
}

-(void)animationUpdate
{
	animationCounter -= ANIM_INTERVAL;
	
	float opacity = animationCounter;
	
	if (opacity > 1.0f)
		opacity = 1.0f;
	
	[self.layer setOpacity:opacity];
	
	if (animationCounter <= 0.0f)
		[self removeFromSuperview];
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[self removeFromSuperview];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
}

static CGRect CGRectShrink(CGRect rect, float amount)
{
	rect.origin.x += amount;
	rect.origin.y += amount;
	
	rect.size.width -= amount * 2.0f;
	rect.size.height -= amount * 2.0f;
	
	return rect;
}

-(void)drawRect:(CGRect)rect 
{
	const int sx = image.size.width / 2;
	const int sy = image.size.height / 2;
	
	CGRect previewRect = CGRectMake((self.bounds.size.width - sx) / 2, (self.bounds.size.height - sy) / 2, sx, sy);
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGContextSetFillColorWithColor(ctx, [UIColor colorWithRed:0.0f green:0.0f blue:0.0f alpha:0.5f].CGColor);
	CGContextFillRect(ctx, self.bounds);
	
	[image drawInRect:previewRect];
	
	CGContextSetStrokeColorWithColor(ctx, [UIColor blackColor].CGColor);
	CGContextSetLineWidth(ctx, 4.0f);
	CGContextStrokeRect(ctx, CGRectShrink(previewRect, 2.0f));
	
	CGContextSetStrokeColorWithColor(ctx, [UIColor whiteColor].CGColor);
	CGContextSetLineWidth(ctx, 2.0f);
	CGContextStrokeRect(ctx, CGRectShrink(previewRect, 2.0f));
}

-(void)dealloc 
{
	LOG_DBG("dealloc: SolutionView", 0);
	
	[self animationEnd];
	
	Assert(animationTimer == nil);
	
	image = nil;
	
    [super dealloc];
}

@end
