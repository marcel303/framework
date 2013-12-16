#include <ctrlsvc.h>
#include <impose.h>
#include <kernel.h>
#include <snproview.h>
#include <utility/utility_common.h>
#include "AnimTimer.h"
#include "Arguments.h"
#include "AudioSession.h"
#include "Benchmark.h"
#include "Calc.h"
#include "Camera.h"
#include "ColHashMap.h"
#include "CompiledShape.h"
#include "EventHandler_PlayerController_Gamepad_Psp.h"
#include "EventManager.h"
#include "FileStream.h"
#include "GameSettings.h"
#include "GameView_Psp.h"
#include "Graphics.h"
#include "Heap.h"
#include "MenuMgr.h"
#include "Parse.h"
#include "Path.h"
#include "PerfCount.h"
#include "psp_prxload.h"
#include "PspKeyboard.h"
#include "PspSaveData.h"
#include "ResIO.h"
#include "ResMgr.h"
#include "SelectionBuffer_Test.h"
#include "SpriteGfx.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "TextureDDS.h"
#include "TexturePVR.h"
#include "TextureRGBA.h"
#include "Textures.h"
#include "Types.h"
#include "UsgResources.h"
#include "Util_Color.h"
#include "Util_ColorEx.h"
#include "WorldGrid.h"

#include "PspMessageBox.h"
#include "PspSaveData.h"

// todo: use scePowerIsLowBattery, display battery icon, unless scePowerIsBatteryCharging 

#define DEFAULT_FULLSCREEN false
#define DEFAULT_DISPLAY_SCALE 1

#define DEFAULT_DISPLAY_SX VIEW_SX
#define DEFAULT_DISPLAY_SY VIEW_SY

#define LOGIC_FPS 60.0f

#define DEAD_ZONE_V1 0x40
#define DEAD_ZONE_V2 0xC0

#define ANALOG_SCALE 32700.0f

static inline int AnalogToInt(int v)
{
	if (v >= DEAD_ZONE_V1 && v < DEAD_ZONE_V2)
		return 0.0f;

	return (v - 127.5f) / 127.5f * ANALOG_SCALE;
}

extern "C"
{
	SCE_MODULE_INFO( critwave, 0, 1, 1 );

	//int sce_newlib_heap_kb_size = 1024 * 16; // initial heap size / kb
	int sce_newlib_heap_kb_size = 1024 * 24; // initial heap size / kb
	char sce_user_main_thread_name[] = "main_thread";
	//int sce_user_main_thread_priority = 32; // 64
	unsigned int sce_user_main_thread_stack_kb_size = 256;
	//int sce_user_main_thread_attribute = SCE_KERNEL_TH_USE_VFPU;

	static bool gExit = false;

	static int HandleExit(int count, int arg1, void *arg2)
	{
		(void)count;
		(void)arg1;
		(void)arg2;

		gExit = true;

		// Make sure all save operation are finished before we exit.

		PspSaveData_Finish(false);

		// Call exit directly if this is a deployment build. Else, let the game shutdown gracefully, so
		// we can capture alloc stats and whatnot..

#if defined(DEPLOYMENT)
		// Call game exit system call (no need to finish the sound thread)
		sceKernelExitGame();
#endif

		return 0;
	}
}

#if defined(DEPLOYMENT) && !defined(__vfpu_enabled__)
//#error "Expected VFPU support to be enabled in deployment builds"
#endif

GameView_Psp* gGameView = 0;

static void ProgressBegin();
static void ProgressUpdate(const char* text);
static void ProgressEnd();
static void ProgressCallback(const char* name);

// FMAC = Flash Media Access

// ***CB methods invoke callback on completion
// sceKernelCreateCallback("POWER", CallbackFunc, cookie);

// todo: add these
//int sceKernelRegisterStdoutPipe(SceUID uid); // redirect stdout to pipe

//todo: read kernel thread docs
//#include <ctrlsvc.h>
//int sceCtrlSetSamplingCycle(0); // sample @ VBLANK time
//todo: ScePspFVector2 for Vec2F?

