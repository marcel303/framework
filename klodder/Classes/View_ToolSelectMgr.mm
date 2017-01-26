#import "AppDelegate.h"
#import "Application.h"
#import "Benchmark.h"
#import "BrushSettingsLibrary.h"
#import "ExceptionLoggerObjC.h"
#import "Settings.h"
#import "View_ColorPickerMgr.h"
#import "View_ToolSettings_Brush.h"
#import "View_ToolSettings_Erazor.h"
#import "View_ToolSettings_Smudge.h"
#import "View_ToolSelectMgr.h"

#define USE_TRANSITION 0

static ToolType ToToolType(ToolViewType toolViewType, bool softBrush);

@implementation View_ToolSelectMgr

@synthesize toolViewType;
@synthesize brushSettingsLibrary;
@synthesize toolType=_toolType;

-(id)initWithApp:(AppDelegate *)_app
{
	HandleExceptionObjcBegin();
	
	if (self = [super initWithApp:_app])
	{
		//[self setModalInPopover:YES];
		[self setPreferredContentSize:CGSizeMake(320.0f, 480.0f)];
		
		brushSettingsLibrary = new BrushSettingsLibrary();
		
		[self loadViewWithCurrentToolSettings];
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)loadView
{
	HandleExceptionObjcBegin();
	
	[self changeView:[self createView:toolViewType]];
	
//	[self brushSettingsChanged];
	
	HandleExceptionObjcEnd(false);
}

-(void)toolViewTypeChanged:(ToolViewType)type
{
	if (type == toolViewType)
		return;
	
	LOG_DBG("toolViewTypeChanged: %d -> %d", (int)toolViewType, (int)type);
	
	[self save];
	
//	[self changeView:nil];
	
	toolViewType = type;
	
	[self changeView:[self createView:type]];
		
	[self load:TRUE];
}

-(ToolType)toolType
{
	return ToToolType(toolViewType, app.brushSettings->patternId == 0);
}

-(void)load:(BOOL)restoreLastBrush
{
	Benchmark bm("View_ToolSelectMgr: load");
	
	Assert(toolViewType != ToolViewType_Undefined);
	
	LOG_DBG("View_ToolSelectMgr: load", 0);
	
	if (restoreLastBrush)
	{
		// let active view select last brush pattern
	
		[activeView load];
	}
	
	// check if selected pattern exists. if not, default to pattern ID 1000
	
	Brush_Pattern* pattern = app.mApplication->BrushLibrarySet_get()->Find(app.brushSettings->patternId);
	
	if (pattern == 0)
	{
		uint32_t patternId = 1000;
		LOG_DBG("pattern %lu does not exist. choosind %lu instead", app.brushSettings->patternId, patternId);
		app.brushSettings->patternId = patternId;
	}
	
	// load related brush settings
	// NOTE: could do this in active view's load as well, but would mean unnecessary code
	
	BrushSettings temp = brushSettingsLibrary->Load(app.brushSettings->mToolType, app.brushSettings->patternId, 0);
	
	[app setBrushSettings:&temp];
	
	// update view
	
	[self brushSettingsChanged];
}

-(void)save
{
	if (toolViewType == ToolViewType_Undefined)
		return;

	LOG_DBG("View_ToolSelectMgr: save", 0);
	
	// save selected pattern ID
	
	[activeView save];
	
	// save brush settings
	
	brushSettingsLibrary->Save(*app.brushSettings);
}

-(void)brushPatternChanged:(uint32_t)patternId
{
	[self save];
	
	app.brushSettings->patternId = patternId;
	
	app.brushSettings->mToolType = self.toolType;
	
	[self load:FALSE];
}

-(void)brushSettingsChanged
{
	[activeView brushSettingsChanged];
}

-(ViewBase*)createView:(ToolViewType)type
{
	Benchmark bm("View_ToolSelectMgr: createView");
	
	// create tool specific view

#ifdef IPAD
	CGRect rect = CGRectMake(0.0f, 0.0f, 320.0f, 480.0f);
#else
	CGRect rect = [UIScreen mainScreen].applicationFrame;
#endif
	
	switch (type)
	{
		case ToolViewType_Brush:
			return [[[View_ToolSettings_Brush alloc] initWithFrame:rect andApp:app controller:self] autorelease];
		case ToolViewType_Smudge:
			return [[[View_ToolSettings_Smudge alloc] initWithFrame:rect andApp:app controller:self] autorelease];
		case ToolViewType_Eraser:
			return [[[View_ToolSettings_Eraser alloc] initWithFrame:rect andApp:app controller:self] autorelease];
		default:
			throw ExceptionVA("not implemented");
	}
}

-(void)changeView:(ViewBase*)view
{
	Benchmark bm("View_ToolSelectMgr: changeView");
	
	LOG_DBG("View_ToolSelectMgr: changeView", 0);
	
	activeView = view;
	
	self.view = activeView;
}

-(void)apply
{
	LOG_DBG("View_ToolSelectMgr: apply", 0);
	
	[self save];
	
	[app applyBrushSettings:self.toolType];
	
	[app applyColor];
}

-(void)loadViewWithCurrentToolSettings
{
	Benchmark bm("View_ToolSelectMgr: loadViewWithCurrentToolSettings");
	
	LOG_DBG("View_ToolSelectMgr: loadViewWithCurrentToolSettings", 0);
	
	[app loadCurrentBrushSettings];
	
	// decide tool view type based on current tool
	
	ToolType toolType = app.mApplication->ToolType_get();
	
	ToolViewType type = ToolViewType_Undefined;
	
	switch (toolType)
	{
		case ToolType_PatternBrush:
		case ToolType_SoftBrush:
		case ToolType_PatternBrushDirect:
		case ToolType_SoftBrushDirect:
			type = ToolViewType_Brush;
			type = ToolViewType_Brush;
			break;
			
		case ToolType_PatternEraser:
		case ToolType_SoftEraser:
			type = ToolViewType_Eraser;
			type = ToolViewType_Eraser;
			break;
			
		case ToolType_PatternSmudge:
		case ToolType_SoftSmudge:
			type = ToolViewType_Smudge;
			type = ToolViewType_Smudge;
			break;
			
		default:
#ifdef DEBUG
			throw ExceptionNA();
#else
			type = ToolViewType_Brush;
#endif
			break;
	}
	
//	[self toolViewTypeChanged:type];
	toolViewType = type;
}

-(BrushSettings*)brushSettings
{
	return app.brushSettings;
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[super viewWillAppear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	// view dismissed; apply settings
	
	[self apply];
	
	[super viewWillDisappear:animated];
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("dealloc: View_ToolSelectMgr", 0);
	
//	self.view = nil;

	delete brushSettingsLibrary;
	
	[super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end

static ToolType ToToolType(ToolViewType toolViewType, bool softBrush)
{
	if (toolViewType == ToolViewType_Brush)
    {
		if (softBrush)
			return ToolType_SoftBrushDirect;
		else
			return ToolType_PatternBrushDirect;
    }
        
	if (toolViewType == ToolViewType_Smudge)
    {
		if (softBrush)
			return ToolType_SoftSmudge;
		else
			return ToolType_PatternSmudge;
    }
    
	if (toolViewType == ToolViewType_Eraser)
    {
		if (softBrush)
			return ToolType_SoftEraser;
		else
			return ToolType_PatternEraser;
    }
	
	return ToolType_Undefined;
}
