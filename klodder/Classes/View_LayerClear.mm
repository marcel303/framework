#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_LayerClear.h"
#import "View_LayerClearMgr.h"
#import "View_LayerClearPreview.h"

#import "AppDelegate.h" // IMG()

#define IMG_BACK [UIImage imageNamed:@IMG("back_pattern")]

@implementation View_LayerClear

@synthesize controller;
@synthesize sliderR;
@synthesize sliderG;
@synthesize sliderB;
@synthesize sliderA;
@synthesize preview;

-(void)updateUi
{
	const float* components = CGColorGetComponents(controller.color.CGColor);
	
	sliderR.value = components[0];
	sliderG.value = components[1];
	sliderB.value = components[2];
	sliderA.value = components[3];
	
	[preview setColor:controller.color];
}

-(void)handleColorChanged:(id)sender
{
	HandleExceptionObjcBegin();
	
	UIColor* color = [UIColor colorWithRed:sliderR.value green:sliderG.value blue:sliderB.value alpha:sliderA.value];
	
	controller.color = color;
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
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
	
	LOG_DBG("dealloc: View_LayerClear", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