class MyEventHandler : public EventHandler
{
public:
	virtual bool OnEvent(Event& event)
	{
		if (event.type == EVT_JOYBUTTON)
		{
			if (event.joy_button.button == INPUT_BUTTON_PSP_CROSS && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_MENU_SELECT));
			if (event.joy_button.button == INPUT_BUTTON_PSP_CIRCLE && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_MENU_BACK));

			if (event.joy_button.button == INPUT_BUTTON_PSP_START && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_PAUSE));
			if (event.joy_button.button == INPUT_BUTTON_PSP_SELECT && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_PAUSE));
		}

		return g_GameState->m_MenuMgr->HandleEvent(event);
	}
};

static MyEventHandler sMyEventHandler;

static void HandleMsgResult(int id, bool result)
{
	LOG_INF("message box result: %d: %s", id, result ? "true" : "false");
}

static void TestMessage()
{
	PspMessageBox_ShowDialog("Hello World", 10, PspMbButtons_YesNo, HandleMsgResult);
	//PspMessageBox_ShowError(0x80110321);
	//PspMessageBox_Finish();
}

#include <libgu.h>

static void TestVFPU()
{
	Vec2F v1(10.0f, 20.0f);
	Vec2F v2(30.0f, 40.0f);
	float s1 = v1 * v2;
	float s2 = v1.Length_get();
	float s3 = v1.LengthSq_get();
	float a = v1.ToAngle();
	v2.Normalize();
	Vec2F v3 = v1.Normal();
	v1.SetZero();
	Vec2F d = v3.UnitDelta(v2);
	Vec2F l = v1.LerpTo(v2, 0.25f);
	Vec2F scl1 = d.Scale(10.0f);
	Vec2F scl2 = d * 10.0f;
	Vec2F cm = scl1 ^ scl2;
}

int main(int argc, char* argv[])
{
	try
	{
		//TestVFPU();

		SceUID exitHandlerId;

		if (sceKernelSetCompiledSdkVersion(SCE_DEVKIT_VERSION) < 0)
		{
			LOG_ERR("failed to set compiled SDK version", 0);
		}
		exitHandlerId = sceKernelCreateCallback("exit", HandleExit, NULL);
		if (exitHandlerId <= 0)
		{
#if !defined(DEPLOYMENT)
			throw ExceptionVA("failed to register exit handler");
#else
			LOG_ERR("failed to register exit handler :%08x", exitHandlerId);
#endif
		}
		if (sceKernelRegisterExitCallback(exitHandlerId) < 0)
		{
			LOG_ERR("failed to register exit callback", 0);
		}
		if (sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITALANALOG) < 0)
		{
			LOG_ERR("failed to set controller sampling mode", 0);
		}
		if (sceKernelChangeCurrentThreadAttr(0, SCE_KERNEL_TH_USE_VFPU) < 0) // enable VFPU
		{
			LOG_WRN("failed to enable VFPU", 0);
		}
		if (sceImposeSetLanguageMode(SCE_UTILITY_LANG_ENGLISH, SCE_UTILITY_CTRL_ASSIGN_CROSS_IS_ENTER) < 0)
		{
			LOG_ERR("failed to set impose language", 0);
		}

		//TestSave();
		//TestSave2();

		if (mix_loadModule() < 0)
		{
			LOG_ERR("failed to load mixer modules", 0);
		}
		if (mix_init() < 0)
		{
			LOG_ERR("failed to initialize mixer", 0);
		}

		HeapInit();

#if defined(DEBUG)
		const int isProviewEnabled = snIsProView();

		if (isProviewEnabled != 0)
		{
			const int version = snGetProViewVersion();

			LOG_INF("SN Systems ProView enabled. version: %d", version);
		}
#endif

		Calc::Initialize();
		Calc::Initialize_Color();

		//

		gGameView = new GameView_Psp();
		gGameView->Initialize();

		//TestMessage();
		//PspMessageBox_ShowDialog("This game uses autosave. Blah", 0, PspMbButtons_Ok, 0);
		//PspKeyboard_Show();

		gGameView->Run();
		
		gGameView->Shutdown();
		delete gGameView;
		gGameView = 0;
		
		if (mix_shutdown() < 0)
		{
			LOG_ERR("failed to shutdown mixer", 0);
		}
		if (mix_unloadModule() < 0)
		{
			LOG_ERR("failed to unload mixer modules", 0);
		}

		DBG_PrintAllocState();

		sceKernelExitGame();

		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());

		return -1;
	}
}

