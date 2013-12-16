#import "AnimTimer.h"
#import "FileStream.h"
#import "GameState.h"
#import "System.h"
#import "TestCode.h"
#import "UsgResources.h"

@implementation TestCode

- (void)initWithGameView:(GameView*)gameView
{
	m_GameView = gameView;
	
	[self initUserInterface];
	[self initTextureAtlas];
	[self initSoundPlayer];
	[self initParticleEffect];
	[self initVectorShape];
	[self initHashMap];
	[self initResIndex];
	[self initWorldGrid];
}

- (void)initUserInterface
{
#if 0
	for (int i = 0; i < 10; ++i)
	{
		m_TestLabel = [[UILabel alloc] initWithFrame:CGRectMake(0.0f, i * 25.0f, 200.0f, 20.0f)];
		
		[m_TestLabel setText:@"Hello World !!!"];
		[m_TestLabel setOpaque:FALSE];
		[m_TestLabel setTextColor:[UIColor whiteColor]];
		[m_TestLabel setBackgroundColor:[UIColor clearColor]];
		
		[self addSubview:m_TestLabel];
	}
#endif
}

- (void)loopUserInterface
{
}

- (void)loopTextureAtlas
{
}

- (void)initTextureAtlas
{
}

- (void)initSoundPlayer
{
	m_SoundEffect1 = g_GameState->m_ResMgr.Get(Resources::SOUND_HIT_01);
	m_SoundEffect2 = g_GameState->m_ResMgr.Get(Resources::SOUND_HIT_01);
	
	[NSTimer scheduledTimerWithTimeInterval:1.0f target:self selector:@selector(loopSoundPlayer) userInfo:nil repeats:TRUE];
}

- (void)loopSoundPlayer
{
//	g_GameState->m_SoundEffects.Play(m_SoundEffect1);
//	g_GameState->m_SoundEffectMgr->Play(m_SoundEffect2);
}

- (void)initParticleEffect
{
	m_ParticleEffect = new ParticleEffect();
}

- (void)loopParticleEffect
{
	m_ParticleEffect->Update(1.0f / 60.0f);
	m_ParticleEffect->Render(g_GameState->m_SpriteGfx);
}

- (void)initVectorShape
{
}

- (void)loopVectorShape
{
//	static int frame = 0;
	
	TextureAtlas* textureAtlas = g_GameState->m_TextureAtlas[g_GameState->m_ActiveDataSet];
	
	//

//	const int n = 0;
	const int n = 50;
//	const int n = 150;
	
	const int worldSize[2] = { 480 * 3, 320 * 3 };
	
	static float px[n];
	static float py[n];
	static float pr[n];
	static float vx[n];
	static float vy[n];
	static float vr[n];
	static AnimTimer* at[n];
	static SpriteColor ac[n];
	static SpriteColor act[n];
	
	static VectorShape* shapes[] =
	{
//		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::ENEMY_BOMBER_01)->data,
//		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::ENEMY_BOMBER_02)->data,
		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::BOSS_01_MODULE_01)->data,
		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::BOSS_01_MODULE_02)->data,
		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::BOSS_01_MODULE_03)->data,
		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::BOSS_03_CENTER)->data,
