#import "AppDelegate.h"
#import "Application.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ColorPicker.h"
#import "View_ColorPickerMgr.h"

@implementation View_ColorPickerMgr

-(id)initWithApp:(AppDelegate *)_app
{
	HandleExceptionObjcBegin();
	
	if (self = [super initWithApp:_app])
	{
		//[self setModalInPopover:YES];
		[self setPreferredContentSize:CGSizeMake(320.0f, 480.0f)];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)loadView
{
	HandleExceptionObjcBegin();
	
#ifdef IPAD
	CGRect rect = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f);
#else
	CGRect rect = [UIScreen mainScreen].applicationFrame;
#endif
	
	self.view = [[[View_ColorPicker alloc] initWithFrame:rect app:app controller:self delegate:self] autorelease];

	HandleExceptionObjcEnd(false);
}

-(void)handleDone
{
	HandleExceptionObjcBegin();
	
	[self dismissModalViewControllerAnimated:YES];
	
	HandleExceptionObjcEnd(false);
}

-(void)applyColor
{
	[app applyColor];
}

-(PickerState*)colorPickerState
{
	return app.colorPickerState;
}

-(void)colorChanged
{
	View_ColorPicker* vw = (View_ColorPicker*)self.view;
	
	[vw updateUi];
}

-(void)openingSwatches
{
	[self applyColor];
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	Rgba color = app.mApplication->BrushColor_get();
	color.rgb[3] = app.mApplication->BrushOpacity_get();
	
	LOG_DBG("updating picker color: %f, %f, %f @ %f", color.rgb[0], color.rgb[1], color.rgb[2], color.rgb[3]);
	
	[self colorPickerState]->Color_set(color);
	
	View_ColorPicker* vwColorPicker = (View_ColorPicker*)self.view;
	
	[vwColorPicker updateUi];
	
	[super viewDidAppear:animated];

	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[self applyColor];
	
	[super viewWillDisappear:animated];

	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ColorPickerMgr", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
