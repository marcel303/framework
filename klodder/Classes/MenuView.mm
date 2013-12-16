#if 0

#import "AppDelegate.h"
#import "ExceptionLogger.h"
#import "ImageView.h"
#import "MenuView.h"
#import "View_ColorPickerMgr.h"
#import "View_EditingMgr.h"
#import "View_LayersMgr.h"
#import "View_ToolSelectMgr.h"

#define IMG_BACK [UIImage imageNamed:@IMG("menu_stub")]

@implementation MenuView

static UIButton* MakeButton(id self, SEL action, float x, float y, float sx, float sy, NSString* text)
{
	UIButton* button = [UIButton buttonWithType:UIButtonTypeRoundedRect];
	[button setTitle:text forState:UIControlStateNormal];
	[button setFrame:CGRectMake(x, y, sx, sy)];
	[button setTitleColor:[UIColor greenColor] forState:UIControlStateNormal];
	[button addTarget:self action:action forControlEvents:UIControlEventTouchUpInside];
	return button;
}

-(id)initWithFrame:(CGRect)frame andApp:(AppDelegate*)app
{
    if ((self = [super initWithFrame:frame])) 
	{
		mAppDelegate = app;
		
		[self addSubview:MakeButton(self, @selector(handleZoomIn), 0.0f, 0.0f, 60.0f, 30.0f, @"Z-IN")];
		[self addSubview:MakeButton(self, @selector(handleZoomOut), 60.0f, 0.0f, 60.0f, 30.0f, @"Z-OUT")];
		[self addSubview:MakeButton(self, @selector(handleColorPicker), 180.0f, 0.0f, 60.0f, 30.0f, @"test 2")];
		[self addSubview:MakeButton(self, @selector(handleEditing), 240.0f, 0.0f, 60.0f, 30.0f, @"test 3")];
    }
	
    return self;
}

-(void)drawRect:(CGRect)rect
{
	try
	{
		[IMG_BACK drawAtPoint:CGPointMake(0.0f, 0.0f)];
	}
	catch (std::exception& e)
	{
		ExceptionLogger::Log(e);
	}
}

-(void)dealloc
{
	LOG_DBG("dealloc: MenuView", 0);
	
    [super dealloc];
}

-(void)handleZoomIn
{
#if 0
#warning
	UIImagePickerController* ip = [[UIImagePickerController alloc] init];
	[ip setSourceType:UIImagePickerControllerSourceTypePhotoLibrary];
	[mAppDelegate.vcEditing presentModalViewController:ip animated:TRUE];
	[ip release];
	return;
#endif
	
//	[mAppDelegate->mViewEditing zoomIn];
//	[mAppDelegate show:mAppDelegate.vcBrushSettings];
	[mAppDelegate show:mAppDelegate.vcToolSelect];
//	mAppDelegate->mApplication->ToolType_set(ToolType_Brush);
}

-(void)handleZoomOut
{
	[mAppDelegate show:mAppDelegate.vcLayers];
}

-(void)handleColorPicker
{
	[mAppDelegate show:mAppDelegate.vcColorPicker];
}

-(void)handleEditing
{
//	[mAppDelegate show:mAppDelegate.vcEditing];
}

@end

#endif
