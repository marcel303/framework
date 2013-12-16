#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ImageButton.h"

@implementation View_ImageButton

-(id)initWithFrame:(CGRect)frame andImage:(NSString*)_name andDelegate:(id)_delegate andClicked:(SEL)_clicked
{
	HandleExceptionObjcBegin();
	
	UIImage* image = [UIImage imageNamed:_name];
	
	frame.size.width = image.size.width;
	frame.size.height = image.size.height;
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setMultipleTouchEnabled:FALSE];
		 
		name = _name;
		delegate = _delegate;
		clicked = _clicked;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[delegate performSelector:clicked];

	HandleExceptionObjcEnd(false);
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	UIImage* image = [UIImage imageNamed:name];
	
	if (image != nil)
	{
		[image drawAtPoint:CGPointMake(0.0f, 0.0f)];
	}
	else
	{
		CGContextRef ctx = UIGraphicsGetCurrentContext();
		CGContextSetFillColorWithColor(ctx, [UIColor redColor].CGColor);
		CGContextFillRect(ctx, self.bounds);
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ImageButton", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