GameView_Psp::GameView_Psp()
{
	m_GameState = 0;
	m_LogicTimer = 0;
	m_SaveIconTimer = 0;
	m_ControlStatePrev = new SceCtrlData();
}

GameView_Psp::~GameView_Psp()
{
	delete m_ControlStatePrev;
	m_ControlStatePrev = 0;
}

void GameView_Psp::Initialize()
{
	UsingBegin (Benchmark gbm("GameView init"))
	{
		gGameView = this;
		
		m_Log = LogCtx("GameView");
		
		EventManager::I().AddEventHandler(&sMyEventHandler, EVENT_PRIO_INTERFACE);
		EventManager::I().Enable(EVENT_PRIO_INTERFACE);

		UsingBegin (Benchmark bm("Graphics System"))
		{
			// Initialize graphics system

			gGraphics.Initialize(480, 272);
		}
		UsingEnd()

		ProgressBegin();
		ProgressUpdate("LOADING..");
		ProgressEnd();

		UsingBegin (Benchmark bm("Audio System"))
		{
			// todo: Initialize audio system
		}
		UsingEnd()

		UsingBegin (Benchmark bm("GameState"))
		{
			// Create game state
		
			m_GameState = new Application();
			m_GameState->Setup(0, 1.0f, ProgressCallback);
		}
		UsingEnd()

		//ProgressEnd();

		// Make sure controller is initialized

		Game::EventHandler_PlayerController_Gamepad_Psp::I();
		
		// Initialize save icon timer

		m_SaveIconTimer = new AnimTimer(g_GameState->m_TimeTracker_Global, false);

		// Initialize logic timer
		
		m_Log.WriteLine(LogLevel_Debug, "Initializing logic timer");
		
		m_LogicTimer = new LogicTimer();
		m_LogicTimer->Setup(new Timer(), true, LOGIC_FPS, 3);

		// Begin main loop
		
		m_Log.WriteLine(LogLevel_Debug, "Starting main loop");
		
		m_LogicTimer->Start();
	}
	UsingEnd();
}

void GameView_Psp::Shutdown()
{
	m_Log.WriteLine(LogLevel_Info, "Shutdown");
	
	// Free objects (in reverse order of creation)
	
	delete m_SaveIconTimer;
	m_SaveIconTimer = 0;

	delete m_LogicTimer;
	m_LogicTimer = 0;
	
	delete m_GameState;
	m_GameState = 0;

	// todo: shutdown audio system

	// todo: shutdown graphics

	gGraphics.Shutdown();

	gGameView = 0;
}

Vec2F GameView_Psp::ScreenToWorld(Vec2F location)
{
	return m_GameState->m_Camera->ViewToWorld(location);
}

void GameView_Psp::Run()
{
	m_Log.WriteLine(LogLevel_Info, "Run");
	
	while (gExit == false)
	{
		UsingBegin(PerfTimer timer(PC_UPDATE))
		{
			Update();
		}
		UsingEnd()
		
		UsingBegin(PerfTimer timer(PC_RENDER))
		{
			Render();
		}
		UsingEnd()

		sceKernelCheckCallback();
	}
}

