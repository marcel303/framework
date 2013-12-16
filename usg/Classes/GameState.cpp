#include "Atlas.h"
#ifdef IPHONEOS
#include "AudioSession.h"
#endif
#include "Benchmark.h"
#include "Camera.h"
#include "ConfigState.h"
#include "Entity.h"
#include "BanditEntity.h" // time dilation based on bandit strength
#include "EntityPlayer.h"
#include "EventManager.h"
#include "FontMap.h"
#include "GameHelp.h"
#include "GameNotification.h"
#include "GameRound.h"
#include "GameSave.h"
#include "GameScore.h"
#include "GameState.h"
#include "Grid_Effect.h"
#include "Graphics.h"
#include "ISoundPlayer.h"
#if defined(IPHONEOS) || defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
	#include "OpenGLCompat.h"
	#include "OpenGLUtil.h"
	#include "SoundEffectMgr_OpenAL.h"
#endif
#if defined(PSP)
	#include "SoundEffectMgr_Psp.h"
#endif
#include "MenuMgr.h"
#include "PerfCount.h"
#include "World.h"
#include "SoundEffectMgr.h"
#include "SoundPlayerFactory.h"
#include "SpriteEffectMgr.h"
#include "StringBuilder.h"
#include "StringEx.h"
#include "System.h"
#include "TempRender.h"
#include "Textures.h"
#include "TouchDLG.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "World.h"

#define SFX_CHANNEL_COUNT 20

// views

#include "View_BanditIntro.h"
#include "View_Credits.h"
#include "View_GameOver.h"
#include "View_GameSelect.h"
#include "View_KeyBoard.h"
#include "View_Main.h"
#include "View_Options.h"
#include "View_Pause.h"
#include "View_PauseTouch.h"
#include "View_ScoreEntry.h"
#include "View_Scores.h"
#if defined(PSP_UI)
#include "View_ScoresPSP.h"
#endif
#include "View_ScoreSubmit.h"
#if defined(IPHONEOS) || defined(BBOS)
#include "View_ScoreAutoSubmit.h"
#endif
#if (defined(IPAD) && 0) || (defined(MACOS) && 0)
#include "View_UpgradeHD.h"
#else
#include "View_Upgrade.h"
#endif
#include "View_CustomSettings.h"
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
#include "View_GamepadSetup.h"
#endif
#include "View_World.h"

#include "AnimSeq.h"
#include "SpriteAnim.h"

//

#define GRS_HOST "grannies-games.com"
#define GRS_PORT 80
#define GRS_PASS "1234567890123456"

#if !defined(DEPLOYMENT)
#pragma message("warning: using test GRS path")
#define GRS_PATH "testweb/grs_server.php"
#else
#define GRS_PATH "grs_server.php"
#endif

#if defined(BBOS_ALLTOUCH)
#include "SocialAPI_ScoreLoop.h"
#else
#include "SocialAPI_Dummy.h"
#endif
#include "SocialIntegration.h"

#if defined(MACOS) || !defined(DEPLOYMENT)
#include "AngleController.h"
#include "FileStream.h"
#include "MD3.h"
#include "EntityPlayer.h"
#include "PlayerController.h"
#endif

//

Application* g_GameState = 0;

//

int EVT_PAUSE = EventManager::I().AllocateEventId();
int EVT_MENU_LEFT = EventManager::I().AllocateEventId();
int EVT_MENU_RIGHT = EventManager::I().AllocateEventId();
int EVT_MENU_UP = EventManager::I().AllocateEventId();
int EVT_MENU_DOWN = EventManager::I().AllocateEventId();
int EVT_MENU_BACK = EventManager::I().AllocateEventId();
int EVT_MENU_SELECT = EventManager::I().AllocateEventId();
int EVT_MENU_NEXT = EventManager::I().AllocateEventId();
int EVT_MENU_PREV = EventManager::I().AllocateEventId();

//

Application::Application()
{
	m_Log = LogCtx("GameState");
	
	m_ScreenScale = 1.0f;

	m_TouchDLG = 0;

	m_Timer = 0;
	m_TimeTracker_GlobalUnsafe = 0;
	m_TimeTracker_Global = 0;
	m_TimeTracker_World = 0;

	m_GameHelp = 0;
	m_HelpState = 0;
	m_GameNotification = 0;
	m_Score = 0;
	m_GameRound = 0;
	m_GameSave = 0;
	m_GameSettings = 0;
	m_SpriteEffectMgr = 0;

	m_SoundPlayer = 0;
	m_SoundEffectMgr = 0;
	m_Music = 0;

	// resources

	for (int i = 0; i < DATASET_COUNT; ++i)
	{
		m_Res_TextureAtlas[i] = 0;
		m_TextureAtlas[i] = 0;
	}
	
	// data sets

	m_ActiveDataSet = -1;
	m_DataSetItr = -1;
	m_DataSetChangeCount = 0;

	// drawing

	m_RenderStageEndCallBack = 0;

	// world
	
	m_World = 0;

	// views
	
	m_ActiveView = View_Undefined;
	m_PreviousView = View_Undefined;
	m_TransitionTime = 0.0f;

	// social

	m_Social = 0;
	m_SocialListener = 0;
}

