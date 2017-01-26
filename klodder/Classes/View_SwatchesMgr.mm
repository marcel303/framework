#import "AppDelegate.h"
#import "Application.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_Swatches.h"
#import "View_SwatchesMgr.h"

#import "Calc.h"

@implementation View_SwatchesMgr

-(Rgba)swatchColorProvide:(int)index
{
	int count = app.mApplication->SwatchMgr_get()->SwatchCount_get();
	
	if (index >= count)
		return Rgba_Make(1.0f, 1.0f, 1.0f);
	
	MacRgba color = app.mApplication->SwatchMgr_get()->Swatch_get(index).Color_get();
	
	return Rgba_Make(color.rgba[0] / 255.0f, color.rgba[1] / 255.0f, color.rgba[2] / 255.0f, color.rgba[3] / 255.0f);
}

-(void)swatchColorSelect:(Rgba)color
{
	app.colorPickerState->Color_set(color);
	app.colorPickerState->Opacity_set(color.rgb[3]);
	[app applyColor];
	
	[self dismissModalViewControllerAnimated:TRUE];
}

-(void)loadView
{
	HandleExceptionObjcBegin();
	
	self.view = [[[View_Swatches alloc] initWithFrame:[UIScreen mainScreen].applicationFrame delegate:self] autorelease];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_SwatchesMgr", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
