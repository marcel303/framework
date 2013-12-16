#import "ExceptionLogger.h"
#import "View_BarButtonItem_Color.h"

@implementation View_BarButtonItem_Color

-(id)initWithDelegate:(id)_delegate action:(SEL)_action color:(Rgba)_color opacity:(float)opacity
{
	CGRect frame = CGRectMake(0.0f, 0.0f, 20.0f, 20.0f);
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:TRUE];
		[self setBackgroundColor:[UIColor blackColor]];
		[self setClearsContextBeforeDrawing:FALSE];
		
		delegate = _delegate;
		action = _action;
		color = _color;
#if 0
		color.rgb[0] *= opacity;
		color.rgb[1] *= opacity;
		color.rgb[2] *= opacity;
#endif
		color.rgb[3] = opacity;
    }
	
    return self;
}

-(void)drawRect:(CGRect)rect 
{
	try
	{
		CGContextRef ctx = UIGraphicsGetCurrentContext();
		
		CGRect r = CGRectMake(0.0f, 0.0f, self.frame.size.width, self.frame.size.height);
		CGRect r1 = CGRectMake(0.0f, 0.0f, self.frame.size.width, self.frame.size.height / 2.0f);
		CGRect r2 = CGRectMake(0.0f, self.frame.size.height / 2.0f, self.frame.size.width, self.frame.size.height / 2.0f);
		
		CGContextSetFillColorWithColor(ctx, [UIColor whiteColor].CGColor);
		UIRectFill(r1);
		CGContextSetFillColorWithColor(ctx, [UIColor blackColor].CGColor);
		UIRectFill(r2);
		
		CGContextSetFillColorWithColor(ctx, [UIColor colorWithRed:color.rgb[0] green:color.rgb[1] blue:color.rgb[2] alpha:color.rgb[3]].CGColor);
	//	CGContextSetAlpha(ctx, color.rgb[3]);
		UIRectFillUsingBlendMode(r, kCGBlendModeSourceAtop);
		CGContextSetStrokeColorWithColor(ctx, [UIColor whiteColor].CGColor);
		UIRectFrame(r);
	}
	catch (std::exception& e)
	{
		ExceptionLogger::Log(e);
	}
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	try
	{
		[delegate performSelector:action];
	}
	catch (std::exception& e)
	{
		ExceptionLogger::Log(e);
	}
}

-(void)dealloc 
{
    [super dealloc];
}

@end
