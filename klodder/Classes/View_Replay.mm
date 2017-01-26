#import "AppDelegate.h"
#import "Application.h"
#import "Deployment.h"
#import "ExceptionLoggerObjC.h"
#import "FileStream.h"
#import "KlodderSystem.h"
#import "LayerMgr.h"
#import "Log.h"
#import "StreamReader.h"
#import "Timer.h"
#import "View_Replay.h"
#import "View_ReplayMgr.h"

#define PAINT_INTERVAL (1.0 / 200.0)
//#define UPDATE_TIME (1.0f / 5.0f)
#define UPDATE_TIME (1.0f / 30.0f)

@implementation View_Replay

@synthesize replayActive;

-(id)initWithFrame:(CGRect)frame app:(AppDelegate*)_app controller:(View_ReplayMgr*)_controller imageId:(ImageId)_imageId
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setUserInteractionEnabled:TRUE];
		[self setOpaque:TRUE];
		[self setMultipleTouchEnabled:FALSE];
		[self setClearsContextBeforeDrawing:NO];
		[self setContentMode:UIViewContentModeScaleAspectFit];
		
		app = _app;
		controller = _controller;
		imageId = _imageId;
		replayActive = false;
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)replayBegin
{
	LOG_DBG("replay: prepare application", 0);
	
	Assert(!replayActive);
	
	replayActive = true;
	
	// prepare application
	
	[app newApplication];
	
	kdImageDescription description = Application::LoadImageDescription(imageId);
	commandStreamPosition = description.commandStream.position;
	
	app.mApplication->ImageReset(imageId);
	app.mApplication->ImageActivate(false);
	app.mApplication->UndoEnabled_set(false);
	app.mApplication->LayerMgr_get()->EditingBegin(true);
//	app.mApplication->ExecuteImageSize(3, 320, 480);
	
	// open command stream
	
	LOG_DBG("replay: open command stream", 0);
	
	std::string commandStreamFileName = app.mApplication->GetPath_CommandStream(imageId);
	
	commandStream = new FileStream(commandStreamFileName.c_str(), OpenMode_Read);
}

-(void)replayEnd
{
	[self paintTimerStop];
	
	//
	
	Assert(replayActive);
	
	replayActive = false;
	
	app.mApplication->LayerMgr_get()->EditingEnd();
	app.mApplication->UndoEnabled_set(true);
	app.mApplication->ImageDeactivate();
}

-(void)paintTimerStart
{
	[self paintTimerStop];
	
	LOG_DBG("replay: start timer", 0);
	
	paintTimer = [[NSTimer scheduledTimerWithTimeInterval:PAINT_INTERVAL target:self selector:@selector(paintTimerUpdate) userInfo:nil repeats:TRUE] retain];
}

-(void)paintTimerStop
{
	if (paintTimer == nil)
		return;
	
	LOG_DBG("replay: stop timer", 0);
	
	[paintTimer invalidate];
	[paintTimer release];
	paintTimer = nil;
}

-(void)paintTimerUpdate
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("replay: update timer", 0);
	
	try
	{
		Timer timer;
		
		while (timer.Time_get() < UPDATE_TIME && paintTimer != nil)
		{
			// read next packet
			
			StreamReader reader(commandStream, false);
			
			CommandPacket packet;
			
			packet.Read(reader);
			
			// execute packet
			
			Assert(app.mApplication->LayerMgr_get()->EditingIsEnabled_get());
			
			app.mApplication->Execute(packet);
			
			// end timer if end reached
			
			if (commandStream->EOF_get() || commandStream->Position_get() >= commandStreamPosition)
			{
				[controller stop];
			}
		}
		
		// check if redraw required
		
		[self updatePaint];
	}
	catch (std::exception& e)
	{
		ExceptionLogger::Log(e);
		
		[self paintTimerStop];
		
		[[[[UIAlertView alloc] initWithTitle:NS(Deployment::ReplayFailedTitle) message:NS(Deployment::ReplayFailedText) delegate:nil cancelButtonTitle:@"OK" otherButtonTitles:nil] autorelease] show];
		
		[app hide];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)updatePaint
{
	AreaI area = app.mApplication->LayerMgr_get()->Validate();
	
	if (area.IsSet_get())
	{
		const float scale = [AppDelegate displayScale];
		
		int x = area.m_Min[0];
		int y = area.m_Min[1];
		int sx = area.m_Max[0] - area.m_Min[0] + 1;
		int sy = area.m_Max[1] - area.m_Min[1] + 1;
		
		[self setNeedsDisplayInRect:CGRectMake(x / scale, y / scale, sx / scale, sy / scale)];
	}
}

-(void)drawRect:(CGRect)rect 
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("drawRect (%f, %f) - (%f, %f)", rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	const float scale = [AppDelegate displayScale];
	
	MacImage* image = app.mApplication->LayerMgr_get()->Merged_get();
	
	// draw image
	
	if (!image)
	{
		LOG_WRN("no image", 0);
		return;
	}
	
	if (image->Sx_get() * image->Sy_get() == 0)
	{
		CGContextSetFillColorWithColor(ctx, [UIColor whiteColor].CGColor);
		CGContextFillRect(ctx, self.bounds);
		return;
	}
	
	CGImageRef cgImage = image->Image_get();
	
	if (!cgImage)
	{
		LOG_WRN("no CG image", 0);
		return;
	}
	
	// draw shadow
	
	const float sx = CGImageGetWidth(cgImage);
	const float sy = CGImageGetHeight(cgImage);
	
	CGContextSetBlendMode(ctx, kCGBlendModeCopy);
	CGRect r = CGRectMake(0.0f, 0.0f, sx / scale, sy / scale);
	CGContextDrawImage(ctx, r, cgImage);
	
	CGImageRelease(cgImage);
	
	HandleExceptionObjcEnd(false);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (replayActive)
	{
		if (paintTimer != nil)
		{
			[controller pause];
		}
		else
		{
			[controller resume];
		}
	}
	else
	{
		[controller start];
	}
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	[self paintTimerStop];
	paintTimer = nil;
	
	delete commandStream;
	commandStream = 0;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
