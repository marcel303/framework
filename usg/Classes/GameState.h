#pragma once

#include <map>
#include "Forward.h"
#include "GameSettings.h" // fixme, move enums to gametypes.h
#include "GameTypes.h"
#include "IView.h"
#include "libgg_forward.h"
#include "libiphone_forward.h"
#include "ResMgr.h"
#include "SelectionBuffer.h"
#include "SelectionMap.h"
#include "SpriteGfx.h"
#include "ParticleEffect.h"
#include "VectorShape.h"

#include "Log.h"

//

#ifdef IPAD
	#define DATASET_COUNT 6
#else
	#define DATASET_COUNT 5
#endif

#define DS_GLOBAL 0
#define DS_GAME 1

//

extern class Application* g_GameState;
extern int EVT_PAUSE;
extern int EVT_MENU_LEFT;
extern int EVT_MENU_RIGHT;
extern int EVT_MENU_UP;
extern int EVT_MENU_DOWN;
extern int EVT_MENU_BACK;
extern int EVT_MENU_NEXT;
extern int EVT_MENU_PREV;
extern int EVT_MENU_SELECT;

//

enum View
{
	View_Undefined = -1,
	View_BanditIntro,
	View_Credits,
	View_GameOver,
	View_GameSelect,
	View_InGame,
	View_KeyBoard,
	View_Main,
	View_Options,
	View_Pause,
//	View_PauseTouch,
	View_ScoreEntry,
	View_Scores,
#if defined(PSP_UI)
	View_ScoresPSP,
#endif
	View_ScoreSubmit,
#if defined(IPHONEOS) || defined(BBOS)
	View_ScoreAutoSubmit,
#endif
	View_Upgrade,
	View_CustomSettings,
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
	View_WinSetup,
#endif
	View__End
};

enum RenderStage
{
	RenderStage_Background, // just the background
	RenderStage_Foreground, // finished drawing enemies and particles
	RenderStage_Interface // finished the interface
};

//

#define PSPGLYPH_X "\1" // cross
#define PSPGLYPH_C "\2" // circle
#define PSPGLYPH_T "\3" // triangle
#define PSPGLYPH_S "\4" // square

//

typedef void (*ApplicationSetupCallback)(const char* name);
typedef void (*ApplicationRenderStageEndCallBack)(RenderStage state);

class Application
{
public:
	Application();
	~Application();
	void Setup(void* openALState, float screenScale, ApplicationSetupCallback callback = 0, ApplicationRenderStageEndCallBack renderStageCallback = 0);

	void ApplyCustomMods();
	
	void GameBegin(Game::GameMode mode, Difficulty difficulty, bool continueGame);
	void GameEnd();
	
	void LevelBegin();
	void LevelEnd();
	
	void WaveBegin();
	void WaveEnd();
	
	void GameOverBegin();
	void GameOverEnd();
	
	//

	float m_ScreenScale;
	
	// --------------------
	// Touch related
	// --------------------
	static void HandleTouchBegin(void* obj, void* arg);
	static void HandleTouchEnd(void* obj, void* arg);
	static void HandleTouchMove(void* obj, void* arg);
	static void HandleZoomIn(void* obj, void* arg);
	static void HandleZoomOut(void* obj, void* arg);
	
	void DBG_RenderWorldGrid();
	
	TouchDelegator* m_TouchDLG;

	// --------------------
	// Resources
	// --------------------
	void Link(ResMgr* resMgr, int dataSetId, TextureAtlas* atlas, int index, const char* texturePrefix);
	
	inline const FontMap* GetFont(int id) const;
	inline FontMap* _GetFont(int id);
	inline const VectorShape* GetShape(int id) const;
	inline Res* GetSound(int id);
	inline const AtlasImageMap* GetTexture(int id) const;
	
	ResMgr m_ResMgr;
	Res* m_Res_TextureAtlas[DATASET_COUNT];
	TextureAtlas* m_TextureAtlas[DATASET_COUNT];
	
	// --------------------
	// DataSets
	// --------------------
	void DataSetSelect(int dataSetId);
	bool DataSetItr();
	void DataSetActivate(int dataSetId);

	int m_ActiveDataSet;
	int m_DataSetItr;
	int m_DataSetChangeCount;

	// --------------------
	// Sound
	// --------------------
	Res* GetMusic();
	void PlayMusic(Res* res1, Res* res2 = 0, Res* res3 = 0, Res* res4 = 0);
	
	ISoundPlayer* m_SoundPlayer;
	ISoundEffectMgr* m_SoundEffectMgr;
	SoundEffectFenceMgr* m_SoundEffects;
	Res* m_Music;
	
	// --------------------
	// Logic
	// --------------------
	void Update(float multiplier);
	void UpdateSB(const VectorShape* shape, float x, float y, float angle, int id);
	void UpdateSBWithScale(const VectorShape* shape, float x, float y, float angle, int id, float scaleX, float scaleY);
	void ClearSB();

	TimeTracker* m_TimeTracker_GlobalUnsafe;
	TimeTracker* m_TimeTracker_Global;
	TimeTracker* m_TimeTracker_World;
	Timer* m_Timer;
	
