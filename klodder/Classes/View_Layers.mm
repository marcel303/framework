#import "AppDelegate.h"
#import "Application.h"
#import "Calc.h"
#import "ExceptionLoggerObjC.h"
#import "LayerMgr.h"
#import "Log.h"
#import "View_DefineBrush.h"
#import "View_MenuSelection.h"
#import "View_Gauge.h"
#import "View_LayerManager.h"
#import "View_LayerManagerLayer.h"
#import "View_Layers.h"
#import "View_LayersMgr.h"

#define IMG_BACK [UIImage imageNamed:@IMG("back_pattern")]

@implementation View_Layers

@synthesize layerMgr;
@synthesize toolBar;
@synthesize navigationBar;

static UIButton* MakeButton(id self, SEL action, float x, float y, float sx, float sy, NSString* text)
{
	UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	[button setTitle:text forState:UIControlStateNormal];
	[button setFrame:CGRectMake(x, y, sx, sy)];
	[button setTitleColor:[UIColor greenColor] forState:UIControlStateNormal];
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)_app controller:(View_LayersMgr*)_controller
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		app = _app;
		controller = _controller;
		
#if 0
		[self addSubview:MakeButton(self, @selector(handleDefineBrush), 10.0f, 370.0f, 200.0f, 40.0f, @"Define brush..")];
#endif

		layerMgr = [[[View_LayerManager alloc] initWithFrame:frame app:app controller:controller] autorelease];
		[self addSubview:layerMgr];
		
		opacityGauge = [[[View_Gauge alloc] initWithLocation:CGPointMake(50.0f, 320.0f) scale:1.0f height:40.0f min:0 max:100 value:100 unit:@"%" delegate:self changed:@selector(handleOpacityChanged:) provideText:nil] autorelease];
		[self addSubview:opacityGauge];
		
#if 0
		toolBar = [[[UIToolbar alloc] initWithFrame:CGRectMake(0.0f, self.bounds.size.height - 40.0f, self.bounds.size.width, 40.0f)] autorelease];
		[self addSubview:toolBar];
		
		navigationBar = [[[UINavigationBar alloc] initWithFrame:CGRectMake(0.0f, 0.0f, self.bounds.size.width, 40.0f)] autorelease];
		UINavigationItem* item = [[[UINavigationItem alloc] init] autorelease];
		item.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithTitle:@"Back" style:UIBarButtonItemStyleBordered target:self action:@selector(handleBack)] autorelease];
		[item setTitle:@"Layers"];
		[navigationBar setItems:[NSArray arrayWithObject:item]];
		[self addSubview:navigationBar];
#endif
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleFocus
{
	[layerMgr handleFocus];
}

-(void)handleBack
{
	[controller handleBack];
}

-(void)updateLayerOrder
{
	[layerMgr alignLayers:TRUE animated:TRUE];
}

-(void)updatePreviewPictures
{
	[layerMgr updatePreviewPictures];
}

-(void)updateUi
{	
	if (controller.focusLayerIndex >= 0)
	{
		View_LayerManagerLayer* layer = [layerMgr layerByIndex:controller.focusLayerIndex];
		
		float opacity = 1.0f;
		
		if (layer.previewOpacity.HasValue_get())
			opacity = layer.previewOpacity.Value_get();
		else
			opacity = app.mApplication->LayerMgr_get()->DataLayerOpacity_get(layer.index);
		
		[opacityGauge setValueDirect:int(opacity * 100.0f)];
		[opacityGauge setEnabled:true];
	}
	else
	{
		[opacityGauge setEnabled:false];
	}
	
	[layerMgr updateUi];
}

-(void)handleOpacityChanged:(NSNumber*)value
{
	[controller handleLayerOpacityChanged:value.intValue / 100.0f];
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
	
	LOG_DBG("dealloc: View_Layers", 0);
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