void Application::Setup(void* openALState, float screenScale, ApplicationSetupCallback callback, ApplicationRenderStageEndCallBack renderStageCallBack)
{
	g_GameState = this;
	
	//
	
	//g_System.CheckNetworkConnectivity();

	m_ScreenScale = screenScale;
	
	m_TouchDLG = new TouchDelegator();

	m_Timer = new Timer();
	m_TimeTracker_GlobalUnsafe = new TimeTracker();
	m_TimeTracker_Global = new TimeTracker();
	m_TimeTracker_World = new TimeTracker();

	m_GameHelp = new Game::GameHelp();
	m_HelpState = new Game::HelpState();
	m_GameNotification = new Game::GameNotification();
	m_Score = new Game::GameScore();
	m_GameRound = new Game::GameRound();
	m_GameSave = new Game::GameSave();
	m_GameSettings = new Game::GameSettings();
	m_SpriteEffectMgr = new Game::SpriteEffectMgr();

	// ----------------------------------------
	// Settings pt 1
	// ----------------------------------------

	ConfigLoad();

	m_GameSettings->Load();

#if !defined(PSP)
	m_GameSettings->m_StartupCount++;

	m_GameSettings->Save();
#endif
	
	// ----------------------------------------
	// Pools
	// ----------------------------------------
	
	if (callback)
		callback("graphics and pools");

	UsingBegin(Benchmark b("Setting up GFX & CD pools"))
	{
		m_SpriteGfx = new SpriteGfx();
#ifdef PSP
		m_SpriteGfx->Setup(40000, 60000, SpriteGfxRenderTime_OnFrameEnd);
#else
		m_SpriteGfx->Setup(4000, 6000, SpriteGfxRenderTime_OnFlush);
#endif
		m_SelectionBuffer.Setup(WORLD_SX, WORLD_SY, 30000);

		m_ParticleEffect.Initialize(DS_GAME);
		//m_ParticleEffect_Primary.Initialize(DS_GAME);
		m_ParticleEffect_UI.Initialize(DS_GAME);
	}
	UsingEnd()

	// ----------------------------------------
	// Resources
	// ----------------------------------------
	
	FixedSizeString<8> texturePrefix;
	if (screenScale == 1.0f)
		texturePrefix = "lq_";
	else if (screenScale == 2.0f)
		texturePrefix = "hq_";
	else
		texturePrefix = "hq_";
	
	m_ResMgr.Initialize(texturePrefix.c_str());
	
	if (callback)
		callback("loading resources");

	UsingBegin(Benchmark b("Loading resource index"))
	{
		m_ResMgr.Load(Resources::ResIndex, sizeof(Resources::ResIndex) / sizeof(CompiledResInfo));
		
		_GetFont(Resources::FONT_CALIBRI_LARGE)->m_Font.SpaceSx_set(10);
		_GetFont(Resources::FONT_CALIBRI_SMALL)->m_Font.SpaceSx_set(4);
		_GetFont(Resources::FONT_USUZI_SMALL)->m_Font.SpaceSx_set(10);
		_GetFont(Resources::FONT_LGS)->m_Font.SpaceSx_set(5);
	}
	UsingEnd()
	
	if (callback)
		callback("loading textures");

	UsingBegin(Benchmark b("Loading texture atlas"))
	{
		for (int i = 0; i < DATASET_COUNT; ++i)
		{
			StringBuilder<32> fileName;
			fileName.AppendFormat("%s%d_%s", texturePrefix.c_str(), i, "textures.atl");
			
			m_Res_TextureAtlas[i] = m_ResMgr.CreateTextureAtlas(fileName.ToString());
			m_Res_TextureAtlas[i]->m_DataSetId = i;
			m_TextureAtlas[i] = (TextureAtlas*)m_Res_TextureAtlas[i]->data;
			m_TextureAtlas[i]->m_Texture->m_DataSetId = i;
		}

		for (int id = 0; id < (int)(sizeof(Textures::IndexToDataSet) / sizeof(int)); ++id)
		{
			const int ds = GetTextureDS(id);
			const int localId = GetTextureID(id);
			AtlasImageMap* map = new AtlasImageMap();
			map->m_TextureAtlas = m_Res_TextureAtlas[ds];
			map->m_Info = (const Atlas_ImageInfo*)m_TextureAtlas[ds]->m_Atlas->GetImage(localId);
			Assert(map->m_Info != 0);
			gTexturesById[id] = map;
		}
	}
	UsingEnd()

	UsingBegin(Benchmark b("Warming up texture cache"))
	{
		for (int i = 0; i < DATASET_COUNT; ++i)
		{
			if (callback)
			{
				StringBuilder<32> sb;
				sb.AppendFormat("loading texture data %d/%d", i + 1, DATASET_COUNT);
				callback(sb.ToString());
			}

			if (m_Res_TextureAtlas[i] == 0)
				continue;

			gGraphics.TextureCreate(m_TextureAtlas[i]->m_Texture);
		}

		if (callback)
			callback("loading some more textures");

		for (int i = 0; i < m_ResMgr.m_ResourceCount; ++i)
		{
#if defined(IPHONEOS)
			if (m_ResMgr.m_Resources[i]->m_Type != ResTypes_TexturePVR &&
			#if defined(IPAD)
				m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureRGBA &&
			#endif
				1)
				continue;
#elif defined(WIN32) || defined(LINUX) || defined(MACOS)
			if (m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureRGBA)
				continue;
#elif defined(PSP)
			if (m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureDDS &&
				m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureRGBA)
				continue;
#elif defined(BBOS)
			if (m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureRGBA)
				continue;
#else
	#error
#endif

			gGraphics.TextureCreate(m_ResMgr.m_Resources[i]);
		}
	}
	UsingEnd()
	
	// todo: move to another method

	if (callback)
		callback("linking resources");
	
	UsingBegin(Benchmark b("Linking resources"))
	{
		for (int i = 0; i < DATASET_COUNT; ++i)
		{
			for (int j = 0; j < m_ResMgr.m_ResourceCount; ++j)
			{
				Link(&m_ResMgr, i, m_TextureAtlas[i], j, texturePrefix.c_str());
			}
		}
	}
	UsingEnd()
	
	if (callback)
		callback("initializing sounds");

	UsingBegin(Benchmark b("Uploading sounds"))
	{
		for (int i = 0; i < m_ResMgr.m_ResourceCount; ++i)
		{
			Res* res = m_ResMgr.Get(i);
			
			if (!res)
				continue;
			
			if (res->m_Type == ResTypes_SoundEffect)
			{
				// todo: use sound effect mgr to warm-up sounds

#if defined(IPHONEOS) || defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
				OpenALState* state = (OpenALState*)openALState;
				state->CreateSound(res);
#endif
			}
		}
	}
	UsingEnd()
	
	// ----------------------------------------
	// Sound
	// ----------------------------------------
	
	if (callback)
		callback("initializing music player");

	UsingBegin(Benchmark b("Initializing sound player"))
	{
#ifdef IPHONEOS
		bool playBackground = AudioSession_PlayBackgroundMusic();
#elif defined(WIN32)
		bool playBackground = false;
#elif defined(LINUX)
		bool playBackground = false;
#elif defined(MACOS)
		bool playBackground = false;
#elif defined(PSP)
		bool playBackground = false;
#elif defined(BBOS)
		bool playBackground = false;
#else
#error
#endif

		m_SoundPlayer = SoundPlayerFactory::Create();

		m_SoundPlayer->Initialize(playBackground);
	}
	UsingEnd()
	
	if (callback)
		callback("initializing sound effect manager");

	UsingBegin(Benchmark b("Initializing sound effect MGT"))
	{
#if defined(IPHONEOS) || defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
		SoundEffectMgr_OpenAL* mgr = new SoundEffectMgr_OpenAL();
		OpenALState* state = (OpenALState*)openALState;
		mgr->Initialize(state, SFX_CHANNEL_COUNT);
		m_SoundEffectMgr = mgr;
#elif defined(PSP)
		SoundEffectMgr_Psp* mgr = new SoundEffectMgr_Psp();
		mgr->Initialize(SFX_CHANNEL_COUNT);
		m_SoundEffectMgr = mgr;
#else
#error
#endif

		m_SoundEffects = new SoundEffectFenceMgr();
		m_SoundEffects->Setup(m_SoundEffectMgr);
	}
	UsingEnd()
	
	// ----------------------------------------
	// Views
	// ----------------------------------------
	m_ActiveView = View_Undefined;
	
	// ----------------------------------------
	// Drawing
	// ----------------------------------------
	
	m_RenderStageEndCallBack = renderStageCallBack;

	// note: add a little space at the bottom to ensure the player's ship is always visible
	
	m_Camera = new Camera();

#ifdef IPAD
	m_Camera->Setup(VIEW_SX, VIEW_SY, Vec2F(0.0f, -30.0f), Vec2F(WORLD_SX, WORLD_SY + 30));
#else
	m_Camera->Setup(VIEW_SX, VIEW_SY, Vec2F(0.0f, -30.0f), Vec2F(WORLD_SX, WORLD_SY + 130));
#endif
	
	// ----------------------------------------
	// Interface
	// ----------------------------------------
	
	if (callback)
		callback("initializing UI");

	UsingBegin(Benchmark b("Initializing interface"))
	{
		m_MenuMgr = new GameMenu::MenuMgr();
		m_MenuMgr->Initialize();
	}
	UsingEnd()
	
	// ----------------------------------------
	// World
	// ----------------------------------------
	
	if (callback)
		callback("creating world");
	
	UsingBegin(Benchmark b("Creating world"))
	{
		m_World = new Game::World();
		
		m_World->Setup();
		
//		m_World->m_TouchZoomController.OnZoomInAnimationBegin = CallBack(this, HandleZoomIn);
//		m_World->m_TouchZoomController.OnZoomOutAnimationEnd = CallBack(this, HandleZoomOut);
	}
	UsingEnd()
	
	m_GameRound->Classic_OnWaveBegin = CallBack(m_World, m_World->HandleWaveBegin);
	m_GameRound->Classic_OnMaxiBossBegin = CallBack(m_World, m_World->HandleBossBegin);
	m_GameRound->Classic_OnLevelCleared = CallBack(m_World, m_World->HandleLevelCleared);
	
	// ----------------------------------------
	// Social
	// ----------------------------------------
	UsingBegin(Benchmark b("Initializing SocialAPI"))
	{
		#if defined(BBOS_ALLTOUCH)
		m_Social = new SocialSC();
		#else
		m_Social = new SocialAPI_Dummy();
		#endif

		Assert(m_Social != 0);

		m_SocialListener = new Game::UsgSocialListener();

		m_Social->Initialize(m_SocialListener);
	}
	UsingEnd()

	// ----------------------------------------
	// ScoreDB
	// ----------------------------------------
	
	UsingBegin(Benchmark b("Initializing ScoreDB"))
	{
		m_Score->Initialize();
	}
	UsingEnd()
	
	// ----------------------------------------
	// Views
	// ----------------------------------------
	
	if (callback)
		callback("creating views");

	UsingBegin(Benchmark b("Creating views"))
	{
		for (int i = 0; i < View__End; ++i)
			m_Views[i] = 0;
		
		m_Views[View_BanditIntro] = new Game::View_BanditIntro((int)screenScale);
		m_Views[View_Credits] = new Game::View_Credits();
		m_Views[View_GameOver] = new Game::View_GameOver();
		m_Views[View_GameSelect] = new Game::View_GameSelect();
		m_Views[View_KeyBoard] = new Game::View_KeyBoard();
		m_Views[View_Main] = new Game::View_Main();
		m_Views[View_Options] = new Game::View_Options();
		m_Views[View_Pause] = new Game::View_Pause();
//		m_Views[View_PauseTouch] = new Game::View_PauseTouch();
		m_Views[View_ScoreEntry] = new Game::View_ScoreEntry();
#if defined(PSP_UI)
		m_Views[View_ScoresPSP] = new Game::View_ScoresPSP();
#endif
		m_Views[View_ScoreSubmit] = new Game::View_ScoreSubmit();
#if defined(IPHONEOS) || defined(BBOS)
		m_Views[View_ScoreAutoSubmit] = new Game::View_ScoreAutoSubmit();
#endif
#if (defined(IPAD) && 0) || (defined(MACOS) && 0)
		m_Views[View_Upgrade] = new Game::View_UpgradeHD();
#else
		m_Views[View_Upgrade] = new Game::View_Upgrade();
#endif
		m_Views[View_CustomSettings] = new Game::View_CustomSettings();
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
		m_Views[View_WinSetup] = new Game::View_WinSetup();
#endif
		
		Game::View_Scores* scoresView = new Game::View_Scores();
		scoresView->Show(scoresView->DefaultDatabase_get(), Difficulty_Easy, m_GameSettings->m_ScoreHistory);
		m_Views[View_Scores] = scoresView;
		
		Game::View_World* worldView = new Game::View_World();
		m_Views[View_InGame] = worldView;
		
		for (int i = 0; i < View__End; ++i)
		{
			View view = (View)i;
			
			if (m_Views[view] == 0)
				throw ExceptionVA("view not instantiated: %d", i);
			
			m_Views[view]->Initialize();
		}
	}
	UsingEnd()
	
	const float helpSx = 250.0f;
	m_GameHelp->Setup(Vec2F((VIEW_SX-helpSx)/2, 48.0f), Vec2F(helpSx, 80.0f));
	m_GameNotification->Setup(Vec2F(100.0f, VIEW_SY-130.0f), Vec2F(VIEW_SX - 100.0f * 2.0f, 40.0f));
	
	// ----------------------------------------
	// Settings pt 2
	// ----------------------------------------
	
	if (callback)
		callback("applying settings");

	UsingBegin(Benchmark b("Applying settings"))
	{
		m_GameSettings->Apply();
	}
	UsingEnd()
	
	// ----------------------------------------
	// Test
	// ----------------------------------------
	
	DBG_RenderMask = 0xFFFFFFFF;
	
#ifndef DEPLOYMENT
#if 0
	Game::View_KeyBoard* view = (Game::View_KeyBoard*)GetView(View_KeyBoard);
	
	view->Show(View_Main);
#elif 0
	ActiveView_set(View_Upgrade);
#elif 0
	ActiveView_set(View_ScoreEntry);
#else
	ActiveView_set(View_Main);
#endif
#else
	ActiveView_set(View_Main);
#endif

	//

	m_TimeTracker_GlobalUnsafe->Time_set(m_Timer->Time_get());
	m_TimeTracker_Global->Time_set(m_Timer->Time_get());
}

