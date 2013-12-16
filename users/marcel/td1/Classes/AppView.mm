#import <QuartzCore/QuartzCore.h>
#import "AppView.h"
#import "AppViewMgr.h"
#import "enemy_info.h"
#import "game.h"
#import "level.h"
#import "OpenGLState.h"
#import "particle.h"
#import "RenderIphone.h"
#import "tower.h"

#define RENDER_INTERVAL (1.0f / 60.0f)

@implementation AppView

@synthesize delegate;

-(id)initWithCoder:(NSCoder *)aDecoder
{
	if ((self = [super initWithCoder:aDecoder]))
	{
		self.contentScaleFactor = [UIScreen mainScreen].scale;
		
		CAEAGLLayer* layer = (CAEAGLLayer*)self.layer;
		int sx = self.bounds.size.width;
		int sy = self.bounds.size.height;
		glState = new OpenGLState();
		glState->Initialize(layer, sx, sy, false, false);
		glState->CreateBuffers();
		
		gRender = new RenderIphone();
		gRender->Init(sx, sy);
		
		gParticleMgr = new ParticleMgr(1000);
		
		gEnemyInfoMgr = new EnemyInfoMgr(1000);
		
		gGame = new Game();
		
		placementState = new PlacementState();
		
		waterGrid = new WaterGrid();
//		waterGrid->SetSize(5, 6);
		waterGrid->SetSize(10, 15);
//		waterGrid->SetSize(20, 30);
//		waterGrid->SetSize(40, 60);
		
		[NSTimer scheduledTimerWithTimeInterval:RENDER_INTERVAL target:self selector:@selector(render) userInfo:nil repeats:YES];
    }
    return self;
}

+(Class)layerClass
{
	return [CAEAGLLayer class];
}

-(void)render
{
	[delegate updateUi];
	
	glState->MakeCurrent();
	
	gGame->Update(RENDER_INTERVAL);
	gParticleMgr->Update(RENDER_INTERVAL);
	waterGrid->Update(RENDER_INTERVAL);
	
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrthof(0.0f, self.bounds.size.width, self.bounds.size.height, 0.0f, -1000.0f, +1000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	glPushMatrix();
	glDisable(GL_BLEND);
	float scaleX = self.bounds.size.width / (waterGrid->m_Sx - 1);
	float scaleY = self.bounds.size.height / (waterGrid->m_Sy - 1);
	glScalef(scaleX, scaleY, 1.0f);
	waterGrid->DrawPrepare();
	waterGrid->Draw();
	glPopMatrix();
	
	gGame->Render();
	gParticleMgr->Render();
	
	if (placementState->isActive)
	{
		if (placementState->isFree)
		{
			gRender->Circle(placementState->location[0], placementState->location[1], placementState->radius, Color(0.0f, 1.0f, 0.0f, 0.5f));
		}
		else
		{
			gRender->Circle(placementState->location[0], placementState->location[1], placementState->radius, Color(1.0f, 0.0f, 0.0f, 0.5f));
		}
	}
	
	glState->Present();
}

-(void)placementSet:(Vec2F)location
{
	placementState->location = location - Vec2F(0.0f, 40.0f);
	placementState->isFree = gGame->Level_get()->IsFree(placementState->location, placementState->radius);
}

-(void)handleExplosion:(Vec2F)location strength:(float)strength
{
	float scaleX = self.bounds.size.width / (waterGrid->m_Sx - 0);
	float scaleY = self.bounds.size.height / (waterGrid->m_Sy - 0);
	location[0] /= scaleX;
	location[1] /= scaleY;
	waterGrid->Impulse(location[0], location[1], strength);
}

-(void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch* touch = [touches anyObject];
	Vec2F location = Vec2F([touch locationInView:self]);
	
	Tower* tower = gGame->Level_get()->HitTest_Tower(location);
	
	if (tower)
	{
		gGame->SelectedTower_set(tower);
	}
	else
	{
		gGame->DeselectTower();
		
		if (gGame->TowerPlacementType_get() != TowerType_Undefined)
		{
			placementState->isActive = true;
			placementState->radius = 15.0f;
			placementState->type = gGame->TowerPlacementType_get();
			placementState->cost = Tower::GetCost(gGame->TowerPlacementType_get(), 0);
		
			[self placementSet:location];
		}
	}
}

-(void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
	if (placementState->isActive)
	{
		placementState->isActive = false;
		
		if (placementState->isFree)
		{
			// check money
			
			if (gGame->Level_get()->BuildMoney_get() >= placementState->cost)
			{
				// subtract money
				
				gGame->Level_get()->BuildMoney_decrease(placementState->cost);
			
				// create tower
				
				TowerDesc desc;
				desc.type = placementState->type;
				desc.position = placementState->location;
				desc.level = 0;
				Tower* tower = new Tower();
				tower->Make(desc);
				gGame->Level_get()->AddTower(tower);
				
				// select tower upon placement
				
				gGame->SelectedTower_set(tower);
				
				[self handleExplosion:placementState->location strength:10.0f];
			}
		}
	}
}

-(void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
	UITouch* touch = [touches anyObject];
	Vec2F location = Vec2F([touch locationInView:self]);
	
	if (placementState->isActive)
	{
		[self placementSet:location];
	}
}

-(void)dealloc 
{
	delete waterGrid;
	waterGrid = 0;
	
	delete placementState;
	placementState = 0;
	
	delete gGame;
	gGame = 0;
	
	delete gEnemyInfoMgr;
	gEnemyInfoMgr = 0;
	
	delete gParticleMgr;
	gParticleMgr = 0;
	
	delete gRender;
	gRender = 0;
	
//	alState->Shutdown();
//	delete alState;
//	alState = 0;
	
	glState->DestroyBuffers();
	glState->Shutdown();
	delete glState;
	glState = 0;
	
	self.delegate = nil;
	
    [super dealloc];
}

@end
