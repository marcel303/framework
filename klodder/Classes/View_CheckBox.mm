#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_CheckBox.h"

@implementation View_CheckBox

@synthesize checked;

-(id)initWithLocation:(CGPoint)location andDelegate:(id)delegate andCallback:(SEL)changed
{
	HandleExceptionObjcBegin();
	
	CGRect frame = CGRectMake(location.x, location.y, 15.0f, 15.0f);
	
	if (self = [super initWithFrame:frame])
	{
		mDelegate = delegate;
		mChanged = changed;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	HandleExceptionObjcBegin();
	
	checked = !checked;
	
	[mDelegate performSelector:mChanged withObject:[NSNumber numberWithBool:checked]];
	
	HandleExceptionObjcEnd(false);
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	// todo: draw checkbox graphic

	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_CheckBox", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