Application::~Application()
{
	for (int i = 0; i < View__End; ++i)
	{
		delete m_Views[i];
		m_Views[i] = 0;
	}
	
	m_Social->Shutdown();
	delete m_Social;
	m_Social = 0;

	delete m_SocialListener;
	m_SocialListener = 0;

	delete m_World;
	m_World = 0;

	delete m_MenuMgr;
	m_MenuMgr = 0;

	delete m_Camera;
	m_Camera = 0;

	delete m_SoundEffects;
	m_SoundEffects = 0;
	m_SoundEffectMgr->Shutdown();
	delete m_SoundEffectMgr;
	m_SoundEffectMgr = 0;
	m_SoundPlayer->Shutdown();
	delete m_SoundPlayer;
	m_SoundPlayer = 0;

	for (int i = 0; i < m_ResMgr.m_ResourceCount; ++i)
	{
		if (
#if !defined(PSP)
			m_ResMgr.m_Resources[i]->m_Type != ResTypes_TexturePVR &&
#endif
#if defined(IPAD)
			m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureRGBA &&
#endif
#if defined(WIN32)
			m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureRGBA &&
#endif
#if defined(PSP)
			m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureDDS &&
			m_ResMgr.m_Resources[i]->m_Type != ResTypes_TextureRGBA &&
#endif
			1)
			continue;

		gGraphics.TextureDestroy(m_ResMgr.m_Resources[i]);
	}

	for (int i = 0; i < DATASET_COUNT; ++i)
	{
		if (m_Res_TextureAtlas[i] == 0)
			continue;

		gGraphics.TextureDestroy(m_TextureAtlas[i]->m_Texture);
	}

	for (std::map<int, AtlasImageMap*>::iterator i = gTexturesById.begin(); i != gTexturesById.end(); ++i)
	{
		AtlasImageMap* map = i->second;
		delete map;
		map = 0;
	}

	gTexturesById.clear();

	for (int i = 0; i < DATASET_COUNT; ++i)
	{
		if (m_Res_TextureAtlas[i] == 0)
			continue;
		m_Res_TextureAtlas[i]->Close();
		delete m_Res_TextureAtlas[i];
		m_Res_TextureAtlas[i] = 0;
	}

	delete m_SpriteGfx;
	m_SpriteGfx = 0;

	//

	delete m_GameHelp; m_GameHelp = 0;
	delete m_HelpState; m_HelpState = 0;
	delete m_GameNotification; m_GameNotification = 0;
	delete m_Score; m_Score = 0;
	delete m_GameRound; m_GameRound = 0;
	delete m_GameSave; m_GameSave = 0;
	delete m_GameSettings; m_GameSettings = 0;
	delete m_SpriteEffectMgr; m_SpriteEffectMgr = 0;

	delete m_TimeTracker_GlobalUnsafe; m_TimeTracker_GlobalUnsafe = 0;
	delete m_TimeTracker_Global; m_TimeTracker_Global = 0;
	delete m_TimeTracker_World; m_TimeTracker_World = 0;
	delete m_Timer; m_Timer = 0;

	delete m_TouchDLG; m_TouchDLG = 0;
}

// --------------------
// Views
// --------------------

void Application::ActiveView_set(View view)
{
	m_Log.WriteLine(LogLevel_Debug, "ActiveView_set: %d", (int)view);
	
	Assert(view != m_ActiveView); // Will crash with debug custom... doesn't matter
	
	// update which interface is shown
	
	m_PreviousView = m_ActiveView;
	
	IView* oldView = GetView(m_ActiveView);
	IView* newView = GetView(view);
	
	if (oldView)
		oldView->HandleFocusLost();
	if (newView)
		newView->HandleFocus();
	
	m_ActiveView = view;
	
	m_TransitionTime = m_TimeTracker_Global->Time_get();
}

IView* Application::GetView(View view)
{
	if (view == View_Undefined)
		return 0;
	
	return m_Views[view];
}

std::string Application::KeyboardText_get()
{
	return KeyboardView_get()->Text_get();
}

void Application::KeyboardText_set(const char* text)
{
	KeyboardView_get()->Text_set(text);
}

// --------------------
// Logic
// --------------------

struct MaxiInsulationQuery
{
	MaxiInsulationQuery() : insulation(0) { }
	
	uint32_t insulation;
};

static void HandleMaxiInsulationQuery(void* obj, void* arg)
{
	MaxiInsulationQuery* query = (MaxiInsulationQuery*)obj;
	Game::Entity* entity = (Game::Entity*)arg;
	
	if (entity->Class_get() == Game::EntityClass_MaxiBoss)
	{
		Bandits::EntityBandit* bandit = (Bandits::EntityBandit*)entity;
		query->insulation += bandit->Bandit_get()->Insulation_get();
	}
}

