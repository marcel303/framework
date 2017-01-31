#import "AppDelegate.h"
#import "ExceptionLoggerObjC.h"
#import "View_Replay.h"
#import "View_ReplayMgr.h"

@implementation View_ReplayMgr

-(id)initWithApp:(AppDelegate *)_app imageId:(ImageId)_imageId
{
	HandleExceptionObjcBegin();
	
	if ((self = [super initWithApp:_app]))
	{
        [self setFullScreenLayout];
		
		self.navigationItem.leftBarButtonItem = [[[UIBarButtonItem alloc] initWithTitle:@"Back" style:UIBarButtonItemStylePlain target:self action:@selector(handleBack)] autorelease];
			
		UIBarButtonItem* item_Restart = [[[UIBarButtonItem alloc] initWithTitle:@"R" style:UIBarButtonItemStylePlain target:self action:@selector(handleRestart)] autorelease];
		UIBarButtonItem* item_Space = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemFlexibleSpace target:nil action:nil] autorelease];
		[self setToolbarItems:[NSArray arrayWithObjects:item_Space, item_Restart, item_Space, nil]];
			
		imageId = _imageId;
		paused = true;
	}
	
	return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)handleBack
{
	HandleExceptionObjcBegin();
	
	[app hide];
	
	HandleExceptionObjcEnd(false);
}

-(void)handleRestart
{
	HandleExceptionObjcBegin();
	
	View_Replay* vw = (View_Replay*)self.view;
	
	if (vw.replayActive)
	{
		[self stop];
	}
	
	[self start];
	
	HandleExceptionObjcEnd(false);
}

-(void)pause
{
	if (paused)
		return;
	
	paused = true;
	
	View_Replay* vw = (View_Replay*)self.view;
	
	// pause replay
	
	[vw paintTimerStop];
	
	// show menu
	
	[self.navigationController setToolbarHidden:FALSE animated:TRUE];
	[self.navigationController setNavigationBarHidden:FALSE animated:TRUE];
}

-(void)resume
{
	if (!paused)
		return;
	
	paused = false;
	
	View_Replay* vw = (View_Replay*)self.view;
	
	// resume replay
	
	[vw paintTimerStart];
	
	// hide menu
	
	[self.navigationController setToolbarHidden:TRUE animated:TRUE];
	[self.navigationController setNavigationBarHidden:TRUE animated:TRUE];
}

-(void)start
{
	View_Replay* vw = (View_Replay*)self.view;
	
	[vw replayBegin];
	
	[self resume];
}

-(void)stop
{
	View_Replay* vw = (View_Replay*)self.view;
	
	[self pause];
	
	[vw replayEnd];
}

-(void)loadView 
{
	HandleExceptionObjcBegin();
	
	self.view = [[[View_Replay alloc] initWithFrame:[UIScreen mainScreen].applicationFrame app:app controller:self imageId:imageId] autorelease];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillAppear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	[self setMenuTransparent];
	
	[self start];
	
	HandleExceptionObjcEnd(false);
}

-(void)viewWillDisappear:(BOOL)animated
{
	HandleExceptionObjcBegin();
	
	View_Replay* vw = (View_Replay*)self.view;
	
	if (!paused)
	{
		// stop timer
		
		[vw paintTimerStop];
	}

	if (vw.replayActive)
	{
		// stop replay
		
		[vw replayEnd];
	}
	
	HandleExceptionObjcEnd(false);
}

-(void)dealloc 
{
	HandleExceptionObjcBegin();
	
	[super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
