#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_HelpMgr.h"

@implementation View_HelpMgr

-(id)initWithApp:(AppDelegate*)_app
{
	HandleExceptionObjcBegin();
	
	if ((self = [super initWithNibName:@"HelpView_iPhone" bundle:nil]))
	{
		app = _app;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleBack:(id)sender
{
	[self dismissModalViewControllerAnimated:YES];
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_HelpMgr", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