void Application::Update(float multiplier)
{
	// Update @ 60 Hz
	
#if defined(BBOS) || defined(IPHONEOS)
	float dt_Global = 1.f / 60.f;
#else
	float dt_Global = m_Timer->Time_get() - m_TimeTracker_GlobalUnsafe->Time_get();
#endif

	m_TimeTracker_GlobalUnsafe->Increment(dt_Global);
	if (dt_Global < 0.001f)
		dt_Global = 0.001f;
	if (dt_Global > 1.0f / 20.0f)
		dt_Global = 1.0f / 20.0f;
	m_TimeTracker_Global->Increment(dt_Global);
	
	// Calculate a multiplier to simulate frame drops and to restore our original balancing based on the original iPhone .. (42-ish FPS during intense battles)
	// Make sure not to slow down if the device is already missing frames!
	float fpsMultiplier = 1.0f;
	if (Game::g_World->AliveMaxiCount_get() > 0 && m_World->m_Player->IsPlaying_get())
	{
		MaxiInsulationQuery query;
		Game::g_World->ForEachDynamic(CallBack(&query, HandleMaxiInsulationQuery));
		const float t = Calc::Min(1.0f, query.insulation / 50.0f);
		fpsMultiplier = Calc::Lerp(1.0f, 0.75f, t);            // Multiplier assuming the device maintains a stready 60 FPS
		fpsMultiplier = dt_Global * 60.0f * fpsMultiplier;    // Actual multiplier given device speed
		fpsMultiplier = Calc::Mid(fpsMultiplier, 0.75f, 1.0f); // Clamp multiplier just to be sure
	}
	
	const float dt_World = 1.0f / 60.0f * multiplier * fpsMultiplier * Game::g_World->m_Player->GetSlowMoTimeDilution();

	// Update interface first
	
	m_MenuMgr->Update(dt_Global); 
	
	if ((m_ActiveView == View_InGame || m_ActiveView == View_Upgrade) && !m_GameRound->GameModeIsIntroScreen())
	{
		m_GameHelp->Update(dt_Global);
		m_HelpState->Update(dt_Global);
	}
	
	if (m_ActiveView == View_InGame)
	{
		m_GameNotification->Update(dt_Global);
	}
	
	IView* activeView = GetView(m_ActiveView);
	
	if (activeView)
	{
		activeView->Update(dt_World);
		activeView->UpdateAnimation(dt_Global);
	}
	
	IView* previousView = GetView(m_PreviousView);
	
	if (previousView)
	{
		if (previousView->IsFadeActive(m_TransitionTime, m_TimeTracker_Global->Time_get()))
			previousView->UpdateAnimation(dt_Global);
	}
	
	// Update animation effects
	
#if SCREENSHOT_MODE == 1
	bool updateParticles = m_World->m_IsControllerPaused == false;
#else
	bool updateParticles = true;
#endif
	
	if (updateParticles)
	{
		//m_ParticleEffect_Primary.Update(dt_World);
		m_ParticleEffect.Update(dt_World);
	}
	
	m_ParticleEffect_UI.Update(dt_Global);
	
	// Update world animation
	// Note: We updated twice previously, in World::Update and here
	//       The update in World has been removed, and we update
	//       *twice* here so we know for sure we don't break anything..
#if SCREENSHOT_MODE == 1
	if (!m_World->m_IsControllerPaused)
	{
		m_World->Update_Animation(dt_World);
		m_World->Update_Animation(dt_World);
	}
#else
	m_World->Update_Animation(dt_World);
	m_World->Update_Animation(dt_World);
#endif
	
	m_SpriteEffectMgr->Update();
	
	m_SoundEffects->Update();

	m_SoundPlayer->Update();
}

// --------------------
// Sound
// --------------------
Res* Application::GetMusic()
{
	return m_Music;
}

void Application::PlayMusic(Res* res1, Res* res2, Res* res3, Res* res4)
{
	m_Music = res1;

	m_SoundPlayer->Play(res1, res2, res3, res4, true);
}

// --------------------
// Logic
// --------------------

void Application::UpdateSB(const VectorShape* shape, float x, float y, float angle, int id)
{
	Assert(shape != 0);
	
	// prepare transform
	
	float axis[4];

	Calc::RotAxis_Fast(angle, axis);
	
	// render collision circles
	
	for (int i = 0; i < shape->m_Shape.m_CollisionCircleCount; ++i)
	{
		const VRCS::Collision_Circle& circle = shape->m_Shape.m_CollisionCircles[i];
		
		Vec2F point;
		
		point[0] = circle.m_Position[0] * axis[0] + circle.m_Position[1] * axis[1] + x;
		point[1] = circle.m_Position[0] * axis[2] + circle.m_Position[1] * axis[3] + y;

		m_SelectionBuffer.Scan_Circle(point, circle.m_Radius, id);
	}
		
	// render collision triangles
	
	for (int i = 0; i < shape->m_Shape.m_CollisionTriangleCount; ++i)
	{
		const float* coords = shape->m_Shape.m_CollisionTriangles[i].m_Coords;
		
		float points[3 * 2];
		
		for (int j = 0; j < 3; ++j)
		{
			points[j * 2 + 0] = coords[0] * axis[0] + coords[1] * axis[1] + x;
			points[j * 2 + 1] = coords[0] * axis[2] + coords[1] * axis[3] + y;
			
			coords += 2;
		}
		
		m_SelectionBuffer.Scan_Triangle(points, id);
	}
	
	// render collision hulls
	
	for (int i = 0; i < shape->m_Shape.m_CollisionHullCount; ++i)
	{
		const float* coords = shape->m_Shape.m_CollisionHulls[i].m_Coords;
		int lineCount = shape->m_Shape.m_CollisionHulls[i].m_LineCount;
		float coords2[VRCS_MAX_HULL_POINTS * 2];
		
		for (int j = 0; j < lineCount; ++j)
		{
			coords2[j * 2 + 0] = coords[0] * axis[0] + coords[1] * axis[1] + x;
			coords2[j * 2 + 1] = coords[0] * axis[2] + coords[1] * axis[3] + y;
			
			coords += 2;
		}
		
		m_SelectionBuffer.Scan_Lines(coords2, lineCount, id);
	}
	
	// render collision rectangles
	
	for (int i = 0; i < shape->m_Shape.m_CollisionRectangleCount; ++i)
	{
		const float* position = shape->m_Shape.m_CollisionRectangles[i].m_Position;
		const float* size = shape->m_Shape.m_CollisionRectangles[i].m_Size;
		
		m_SelectionBuffer.Scan_Rect(Vec2F(x + position[0], y + position[1]), Vec2F(x + position[0] + size[0], y + position[1] + size[1]), id);
		
/*		LOG(LogLevel_Debug, "rect: %03.2f %03.2f, %03.2f %03.2f",
			position[0],
			position[1],
			size[0],
			size[1]);*/
	}
}

void Application::UpdateSBWithScale(const VectorShape* shape, float x, float y, float angle, int id, float scaleX, float scaleY)
{
	Assert(shape != 0);
	
	// prepare transform
	
	float axis[4];

	Calc::RotAxis_Fast(angle, axis);
	
	axis[0] *= scaleX;
	axis[1] *= scaleY;
	axis[2] *= scaleX;
	axis[3] *= scaleY;
	
	// render collision circles
	
	for (int i = 0; i < shape->m_Shape.m_CollisionCircleCount; ++i)
	{
		const VRCS::Collision_Circle& circle = shape->m_Shape.m_CollisionCircles[i];
		
		Vec2F point;
		
		point[0] = circle.m_Position[0] * axis[0] + circle.m_Position[1] * axis[1] + x;
		point[1] = circle.m_Position[0] * axis[2] + circle.m_Position[1] * axis[3] + y;

		m_SelectionBuffer.Scan_Circle(point, circle.m_Radius, id);
	}
		
	// render collision triangles
	
	for (int i = 0; i < shape->m_Shape.m_CollisionTriangleCount; ++i)
	{
		const float* coords = shape->m_Shape.m_CollisionTriangles[i].m_Coords;
		
		float points[3 * 2];
		
		for (int j = 0; j < 3; ++j)
		{
			points[j * 2 + 0] = coords[0] * axis[0] + coords[1] * axis[1] + x;
			points[j * 2 + 1] = coords[0] * axis[2] + coords[1] * axis[3] + y;
			
			coords += 2;
		}
		
		m_SelectionBuffer.Scan_Triangle(points, id);
	}
	
	// render collision hulls
	
	for (int i = 0; i < shape->m_Shape.m_CollisionHullCount; ++i)
	{
		const float* coords = shape->m_Shape.m_CollisionHulls[i].m_Coords;
		const int lineCount = shape->m_Shape.m_CollisionHulls[i].m_LineCount;
		float coords2[VRCS_MAX_HULL_POINTS * 2];
		
		for (int j = 0; j < lineCount; ++j)
		{
			coords2[j * 2 + 0] = coords[0] * axis[0] + coords[1] * axis[1] + x;
			coords2[j * 2 + 1] = coords[0] * axis[2] + coords[1] * axis[3] + y;
			
			coords += 2;
		}
		
		m_SelectionBuffer.Scan_Lines(coords2, lineCount, id);
	}
	
	// render collision rectangles
	
	for (int i = 0; i < shape->m_Shape.m_CollisionRectangleCount; ++i)
	{
		const float* position = shape->m_Shape.m_CollisionRectangles[i].m_Position;
		const float* size = shape->m_Shape.m_CollisionRectangles[i].m_Size;
		
		float sx = fabsf(scaleX);
		float sy = fabsf(scaleY);
		
		m_SelectionBuffer.Scan_Rect(Vec2F(x + position[0] * sx, y + position[1] * sy), Vec2F(x + (position[0] + size[0]) * sx, y + (position[1] + size[1]) * sy), id);
		
/*		LOG(LogLevel_Debug, "rect: %03.2f %03.2f, %03.2f %03.2f",
			position[0],
			position[1],
			size[0],
			size[1]);*/
	}
}

