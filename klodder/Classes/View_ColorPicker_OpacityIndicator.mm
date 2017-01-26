#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ColorPicker_OpacityIndicator.h"

#import "AppDelegate.h" // IMG()

#define IMG_CIRCLE [UIImage imageNamed:@IMG("indicator_circle")]

@implementation View_ColorPicker_OpacityIndicator

-(id)initWithDelegate:(id<PickerDelegate>)_delegate;
{
	HandleExceptionObjcBegin();
	
	UIImage* circle = IMG_CIRCLE;
	
	CGRect frame = CGRectMake(0.0f, 0.0f, circle.size.width, circle.size.height);
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		delegate = _delegate;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

- (void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	Rgba color = delegate.colorPickerState->Color_get();
	float opacity = delegate.colorPickerState->Opacity_get();
	
	float rgba[4];
	float radius = 8.0f;
	Vec2F mid(self.frame.size.width / 2.0f, self.frame.size.height / 2.0f);
	CGRect area = CGRectMake(mid[0] - radius, mid[1] - radius, radius * 2.0f, radius * 2.0f);
	
	// draw colour over white
	
	rgba[0] = 1.0f * (1.0f - opacity) + color.rgb[0] * opacity;
	rgba[1] = 1.0f * (1.0f - opacity) + color.rgb[1] * opacity;
	rgba[2] = 1.0f * (1.0f - opacity) + color.rgb[2] * opacity;
	rgba[3] = 1.0f;
	CGContextSaveGState(ctx);
	CGContextClipToRect(ctx, CGRectMake(0.0f, 0.0f, self.frame.size.width, self.frame.size.height / 2.0f));
	CGContextSetFillColor(ctx, rgba);
	CGContextFillEllipseInRect(ctx, area);
	CGContextRestoreGState(ctx);
	
	// draw colour over black
	rgba[0] = 0.0f * (1.0f - opacity) + color.rgb[0] * opacity;
	rgba[1] = 0.0f * (1.0f - opacity) + color.rgb[1] * opacity;
	rgba[2] = 0.0f * (1.0f - opacity) + color.rgb[2] * opacity;
	rgba[3] = 1.0f;
	CGContextSaveGState(ctx);
	CGContextClipToRect(ctx, CGRectMake(0.0f, self.frame.size.height / 2.0f, self.frame.size.width, self.frame.size.height / 2.0f));
	CGContextSetFillColor(ctx, rgba);
	CGContextFillEllipseInRect(ctx, area);
	CGContextRestoreGState(ctx);
	
	[IMG_CIRCLE drawAtPoint:CGPointMake(0.0f, 0.0f)];

	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ColorPicker_OpacityIndicator", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
