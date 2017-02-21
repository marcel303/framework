#import "AppDelegate.h"
#import "Application.h"
#import "Benchmark.h"
#import "BrushSettingsLibrary.h"
#import "Calc.h"
#import "ColorView.h"
#import "ExceptionLoggerObjC.h"
#import "FacebookState.h"
#import "FlickrState.h"
#import "KlodderSystem.h"
#import "LayerMgr.h"
#import "localhostAddresses.h"

#import "TestCode_iPhone.h"

#import "View_EditingMgr.h"
#import "View_PictureGalleryMgr.h"

/*
todo:
- explicitly set checker board size, remove display scale from Application
- explicitly determine desired gallery thumbnail size. set it on Application init
    - make a list of desired sizes per device class, display scale
- fix gallery: use thumbnail size from centralized device/scale size manager. make sure thumbnails are centered horizontally
- add a nice picture frame around thumbnails?

*/
//#define MENU_HEIGHT 58.0f

#ifdef IPAD
NSString* cancelTitle = nil;
#else
NSString* cancelTitle = @"Cancel";
#endif

@implementation AppDelegate

static void HandleChange(void* obj, void* arg);

@synthesize window;
@synthesize rootController;
@synthesize mApplication;
#if BUILD_FACEBOOK
@synthesize facebookState;
#endif
#if BUILD_FLICKR
@synthesize flickrState;
#endif

@synthesize vcEditing;
@synthesize vcPictureGallery;
@synthesize brushSettings;
@synthesize colorPickerState;

@synthesize addresses;