	// --------------------
	// Collision detection
	// --------------------
	SelectionBuffer m_SelectionBuffer;
	SelectionMap m_SelectionMap;

	// --------------------
	// Drawing
	// --------------------
	void Render(); // renders current view
	void RenderStageEnd(RenderStage stage)
	{
		if (m_RenderStageEndCallBack != 0)
			m_RenderStageEndCallBack(stage);
	}
	
	// views
	void Render_View();

	// support
	void RenderBackground(); // solid color background pass. color is modulated with texture
	void RenderBackgroundPrimary();
	void RenderShadows(); // subtraction blended shadow pass.
	void RenderPrimaryBelow(); // addition blended primary pass. color is added to texture
	void RenderPrimary(bool renderParticles); // alpha blended primary pass. color is added to texture
	void RenderAdditive(bool renderIndicators, bool renderSpray, bool renderParticles); // addition blended particle pass. color is modulated with texture
	void RenderInterface(bool renderMenu, bool renderHud, bool renderControllers, bool renderIntermezzo); // alpha blended interface pass. color is modulated with texture
	
	inline VectorShape::DrawMode DrawMode_get() const
	{
		return m_DrawMode;
	}
	void DrawMode_set(VectorShape::DrawMode drawMode);
	void Render(const VectorShape* shape, Vec2F pos, float angle, SpriteColor color);
	void RenderWithScale(const VectorShape* shape, Vec2F pos, float angle, SpriteColor color, float scaleX, float scaleY);
	void RenderSprite(const VectorShape* shape, Vec2F pos, float angle, SpriteColor color);
	void RenderBGFade(float opacity);
	
	SpriteGfx* m_SpriteGfx;
	Camera* m_Camera;
	//ParticleEffect m_ParticleEffect_Primary;
	ParticleEffect m_ParticleEffect;
	ParticleEffect m_ParticleEffect_UI;
	VectorShape::DrawMode m_DrawMode;
	Game::SpriteEffectMgr* m_SpriteEffectMgr;
	ApplicationRenderStageEndCallBack m_RenderStageEndCallBack;

	// --------------------
	// Menus
	// --------------------
	inline GameMenu::MenuMgr* Interface_get()
	{
		return m_MenuMgr;
	}
	
	GameMenu::MenuMgr* m_MenuMgr;
	
	Game::GameHelp* m_GameHelp;
	Game::HelpState* m_HelpState;
	Game::GameNotification* m_GameNotification;
	
	// --------------------
	// Views
	// --------------------
	void ActiveView_set(View view);
	inline View ActiveView_get() const
	{
		return m_ActiveView;
	}
	IView* GetView(View view);
	inline Game::View_KeyBoard* KeyboardView_get()
	{
		return (Game::View_KeyBoard*)GetView(View_KeyBoard);
	}
	std::string KeyboardText_get();
	void KeyboardText_set(const char* text);
	
	View m_ActiveView;
	View m_PreviousView;
	IView* m_Views[View__End];
	float m_TransitionTime;
	
	// --------------------
	// View: InGame
	// --------------------
	Game::World* m_World;
	Game::GameScore* m_Score;
	Game::GameRound* m_GameRound;
	Game::GameSave* m_GameSave;
	
	// --------------------
	// Social
	// --------------------
	SocialAPI * m_Social;
	Game::UsgSocialListener * m_SocialListener;

	// --------------------
	// Settings
	// --------------------
	Game::GameSettings* m_GameSettings;

	// --------------------
	// Debugging
	// --------------------
	void DBG_TestGrs();
	
#ifdef DEBUG
	void DBG_Console_Render();
	void DBG_Console_WriteLine(const char* text, ...);
	
	std::vector<std::string> m_DBG_ConsoleLines;
#endif
	
	int DBG_RenderMask;
	
	//
	
	LogCtx m_Log;
};

inline const FontMap* Application::GetFont(int id) const
{
	const Res* res = m_ResMgr.Get(id);
	Assert(res->m_Type == ResTypes_Font);
	return (const FontMap*)res->data;
}

inline FontMap* Application::_GetFont(int id)
{
	Res* res = m_ResMgr.Get(id);
	Assert(res->m_Type == ResTypes_Font);
	return (FontMap*)res->data;
}

inline const VectorShape* Application::GetShape(int id) const
{
	const Res* res = m_ResMgr.Get(id);
	//Assert(res->m_DataSetId == DS_GAME);
	return (const VectorShape*)res->data;
}

inline Res* Application::GetSound(int id)
{
	Res* res = m_ResMgr.Get(id);
	//Assert(res->m_DataSetId == 0);
	return res;
}

extern int GetTextureDS(int id);
extern int GetTextureID(int id);
extern std::map<int, AtlasImageMap*> gTexturesById;

inline const AtlasImageMap* Application::GetTexture(int id) const
{
	std::map<int, AtlasImageMap*>::iterator i = gTexturesById.find(id);
	if (i != gTexturesById.end())
		return i->second;

	Assert(false);
	return 0;
}
