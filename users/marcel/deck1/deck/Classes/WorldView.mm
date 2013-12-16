#import <QuartzCore/CALayer.h>
#import "Game.h"
#import "Input.h"
#import "OpenGLState.h"
#import "render.h"
#import "World.h"
#import "WorldView.h"

#define UPDATE_INTERVAL (1.0f / 120.0f)

@implementation WorldView

-(id)initWithCoder:(NSCoder*)coder
{
    if ((self = [super initWithCoder:coder])) 
	{
		[self setUserInteractionEnabled:YES];
		[self setMultipleTouchEnabled:NO];
		
		CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
		
		gl = new OpenGLState();
		gl->Initialize(layer, self.bounds.size.width, self.bounds.size.height, false, false);
		gl->CreateBuffers();
    }
	
    return self;
}

+(Class)layerClass
{
	return [CAEAGLLayer class];
}

-(void)updateBegin
{
	updateTimer = [NSTimer scheduledTimerWithTimeInterval:UPDATE_INTERVAL target:self selector:@selector(updateElapsed) userInfo:nil repeats:YES];
}

-(void)updateEnd
{
	[updateTimer invalidate];
	updateTimer = nil;
}

-(void)updateElapsed
{
	// update game
	
	gGame->Update(UPDATE_INTERVAL);
	
	// render game
	
	gl->MakeCurrent();
	
	gGame->Render();
	
	gRender->Present();
	
	gl->Present();
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView:self];
	
	gInput->mTouchActive = true;
	gInput->mTouchPos[0] = location.x;
	gInput->mTouchPos[1] = location.y;
	gInput->mTouchPosDelta[0] = 0;
	gInput->mTouchPosDelta[1] = 0;
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	gInput->mTouchActive = false;
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	gInput->mTouchActive = false;
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch* touch = [touches anyObject];
	CGPoint location = [touch locationInView:self];
	CGPoint location2 = [touch previousLocationInView:self];
	
	gInput->mTouchPos[0] = location.x;
	gInput->mTouchPos[1] = location.y;
	
	gInput->mTouchPosDelta[0] += location.x - location2.x;
	gInput->mTouchPosDelta[1] += location.y - location2.y;
}

-(void)dealloc 
{
	[self updateEnd];
	
	gl->DestroyBuffers();
	gl->Shutdown();
	delete gl;
	gl = 0;
	
    [super dealloc];
}

@end