-(BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
#if 0
#warning
	DBG_ProfileFilter();
	DBG_ProfileBitmap();
	DBG_ProfileMacImage();
	exit(0);
#endif
	
	HandleExceptionObjcBegin();
	
	Benchmark bm("AppDelegate: didFinishLaunchingWithOptions");

	[[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(handleAddressesResolved:) name:@"LocalhostAdressesResolved" object:nil];
	[localhostAddresses performSelectorInBackground:@selector(list) withObject:nil];
	
#if 1
	[application setNetworkActivityIndicatorVisible:FALSE];
	[application setStatusBarHidden:TRUE withAnimation:UIStatusBarAnimationNone];
#else
//	[application setStatusBarStyle:UIStatusBarStyleBlackTranslucent];
	[application setNetworkActivityIndicatorVisible:TRUE];
	[application setStatusBarHidden:FALSE animated:FALSE];
#endif
	[application setIdleTimerDisabled:TRUE];
	
	[window setMultipleTouchEnabled:TRUE];
#if 1
	[window setOpaque:TRUE];
	[window setBackgroundColor:[UIColor whiteColor]];
	[window setClearsContextBeforeDrawing:FALSE];
#endif
	
//	[window setBounds:[UIScreen mainScreen].bounds];
	
	[window makeKeyAndVisible];

	Calc::Initialize();
	
#ifdef DEBUG
//	ExceptionLogger::Log(ExceptionVA("test exception log"));
//	DBG_TestFileArchive();
//	DBG_TestTiff();
//	DBG_TestPhotoshop();
//	DBG_TestHtmlTemplate();
//	DBG_TestHslRgb();
//	DBG_TestDataStream();
#endif
	
	mApplication = 0;
	
#if BUILD_FACEBOOK
	facebookState = [[FacebookState alloc] init];
	[facebookState resume];
#endif
#if BUILD_FLICKR
	flickrState = [[FlickrState alloc] init];
	[flickrState resume];
#endif
	
	NSLog(@"Creating touch mgr");
	
	//CGRect frame = [window frame];
	//CGRect childFrame = frame;
//	CGRect menuFrame = CGRectMake(frame.origin.x, frame.origin.y + frame.size.height - MENU_HEIGHT, frame.size.width, MENU_HEIGHT);
	
	brushSettings = new BrushSettings();
	colorPickerState = new PickerState();
	
	LOG_INF("create: vcEditing", 0);
	vcEditing = [[View_EditingMgr alloc] initWithApp:self];
#if BUILD_HTTPSERVER
	LOG_INF("create: vcHttpServer", 0);
	vcHttpServer = [[View_HttpServerMgr alloc] initWithApp:self];
#endif
	LOG_INF("create: vcPictureGallery", 0);
	vcPictureGallery = [[View_PictureGalleryMgr alloc] initWithApp:self];
	
	//
	
	LOG_INF("setting up view controller", 0);
	
	rootController = [[UINavigationController alloc] initWithRootViewController:vcPictureGallery];
	[rootController setToolbarHidden:FALSE animated:FALSE];
//	[rootController.toolbar setBarStyle:UIBarStyleBlackTranslucent];	
//	[rootController.navigationBar setBarStyle:UIBarStyleBlackTranslucent];
#if 0
	rootController.toolbar.tintColor = [UIColor colorWithRed:0.12f green:0.1f blue:0.0f alpha:0.0f];
	rootController.toolbar.translucent = TRUE;
#endif

	mActiveView = rootController;
	
	[window setRootViewController:rootController];
    
#if BUILD_FLICKR
	NSURL* url = [launchOptions objectForKey:UIApplicationLaunchOptionsURLKey];
	
	if (url)
	{
		[flickrState handleOpenUrl:url];
	}
#endif
	
#ifdef DEBUG
#if 0
	[NSTimer scheduledTimerWithTimeInterval:5.0f target:self selector:@selector(memoryReport) userInfo:nil repeats:YES];
#endif
#endif
	
	return YES;

	HandleExceptionObjcEnd(false);
	
	return NO;
}

static void HandleChange(void* obj, void* arg)
{
	AppDelegate* app = (AppDelegate*)obj;
	ChangeType type = *(ChangeType*)arg;
	
	[app handleChange:type];
}

-(void)handleChange:(ChangeType)type
{
	switch (type)
	{
		case ChangeType_Color:
		{
			Rgba color = mApplication->BrushColor_get();
			color.rgb[3] = mApplication->BrushOpacity_get();
			colorPickerState->Color_set(color);
			if (mActiveView == vcEditing)
				[vcEditing updateUi];
			break;
		}
		case ChangeType_LayerOrder:
			break;
		case ChangeType_Tool:
			break;
		case ChangeType_Undo:
			if (mActiveView == vcEditing)
				[vcEditing updateUi];
			break;
	}
}

-(void)show:(UIViewController*)vc animated:(BOOL)animated
{
	if (false)
	{
		[UIView beginAnimations:nil context:NULL];
		[UIView setAnimationDuration:1.0];
		
		[rootController pushViewController:vc animated:FALSE];
		
		const int mode = rand() % 4;
		
		if (mode == 0)
			[UIView setAnimationTransition:UIViewAnimationTransitionCurlDown forView:window cache:YES];
		if (mode == 1)
			[UIView setAnimationTransition:UIViewAnimationTransitionCurlUp forView:window cache:YES];
		if (mode == 2)
			[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromLeft forView:window cache:YES];
		if (mode == 3)
			[UIView setAnimationTransition:UIViewAnimationTransitionFlipFromRight forView:window cache:YES];
		
		[UIView commitAnimations];
	}
	else
	{
		[rootController pushViewController:vc animated:animated];
	}
	
	mActiveView = vc;
}

-(void)show:(UIViewController*)vc
{
	[self show:vc animated:TRUE];
}

-(void)hide
{
	[self hideWithAnimation:FALSE];
}

-(void)hideWithAnimation:(BOOL)animated
{
	[rootController popViewControllerAnimated:animated];
	
	mActiveView = [rootController topViewController];
}

-(void)newApplication
{
	delete mApplication;
	
	mApplication = new Application([AppDelegate displayScale]);
	mApplication->OnChange = CallBack(self, HandleChange);
	
	mApplication->Setup(0, gSystem.GetResourcePath("brushes_lq.lib").c_str(), gSystem.GetDocumentPath("brushes_cs.lib").c_str(), 1, Rgba_Make(0.9f, 0.9f, 0.9f, 1.0f), Rgba_Make(0.8f, 0.8f, 0.8f, 1.0f));
}

-(void)setBrushSettings:(BrushSettings*)_brushSettings
{
	Assert(_brushSettings != 0);

	*brushSettings = *_brushSettings;
}

-(void)applyBrushSettings:(ToolType)toolType
{
	switch (toolType)
	{
		case ToolType_SoftBrush:
		{
			ToolSettings_BrushSoft settings(
				brushSettings->diameter,
				brushSettings->hardness,
				brushSettings->spacing);
			mApplication->ToolSelect_BrushSoft(settings);
			break;
		}
		case ToolType_PatternBrush:
		{
			ToolSettings_BrushPattern settings(
				brushSettings->diameter,
				brushSettings->patternId,
				brushSettings->spacing);
			mApplication->ToolSelect_BrushPattern(settings);
			break;
		}
		case ToolType_SoftBrushDirect:
		{
			ToolSettings_BrushSoftDirect settings(
				brushSettings->diameter,
				brushSettings->hardness,
				brushSettings->spacing);
			mApplication->ToolSelect_BrushSoftDirect(settings);
			break;
		}
		case ToolType_PatternBrushDirect:
		{
			ToolSettings_BrushPatternDirect settings(
				brushSettings->diameter,
				brushSettings->patternId,
				brushSettings->spacing);
			mApplication->ToolSelect_BrushPatternDirect(settings);
			break;
		}
		case ToolType_SoftSmudge:
		{
			ToolSettings_SmudgeSoft settings(
				brushSettings->diameter,
				brushSettings->hardness,
				brushSettings->spacing,
				brushSettings->strength);
			mApplication->ToolSelect_SmudgeSoft(settings);
			break;
		}
		case ToolType_PatternSmudge:
		{
			ToolSettings_SmudgePattern settings(
				brushSettings->diameter,
				brushSettings->patternId,
				brushSettings->spacing,
				brushSettings->strength);
			mApplication->ToolSelect_SmudgePattern(settings);
			break;
		}
		case ToolType_SoftEraser:
		{
			ToolSettings_EraserSoft settings(
				brushSettings->diameter,
				brushSettings->hardness,
				brushSettings->spacing);
			mApplication->ToolSelect_EraserSoft(settings);
			break;
		}
		case ToolType_PatternEraser:
		{
			ToolSettings_EraserPattern settings(
				brushSettings->diameter,
				brushSettings->patternId,
				brushSettings->spacing);
			mApplication->ToolSelect_EraserPattern(settings);
			break;
		}
			
		default:
			throw ExceptionVA("not implemented");
	}
}

-(void)loadCurrentBrushSettings
{
	brushSettings->mToolType = mApplication->ToolType_get();
	
	switch (brushSettings->mToolType)
	{
		case ToolType_PatternBrush:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: brushPattern", 0);
			brushSettings->patternId = mApplication->mToolSettings_BrushPattern.patternId;
			brushSettings->diameter = mApplication->mToolSettings_BrushPattern.diameter;
			brushSettings->hardness = 0.0f;
			brushSettings->spacing = mApplication->mToolSettings_BrushPattern.spacing;
			brushSettings->strength = 0.0f;
			break;
		case ToolType_SoftBrush:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: brushSoft", 0);
			brushSettings->patternId = 0;
			brushSettings->diameter = mApplication->mToolSettings_BrushSoft.diameter;
			brushSettings->hardness = mApplication->mToolSettings_BrushSoft.hardness;
			brushSettings->spacing = mApplication->mToolSettings_BrushSoft.spacing;
			brushSettings->strength = 0.0f;
			break;
		case ToolType_PatternBrushDirect:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: brushPatternDirect", 0);
			brushSettings->patternId = mApplication->mToolSettings_BrushPatternDirect.patternId;
			brushSettings->diameter = mApplication->mToolSettings_BrushPatternDirect.diameter;
			brushSettings->hardness = 0.0f;
			brushSettings->spacing = mApplication->mToolSettings_BrushPatternDirect.spacing;
			brushSettings->strength = 0.0f;
			break;
		case ToolType_SoftBrushDirect:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: brushSoftDirect", 0);
			brushSettings->patternId = 0;
			brushSettings->diameter = mApplication->mToolSettings_BrushSoftDirect.diameter;
			brushSettings->hardness = mApplication->mToolSettings_BrushSoftDirect.hardness;
			brushSettings->spacing = mApplication->mToolSettings_BrushSoftDirect.spacing;
			brushSettings->strength = 0.0f;
			break;
		case ToolType_PatternEraser:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: eraserPattern", 0);
			brushSettings->patternId = mApplication->mToolSettings_EraserPattern.patternId;
			brushSettings->diameter = mApplication->mToolSettings_EraserPattern.diameter;
			brushSettings->hardness = 0.0f;
			brushSettings->spacing = mApplication->mToolSettings_EraserPattern.spacing;
			brushSettings->strength = 0.0f;
			break;
		case ToolType_SoftEraser:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: eraserSoft", 0);
			brushSettings->patternId = 0;
			brushSettings->diameter = mApplication->mToolSettings_EraserSoft.diameter;
			brushSettings->hardness = mApplication->mToolSettings_EraserSoft.hardness;
			brushSettings->spacing = mApplication->mToolSettings_EraserSoft.spacing;
			brushSettings->strength = 0.0f;
			break;
		case ToolType_PatternSmudge:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: smudgePattern", 0);
			brushSettings->patternId = mApplication->mToolSettings_SmudgePattern.patternId;
			brushSettings->diameter = mApplication->mToolSettings_SmudgePattern.diameter;
			brushSettings->hardness = 0.0f;
			brushSettings->spacing = mApplication->mToolSettings_SmudgePattern.spacing;
			brushSettings->strength = mApplication->mToolSettings_SmudgePattern.strength;
			break;
		case ToolType_SoftSmudge:
			LOG_DBG("AppDelegate: loadCurrentBrushSettings: smudgeSoft", 0);
			brushSettings->patternId = 0;
			brushSettings->diameter = mApplication->mToolSettings_SmudgeSoft.diameter;
			brushSettings->hardness = mApplication->mToolSettings_SmudgeSoft.hardness;
			brushSettings->spacing = mApplication->mToolSettings_SmudgeSoft.spacing;
			brushSettings->strength = mApplication->mToolSettings_SmudgeSoft.strength;
			break;
		default:
#ifdef DEBUG
			throw ExceptionNA();
#else
			brushSettings->mToolType = ToolType_SoftBrush;
			brushSettings->patternId = 0;
			brushSettings->diameter = mApplication->mToolSettings_BrushSoft.diameter;
			brushSettings->hardness = mApplication->mToolSettings_BrushSoft.hardness;
			brushSettings->spacing = mApplication->mToolSettings_BrushSoft.spacing;
			brushSettings->strength = 0.0f;
#endif
			break;
	}
}

-(void)saveBrushSettings
{
	BrushSettingsLibrary library;
	
	library.Save(*brushSettings);
}

-(void)applyColor
{
	Rgba color = colorPickerState->Color_get();
	
	LOG_DBG("updating app color: %f, %f, %f @ %f", color.rgb[0], color.rgb[1], color.rgb[2], colorPickerState->Opacity_get());
	
	mApplication->ColorSelect(color.rgb[0], color.rgb[1], color.rgb[2], colorPickerState->Opacity_get());
	
	// add or update swatch list
	
	Swatch swatch;
	
	swatch.Color_set(MacRgba_Make(color.rgb[0] * 255.0f, color.rgb[1] * 255.0f, color.rgb[2] * 255.0f, colorPickerState->Opacity_get() * 255.0f));
	
	mApplication->SwatchMgr_get()->AddOrUpdate(swatch);
}

-(void)handleAddressesResolved:(NSNotification*)notification
{
	if (notification)
	{
		[addresses release];
		addresses = [[notification object] copy];
		NSLog(@"addresses: %@", addresses);
	}
	
	[[NSNotificationCenter defaultCenter] postNotificationName:@"LocalhostAdressesResolved2" object:addresses];
}

+(UIImage*)loadImageResource:(NSString*)name
{
	NSString* path = [name stringByDeletingPathExtension];
	NSString* ext = [name pathExtension];
	
	return [[UIImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:path ofType:ext]];
}

+(UIImage*)uiImageResize:(UIImage*)image size:(Vec2I)size
{
	if (image.size.width == size[0] && image.size.height == size[1])
		return image;
	
	UIImage* result;
	
	UIGraphicsBeginImageContext(CGSizeMake(size[0], size[1]));
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	[image drawInRect:CGRectMake(0.0f, 0.0f, size[0], size[1])];
	result = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();

	return result;
}

+(UIImage*)macImageToUiImage:(const MacImage*)image size:(Vec2I)size
{
	CGImageRef cgImage = image->ImageWithAlpha_get();
	
	UIImage* result = [UIImage imageWithCGImage:cgImage];

	CGImageRelease(cgImage);
	
	return result;
}

+(UIImage*)macImageToUiImage:(const MacImage*)image
{
	return [AppDelegate macImageToUiImage:image size:image->Size_get()];
}

+(MacImage*)uiImageToMacImage:(UIImage*)image size:(Vec2I)size
{
	image = [self uiImageResize:image size:size];
	
	MacImage* result = new MacImage();
	
	result->Size_set(size[0], size[1], false);
	
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGContextRef ctx = CGBitmapContextCreate(result->Data_get(), size[0], size[1], 8, 4 * size[0], colorSpace, kCGImageAlphaPremultipliedLast);
	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGColorSpaceRelease(colorSpace);
	CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, size[0], size[1]), image.CGImage);
	CGContextRelease(ctx);
	
	return result;
}

