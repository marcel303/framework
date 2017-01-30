#import "AppDelegate.h"
#import "Application.h"
#import "ExceptionLoggerObjC.h"
#import "KlodderSystem.h"
#import "LayerMgr.h"
#import "Log.h"
#import "View_BarButtonItem_Color.h"
#import "View_ColorPickerMgr.h"
#import "View_Editing.h"
#import "View_EditingImage.h"
#import "View_EditingMgr.h"
#import "View_LayersMgr.h"
#import "View_PictureGalleryMgr.h"
#import "View_ToolSelectMgr.h"

#import "BezierTraveller.h" // fixme, test code
#import "Calc.h" // fixme, test code

#define MENU_HIDE_INTERVAL 2.6f

@implementation View_EditingMgr

-(id)initWithApp:(AppDelegate*)_app
{
	HandleExceptionObjcBegin();
	
	if (self = [super initWithApp:_app])
	{
		[self setFullScreenLayout];
        
		self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithTitle:@"Back" style:UIBarButtonItemStylePlain target:self action:@selector(handleDoneAnimated)] autorelease];

		UIBarButtonItem* item_Done = [[[UIBarButtonItem alloc] initWithTitle:@"Done" style:UIBarButtonItemStylePlain target:self action:@selector(handleDoneAnimated)] autorelease];
		self.navigationItem.rightBarButtonItem = item_Done;
		self.title = @"Drawing";
		
		lastZoom = 4.0f;
		
		doneRequested = false;
		
#ifdef IPAD
		popoverController = nil;