void GameView_Psp::UpdateInput()
{
	// read PSP controller state

	SceCtrlData state;

	if (sceCtrlPeekBufferPositive(&state, 1) < 0)
		throw ExceptionVA("unable to read controller buffer");

	if (state.Buttons & SCE_CTRL_INTERCEPTED)
		return;
	if (PspMessageBox_IsActive() || PspKeyboard_IsActive())
		return;

	EventManager::I().AddEvent(Event(EVT_JOYMOVE_ABS, 0, 0, AnalogToInt(state.Lx)));
	EventManager::I().AddEvent(Event(EVT_JOYMOVE_ABS, 0, 1, AnalogToInt(state.Ly)));

	int buttonChanges = state.Buttons ^ m_ControlStatePrev->Buttons;
	
	if (buttonChanges & SCE_CTRL_UP)
		EventManager::I().AddEvent(Event(EVT_KEY, IK_UP, state.Buttons & SCE_CTRL_UP));
	if (buttonChanges & SCE_CTRL_DOWN)
		EventManager::I().AddEvent(Event(EVT_KEY, IK_DOWN, state.Buttons & SCE_CTRL_DOWN));
	if (buttonChanges & SCE_CTRL_LEFT)
		EventManager::I().AddEvent(Event(EVT_KEY, IK_LEFT, state.Buttons & SCE_CTRL_LEFT));
	if (buttonChanges & SCE_CTRL_RIGHT)
		EventManager::I().AddEvent(Event(EVT_KEY, IK_RIGHT, state.Buttons & SCE_CTRL_RIGHT));

	if (buttonChanges & SCE_CTRL_CIRCLE)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_CIRCLE, state.Buttons & SCE_CTRL_CIRCLE));
	if (buttonChanges & SCE_CTRL_CROSS)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_CROSS, state.Buttons & SCE_CTRL_CROSS));
	if (buttonChanges & SCE_CTRL_SQUARE)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_SQUARE, state.Buttons & SCE_CTRL_SQUARE));
	if (buttonChanges & SCE_CTRL_TRIANGLE)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_TRIANGLE, state.Buttons & SCE_CTRL_TRIANGLE));

	if (buttonChanges & SCE_CTRL_SELECT)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_SELECT, state.Buttons & SCE_CTRL_SELECT));
	if (buttonChanges & SCE_CTRL_START)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_START, state.Buttons & SCE_CTRL_START));
	if (buttonChanges & SCE_CTRL_L)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_TOPLEFT, state.Buttons & SCE_CTRL_L));
	if (buttonChanges & SCE_CTRL_R)
		EventManager::I().AddEvent(Event(EVT_JOYBUTTON, 0, INPUT_BUTTON_PSP_TOPRIGHT, state.Buttons & SCE_CTRL_R));

	*m_ControlStatePrev = state;

	// note: according to the TRC, the title should continue unaware of the home pop-up being visible or not
	/*if (state.Buttons & SCE_CTRL_INTERCEPTED && g_GameState->ActiveView_get() == View_InGame)
	{
		g_GameState->ActiveView_set(View_Pause);
	}*/
}

void GameView_Psp::Update()
{
	//Benchmark bm(__FUNCTION__);

	// prevent the LCD screen from powering down during all but these views

	View view = g_GameState->ActiveView_get();

	if (view != View_Pause && view != View_Options && view != View_Upgrade)
	{
		sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DEFAULT);
	}

	// update save system

	if (PspSaveData_Update())
	{
		m_SaveIconTimer->Start(AnimTimerMode_TimeBased, true, 0.3f, AnimTimerRepeat_None);
	}

	// update input

	UpdateInput();

	// process events

	EventManager::I().Purge();

	// update loop w/ time recovery
	
	bool update = true;

	if (update)
	{
		m_GameState->Update();

		//g_ViewListener.Update(1.0f / LOGIC_FPS);
	}
}

