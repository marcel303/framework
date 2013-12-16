#include "Atlas.h"
#include "GameState.h"
#include "Log.h"
#include "LogicTimer.h"
#include "ParticleEffect.h"
#include "ResIndex.h"
#include "ResMgr.h"
#include "SoundEffectMgr.h"
#include "TouchMgr_Win32.h"
#include "VectorShape.h"

class GameView_Psp
{
public:
	GameView_Psp();
	~GameView_Psp();
	void Initialize();
	void Shutdown();

	void Setup();

	void Update();
	void UpdateInput();
	void Render();
	void Run();

//private:
	Vec2F ScreenToWorld(Vec2F location);
	
	Application* m_GameState;
	LogicTimer* m_LogicTimer;
	AnimTimer* m_SaveIconTimer;

	struct SceCtrlData* m_ControlStatePrev;
	
	LogCtx m_Log;
};

extern GameView_Psp* gGameView;
