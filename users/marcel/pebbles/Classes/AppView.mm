#import <QuartzCore/QuartzCore.h>
#import "AppView.h"
#import "Calc.h"
#import "Game.h"
#import "OpenGLState.h"
#import "render_iphone.h"
#import "Thingy.h"
#import "Types.h"

#define UPDATE_INTERVAL (1.0f / 30.0f)
#define MAX_PLAYERS 2

void PlayerInfoHud::Render()
{
	Color color[2] =
	{
		Color(1.0f, 0.0f, 0.0f),
		Color(0.0f, 0.0f, 1.0f)
	};
	
	gRender->Quad(mX, mY, mX + 160, mY + 40, Color(0.3f, 0.3f, 0.3f));
	gRender->Quad(mX, mY, mX + 160 * mScore / mMaxScore, mY + 40, color[mPlayerIdx]);
}

@implementation AppView

- (id)initWithFrame:(CGRect)frame 
{
    if ((self = [super initWithFrame:frame])) 
	{
		CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
		
		glState = new OpenGLState();
		glState->Initialize(layer, 320, 480, false, false);
		glState->CreateBuffers();
		
		gRender = new RenderIphone();
		gRender->Init(320, 480);
		
		gGame = new Game();
		gGame->Begin();
		
		activeThingy = 0;
		
		playerHud[0] = new PlayerInfoHud(0, 440, 0, 1000, 0);
		playerHud[1] = new PlayerInfoHud(160, 440, 0, 1000, 1);
		
		[self updateBegin];
    }
	
    return self;
}

+(Class)layerClass
{
	return [CAEAGLLayer class];
}

-(void)render
{
	glState->MakeCurrent();
	
	gRender->MakeCurrent();
	
	gRender->Clear();
	
//	gRender->Quad(0.0f, 0.0f, 160.0f, 240.0f, Color(1.0f, 0.0f, 0.0f));
	
	gGame->Render();
	
	for (int i = 0; i < MAX_PLAYERS; ++i)
		playerHud[i]->Render();
	
	gRender->Present();
	
	glState->Present();
}

-(void)updateBegin
{
	[self updateEnd];
	
	updateTimer = [NSTimer scheduledTimerWithTimeInterval:UPDATE_INTERVAL target:self selector:@selector(update) userInfo:nil repeats:YES];
}

-(void)updateEnd
{
	[updateTimer invalidate];
	updateTimer = nil;
}

-(void)update
{
	gGame->Update(UPDATE_INTERVAL);
	
	playerHud[0]->mScore = gGame->GetplayerState(0)->mScore;
	playerHud[1]->mScore = gGame->GetplayerState(1)->mScore;
	
	[self render];
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGPoint location = [[touches anyObject] locationInView:self];
	
	activeThingy = gGame->HitThingy(location.x, location.y, 30.0f);
	
	touchX = location.x;
	touchY = location.y;
	touchActive = false;
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	activeThingy = 0;
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	activeThingy = 0;
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGPoint location = [[touches anyObject] locationInView:self];
	
	if (activeThingy)
	{
		float dx = touchX - location.x;
		float dy = touchY - location.y;
		float d = std::sqrt(dx * dx + dy * dy);
		
		touchActive = d >= 20.0f;
		
		if (touchActive)
		{
			float angle = Vec2F(dx, dy).ToAngle();
			
			activeThingy->MakeAngle(angle);
		}
	}
}

- (void)dealloc 
{
	[self updateEnd];
	
	delete playerHud[0];
	delete playerHud[1];
	
	glState->DestroyBuffers();
	glState->Shutdown();
	delete glState;
	glState = 0;
	
    [super dealloc];
}


@end
