#include "Forward.h"
#include "Log.h"

class GameView_Win32
{
public:
	GameView_Win32();
	~GameView_Win32();
	void Initialize();
	void Shutdown();

	void Setup();

	void Update();
	void Render();
	void Run();

	Vec2F ScreenToWorld(Vec2F location);

	TouchMgr_Win32* m_TouchMgr;
	DisplaySDL* m_Display;
	OpenALState* m_OpenALState;
	
	Application* m_GameState;
	LogicTimer* m_LogicTimer;
	
	LogCtx m_Log;
};

extern GameView_Win32* gGameView;