+(MacImage*)uiImageToMacImage:(UIImage*)image
{
	return [AppDelegate uiImageToMacImage:image size:Vec2I(image.size.width, image.size.height)];
}

+(UIImage*)flipY:(UIImage*)image
{
	UIGraphicsBeginImageContext(CGSizeMake(image.size.width, image.size.height));
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGContextDrawImage(ctx, CGRectMake(0.0f, 0.0f, image.size.width, image.size.height), image.CGImage);
	image = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return image;
}

-(UIImage*)drawingToImage
{
	MacImage* image = mApplication->LayerMgr_get()->Merged_get();
	CGImageRef cgImage = image->ImageWithAlpha_get();
	UIImage* uiImage = [UIImage imageWithCGImage:cgImage];
	CGImageRelease(cgImage);
	uiImage = [AppDelegate flipY:uiImage];
	return uiImage;
}

+(NSArray*)scanForPictures:(const char*)path
{
	NSFileManager* fm = [[NSFileManager alloc] init];
	
	NSMutableArray* result = [[[NSMutableArray alloc] init] autorelease];
	
	NSError* error = nil;
	
	NSArray* content = [fm contentsOfDirectoryAtPath:[NSString stringWithCString:path encoding:NSASCIIStringEncoding] error:&error];
	
	for (NSUInteger i = 0; i < [content count]; ++i)
	{
		NSString* fileName = [content objectAtIndex:i];
		
		NSString* extension = [fileName pathExtension];
		
		if ([extension isEqualToString: @"xml"]) 
		{
			NSString* name = [fileName stringByDeletingPathExtension];
			
			[result addObject:name];
		}
	}
	
	[fm release];
	
	return result;
}

