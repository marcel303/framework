#import "ExceptionLoggerObjC.h"
#import "View_Gauge.h"
#import "View_Gauge_Button.h"

#import "AppDelegate.h" // IMG()

#define IMG_GAUGE_BUTTON_LESS_0 [UIImage imageNamed:@IMG("gauge_button_less_0")]
#define IMG_GAUGE_BUTTON_LESS_1 [UIImage imageNamed:@IMG("gauge_button_less_1")]
#define IMG_GAUGE_BUTTON_MORE_0 [UIImage imageNamed:@IMG("gauge_button_more_0")]
#define IMG_GAUGE_BUTTON_MORE_1 [UIImage imageNamed:@IMG("gauge_button_more_1")]

@implementation View_Gauge_Button

-(id)initWithFrame:(CGRect)frame gauge:(View_Gauge*)_gauge direction:(int)_direction
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		[self setMultipleTouchEnabled:FALSE];
		
		gauge = _gauge;
		direction = _direction;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	UIImage* back;
	
	if (direction < 0)
	{
		if (gauge.enabled)
			back = IMG_GAUGE_BUTTON_LESS_1;
		else
			back = IMG_GAUGE_BUTTON_LESS_0;
	}
	else
	{
		if (gauge.enabled)
			back = IMG_GAUGE_BUTTON_MORE_1;
		else
			back = IMG_GAUGE_BUTTON_MORE_0;
	}
	
	int x = (self.frame.size.width - back.size.width) / 2.0f;
	int y = (self.frame.size.height - back.size.height) / 2.0f;
	
	[back drawAtPoint:CGPointMake(x, y)];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	if (!gauge.enabled)
		return;
	
	// make a single adjustment..
	
	[gauge setValue:gauge.value + direction];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
