#import "AppDelegate.h"
#import "ExceptionLoggerObjC.h"
#import "View_About.h"
#import "View_AboutMgr.h"

#define ABOUT_URL @"http://grannies-games.com/klodder/about-iphone.php"

@implementation View_AboutMgr

-(id)initWithApp:(AppDelegate*)_app
{
	HandleExceptionObjcBegin();
	
	if ((self = [super initWithNibName:@"AboutView_iPhone" bundle:nil]))
	{
		app = _app;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleBack:(id)sender
{
	[self dismissViewControllerAnimated:YES completion:NULL];
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	View_About* vw = (View_About*)self.view;
	[vw.webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:ABOUT_URL]]];
	
	[super viewWillAppear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[super viewWillDisappear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