+(void)applyWatermark:(MacImage*)dst
{
	// load watermark
	
	UIImage* uiWatermark = [AppDelegate loadImageResource:@"mark_lite.png"];
	
	MacImage* watermark = [AppDelegate uiImageToMacImage:uiWatermark];
	
	[uiWatermark release];
	
	watermark->FlipY_InPlace();
	
	// blit with alpha
	
	int x = dst->Sx_get() - watermark->Sx_get();
	int y = dst->Sy_get() - watermark->Sy_get();
	
	watermark->BlitAlpha(dst, 0, 0, x, y, watermark->Sx_get(), watermark->Sy_get());
	
	delete watermark;
}

+(float)displayScale
{
	return [UIScreen mainScreen].scale;
}

-(void)memoryReport
{
	DBG_PrintAllocState();
}

-(void)applicationDidBecomeActive:(UIApplication *)application
{
	// nop
}

-(void)applicationWillResignActive:(UIApplication *)application
{
	// nop
}

-(void)applicationDidEnterBackground:(UIApplication *)application 
{
	// nop
}

-(void)applicationWillEnterForeground:(UIApplication *)application 
{
	// nop
}

-(void)applicationDidReceiveMemoryWarning
{
	HandleExceptionObjcBegin();
	
	LOG_WRN("system memory low", 0);
	
	HandleExceptionObjcEnd(true);
}

-(void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	
	[addresses release];
	addresses = nil;
	
	// release all view controllers
	
	[vcEditing release];
	vcEditing = nil;
	[vcPictureGallery release];
	vcPictureGallery = nil;
	
	delete mApplication;
	mApplication = 0;
	
#if BUILD_FACEBOOK
	[facebookState release];
	facebookState = nil;
#endif
#if BUILD_FLICKR
	[flickrState release];
	flickrState = nil;
#endif
	
	delete brushSettings;
	brushSettings = 0;
	delete colorPickerState;
	colorPickerState = 0;
	
    [window release];
	
    [super dealloc];
}

@end

