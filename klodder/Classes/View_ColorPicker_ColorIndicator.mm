#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ColorPicker_ColorIndicator.h"

@implementation View_ColorPicker_ColorIndicator

-(id)initWithFrame:(CGRect)frame 
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
        // Initialization code
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ColorPicker_ColorIndicator", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