void GameView_Psp::Render()
{
	//Benchmark bm(__FUNCTION__);

	g_PerfCount.Set_Count(PC_RENDER_FLUSH, 0);

	// select async present or finish on main thread based on utility status

	const bool asyncSwap = 
		PspMessageBox_IsActive() == false &&
		PspKeyboard_IsActive() == false;

	gGraphics.AsyncSwapEnabled_set(asyncSwap);

	//

	gGraphics.MakeCurrent();
	
	UsingBegin(PerfTimer timer(PC_RENDER_SETUP))
	{
		// prepare OpenGL transformation for landscape mode drawing with X axis right and Y axis down
		
		Mat4x4 matP;
		matP.MakeOrthoLH(0.0f, VIEW_SX, 0.0f, VIEW_SY, 0.5f, 100.0f);
		gGraphics.MatrixSet(MatrixType_Projection, matP);

		Mat4x4 matW;
		matW.MakeIdentity();
		gGraphics.MatrixSet(MatrixType_World, matW);
	}
	UsingEnd()

	m_GameState->Render();

	// draw save icon

	if (m_SaveIconTimer->IsRunning_get())
	{
		gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
		const float t = m_SaveIconTimer->Progress_get();
		g_GameState->Render(g_GameState->GetShape(Resources::PSP_SAVE_ICON), Vec2F(VIEW_SX - 5, 5.0f), 0.0f, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, t));
		g_GameState->m_SpriteGfx->Flush();
		g_GameState->m_SpriteGfx->FrameEnd();
	}

	if (asyncSwap)
	{
		UsingBegin(PerfTimer timer(PC_RENDER_PRESENT))
		{
			// present rendered output to screen

			gGraphics.Present();
		}
		UsingEnd()
	}
	else
	{
		LOG_WRN("manual sync", 0);

		sceGuFinish();
		sceGuSync(SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT);

		// update utilities

		PspMessageBox_Update();
		PspKeyboard_Update();

		sceGuSync(SCEGU_SYNC_FINISH, SCEGU_SYNC_WAIT);

		gGraphics.mRenderInProgress = false;

		gGraphics.ManualSwap();
	}
}

//

static Res* sProgressBack = 0;
static SpriteGfx* sProgressGfx = 0;

static void ProgressBegin()
{
	sProgressGfx = new SpriteGfx(128, 256, SpriteGfxRenderTime_OnFrameEnd);

	// use ResMgr to load load image.

	ResMgr resMgr;

	sProgressBack = resMgr.CreateTextureDDS("psp_load_01.dds");

	if (sProgressBack == 0)
		throw ExceptionVA("unable to load loading screen texture");

	gGraphics.TextureCreate(sProgressBack);
}

static void ProgressUpdate(const char* text)
{
	SpriteGfx& gfx = *sProgressGfx;

	for (int i = 0; i < 2; ++i)
	{
		gGraphics.MakeCurrent();
		Mat4x4 mat;
		mat.MakeOrthoLH(0.0f, VIEW_SX, 0.0f, VIEW_SY, 0.5f, 100.0f);
		gGraphics.MatrixSet(MatrixType_Projection, mat);
		mat.MakeIdentity();
		gGraphics.MatrixSet(MatrixType_World, mat);
		gGraphics.BlendModeSet(BlendMode_Normal_Opaque);
		gGraphics.TextureSet(sProgressBack);
		TempRenderBegin(&gfx);
		const float texCoord1[] = { 0.0f, 0.0f };
		const float texCoord2[] = { 1.0f, 1.0f };
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), SpriteColors::White, texCoord1, texCoord2);
		gfx.Flush();
		gGraphics.TextureSet(0);
		sceGuDebugPrint(4, VIEW_SY - 8 - 4, SpriteColor_Make(50, 100, 200, 255).rgba, text);
		//const float sx = Calc::Random(VIEW_SX);
		//RenderRect(Vec2F(0.0f, VIEW_SY/2.0f), Vec2F(sx, 50.0f), SpriteColors::White, texCoord1, texCoord2);
		gfx.Flush();
		gfx.FrameEnd();
		gGraphics.Present();
	}
}

static void ProgressEnd()
{
	if (sProgressBack)
		gGraphics.TextureDestroy(sProgressBack);

	delete sProgressBack;
	sProgressBack = 0;

	delete sProgressGfx;
	sProgressGfx = 0;
}

static void ProgressCallback(const char* name)
{
	//ProgressUpdate(name);
}