void Application::ClearSB()
{
	m_SelectionBuffer.Clear();
}

void Application::Render()
{
	//LOG_DBG("data set change count: %d", m_DataSetChangeCount);

	m_DataSetChangeCount = 0;

	TempRenderBegin(m_SpriteGfx);

	m_DrawMode = VectorShape::DrawMode_Texture;
	
	Render_View();

	m_SpriteGfx->FrameEnd();
	
#if defined(MACOS) || !defined(DEPLOYMENT)
#if 0
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glEnable(GL_LIGHT0);
	float lightDir[4] = { -1.0f, -1.0f, -1.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_POSITION, lightDir);
	glEnable(GL_LIGHTING);
	glEnable(GL_NORMALIZE);
//	gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_13));
	gGraphics.TextureSet(m_ResMgr.Get(Resources::PLAYER_SHIP_3D));
	gGraphics.BlendModeSet(BlendMode_Normal_Opaque);
	Mat4x4 matWT;
	Vec2F p = m_World->m_Player->Position_get();
	matWT.MakeTranslation(p[0], p[1], 0.0f);
	Mat4x4 matWR1, matWR2, matWR3;
	matWR1.MakeIdentity();
	{
		Mat4x4 m1, m2, m3;
		m1.MakeRotationZ(Calc::mPI); // fixup
		m2.MakeRotationX(Calc::mPI); // fixup
		m3.MakeTranslation(12.0f, 0.0f, 0.0f);
		matWR1 = m3 * m2 * m1;
	}
	matWR2.MakeIdentity();
	matWR2.MakeRotationX(sinf(g_TimerRT.Time_get()) * 0.5f); // ship angle based on steering angle
	matWR3.MakeIdentity();
	matWR3.MakeRotationZ(-m_World->m_PlayerController->DrawingAngle_get());
	Mat4x4 matWR = matWR3 * matWR2 * matWR1;
	Mat4x4 matWS;
	float scale = 2.0f;
	matWS.MakeScaling(scale, scale, scale);
	Mat4x4 matW = matWT * matWR * matWS;
	m_Camera->ApplyGL(false);
	gGraphics.MatrixSet(MatrixType_World, matW);
	static MD3Model* model = 0;
	if (model == 0)
	{
		FileStream stream(g_System.GetResourcePath("player_ship_3d.md3").c_str(), OpenMode_Read);
		model = MD3Loader::Load(&stream);
		stream.Close();
	}
	RenderMD3(model);
	//delete model;
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
#endif
#endif
}

void Application::Render_View()
{
	bool drawPrev = false;
	bool drawCurr = false;
	
	if (m_PreviousView != View_Undefined)
		drawPrev = m_Views[m_PreviousView]->IsFadeActive(m_TransitionTime, m_TimeTracker_Global->Time_get());
	
	if (m_ActiveView != View_Undefined)
		drawCurr = true;
	
	int mask = 0;
	
	if (drawPrev)
		mask |= m_Views[m_PreviousView]->RenderMask_get();
	if (drawCurr)
		mask |= m_Views[m_ActiveView]->RenderMask_get();
	
	if (m_GameSettings->m_StartupCount >= 3 && g_System.IsHacked() && m_TimeTracker_Global->Time_get() >= 60.0f && m_GameRound->GameMode_get() != Game::GameMode_IntroScreen)
		mask |= RenderMask_HackWarning;
	
#ifndef DEPLOYMENT
	mask &= DBG_RenderMask;
#endif
	
	// WORLD VIEW BEGIN
	
	gGraphics.MatrixPush(MatrixType_Projection);
	
	UsingBegin(PerfTimer timer(PC_RENDER_SETUP))
	{
		// set camera transform
		
		m_Camera->Clip();
		
		bool semi3d = m_GameSettings->m_CameraSemi3dEnabled;
		
		m_Camera->ApplyGL(semi3d);
	}
	UsingEnd()
	
	//

	if (mask & RenderMask_WorldBackground)
	{
		DataSetSelect(DS_GLOBAL);

		UsingBegin(PerfTimer timer(PC_RENDER_BACKGROUND))
		{
			// render background pass
			
			RenderBackground();
		}
		UsingEnd();
	}
	
	RenderStageEnd(RenderStage_Background);

	// set main atlas texture
	
	if (mask & RenderMask_WorldPrimary)
	{
		UsingBegin(PerfTimer timer(PC_RENDER_SHADOWS))
		{
			while (DataSetItr())
			{
				// render shadows pass
				
				RenderShadows();
			}
		}
		UsingEnd();
		
		UsingBegin(PerfTimer timer(PC_RENDER_PRIMARY))
		{
			// render foreground pass
			
			while (DataSetItr())
			{
				RenderPrimaryBelow();
			}
			
			while (DataSetItr())
			{
				RenderPrimary(true);
			}
		}
		UsingEnd();
	}

	bool renderIndicators = (mask & RenderMask_Indicators) != 0;
	bool renderSpray = (mask & RenderMask_SprayAngles) != 0;
	bool renderParticles = (mask & RenderMask_Particles) != 0;
	
	UsingBegin(PerfTimer timer(PC_RENDER_PARTICLES))
	{
		// render particle effect pass
		
		while (DataSetItr())
		{
			RenderAdditive(renderIndicators, renderSpray, renderParticles);
		}
	}
	UsingEnd();

	if (mask & RenderMask_WorldBackground)
	{
		while (DataSetItr())
		{
			RenderBackgroundPrimary();
		}
	}
	
	RenderStageEnd(RenderStage_Foreground);

	// WORLD VIEW END
	
	gGraphics.MatrixPop(MatrixType_Projection);
	
	// SCREEN VIEW BEGIN

	gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
	
	if (drawCurr)
	{
		while (DataSetItr())
		{
			m_Views[m_ActiveView]->Render();
		}
	}
	
	if (drawPrev)
	{
		while (DataSetItr())
		{
			m_Views[m_PreviousView]->Render();
		}
	}

	//
	
	bool renderMenu = (mask & RenderMask_Interface) != 0;
	bool renderHudInfo = (mask & RenderMask_HudInfo) != 0;
	bool renderHudPlayer = (mask & RenderMask_HudPlayer) != 0;
	bool renderIntermezzo = (mask & RenderMask_Intermezzo) != 0;
	
	UsingBegin(PerfTimer timer(PC_RENDER_INTERFACE))
	{
		// render interfaces
		
		while (DataSetItr())
		{
			RenderInterface(renderMenu, renderHudInfo, renderHudPlayer, renderIntermezzo);
		}
	}
	UsingEnd()

	RenderStageEnd(RenderStage_Interface);
	
#if defined(IPHONEOS)
	if (mask & RenderMask_HackWarning)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		float alpha = 0.4f + (sinf(m_TimeTracker_Global->Time_get()) + 1.0f) / 2.0f * 0.3f;
		
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0.0f, 0.0f, 0.0f, alpha, 0);
		m_SpriteGfx->Flush();

		glEnable(GL_TEXTURE_2D);
		const FontMap* font = GetFont(Resources::FONT_USUZI_SMALL);
		SpriteColor color = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, 1.0f);
		RenderText(Vec2F(VIEW_SX/2.0f, VIEW_SY/2.0f-40.0f), Vec2F(0.0f, 0.0f), font, color, TextAlignment_Center, TextAlignment_Center, true, "this copy is illegal");
		RenderText(Vec2F(VIEW_SX/2.0f, VIEW_SY/2.0f-20.0f), Vec2F(0.0f, 0.0f), font, color, TextAlignment_Center, TextAlignment_Center, true, "please show us your support");
		RenderText(Vec2F(VIEW_SX/2.0f, VIEW_SY/2.0f), Vec2F(0.0f, 0.0f), font, color, TextAlignment_Center, TextAlignment_Center, true, "by buying this game");
		RenderText(Vec2F(VIEW_SX/2.0f, VIEW_SY/2.0f+20.0f), Vec2F(0.0f, 0.0f), font, color, TextAlignment_Center, TextAlignment_Center, true, "thanks");
		m_SpriteGfx->Flush();
		glDisable(GL_TEXTURE_2D);
		
		glDisable(GL_BLEND);
	}
