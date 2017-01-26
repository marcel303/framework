#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ToolSelect_NavBar.h"

@implementation View_ToolSelect_NavBar

-(id)initWithFrame:(CGRect)frame name:(NSString*)name controller:(View_ToolSelectMgr*)_controller
{
	HandleExceptionObjcBegin();
	
	frame.size.height = 44.0f;
	
    if ((self = [super initWithFrame:frame])) 
	{
		controller = _controller;
		
		UINavigationBar* navigationBar = [[[UINavigationBar alloc] initWithFrame:self.bounds] autorelease];
		[navigationBar setBarStyle:UIBarStyleDefault];
		UINavigationItem* item = [[[UINavigationItem alloc] init] autorelease];
		item.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithTitle:@"Close" style:UIBarButtonItemStylePlain target:self action:@selector(handleDone)] autorelease];
		[item setTitle:name];
		[navigationBar setItems:[NSArray arrayWithObject:item]];
		[self addSubview:navigationBar];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleDone
{
	[controller dismissModalViewControllerAnimated:YES];
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ToolSelect_NavBar", 0);
		
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}


@end
