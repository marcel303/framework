#import "AppDelegate.h"
#import "Application.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ColorPicker.h"
#import "View_LayerClear.h"
#import "View_LayerClearMgr.h"

@implementation View_LayerClearMgr

@synthesize color;

-(id)initWithApp:(AppDelegate*)_app index:(int)_index;
{
	HandleExceptionObjcBegin();
	
	if ((self = [super initWithNibName:@"LayerClear_iPhone" bundle:nil]))
	{
		//[self setFullScreenLayout];
		
		self.title = @"Clear layer";
		self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(handleDone:)] autorelease];
		
		app = _app;
		index = _index;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)colorChanged
{
	View_LayerClear* vw = (View_LayerClear*)self.view;
	
	[vw updateUi];
}

-(void)clearLayer
{
	LOG_DBG("clearLayer", 0);
	
	const CGFloat* components = CGColorGetComponents(color.CGColor);
	
	app.mApplication->DataLayerClear(index, components[0], components[1], components[2], components[3]);
}

-(void)handleDone:(id)sender
{
	HandleExceptionObjcBegin();
	
	[self clearLayer];
	
	[self dismissViewControllerAnimated:TRUE completion:NULL];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[self setMenuOpaque];
    
    Rgba brushColor = app.mApplication->BrushColor_get();
    color = [[UIColor colorWithRed:brushColor.rgb[0] green:brushColor.rgb[1] blue:brushColor.rgb[2] alpha:brushColor.rgb[3]] retain];
    [self colorChanged];

	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_LayerClearMgr", 0);
	
	self.color = nil;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
