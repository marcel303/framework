#import <QuartzCore/QuartzCore.h>
#import "board.h"
#import "BoardView.h"
#import "game.h"
#import "OpenGLState.h"
#import "render.h"
#import "texture.h"

#define ANIMATION_INTERVAL (1.0f / 30.0f)

static void HandleAnimationTrigger(void* obj, void* arg);

@implementation BoardView

-(id)initWithCoder:(NSCoder *)aDecoder
{
	if ((self = [super initWithCoder:aDecoder]))
	{
		const int sx = self.bounds.size.width;
		const int sy = self.bounds.size.height;
		
		CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
		
		glState = new OpenGLState();
		glState->Initialize(layer, sx, sy, false, true);
		glState->CreateBuffers();
		
		gGame->Attach(CallBack(self, HandleAnimationTrigger));
		
		// create buttons: horizontal
		
		for (int x = 1; x <= 6; ++x)
		{
			// top
			buttons.push_back(ButtonDef(20.0f + x * 40.0f, 20.0f, 15.0f, Color(1.0f, 0.5f, 0.5f), 0, +1, x - 1));
			
			// bottom
			buttons.push_back(ButtonDef(20.0f + x * 40.0f, 320.0f - 20.0f, 15.0f, Color(1.0f, 0.5f, 0.5f), 0, -1, x - 1));
		}
		
		// create buttons: vertical
	
		for (int y = 1; y <= 6; ++y)
		{
			// left
			buttons.push_back(ButtonDef(20.0f, 20.0f + y * 40.0f, 15.0f, Color(0.5f, 0.5f, 1.0f), 1, +1, y - 1));
			
			// right
			buttons.push_back(ButtonDef(320.0f - 20.0f, 20.0f + y * 40.0f, 15.0f, Color(0.5f, 0.5f, 1.0f), 1, -1, y - 1));
		}
		
		activeButton = 0;
		activeButtonFocus = false;
		
		TextureRGBA* button = LoadTexture_TGA("button_rotate.tga");
		ToTexture(button, buttonTextureId);
		delete button;
		
		[self render];
    }
	
    return self;
}

+(Class)layerClass
{
	return [CAEAGLLayer class];
}

-(void)handleShuffle:(id)sender
{
}

-(void)performShuffle
{
}

-(void)handleWin
{
}

-(void)handleAnimationTrigger
{
	LOG_DBG("BoardView: handleAnimationTrigger", 0);
	
	[self animationBegin];
}

-(void)render
{
	glState->MakeCurrent();
	gRender->MakeCurrent();
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// draw board
	
	glPushMatrix();
	glTranslatef(40.0f, 40.0f, 0.0f);
	
	gGame->Board_get()->Render();
	
	glPopMatrix();
	
	// draw buttons
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, buttonTextureId);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	for (size_t i = 0; i < buttons.size(); ++i)
	{
		ButtonDef& b = buttons[i];
		
		Color c = b.c;
		
		if (&b == activeButton && activeButtonFocus)
			c = Color(1.0f, 1.0f, 1.0f);
		
		c.a = 0.99f;
		
		//gRender->Circle(b.x, b.y, b.r, c);
		gRender->QuadTex(b.x - b.r, b.y - b.r, b.x + b.r, b.y + b.r, c, 0.0f, 0.0f, 1.0f, 1.0f);
	}
	
	glDisable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	
	glState->Present();
}

-(void)animationBegin
{
	if (animationTimer != nil)
		return;
	
	LOG_DBG("BoardView: animationBegin", 0);
	
	animationTimer = [NSTimer scheduledTimerWithTimeInterval:ANIMATION_INTERVAL target:self selector:@selector(animationUpdate) userInfo:nil repeats:YES];
}

-(void)animationEnd
{
	LOG_DBG("BoardView: animationEnd", 0);
	
	[animationTimer invalidate];
	animationTimer = nil;
}

-(void)animationUpdate
{
	gGame->Board_get()->Update(ANIMATION_INTERVAL);
	
	[self render];
	
	// check if animation complete
	
	if (!gGame->Board_get()->IsAnimating_get())
	{
		[self animationEnd];
	}
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGPoint location = [[touches anyObject] locationInView:self];
	
	// todo: determine row/column and direction
	
	activeButton = 0;
	activeButtonFocus = false;
	
	for (size_t i = 0; i < buttons.size(); ++i)
	{
		ButtonDef& b = buttons[i];
		
		if (!b.IsInside(location.x, location.y))
			continue;
		
		activeButton = &b;
		activeButtonFocus = true;
	}
	
	[self render];
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGPoint location = [[touches anyObject] locationInView:self];
	
	if (activeButton && activeButtonFocus)
	{
		int coordIdx = activeButton->axis;
		int coord[2];
		coord[coordIdx] = activeButton->location;
		int direction = activeButton->direction;

		for (int i = 0; i < 6; ++i)
		{
			int t = direction < 0 ? 6 - 1 - i : i;
			float delay = t / 10.0f;
			coord[1 - coordIdx] = i;
			BoardTile* tile = gGame->Board_get()->Tile_get(coord[0], coord[1]);
			if (coordIdx == 0)
				tile->RotateX(direction, delay);
			if (coordIdx == 1)
				tile->RotateY(direction, delay);
		}
	}
	
	activeButton = 0;
	activeButtonFocus = false;
	
	[self render];
}

-(void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	activeButton = 0;
	activeButtonFocus = false;
	
	[self render];
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	CGPoint location = [[touches anyObject] locationInView:self];
	
	if (activeButton)
	{
		activeButtonFocus = activeButton->IsInside(location.x, location.y);
	}
	
	[self render];
}

-(void)dealloc 
{
	[self animationEnd];
	
	delete glState;
	glState = 0;
	
    [super dealloc];
}

@end

static void HandleAnimationTrigger(void* obj, void* arg)
{
	BoardView* view = (BoardView*)obj;
	
	[view handleAnimationTrigger];
}
