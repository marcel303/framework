#include <screen/screen.h>
#include "Forward.h"
#include "Gamepad_BBOS.h"
#include "Log.h"
#include "OpenGLState_BBOS.h"
//#include "ScoreLoopMgr.h"

class GameView_Bbos
{
public:
	GameView_Bbos();
	~GameView_Bbos();
	void Initialize();
	void Shutdown();

	void Setup();

	void Update();
	void Render();
	void Run();
	
	void Pause();
	void Resume();

	void HandleTouch_Begin(int index, int x, int y);
	void HandleTouch_Move(int index, int x, int y);
	void HandleTouch_End(int index);
	void HandleTouch_Cancel(int index);
	
	Vec2F ScreenToWorld(Vec2F location, bool offset);

	enum InputMode
	{
		InputMode_Touch,
		InputMode_Gamepad
	};

	void InputMode_set(InputMode mode);

	bool m_IsInitialized;
	bool m_IsBackgrounded;
	
	screen_context_t mScreenCtx;
	bool mHasTilt;
	TouchMgr* m_TouchMgr;
	OpenGLState_BBOS* m_OpenGLState;
	OpenALState* m_OpenALState;
	Gamepad_BBOS * m_Gamepad;
	bool m_GamepadIsEnabled;
	InputMode m_InputMode;
	
	Application* m_GameState;
	LogicTimer* m_LogicTimer;
	
	LogCtx m_Log;
};

extern GameView_Bbos* gGameView;