#endif
	
#if defined(DEBUG) && !defined(PSP)
//	DBG_Console_WriteLine("enemy count: %d", Game::g_World->AliveEnemiesCount_get());
	
	DBG_Console_Render();
#endif
	
	// SCREEN VIEW END
}

void Application::RenderBackground()
{
	gGraphics.BlendModeSet(BlendMode_Normal_Opaque);

#if defined(MACOS)
	gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_09));
#elif defined(IPAD) || defined(WIN32)
	gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_09));
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_10));
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_11));
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_12));
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_13));
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_14));
#else
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_04));
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_05));
	//gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_07));
	gGraphics.TextureSet(m_ResMgr.Get(Resources::BACKGROUND_09));
#endif
	
	//

	//DBG_RenderWorldGrid();
	m_World->Render_Background();
	
	m_SpriteGfx->Flush();
}

void Application::RenderBackgroundPrimary()
{
	if (g_GameState->m_GameRound->GameModeTest(Game::GMF_Classic | Game::GMF_IntroScreen))
	{
		gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
		
		m_World->Render_Background_Primary();
		
		m_SpriteGfx->Flush();
	}
}

void Application::RenderShadows()
{
	gGraphics.BlendModeSet(BlendMode_Subtractive);
	
	m_DrawMode = VectorShape::DrawMode_Silhouette;
	
	m_World->Render_Primary();
	
	m_SpriteGfx->Flush();
}

void Application::RenderPrimaryBelow()
{
	gGraphics.BlendModeSet(BlendMode_Additive);
	
	//gGraphics.TextureSet(m_TextureAtlas->m_Texture);
	
	m_World->Render_Primary_Below();
	
	m_SpriteGfx->Flush();
}

void Application::RenderPrimary(bool renderParticles)
{
	gGraphics.BlendModeSet(BlendMode_Normal_Transparent_Add);
	
	m_DrawMode = VectorShape::DrawMode_Texture;
	
	m_World->Render_Primary();

	if (renderParticles)
	{
		//m_ParticleEffect_Primary.Render(m_SpriteGfx);
	}
	
	m_SpriteGfx->Flush();
	
	//
	
	m_SpriteEffectMgr->Render();
	
	m_SpriteGfx->Flush();
	
	//gGraphics.TextureSet(m_TextureAtlas->m_Texture);
}

void Application::RenderAdditive(bool renderIndicators, bool renderSpray, bool renderParticles)
{
	gGraphics.BlendModeSet(BlendMode_Additive);
	
	m_GameRound->RenderAdditive();
	
	m_World->Render_Additive();
	
	if (renderSpray)
	{
		m_World->m_Player->Render_Spray();
	}
	
	if (renderParticles)
	{
		m_ParticleEffect.Render(m_SpriteGfx);
	}

	m_SpriteGfx->Flush();

	gGraphics.MatrixPush(MatrixType_Projection);
	g_GameState->m_Camera->ApplyGL(false);

	if (renderIndicators)
	{
		m_World->Render_Indicators();
	}

	m_SpriteGfx->Flush();

	gGraphics.MatrixPop(MatrixType_Projection);
}

void Application::RenderInterface(bool renderMenu, bool renderHud, bool renderControllers, bool renderIntermezzo)
{
	gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
	
	if (renderHud)
		Game::g_World->Render_HudInfo();
	if (renderControllers)
		Game::g_World->Render_HudPlayer();
	if (renderMenu)
	{
		m_MenuMgr->Render();
		
		if (m_ActiveView == View_InGame)
		{
			m_GameHelp->Render();
			m_HelpState->Render();
			m_GameNotification->Render();
		}
	}
	if (renderIntermezzo)
		Game::g_World->Render_Intermezzo();
	if (renderHud)
	{
		float opacity = Game::g_World->HudOpacity_get();
		
		StringBuilder<32> sb;
		sb.AppendFormat("%06d", m_Score->Score_get());
		int font = 0;
#if !defined(PSP) || 1
		font = Resources::FONT_CALIBRI_LARGE;
#else
		font = Resources::FONT_CALIBRI_SMALL;
#endif
		RenderText(Vec2F(VIEW_SX/2.0f, -4.0f), Vec2F(0.0f, 0.0f), GetFont(font), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, opacity), TextAlignment_Center, TextAlignment_Top, true, sb.ToString());
	}
	
	m_SpriteGfx->Flush();

	// always render UI particles
	
	gGraphics.BlendModeSet(BlendMode_Additive);
	
	// todo: make this nicer

	GetView(m_ActiveView)->Render_Additive();

	DataSetActivate(DS_GAME);

	m_ParticleEffect_UI.Render(m_SpriteGfx);
	
	m_SpriteGfx->Flush();
}

//

void Application::GameBegin(Game::GameMode mode, Difficulty difficulty, bool continueGame)
{
	switch (mode)
	{
		case Game::GameMode_IntroScreen:
		{
			Assert(!continueGame);
			
#ifdef IPAD
			m_Camera->Setup(VIEW_SX, VIEW_SY, Vec2F(0.0f, -30.0f), Vec2F(WORLD_SX, WORLD_SY + 30));
#else
			m_Camera->Setup(VIEW_SX, VIEW_SY, Vec2F(0.0f, -30.0f), Vec2F(WORLD_SX, WORLD_SY + 130));
#endif
			
			// Game mode
			
			m_GameRound->Setup(mode, m_GameRound->Modifier_Difficulty_get());
		}
		break;
			
		case Game::GameMode_ClassicLearn:
		case Game::GameMode_ClassicPlay:
		{
#ifdef IPAD
			m_Camera->Setup(VIEW_SX, VIEW_SY, Vec2F(0.0f, -30.0f), Vec2F(WORLD_SX, WORLD_SY + 30));
#else
			m_Camera->Setup(VIEW_SX, VIEW_SY, Vec2F(0.0f, -30.0f), Vec2F(WORLD_SX, WORLD_SY + 130));
#endif
			
			// World / score / round / continue / intermezzo
			
			if (continueGame)
			{
				mode = Game::GameMode_ClassicPlay;
				difficulty = Difficulty_Easy;
			}
			
			// Game mode
			
			m_GameRound->Setup(mode, difficulty);
			
			// World
			
			m_World->Setup();
			
			m_Score->Initialize();
			
			// Game round
			
			m_GameRound->Clear();
			
			// Load game
			
			if (continueGame)
			{
				if (m_GameSave->HasSave_get())
				{
					try
					{
						m_GameSave->Load();
						
						// Apply patches.
						
						difficulty = g_GameState->m_GameRound->Modifier_Difficulty_get(); // store difficulty because we init gameround with difficulty later on
						
						int level = g_GameState->m_GameRound->Classic_Level_get();
						
						g_GameState->m_GameRound->Classic_Level_set(level - 1); // decrease level, because it will be increase in NextLevel()
					}
					catch (std::exception& e)
					{
						m_Log.WriteLine(LogLevel_Error, "failed to load game: %s", e.what());
						
						m_GameSave->Clear();
					}
				}
				
				m_GameRound->Setup(mode, difficulty);
			}
			else
			{
				m_GameRound->Setup(mode, difficulty);
			}
			
			m_GameRound->Classic_RoundState_set(Game::RoundState_PlayWaves);
			
			// Intermezzo
			
			m_World->m_IntermezzoMgr.Start(Game::IntermezzoType_LevelBegin);
			
			// Tutorial
			
			if (mode == Game::GameMode_ClassicPlay)
				m_HelpState->GameBegin(false);
			else
				m_HelpState->GameBegin(true);
			
			// show bandit intro view
			
			if (difficulty == Difficulty_Custom)
			{
				ApplyCustomMods();
				
				if(m_GameSettings->m_CustomSettings.Boss_Toggle)
					g_GameState->ActiveView_set(::View_BanditIntro);
				else
					g_GameState->ActiveView_set(::View_InGame);
			}
			else
			{
				g_GameState->ActiveView_set(::View_BanditIntro);
			}
		}			
		break;
			
		case Game::GameMode_InvadersPlay:
		{
			Assert(!continueGame);
			
			m_Camera->Setup(VIEW_SX, VIEW_SY, Vec2F(0.0f, 0.0f), Vec2F(float(VIEW_SX), float(VIEW_SY)));
			
			// Music
			
		#ifdef MACOS
			PlayMusic(m_ResMgr.Get(Resources::BGM_GAME1));
		#else
			PlayMusic(m_ResMgr.Get(Resources::BGM_GAME3));
		#endif
			
			// Game mode
			
			m_GameRound->Setup(mode, difficulty);
			
			// World
			
			m_World->Setup();
			
			m_Score->Initialize();
			
			// Game round
			
			m_GameRound->Clear();
			
			// Round state
			
			//m_GameRound->Setup(Difficulty_Easy);
			
			// Intermezzo
			
			m_World->m_IntermezzoMgr.Start(Game::IntermezzoType_LevelBegin);

			// Tutorial
			
			m_HelpState->GameBegin(false);

			// Change view
			
			g_GameState->ActiveView_set(::View_InGame);
		}
		break;
	}
}

