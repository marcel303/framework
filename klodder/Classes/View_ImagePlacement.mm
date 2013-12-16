#import "AppDelegate.h"
#import "ExceptionLoggerObjC.h"
#import "Log.h"
#import "View_ImagePlacement.h"
#import "View_ImagePlacementMgr.h"

@implementation View_ImagePlacement

@synthesize image;
@synthesize pivotView;

-(id)initWithFrame:(CGRect)frame controller:(View_ImagePlacementMgr*)_controller
{
	HandleExceptionObjcBegin();
	
    if ((self = [super initWithFrame:frame])) 
	{
		[self setMultipleTouchEnabled:FALSE];
		[self setBackgroundColor:[UIColor blackColor]];
		[self setOpaque:TRUE];
		
		controller = _controller;
		
		self.image = [[[UIImageView alloc] initWithImage:controller.image] autorelease];
		[self addSubview:image];
		
		//self.pivotView = [[[UIImageView alloc] initWithImage:[AppDelegate loadImageResource:@"pivot.png"]] autorelease];
		self.pivotView = [[[UIImageView alloc] initWithImage:[[UIImage imageNamed:@"pivot.png"] retain]] autorelease];
		[self addSubview:pivotView];
		[self.pivotView setCenter:CGPointMake(0.0f, 0.0f)];
    }
	
    return self;
	
	HandleExceptionObjcEnd(false);
	
	return nil;
}

-(void)updateUi
{	
	LOG_DBG("View_ImagePlacement: updateUi", 0);
	
	[self.image setCenter:CGPointMake(0.0f, 0.0f)];
	[self.image setTransform:controller.imageTransform];
	
	[self.pivotView setTransform:controller.pivotTransform];
	
#ifdef DEBUG
	Vec2I location = controller.location;
	LOG_DBG("placement: location=%d,%d", location[0], location[1]);
#if 0
	[debugView removeFromSuperview];
	UIImage* debugImage = [controller DBG_renderFinalImage];
	debugView = [[[UIImageView alloc] initWithImage:debugImage] autorelease];
	[self addSubview:debugView];
#endif
#endif
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	[controller handleAdjustmentBegin];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	[controller handleAdjustmentEnd];
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	[controller handleAdjustmentEnd];
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch* touch = [touches anyObject];
	CGPoint location0 = [touch previousLocationInView:self];
	CGPoint location1 = [touch locationInView:self];
	
	Vec2I delta = Vec2I(location1) - Vec2I(location0);
	Vec2I oldLocation = controller.location;
	Vec2I newLocation = oldLocation + delta;
	
	[controller setLocation:newLocation];
}

-(void)dealloc
{
	HandleExceptionObjcBegin();
	
	LOG_DBG("View_ImagePlacement: dealloc", 0);
	
	[self.image removeFromSuperview];
	self.image = nil;
	
	[self.pivotView removeFromSuperview];
	self.pivotView = nil;
	
    [super dealloc];
	
	HandleExceptionObjcEnd(false);
}

@end
