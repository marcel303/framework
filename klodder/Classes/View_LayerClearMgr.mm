#import "AppDelegate.h"
#import "Application.h"
#import "ExceptionLoggerObjC.h"
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
		[self setWantsFullScreenLayout:YES];
		
		self.title = @"Clear layer";
		self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone target:self action:@selector(handleDone:)] autorelease];
		
		app = _app;
		index = _index;
		
		Rgba brushColor = app.mApplication->BrushColor_get();
		color = [[UIColor colorWithRed:brushColor.rgb[0] green:brushColor.rgb[1] blue:brushColor.rgb[2] alpha:brushColor.rgb[3]] retain];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)colorChanged
{
	View_ColorPicker* vw = (View_ColorPicker*)self.view;
	
	[vw updateUi];
}

-(void)clearLayer
{
	LOG_DBG("clearLayer", 0);
	
	const float* components = CGColorGetComponents(color.CGColor);
	
	app.mApplication->DataLayerClear(index, components[0], components[1], components[2], components[3]);
}

-(void)handleDone:(id)sender
{
	HandleExceptionObjcBegin();
	
	[self clearLayer];
	
	[self dismissModalViewControllerAnimated:TRUE];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[self setMenuOpaque];
	
	View_LayerClear* vw = (View_LayerClear*)self.view;
	[vw updateUi];

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