void Application::ApplyCustomMods()
{
	Game::g_World->m_Player->SetAllUpgradeLevel(g_GameState->m_GameSettings->m_CustomSettings.UpgradesUnlock_Toggle);
	
	if (g_GameState->m_GameSettings->m_CustomSettings.Invuln_Toggle)
		g_GameState->m_GameSettings->ToggleInvuln();

	g_GameState->m_GameRound->Classic_Level_set(g_GameState->m_GameSettings->m_CustomSettings.StartLevel);

}

void Application::GameEnd()
{
	Game::g_World->HandleGameEnd();
}

void Application::LevelBegin()
{
}

void Application::LevelEnd()
{
	//
}

//

void Application::DrawMode_set(VectorShape::DrawMode drawMode)
{
	m_DrawMode = drawMode;
}

void Application::Render(const VectorShape* shape, Vec2F pos, float angle, SpriteColor color)
{
	Assert(shape != 0);

	RenderWithScale(shape, pos, angle, color, 1.0f, 1.0f);
}

void Application::RenderWithScale(const VectorShape* shape, Vec2F pos, float angle, SpriteColor color, float scaleX, float scaleY)
{
	Assert(shape != 0);
	
	DataSetActivate(shape->m_TextureAtlas->m_Texture->m_DataSetId);

	if (m_DrawMode == VectorShape::DrawMode_Silhouette && !shape->m_Shape.m_HasSilhouette)
		return;
	
	const Atlas_ImageInfo* image = 0;
	
	if (m_DrawMode == VectorShape::DrawMode_Texture)
		image = shape->m_Texture;
	if (m_DrawMode == VectorShape::DrawMode_Silhouette)
		image = shape->m_Silhouette;
	
	if (m_DrawMode == VectorShape::DrawMode_Silhouette)
		color = SpriteColors::White;
	
	// offset (7, 7) if draw mode is silhouette
	
	if (m_DrawMode == VectorShape::DrawMode_Silhouette)
	{
#ifdef IPAD
		pos[0] += 4.0f;
		pos[1] += 4.0f;
#else
		pos[0] += 7.0f;
		pos[1] += 7.0f;
#endif
//		const int v = 31;
		const int v = 100;
		color = SpriteColor_Modulate(color, SpriteColor_Make(v, v, v, 255));
	}
	
	const int rgba = color.rgba;
	
	float axis[4];

	Calc::RotAxis_Fast(angle, axis);
//	Calc::RotAxis(angle, axis);
	
	axis[0] *= scaleX;
	axis[1] *= scaleY;
	axis[2] *= scaleX;
	axis[3] *= scaleY;
	
	const VRCS::Graphics_Mesh& mesh = shape->m_Shape.m_GraphicsMesh;
	
	SpriteGfx* gfx = m_SpriteGfx;

	gfx->Reserve(mesh.m_VertexCount, mesh.m_IndexCount);
	gfx->WriteBegin();
	
	const VRCS::Graphics_MeshVertex* vertex = mesh.m_Vertices;
	
	for (int i = 0; i < mesh.m_VertexCount; ++i, ++vertex)
	{
		// transform vertex
		
		const float tx = vertex->m_Position[0] * axis[0] + vertex->m_Position[1] * axis[1] + pos[0];
		const float ty = vertex->m_Position[0] * axis[2] + vertex->m_Position[1] * axis[3] + pos[1];
		
		// write vertex
		
		gfx->WriteVertex(tx, ty, rgba,
			image->m_TexCoord[0] + vertex->m_TexCoord[0] * image->m_TexSize[0],
			image->m_TexCoord[1] + vertex->m_TexCoord[1] * image->m_TexSize[1]);
	}
	
	gfx->WriteIndexN(mesh.m_Indices, mesh.m_IndexCount);
	
	gfx->WriteEnd();
}

void Application::RenderSprite(const VectorShape* shape, Vec2F pos, float angle, SpriteColor color)
{
#if VRCS_USE_SPRITE
	SpriteGfx* gfx = m_SpriteGfx;

	DataSetActivate(shape->m_TextureAtlas->m_Texture->m_DataSetId);

	if (m_DrawMode != VectorShape::DrawMode_Texture || !shape->m_Shape.m_HasSprite)
		return;

	const Atlas_ImageInfo* image = m_TextureAtlas[m_ActiveDataSet]->m_Atlas->GetImage(Textures::COLOR_BLACK);
	
	float axis[4];

	Calc::RotAxis_Fast(angle, axis);
	
	const VRCS::Graphics_Sprite& sprite = shape->m_Shape.m_GraphicsSprite;
	
	gfx->Reserve(sprite.m_VertexCount, sprite.m_IndexCount);
	gfx->WriteBegin();
	
	for (int i = 0; i < sprite.m_VertexCount; ++i)
	{
		const VRCS::Graphics_SpriteVertex& vertex = sprite.m_Vertices[i];
		
		// transform vertex
		
		const float tx = vertex.m_Position[0] * axis[0] + vertex.m_Position[1] * axis[1] + pos[0];
		const float ty = vertex.m_Position[0] * axis[2] + vertex.m_Position[1] * axis[3] + pos[1];
		
		// write vertex
		
		gfx->WriteVertex(tx, ty, color.rgba,
			image->m_TexCoord[0],
			image->m_TexCoord[1]);
	}
	
	gfx->WriteIndexN(sprite.m_Indices, sprite.m_IndexCount);
	
	gfx->WriteEnd();
#endif
}

void Application::RenderBGFade(float opacity)
{
	const AtlasImageMap* backImage = g_GameState->GetTexture(Textures::COLOR_WHITE);
	g_GameState->DataSetActivate(backImage->m_TextureAtlas->m_DataSetId);
	SpriteGfx* gfx = m_SpriteGfx;
	gfx->Reserve(4, 6);
	gfx->WriteBegin();
	const float u = backImage->m_Info->m_TexCoord[0];
	const float v = backImage->m_Info->m_TexCoord[1];
	const SpriteColor baseColor = Calc::Color_FromHue(Game::g_World->m_GridEffect->BaseHue_get() + 0.05f);
	const SpriteColor color1 = SpriteColor_Make(baseColor.v[0], baseColor.v[1], baseColor.v[2], (int)(255 * opacity));
	const SpriteColor color2 = SpriteColor_Make(baseColor.v[0], baseColor.v[1], baseColor.v[2], 0);
	gfx->WriteVertex(0.0f,         0.0f,         color1.rgba, u, v);
	gfx->WriteVertex(VIEW_SX/2.0f, 0.0f,         color2.rgba, u, v);
	gfx->WriteVertex(VIEW_SX/2.0f, VIEW_SY/1.0f, color2.rgba, u, v);
	gfx->WriteVertex(0.0f,         VIEW_SY/1.0f, color1.rgba, u, v);
	gfx->WriteIndex3(0, 1, 2);
	gfx->WriteIndex3(0, 2, 3);
	gfx->WriteEnd();
}

//

void Application::HandleTouchBegin(void* obj, void* arg)
{
	Application* self = (Application*)obj;

	self->m_TouchDLG->HandleTouchBegin(self->m_TouchDLG, arg);
}