//		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::BOSS_03_MAIN)->data,
		(VectorShape*)g_GameState->m_ResMgr.Get(Resources::BOSS_03_MODULE_01)->data
	};
	
	const static int shapeCount = sizeof(shapes) / sizeof(VectorShape*);
	
	static bool initialized = false;

	if (!initialized)
	{
		for (int i = 0; i < n; ++i)
		{
			px[i] = rand() % worldSize[0];
			py[i] = rand() % worldSize[1];
			pr[i] = Calc::Random(0.0f, Calc::mPI * 2.0f);
			vx[i] = Calc::Random(-1.0f, +1.0f);
			vy[i] = Calc::Random(-1.0f, +1.0f);
			vr[i] = Calc::Random(-1.0f, +1.0f) * 0.05f;
			at[i] = new AnimTimer(g_GameState->m_TimeTracker_Global, false);
			at[i]->Start(AnimTimerMode_TimeBased, false, Calc::Random(0.5f, 2.0f), AnimTimerRepeat_Loop);
			ac[i] = SpriteColor_Make(128 + (rand() & 127), 128 + (rand() & 127), 128 + (rand() & 127), 255);
		}
		
		for (int i = 0; i < shapeCount; ++i)
			shapes[i]->SetTextureAtlas(textureAtlas, "");
		
		initialized = true;
	}
	else
	{
		for (int i = 0; i < n; ++i)	
		{
			const float nx = px[i] + vx[i];
			const float ny = py[i] + vy[i];
			
			if (nx < 0.0f || nx > worldSize[0])
			{
				vx[i] *= -1.0f;
				vr[i] *= -1.0f;
			}
			else
				px[i] = nx;
			
			if (ny < 0.0f || ny > worldSize[1])
			{
				vy[i] *= -1.0f;
				vr[i] *= -1.0f;
			}
			else
				py[i] = ny;
			
			pr[i] += vr[i];
		}
	}
	
	//
	
	for (int i = 0; i < n; ++i)
	{
		at[i]->Tick();
		
		float progress = at[i]->Progress_get();
		
		if (progress < 0.5f)
			progress = progress * 2.0f;
		else
			progress = (1.0f - progress) * 2.0f;
		
		int c = (int)(64.0f + progress * 191.0f);
		
		SpriteColor color = SpriteColor_Make(c, c, c, 255);
		act[i] = SpriteColor_Modulate(ac[i], color);
//		act[i] = SpriteColor_Make(31, 31, 31, 255);
	}
	
	//
	
	SpriteColor silColor = SpriteColor_Make(31, 31, 31, 255);
	
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	g_GameState->DrawMode_set(VectorShape::DrawMode_Silhouette);
	
	for (int i = 0; i < n; ++i)
	{
		VectorShape* shape;
		
		const int index = i % shapeCount;
		
		shape = shapes[index];

		g_GameState->Render(shape, Vec2F(px[i] + 10.0f, py[i] + 10.0f), pr[i], silColor);
	}
	
//	glBlendFunc(GL_ONE, GL_ONE);
	
	g_GameState->DrawMode_set(VectorShape::DrawMode_Texture);
	
	for (int i = 0; i < n; ++i)
	{
		VectorShape* shape;
		
		const int index = i % shapeCount;
		
		shape = shapes[index];
		
		//
		
#if 0
		g_GameState->UpdateSB(shape, x, y, angle, 10);
#endif

		g_GameState->Render(shape, Vec2F(px[i], py[i]), pr[i], act[i]);
	}

//	m_GameState->m_SpriteGfx.Flush();
	
//	frame++;
}

- (void)initHashMap
{
}

- (void)loopHashMap
{
}

- (void)initResIndex
{
	m_ResIndex = new ResIndex();
	
	FileStream stream;
	stream.Open(g_System.GetResourcePath("Resources.idx").c_str(), OpenMode_Read);
	
	m_ResIndex->LoadBinary(&stream);
}

- (void)loopResIndex
{
}

- (void)initWorldGrid
{
	m_WorldGrid = new Game::WorldGrid();
	
	m_WorldGrid->Setup(480 * 3, 320 * 3, 128.0f, 128.0f);
	
	m_WorldGridEntities[0] = new Vec2F(10.0f, 10.0f);
	m_WorldGridEntities[0] = new Vec2F(20.0f, -10.0f);
	m_WorldGridEntities[0] = new Vec2F(30.0f, 10.0f);
	m_WorldGridEntities[0] = new Vec2F(-40.0f, 10.0f);
	
	m_WorldGrid->Add(*m_WorldGridEntities[0], m_WorldGridEntities[0]);
	m_WorldGrid->Add(*m_WorldGridEntities[1], m_WorldGridEntities[1]);
	m_WorldGrid->Add(*m_WorldGridEntities[2], m_WorldGridEntities[2]);
	m_WorldGrid->Add(*m_WorldGridEntities[3], m_WorldGridEntities[3]);
	
#if 0
	delete m_WorldGridEntities[0];
	delete m_WorldGridEntities[1];
	delete m_WorldGridEntities[2];
	delete m_WorldGridEntities[3];
	
	delete m_WorldGrid;
#endif
}

- (void)loopWorldGrid
{
	for (int i = 0; i < 100; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			Vec2F oldPosition = *m_WorldGridEntities[j];
			
			*m_WorldGridEntities[j] += Vec2F(50.0f, 50.0f);
			
			Vec2F newPosition = *m_WorldGridEntities[j];
			
			m_WorldGrid->Update(oldPosition, newPosition, m_WorldGridEntities[j]);
		}
	}
}

@end
