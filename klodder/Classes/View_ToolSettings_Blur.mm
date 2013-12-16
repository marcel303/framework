#import "ExceptionLoggerObjC.h"
#import "View_ToolSettings_Blur.h"

#define IMG_BACK [UIImage imageNamed:@"back_pattern.png"]

@implementation View_ToolSettings_Blur

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app controller:(View_ToolSelectMgr*)_controller
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		controller = _controller;
		
		mToolType = [[View_ToolType alloc] initWithFrame:CGRectMake(0.0f, 0.0f, 320.0f, 50.0f) toolType:ToolViewType_Brush delegate:controller];
		[self addSubview:mToolType];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	[IMG_BACK drawAsPatternInRect:self.frame];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_BlurSettings", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