void Application::HandleTouchEnd(void* obj, void* arg)
{
	Application* self = (Application*)obj;
	
	self->m_TouchDLG->HandleTouchEnd(self->m_TouchDLG, arg);
}

void Application::HandleTouchMove(void* obj, void* arg)
{
	Application* self = (Application*)obj;

	self->m_TouchDLG->HandleTouchMove(self->m_TouchDLG, arg);
}

//

void Application::DBG_RenderWorldGrid()
{
	SpriteGfx* gfx = m_SpriteGfx;

	Game::Grid_Effect& wg = *m_World->m_GridEffect;
	
	const float scaleU = 1.0f / WORLD_SX;
	const float scaleV = 1.0f / WORLD_SY;
	
	const float cellSx = wg.DBG_CellSx_get();
	const float cellSy = wg.DBG_CellSy_get();
	
	Vec2F min;
	Vec2F max;
	
	min[1] = 0.0f;
	max[1] = cellSy;
	
	for (int y = 0; y < wg.DBG_GridSy_get(); ++y)
	{
		min[0] = 0.0f;
		max[0] = cellSx;
		
		for (int x = 0; x < wg.DBG_GridSx_get(); ++x)
		{
			const Game::Grid_Effect::Cell* cell = wg.DBG_GetCell(x, y);
			
			gfx->Reserve(4, 6);
			gfx->WriteBegin();
			
//			const int base = 63;
			const int base = 31;
			
			int v = base + cell->m_ObjectCount * 64;
			
			if (v > 255)
				v = 255;
			
			SpriteColor color = SpriteColor_Make(v, base, base, 255);
//			SpriteColor color = SpriteColor_Make(v, 0, 0, 255);
//			SpriteColor color = SpriteColor_Make(0, v, 0, 255);
//			SpriteColor color = SpriteColor_Make(0, 0, v, 255);
			//SpriteColor color = Calc::Color_FromHue((x + y) / 40.0f + m_TimeTracker_World->Time_get() * 0.1f);
			
			color = SpriteColor_Modulate(color, SpriteColor_Make(v, v, v, 255));
			
			gfx->WriteVertex(min[0], min[1], color.rgba, min[0] * scaleU, min[1] * scaleV);
			gfx->WriteVertex(max[0], min[1], color.rgba, max[0] * scaleU, min[1] * scaleV);
			gfx->WriteVertex(max[0], max[1], color.rgba, max[0] * scaleU, max[1] * scaleV);
			gfx->WriteVertex(min[0], max[1], color.rgba, min[0] * scaleU, max[1] * scaleV);

			const SpriteIndex index[] = { 0, 1, 2, 0, 2, 3 };

			gfx->WriteIndexN(index, 6);
			
			gfx->WriteEnd();
			
			min[0] += cellSx;
			max[0] += cellSx;
		}
		
		min[1] += cellSy;
		max[1] += cellSy;
	}
	
	gfx->Flush();
}

void Application::Link(ResMgr* resMgr, int dataSetId, TextureAtlas* atlas, int index, const char* texturePrefix)
{
	const Res* res = resMgr->m_Resources[index];
	
	Assert(res != 0);

	if (res->m_DataSetId != dataSetId)
		return;

	Assert(atlas->m_Texture->m_DataSetId == dataSetId);
	
	if (res->m_Type == ResTypes_Font)
	{
		// resolve texture atlas image
		
		FontMap* font = (FontMap*)resMgr->Get(index)->data;
		
		font->SetTextureAtlas(atlas, texturePrefix);
		font->m_DataSetId = res->m_DataSetId;
	}
	
	if (res->m_Type == ResTypes_VectorGraphic)
	{
		// resolve texture atlas image
		
		VectorShape* shape = (VectorShape*)resMgr->Get(index)->data;
		
		shape->SetTextureAtlas(atlas, texturePrefix);
	}
}

void Application::DataSetSelect(int dataSetId)
{
	if (dataSetId < 0 || dataSetId >= DATASET_COUNT)
		throw ExceptionVA("invalid data set ID");

	m_ActiveDataSet = dataSetId;

	gGraphics.TextureSet(m_TextureAtlas[dataSetId]->m_Texture);
}

bool Application::DataSetItr()
{
	if (m_DataSetItr < 0)
		m_DataSetItr = -1;

	m_DataSetItr++;

	if (m_DataSetItr >= 1)
	{
		m_DataSetItr = -1;
		return false;
	}
	else
	{
		DataSetActivate(m_DataSetItr);
		
		return true;
	}
}

void Application::DataSetActivate(int dataSetId)
{
	if (dataSetId == m_ActiveDataSet)
		return;

	//LOG_DBG("change active data set: %d -> %d", m_ActiveDataSet, dataSetId);

	m_SpriteGfx->Flush();

	m_ActiveDataSet = dataSetId;

	gGraphics.TextureSet(m_TextureAtlas[m_ActiveDataSet]->m_Texture);

	m_DataSetChangeCount++;
}

#if defined(DEBUG) && 0

static void DBG_TestGrs_HandleResult(void* obj, void* arg)
{
	GRS::QueryResult* result = (GRS::QueryResult*)arg;
	
	if (result->m_RequestType == GRS::RequestType_QueryList)
	{
		for (size_t i = 0; i < result->m_Scores.size(); ++i)
		{
			LOG(LogLevel_Info, "high score: %s: %f", result->m_Scores[i].m_UserName.c_str(), result->m_Scores[i].m_Score);
		}
	}
	if (result->m_RequestType == GRS::RequestType_QueryRank)
	{
		LOG(LogLevel_Info, "rank position: %d", result->m_Position);
	}
}

#endif

void Application::DBG_TestGrs()
{
#ifdef DEBUG
	
#if 0
	Base64::DBG_SelfTest();
	
	GRS::Protocol::DBG_SelfTest();
#endif
	
#if 0
	// test HTTP client
	
	GRS::GameInfo gameInfo;
	gameInfo.Setup("USG", 0, 0, GRS_PASS);
	
	GRS::HttpClient* client = new GRS::HttpClient();
	
	client->Initialize(GRS_HOST, GRS_PORT, GRS_PATH, gameInfo);
	client->OnQueryListResult = CallBack(0, DBG_TestGrs_HandleResult);
	client->OnQueryRankResult = CallBack(0, DBG_TestGrs_HandleResult);
	
	int gameId = 0;
	int gameMode = 0;
	
	//
	
	GRS::ListQuery listQuery;
	listQuery.Setup(gameId, gameMode, 100, GRS::UserId(g_System.GetDeviceId()), 0, 100, 0, 0);
	client->RequestHighScoreList(listQuery);
	
	GRS::RankQuery rankQuery;
	rankQuery.Setup(gameId, gameMode, 100.0f);
	client->RequestHighScoreRank(rankQuery);
	
	GRS::HighScore highScore;
	highScore.Setup(gameMode, GRS_MAX_HISTORY, GRS::UserId(g_System.GetDeviceId()), "testuser", "blaat", Calc::Random(10.0f, 1000.0f), GRS::TimeStamp::FromSystemTime(), GRS::GpsLocation(), GRS::CountryCode::FromString(g_System.GetCountryCode()), false);
	client->SubmitHighScore(highScore);
	
	client->SubmitException("test crash log");
#endif
	
#endif
}

#if defined(DEBUG) && !defined(PSP)

void Application::DBG_Console_Render()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	
	float x = 0.0f;
	float y = 0.0f;
	
	for (size_t i = 0; i < m_DBG_ConsoleLines.size(); ++i)
	{
		RenderText(Vec2F(x, y), Vec2F(0.0f, 0.0f), GetFont(Resources::FONT_LGS), SpriteColors::White, TextAlignment_Left, TextAlignment_Top, true, m_DBG_ConsoleLines[i].c_str());
		y += 10.0f;
	}
	
	m_DBG_ConsoleLines.clear();
	
	m_SpriteGfx->Flush();
	
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void Application::DBG_Console_WriteLine(const char* text, ...)
{
	VA_SPRINTF(text, temp, text);
	
	m_DBG_ConsoleLines.push_back(temp);
}

#endif

//

std::map<int, AtlasImageMap*> gTexturesById;

int GetTextureDS(int id)
{
	return Textures::IndexToDataSet[id];
}

int GetTextureID(int id)
{
	return Textures::IndexToDataSetIndex[id];
}
