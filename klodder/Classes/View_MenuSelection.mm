#import "AppDelegate.h"
#import "ExceptionLoggerObjC.h"
#import "View_MenuSelection.h"

#define MENU_COUNT 5
#define SELECTION_SIZE 15.0f
#define SPACING 10.0f
#define BLAAAAT 0.0f

#define IMG_MENU_INDICATOR [UIImage imageNamed:@IMG("button_done")]

@implementation View_MenuSelection

-(id)initWithIndex:(int)_index frame:(CGRect)frame app:(AppDelegate*)_app controller:(UIViewController*)_controller
{	
	HandleExceptionObjcBegin();
	
	float sx = [UIScreen mainScreen].applicationFrame.size.width;
	float sy = 50.0f;
	
	frame.origin.y = frame.size.height - sy;
	frame.origin.x = 0.0f;
	frame.size.width = sx;
	frame.size.height = sy;
	//frame.size = image.size;
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setOpaque:FALSE];
		[self setBackgroundColor:[UIColor clearColor]];
		
		index = _index;
		app = _app;
		controller = _controller;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	float dx = floorf((self.frame.size.width - [IMG_MENU_INDICATOR size].width) / 2.0f);
	float dy = floorf((self.frame.size.height - [IMG_MENU_INDICATOR size].height) / 2.0f);
	
	[IMG_MENU_INDICATOR drawAtPoint:CGPointMake(dx, dy)];
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	[controller dismissModalViewControllerAnimated:YES];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_MenuSelection", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