#endif
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleColorPicker
{
	HandleExceptionObjcBegin();
	
#ifdef IPAD
	[self menuHideStop];
	
	if (popoverController != nil)
	{
		[popoverController dismissPopoverAnimated:NO];
		popoverController = nil;
	}
	
	if (popoverController == nil)
	{
		View_ColorPickerMgr* subController = [[[View_ColorPickerMgr alloc] initWithApp:app] autorelease];
		popoverController = [[UIPopoverController alloc] initWithContentViewController:subController];
		popoverController.delegate = self;
		[popoverController presentPopoverFromBarButtonItem:item_Color permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
	}
	else
	{
		[popoverController dismissPopoverAnimated:YES];
		popoverController = nil;
	}
#else
	View_ColorPickerMgr* subController = [[[View_ColorPickerMgr alloc] initWithApp:app] autorelease];
    [self presentViewController:subController animated:YES completion:NULL];
#endif
	
	HandleExceptionObjcEnd(false);
}

-(void)handleToolSettings
{
	HandleExceptionObjcBegin();
	
#ifdef IPAD
	[self menuHideStop];
	
	if (popoverController != nil)
	{
		[popoverController dismissPopoverAnimated:NO];
		popoverController = nil;
	}
	
	if (popoverController == nil)
	{
		View_ToolSelectMgr* subController = [[[View_ToolSelectMgr alloc] initWithApp:app] autorelease];
		popoverController = [[UIPopoverController alloc] initWithContentViewController:subController];
		popoverController.delegate = self;
		[popoverController presentPopoverFromBarButtonItem:item_Tool permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
	}
	else
	{
		[popoverController dismissPopoverAnimated:YES];
		popoverController = nil;
	}
#else
	View_ToolSelectMgr* subController = [[[View_ToolSelectMgr alloc] initWithApp:app] autorelease];
    [self presentViewController:subController animated:TRUE completion:NULL];
#endif
	
	HandleExceptionObjcEnd(false);
}

-(void)handleLayers
{
	HandleExceptionObjcBegin();

#ifdef IPAD
	[self menuHideStop];
	
	if (popoverController != nil)
	{
		[popoverController dismissPopoverAnimated:NO];
		popoverController = nil;
	}
	
	if (popoverController == nil)
	{
		View_LayersMgr* subController = [[[View_LayersMgr alloc] initWithApp:app] autorelease];
		popoverController = [[UIPopoverController alloc] initWithContentViewController:subController];
		popoverController.delegate = self;
		[popoverController presentPopoverFromBarButtonItem:item_Layers permittedArrowDirections:UIPopoverArrowDirectionAny animated:YES];
	}
	else
	{
		[popoverController dismissPopoverAnimated:YES];
		popoverController = nil;
	}
#else
	View_LayersMgr* subController = [[[View_LayersMgr alloc] initWithApp:app] autorelease];
	[self.navigationController pushViewController:subController animated:TRUE];
	//[self presentModalViewController:subController animated:TRUE];
#endif
	
	HandleExceptionObjcEnd(false);
}

-(void)handleUndo
{
	HandleExceptionObjcBegin();

	[self menuHideStart];
	
	app.mApplication->Undo();
	
	[[self getImageView] updatePaint];
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleRedo
{
	HandleExceptionObjcBegin();
	
	[self menuHideStart];
	
	app.mApplication->Redo();
	
	[[self getImageView] updatePaint];
	
	[self updateUi];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleDoneAnimated
{
	HandleExceptionObjcBegin();
	
	doneRequested = true;
	
	float zoom = [[self getImageView] snapZoom:[self getImageView].zoom];
	Vec2F pan = [[self getImageView] snapPan:[self getImageView].pan];
	
	[[self getImageView] setZoom:zoom animated:FALSE];
	[[self getImageView] setPan:pan animated:FALSE];
	
	[[self getImageView] transformToCenter:TRUE];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleDone
{
	HandleExceptionObjcBegin();
	
	View_Editing* vw = [self getView];
	[vw activityIndicatorShow];
	
//	[self performSelectorInBackground:@selector(saveBegin) withObject:nil];
	
	[self saveBegin];
	[self saveEnd];
	
	HandleExceptionObjcEnd(false);
}

-(void)saveBegin
{
	HandleExceptionObjcBegin();
	
//	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	
	app.mApplication->ImageDeactivate();
	app.mApplication->SaveImage();
	
//	[pool release];
	
//	[self performSelectorOnMainThread:@selector(saveEnd) withObject:nil waitUntilDone:FALSE];
	
	HandleExceptionObjcEnd(false);
}

-(void)saveEnd
{
	HandleExceptionObjcBegin();
	
	View_Editing* vw = [self getView];
	[vw activityIndicatorHide];
	
//	[self.navigationItem popViewControllerAnimated:NO];
	[app hide];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleBrushSize
{
	View_Editing* vw = [self getView];
	
	[vw showMiniBrushSize];
}

-(void)handleZoomToggle:(Vec2F)pan
{
	HandleExceptionObjcBegin();
	
	View_EditingImage* image = [self getImageView];
	
	// toggle zoom between 100% and custom zoom
	
	const float zoom = [image snapZoom:image.zoom];
	const float treshold = 2.0f;
	
	if (zoom >= treshold)
	{
		lastZoom = image.zoom;
		[image transformToCenter:TRUE];
	}
	else
	{
		const float newZoom = Calc::Max(4.0f, lastZoom);
		
		[image setPan:pan animated:TRUE];
		[image setZoom:newZoom animated:TRUE];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)handleImageAnimationEnd
{
	HandleExceptionObjcBegin();
	
	if (doneRequested)
	{
		[self handleDone];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)newPainting
{
	HandleExceptionObjcBegin();

	View_Editing* vwEditing = (View_Editing*)self.view;
	View_EditingImage* vw = vwEditing.image;
	
	[vw setZoom:1.0f animated:FALSE];
	[vw setPan:Vec2F(0.0f, 0.0f) animated:FALSE];
	
	HandleExceptionObjcEnd(false);
}

-(View_Editing*)getView
{
	return (View_Editing*)self.view;
}

-(View_EditingImage*)getImageView
{
	View_Editing* vw = [self getView];
	
	return vw.image;
}

-(void)setEyedropperEnabled:(BOOL)enabled
{
	[[self getView] setEyeDropperVisible:enabled];
}

-(void)setEyedropperLocation:(CGPoint)location color:(UIColor*)color
{
	[[self getView] setEyedropperLocation:location color:color];
}

-(void)setMagicMenuEnabled:(BOOL)enabled
{
	View_Editing* vw = [self getView];
	
	[vw setMagicMenuVisible:enabled];
}

-(void)handleMagicAction:(MagicAction)action
{
	HandleExceptionObjcBegin();
	
//	View_Editing* vw = [self getView];
	View_EditingImage* image = [self getImageView];
	
	switch (action)
	{
		case MagicAction_None:
			break;
		case MagicAction_Left:
			// toggle mirrored drawing
			image.mirrorX = !image.mirrorX;
			break;
		case MagicAction_Right:
			[self handleBrushSize];
			break;
		case MagicAction_Top:
			[self handleZoomToggle:image.pan];
			break;
		case MagicAction_Middle:
			[self showMenu:TRUE];
			[self setMagicMenuEnabled:FALSE];
			break;
	}
	
	HandleExceptionObjcEnd(false);
}

-(UIImage*)provideMagicImage:(MagicAction)action
{
	switch (action)
	{
		case MagicAction_Top:
			return [[UIImage imageNamed:@IMG("magic_zoom")] retain];
		case MagicAction_Left:
			return [[UIImage imageNamed:@IMG("magic_tooltype")] retain];
		case MagicAction_Right:
			return [[UIImage imageNamed:@IMG("magic_brushsize")] retain];
		case MagicAction_Middle:
			return [[UIImage imageNamed:@IMG("magic_main")] retain];
		default:
			return [[UIImage imageNamed:@IMG("icon_zoom")] retain];
	}
}

-(void)showMenu:(BOOL)visible
{
	BOOL hidden = !visible;
	BOOL animated = FALSE;
	
	[self.navigationController setToolbarHidden:hidden animated:animated];
	[self.navigationController setNavigationBarHidden:hidden animated:animated];
	
	if (visible)
	{
		[self menuHideStart];
	}
}

-(void)hideMenu
{
	[self menuHideStop];
	
	[self showMenu:FALSE];
	[self setMagicMenuEnabled:TRUE];
}

-(void)menuHideStart
{
	[self menuHideStop];
	
	menuTimer = [NSTimer scheduledTimerWithTimeInterval:MENU_HIDE_INTERVAL target:self selector:@selector(hideMenu) userInfo:nil repeats:YES];
}

-(void)menuHideStop
{
	if (menuTimer == nil)
		return;
	
	[menuTimer invalidate];
	menuTimer = nil;
}

-(void)updateUi
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_EditingMgr: updateUi", 0);
	
	View_BarButtonItem_Color* vw_Color = [[[View_BarButtonItem_Color alloc] initWithDelegate:self action:@selector(handleColorPicker) color:app.mApplication->BrushColor_get() opacity:app.mApplication->BrushOpacity_get()] autorelease];
	[vw_Color setFrame:CGRectMake(0.0f, 0.0f, 50.0f, 20.0f)];
//	[vw_Color setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleTopMargin];
//	[vw_Color setContentMode:UIViewContentModeScaleToFill];
	item_Color = [[[UIBarButtonItem alloc] initWithCustomView:vw_Color] autorelease];
	item_Tool = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_tool")] style:UIBarButtonItemStylePlain target:self action:@selector(handleToolSettings)] autorelease];
	item_Layers = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_layers")] style:UIBarButtonItemStylePlain target:self action:@selector(handleLayers)] autorelease];
	UIBarButtonItem* item_Undo = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_undo")]style:UIBarButtonItemStylePlain target:self action:@selector(handleUndo)] autorelease];
	UIBarButtonItem* item_Redo = [[[UIBarButtonItem alloc] initWithImage:[UIImage imageNamed:@IMG("button_redo")] style:UIBarButtonItemStylePlain target:self action:@selector(handleRedo)] autorelease];
	UIBarButtonItem* item_DBG_TestBezierTraveller = nil;
#ifndef DEPLOYMENT
	item_DBG_TestBezierTraveller = [[[UIBarButtonItem alloc] initWithTitle:@"B" style:UIBarButtonItemStylePlain target:self action:@selector(DBG_handleTestBezierTraveller)] autorelease];
#endif
	
	UIBarButtonItem* item_Space = [[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil];
	[item_Undo setEnabled:app.mApplication->HasUndo_get()];
	[item_Redo setEnabled:app.mApplication->HasRedo_get()];
	
	[self setToolbarItems:[NSArray arrayWithObjects:item_Color, item_Space, item_Tool, item_Space, item_Layers, item_Space, item_Undo, item_Space, item_Redo, item_DBG_TestBezierTraveller, nil]];

	HandleExceptionObjcEnd(false);
}

#ifdef IPAD
// UIPopoverControllerDelegate

-(void)popoverControllerDidDismissPopover:(UIPopoverController*)_popoverController
{
	popoverController = nil;
	//[popoverController autorelease];
}

-(BOOL)popoverControllerShouldDismissPopover:(UIPopoverController*)popoverController
{
	return YES;
}
#endif

static void RenderBezier(void* obj, BezierTravellerState state, float x, float y)
{
	Application* app = (Application*)obj;
	
	if (state == BezierTravellerState_Begin)
		app->StrokeBegin(app->LayerMgr_get()->EditingDataLayer_get(), false, false, x, y);
	if (state == BezierTravellerState_End)
		app->StrokeEnd();
	if (state == BezierTravellerState_Update)
		app->StrokeMove(x, y);
}

-(void)DBG_handleTestBezierTraveller
{
	HandleExceptionObjcBegin();
	
	[self menuHideStart];
	
	Vec2I size = app.mApplication->LayerMgr_get()->Size_get();
	
	BezierTraveller traveller;
	
	traveller.Setup(1.0f, RenderBezier, 0, app.mApplication);
	
	traveller.Begin(0.0f, 0.0f);
	
	for (int i = 0; i < 10; ++i)
	{
		const float x = Calc::Random(size[0]);
		const float y = Calc::Random(size[1]);
		
		traveller.Update(x, y);
	}
	
	traveller.End(100.0f, 100.0f);
	
	HandleExceptionObjcEnd(false);
}

-(void)loadView
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("view create", 0);
	
	self.view = [[[View_Editing alloc] initWithFrame:[UIScreen mainScreen].applicationFrame andApp:app controller:self] autorelease];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("edit: appear", 0);
	
	[self setMenuTransparent];
	
	doneRequested = false;
	
	if (!app.mApplication->LayerMgr_get()->EditingIsEnabled_get())
		app.mApplication->LayerMgr_get()->EditingBegin(true);
	
	View_Editing* vw = (View_Editing*)self.view;
	
	[vw handleFocus];
	
	[self showMenu:FALSE];
	[self setMagicMenuEnabled:TRUE];
	
	[self updateUi];
	
	[super viewWillAppear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[super viewWillDisappear:animated];
	
	LOG_DBG("edit: disappear", 0);
	
	if (app.mApplication->LayerMgr_get()->EditingIsEnabled_get())
		app.mApplication->LayerMgr_get()->EditingEnd();
	
	[self showMenu:TRUE];
	[self menuHideStop];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	[self menuHideStop];
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
