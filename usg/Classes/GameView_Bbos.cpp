/*

	15 seconds to check dependencies (nothing needs to compile)
 	 3 seconds to compile a single file + link
	90 seconds to download/debug

 */
#include <bps/accelerometer.h>
#include <bps/bps.h>
#include <bps/dialog.h>
#include <bps/event.h>
#include <bps/navigator.h>
#include <bps/orientation.h>
#include <bps/screen.h>
#include <bps/virtualkeyboard.h>
#include <unistd.h>
#include "Benchmark.h"
#include "Calc.h"
#include "Camera.h"
#include "ConfigState.h"
#include "Event.h"
#include "EventHandler_PlayerController_Gamepad.h"
#include "EventManager.h"
#include "GameState.h"
#include "GameView_Bbos.h"
#include "LogicTimer.h"
#include "MenuMgr.h"
#include "OpenALState.h"
#include "OpenGLUtil.h"
#include "PerfCount.h"
#ifdef BBOS_ALLTOUCH
#include "SocialAPI_ScoreLoop.h"
#else
#include "SocialAPI_Dummy.h"
#endif
#include "ISoundPlayer.h"
#include "SpriteGfx.h"
#include "System.h"
#include "TempRender.h"
#include "Textures.h"
#include "TimeTracker.h"
#include "TouchMgr.h"
#include "Types.h"
#include "Util_ColorEx.h"

#include "Game/GameSettings.h"

#define SCORELOOP_GAME_ID "df95d245-b9f4-454f-b843-9cd17aeb6ac6"
#define SCORELOOP_GAME_VERSION "1.0"
#define SCORELOOP_GAME_KEY "mPo6xrw+6oCUUCzpAB7bipsqmFfFomR9HK2+x3bcWSked+SVV9HQTg=="

#define LOGIC_FPS 60.0f
#define FADEIN_FRAMES 60

#if defined(DEBUG)
	#define TOUCH_DEBUG 0
#else
	#define TOUCH_DEBUG 0
#endif

GameView_Bbos* gGameView = 0;

static int sFadeinFrame = FADEIN_FRAMES;

#if TOUCH_DEBUG
#include "Graphics.h"
#include "StringBuilder.h"
#include "UsgResources.h"
static Vec2F s_LastTouchPos(0.0f, 0.0f);
static float s_LastTouchTimer = 0.0f;
#endif

static void HandleException(std::exception & e)
{
	LOG_ERR(e.what(), 0);
}

static void FixOrientation()
{
#ifdef BBOS_ALTOUCH
	orientation_direction_t orientation;
	int angle = 0;

	orientation_get(&orientation, &angle);

	int angle2 = atoi(getenv("ORIENTATION"));

	LOG_INF("orientation: %d degrees (vs %d)", angle, angle2);

	if (navigator_set_orientation_mode(NAVIGATOR_LANDSCAPE, NULL) != BPS_SUCCESS)
	{
		LOG_ERR("Failed: navigator_set_orientation_mode", 0);
	}

	if (navigator_rotation_lock(true) != BPS_SUCCESS)
	{
		LOG_ERR("Failed: navigator_rotation_lock", 0);
	}
#endif
}

class MyEventHandler : public EventHandler
{
public:
	virtual bool OnEvent(Event& event)
	{
		if (event.type == EVT_JOYBUTTON)
		{
			// translate gamepad events into menu/key events

			if (event.joy_button.button == GamepadButton_Right_Down && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_MENU_SELECT));
			if (event.joy_button.button == GamepadButton_Right_Right && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_MENU_BACK));

			if (event.joy_button.button == GamepadButton_Start && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_PAUSE));
			if (event.joy_button.button == GamepadButton_Select && event.joy_button.state)
				EventManager::I().AddEvent(Event(EVT_PAUSE));

			if (event.joy_button.button == GamepadButton_Left_Up)
				EventManager::I().AddEvent(Event(EVT_KEY, IK_UP, event.joy_button.state));
			if (event.joy_button.button == GamepadButton_Left_Down)
				EventManager::I().AddEvent(Event(EVT_KEY, IK_DOWN, event.joy_button.state));
			if (event.joy_button.button == GamepadButton_Left_Left)
				EventManager::I().AddEvent(Event(EVT_KEY, IK_LEFT, event.joy_button.state));
			if (event.joy_button.button == GamepadButton_Left_Right)
				EventManager::I().AddEvent(Event(EVT_KEY, IK_RIGHT, event.joy_button.state));
		}

		return false;
	}
};

class UiEventHandler : public EventHandler
{
public:
	virtual bool OnEvent(Event& event)
	{
		return g_GameState->m_MenuMgr->HandleEvent(event);
	}
};

static MyEventHandler s_myEventHandler;
static UiEventHandler s_uiEventHandler;

GameView_Bbos::GameView_Bbos()
{
	m_IsInitialized = false;
	m_IsBackgrounded = false;

	mHasTilt = false;

	m_TouchMgr = 0;
	m_OpenGLState = 0;
	m_OpenALState = 0;
	m_Gamepad = 0;
	m_GamepadIsEnabled = false;
	m_InputMode = InputMode_Touch;

	m_GameState = 0;
	m_LogicTimer = 0;
}

GameView_Bbos::~GameView_Bbos()
{
}

void GameView_Bbos::Initialize()
{
	try
	{
		Assert(!m_IsInitialized);
		if (m_IsInitialized)
			return;
		
		m_IsInitialized = true;

		UsingBegin (Benchmark gbm("GameView init"))
		{
			gGameView = this;
			
			m_Log = LogCtx("GameView");

			// Initialize the BPS library

			if (bps_initialize() != BPS_SUCCESS)
			{
				LOG_ERR("Failed: bps_initialize", 0);
			}

		#ifdef DEBUG
			bps_set_verbosity(2);
		#endif

			if (navigator_request_events(0) != BPS_SUCCESS)
			{
				LOG_ERR("Failed: navigator_request_events", 0);
			}

			if (dialog_request_events(0) != BPS_SUCCESS)
			{
				LOG_ERR("Failed: dialog_request_events", 0);
			}

			// Create a screen context that will be used to create an EGL surface to to receive libscreen events

			screen_create_context(&mScreenCtx, 0);

			// Signal BPS library that navigator and screen events will be requested.

			if (screen_request_events(mScreenCtx) != BPS_SUCCESS)
			{
				LOG_ERR("Failed: screen_request_events", 0);
			}

			mHasTilt = accelerometer_is_supported();

			if (mHasTilt)
			{
				if (accelerometer_set_update_frequency(FREQ_40_HZ) != BPS_SUCCESS)
				{
					LOG_ERR("Failed: accelerometer_set_update_frequency", 0);
				}
			}

			//

			if (!SocialSC_Initialize(SCORELOOP_GAME_ID, SCORELOOP_GAME_VERSION, SCORELOOP_GAME_KEY))
			{
				LOG_ERR("Failed: SocialSC_Initialize", 0);
			}

			/*if (!SocialSCUI_Initialize())
			{
				LOG_ERR("Failed: SocialSCUI_Initialize", 0);
			}*/

			FixOrientation();

			// Create touch manager
			
			m_TouchMgr = new TouchMgr();
			
			UsingBegin (Benchmark bm("OpenGL"))
			{
				// Initialize OpenGL
			
				m_OpenGLState = new OpenGLState_BBOS();
			
				bool trueColor = true;
				
				if (!m_OpenGLState->Initialize(mScreenCtx, VIEW_SX, VIEW_SY, trueColor))
				{
					LOG_ERR("Failed to initialize OpenGL", 0);
				}
			}
			UsingEnd()

			// todo: try to redraw screen. does it work? can we use it to implement a simple loading screen?

			UsingBegin (Benchmark bm("OpenAL"))
			{
				// Initialize OpenAL

				m_OpenALState = new OpenALState();

				if (!m_OpenALState->Initialize())
				{
					m_Log.WriteLine(LogLevel_Error, "Failed to initialize OpenAL");
				}
			}
			UsingEnd()

			EventManager::I().AddEventHandler(&s_myEventHandler, EVENT_PRIO_NORMALIZE);
			EventManager::I().Enable(EVENT_PRIO_NORMALIZE);
			EventManager::I().AddEventHandler(&s_uiEventHandler, EVENT_PRIO_INTERFACE);
			EventManager::I().Enable(EVENT_PRIO_INTERFACE);

			Game::EventHandler_PlayerController_Gamepad::I().Initialize();

			UsingBegin (Benchmark bm("Gamepad"))
			{
				m_Gamepad = new Gamepad_BBOS(mScreenCtx);

				if (!m_Gamepad->Initialize())
				{
					m_Log.WriteLine(LogLevel_Error, "Failed to initialize gamepad");
				}
			}
			UsingEnd()

			UsingBegin (Benchmark bm("GameState"))
			{
				// Create game state
			
				m_GameState = new Application();
				m_GameState->Setup(m_OpenALState, 2.f);
	//			m_GameState->ActiveView_set(View_Main);
			}
			UsingEnd()

			// Route touch input to game state

			m_Log.WriteLine(LogLevel_Debug, "Setting up touch callbacks");

			m_TouchMgr->OnTouchBegin = CallBack(m_GameState, Application::HandleTouchBegin);
			m_TouchMgr->OnTouchEnd = CallBack(m_GameState, Application::HandleTouchEnd);
			m_TouchMgr->OnTouchMove = CallBack(m_GameState, Application::HandleTouchMove);
			
			// Initialize logic timer
			
			m_Log.WriteLine(LogLevel_Debug, "Initializing logic timer");
			
			m_LogicTimer = new LogicTimer();
			m_LogicTimer->Setup(new Timer(), true, 60.0f, 3);
			
			// Begin main loop
			
			m_Log.WriteLine(LogLevel_Debug, "Starting main loop");
			
			m_LogicTimer->Start();

			//

			if (/*m_Gamepad->IsConnected() && */false) // always start up in touch mode
			{
				InputMode_set(InputMode_Gamepad);
			}
			else
			{
				InputMode_set(InputMode_Touch);
			}
		}
		UsingEnd();
	}
	catch (Exception& e)
	{
		HandleException(e);
		
		throw e;
	}
}

void GameView_Bbos::Shutdown()
{
	try
	{
		m_Log.WriteLine(LogLevel_Info, "dealloc");
		
		// Free objects (in reverse order of creation)
		
		delete m_LogicTimer;
		m_LogicTimer = 0;
		
		Game::EventHandler_PlayerController_Gamepad::I().Shutdown();

		EventManager::I().Disable(EVENT_PRIO_NORMALIZE);
		EventManager::I().RemoveEventHandler(&s_myEventHandler, EVENT_PRIO_NORMALIZE);
		EventManager::I().Disable(EVENT_PRIO_INTERFACE);
		EventManager::I().RemoveEventHandler(&s_uiEventHandler, EVENT_PRIO_INTERFACE);

		delete m_GameState;
		m_GameState = 0;
		
		m_Gamepad->Shutdown();
		delete m_Gamepad;
		m_Gamepad = 0;

		m_OpenALState->Shutdown();
		delete m_OpenALState;
		m_OpenALState = 0;
		
		delete m_TouchMgr;
		m_TouchMgr = 0;

		m_OpenGLState->Shutdown();
		delete m_OpenGLState;
		m_OpenGLState = 0;
		
		//SocialSCUI_Shutdown();
		SocialSC_Shutdown();

		LOG_INF("shutting down BBOS services", 0);

		if (mHasTilt)
		{
			if (accelerometer_set_update_frequency(FREQ_1_HZ) != BPS_SUCCESS)
			{
				LOG_ERR("Failed: accelerometer_set_update_frequency", 0);
			}
		}

		screen_stop_events(mScreenCtx);

		bps_shutdown();

		screen_destroy_context(mScreenCtx);

		LOG_INF("shutting down BBOS services [done]", 0);
	}
	catch (Exception& e)
	{
		HandleException(e);
	}
}

static Vec2F ScreenToView(Vec2F location)
{
#if defined(BBOS_ALLTOUCH) && 0
	float temp = location[0];
	location[0] = SCREEN_SX - location[1];
	location[1] = temp;
#endif

	if (g_GameState->m_GameSettings->m_ScreenFlip)
	{
		location[0] = SCREEN_SX - location[0];
		location[1] = SCREEN_SY - location[1];
	}

	location[0] /= SCREEN_SX / float(VIEW_SX);
	location[1] /= SCREEN_SY / float(VIEW_SY);

	return location;
}

Vec2F GameView_Bbos::ScreenToWorld(Vec2F location, bool offset)
{
	location = ScreenToView(location);

	return m_GameState->m_Camera->ViewToWorld(location);
}

void GameView_Bbos::HandleTouch_Begin(int index, int x, int y)
{
	try
	{
		if (m_IsInitialized == false)
			return;

		InputMode_set(InputMode_Touch);
		
		Vec2F location(x, y);

		Vec2F locationV = ScreenToView(location);
		Vec2F locationW = ScreenToWorld(location, false);
		Vec2F locationWOffset = ScreenToWorld(location, true);

		//LOG_INF("evt: touch begin (%g, %g) %d", locationV[0], locationV[1], index);

		m_TouchMgr->TouchBegin(index, locationV, locationW, locationWOffset);
			
#if TOUCH_DEBUG
		s_LastTouchPos = locationV;
		s_LastTouchTimer = 1.0f;
#endif
	}
	catch (Exception& e)
	{
		HandleException(e);
	}
}

void GameView_Bbos::HandleTouch_Move(int index, int x, int y)
{
	try
	{
		if (m_IsInitialized == false)
			return;
		if (m_GamepadIsEnabled == true)
			return;
			
		Vec2F location(x, y);

		Vec2F locationV = ScreenToView(location);
		Vec2F locationW = ScreenToWorld(location, false);
		Vec2F locationWOffset = ScreenToWorld(location, true);

		//LOG_INF("evt: touch move (%g, %g) %d", locationV[0], locationV[1], index);

		m_TouchMgr->TouchMoved(index, locationV, locationW, locationWOffset);
			
#if TOUCH_DEBUG
		s_LastTouchPos = locationV;
		s_LastTouchTimer = 1.0f;
#endif
	}
	catch (Exception& e)
	{
		HandleException(e);
	}
}

void GameView_Bbos::HandleTouch_End(int index)
{
	try
	{
		if (m_IsInitialized == false)
			return;
		if (m_GamepadIsEnabled == true)
			return;

		//LOG_INF("evt: touch end %d", index);

		m_TouchMgr->TouchEnd(index);
	}
	catch (Exception& e)
	{
		HandleException(e);
	}
}

void GameView_Bbos::HandleTouch_Cancel(int index)
{
	if (m_IsInitialized == false)
		return;
	if (m_GamepadIsEnabled == true)
		return;
	
	HandleTouch_End(index);
}

void GameView_Bbos::Pause()
{
	try
	{
		m_Log.WriteLine(LogLevel_Debug, "Pause");
		
		Assert(!m_IsBackgrounded);

		m_LogicTimer->Pause();

		m_GameState->m_SoundPlayer->Stop();

		m_OpenALState->Activation_set(false);

		m_IsBackgrounded = true;
	}
	catch (Exception& e)
	{
		HandleException(e);
	}
}

void GameView_Bbos::Resume()
{
	try
	{
		m_Log.WriteLine(LogLevel_Debug, "Resume");
		
		Assert(m_IsBackgrounded);

		m_IsBackgrounded = false;

		m_OpenALState->Activation_set(true);

		m_GameState->m_SoundPlayer->Start();

		ConfigLoad();

		m_LogicTimer->Resume();
		
		//g_TimerRT.Time_set(gPauseTime);
		
		g_GameState->m_TimeTracker_Global->Time_set(g_TimerRT.Time_get());
	}
	catch (Exception& e)
	{
		HandleException(e);
	}
}

void GameView_Bbos::Run()
{
	m_Log.WriteLine(LogLevel_Info, "Run");

	float step = 1.0f / (LOGIC_FPS + 2.0f);

	bool stop = false;

	while (stop == false)
	{
#if defined(DEBUG) && 0
		static int n = 100;
		if (--n == 0)
			stop = true;
#endif

		// event loop

        while (true)
        {
        	bps_event_t * event = 0;

        	int rc = bps_get_event(&event, 0);
        	Assert(rc == BPS_SUCCESS);

            if (event != 0)
            {
				#ifdef BBOS_ALLTOUCH
            	SocialSC * social = (SocialSC*)m_GameState->m_Social;
				#else
            	SocialAPI_Dummy * social = (SocialAPI_Dummy*)m_GameState->m_Social;
				#endif

            	social->HandleEvent(event);

            	m_Gamepad->HandleEvent(event);

            	//SocialSCUI_HandleEvent(event);

                int domain = bps_event_get_domain(event);

                if (domain == screen_get_domain())
                {
                	screen_event_t screen_event = screen_event_get_event(event);

                	int screen_val;

                	screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val);

                	//LOG_DBG("evt: got screen event: %d", screen_val);

					switch (screen_val)
					{
						case SCREEN_EVENT_MTOUCH_TOUCH:
						{
							int position[2];
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_POSITION, position);
							int touchId;
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TOUCH_ID, &touchId);
							HandleTouch_Begin(touchId, position[0], position[1]);
							break;
						}
						case SCREEN_EVENT_MTOUCH_MOVE:
						{
							int position[2];
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_POSITION, position);
							int touchId;
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TOUCH_ID, &touchId);
							HandleTouch_Move(touchId, position[0], position[1]);
							break;
						}
						case SCREEN_EVENT_MTOUCH_RELEASE:
						{
							int position[2];
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_POSITION, position);
							int touchId;
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TOUCH_ID, &touchId);
							HandleTouch_End(touchId);
							break;
						}
						case SCREEN_EVENT_POINTER:
						{
							int position[2];
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_SOURCE_POSITION, position);
							int buttons = 0;
							screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_BUTTONS, &buttons);
							static bool oldPressed = false;
							bool newPressed = (buttons & SCREEN_LEFT_MOUSE_BUTTON) != 0;
							if (oldPressed != newPressed)
							{
								oldPressed = newPressed;
								if (newPressed)
								{
									// mouse went from released -> pressed
									HandleTouch_Begin(0, position[0], position[1]);
								}
								else
								{
									// mouse went from pressed -> released
									HandleTouch_End(0);
								}
							}
							else if (newPressed)
							{
								// mouse move
								HandleTouch_Move(0, position[0], position[1]);
							}

							break;
						}
						default:
							break;
					}
                }
                else if (domain == navigator_get_domain())
                {
                	unsigned int code = bps_event_get_code(event);

                	//LOG_DBG("evt: got navigator event: %x", code);

                	switch (code)
                	{
                		case NAVIGATOR_EXIT:
                		{
                			stop = true;
                			break;
                		}
                		case NAVIGATOR_WINDOW_ACTIVE:
                		{
                			if (m_IsBackgrounded)
                			{
                				Resume();
                			}
                			break;
                		}
                		case NAVIGATOR_WINDOW_INACTIVE:
                		{
                			if (!m_IsBackgrounded)
                			{
                				Pause();
                			}
                			break;
                		}
                		case NAVIGATOR_WINDOW_STATE:
                		{
                			navigator_window_state_t state = navigator_event_get_window_state(event);

                			if (state == NAVIGATOR_WINDOW_THUMBNAIL)
                			{
                				if (!m_IsBackgrounded)
                				{
                					Pause();
                				}
                			}
                			if (state == NAVIGATOR_WINDOW_FULLSCREEN)
                			{
                				if (m_IsBackgrounded)
                				{
                					Resume();
                				}
                			}
                			break;
                		}
                		default:
                			break;
                	}
                }
                else if (domain == virtualkeyboard_get_domain())
                {
                	/*
					switch (code)
					{
						case VIRTUALKEYBOARD_EVENT_VISIBLE:
							keyboard_visible = true;
							break;
						case VIRTUALKEYBOARD_EVENT_HIDDEN:
							keyboard_visible = false;
							break;
					}
					*/
                }
            }
            else
            {
                break;
            }
        }

        //

        if (m_IsBackgrounded == false)
        {
			static GamepadState oldGamepadState;

			if (m_Gamepad->IsConnected())
			{
				GamepadState newGamepadState = m_Gamepad->GetState();

				if (m_InputMode != InputMode_Gamepad)
				{
					bool wantsFocus = false;

					for (int i = 0; i < GamepadState::kNumButtons; ++i)
						if (!oldGamepadState.buttons[i] && newGamepadState.buttons[i])
							wantsFocus = true;
					for (int i = 0; i < GamepadState::kNumAnalogs; ++i)
					{
						float value = newGamepadState.analogs[i] / float(1 << 15);
						if (Calc::Abs(value) >= 0.1f)
							wantsFocus = true;
					}

					if (wantsFocus)
					{
						InputMode_set(InputMode_Gamepad);
					}
				}
				else
				{
					for (int i = 0; i < GamepadState::kNumButtons; ++i)
					{
						if (oldGamepadState.buttons[i] != newGamepadState.buttons[i])
						{
							LOG_INF("gamepad button %d changed state to %d", i, newGamepadState.buttons[i] ? 1 : 0);

							Event e(EVT_JOYBUTTON, 0, i, newGamepadState.buttons[i]);

							EventManager::I().AddEvent(e);
						}
					}

					for (int i = 0; i < GamepadState::kNumAnalogs; ++i)
					{
						Event e(EVT_JOYMOVE_ABS, 0, i, newGamepadState.analogs[i]);

						EventManager::I().AddEvent(e);
					}

					static bool active[2] = { false, false };
					static float integrate[2] = { 0.f, 0.f };
					const float timeStep = 1.f / 60.f; // title runs at 60 Hz
					const float updateRate = 3.f; // simulate 3 events per second, at maximum analog stick extent
					const float updateStep = 1.f / updateRate;
					const float delta[2] =
						{
							newGamepadState.analogs[GamepadAnalog_LeftX] / float(1 << 15),
							newGamepadState.analogs[GamepadAnalog_LeftY] / float(1 << 15)
						};
					for (int i = 0; i < 2; ++i)
					{
						bool wasActive = active[i];
						active[i] = Calc::Abs(delta[i]) >= 0.8f;
						if (!wasActive && active[i])
							integrate[i] = delta[i] < 0.f ? -updateStep : +updateStep;
						else
							integrate[i] = active[i] ? integrate[i] + delta[i] * timeStep : 0.f;
						while (Calc::Abs(integrate[i]) > updateStep)
						{
							static const int code[4] = { IK_LEFT, IK_RIGHT, IK_UP, IK_DOWN };
							const int sign = integrate[i] < 0.f ? -1 : +1;
							const int codeIdx = i * 2 + (sign < 0 ? 0 : 1);
							EventManager::I().AddEvent(Event(EVT_KEY, code[codeIdx], 1));
							EventManager::I().AddEvent(Event(EVT_KEY, code[codeIdx], 0));
							integrate[i] -= updateStep * sign;
						}
					}
				}

				oldGamepadState = newGamepadState;
			}
			else
			{
				memset(&oldGamepadState, 0, sizeof(oldGamepadState));
			}
        }

		//

        if (m_IsBackgrounded == false)
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
        }
        else
        {
        	// how the hell does background mode work on BBOS?

        	sleep(0);
        }
	}
}

void GameView_Bbos::Update()
{
	// process events

	EventManager::I().Purge();

	// process game

	m_GameState->Update(1.0f);
	
	m_LogicTimer->EndUpdate();
}

void GameView_Bbos::Render()
{
	g_PerfCount.Set_Count(PC_RENDER_FLUSH, 0);
	
	UsingBegin(PerfTimer timer(PC_RENDER_MAKECURRENT))
	{	
		// prepare OpenGL context
		
		m_OpenGLState->MakeCurrent();
	}
	UsingEnd()
	
	// clear surface
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
//	glClearColor(0.0f, 0.0f, 0.05f, 1.0f);
//	glClear(GL_COLOR_BUFFER_BIT);
	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	
	UsingBegin(PerfTimer timer(PC_RENDER_SETUP))
	{
		// prepare OpenGL transformation for landscape mode drawing with X axis right and Y axis down

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
#if defined(BBOS_ALLTOUCH) && 0
		glOrthof(0.0f, VIEW_SY, VIEW_SX, 0.0f, -1000.0f, 1000.0f);
		glTranslatef(VIEW_SY/2.f, VIEW_SX/2.f, 0.f);
		glRotatef(-90.f, 0.f, 0.f, 1.f);
		glTranslatef(-VIEW_SX/2.f, -VIEW_SY/2.f, 0.f);
#else
		if (!g_GameState->m_GameSettings->m_ScreenFlip)
			glOrthof(0.0f, VIEW_SX, VIEW_SY, 0.0f, -1000.0f, 1000.0f);
		else
			glOrthof(VIEW_SX, 0.0f, 0.0f, VIEW_SY, -1000.0f, 1000.0f);
#endif
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	UsingEnd()
	
	m_GameState->Render();
	
#if TOUCH_DEBUG
	s_LastTouchTimer -= 1.0f / 60.0f;
	if (s_LastTouchTimer > 0.0f)
	{
		float opacity = Calc::Min(1.0f, s_LastTouchTimer * 2.0f);
		gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
		TempRenderBegin(g_GameState->m_SpriteGfx);
		StringBuilder<32> sb;
		sb.AppendFormat("[%d, %d]", (int)s_LastTouchPos[0], (int)s_LastTouchPos[1]);
		RenderText(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, opacity), TextAlignment_Left, TextAlignment_Top, true, sb.ToString());
		g_GameState->m_SpriteGfx->Flush();
		g_GameState->m_SpriteGfx->FrameEnd();
	}
#endif
	
#if 1
	if (sFadeinFrame > 0)
	{
		float opacity = sFadeinFrame / float(FADEIN_FRAMES);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		TempRenderBegin(g_GameState->m_SpriteGfx);
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY), 1.0f, 1.0f, 1.0f, opacity, g_GameState->GetTexture(Textures::COLOR_WHITE));
		g_GameState->m_SpriteGfx->Flush();
		sFadeinFrame--;
	}
#endif

	UsingBegin(PerfTimer timer(PC_RENDER_PRESENT))
	{
		// present rendered output to screen
		
		m_OpenGLState->Present();
	}
	UsingEnd()
}

void GameView_Bbos::InputMode_set(InputMode mode)
{
	if (mode != m_InputMode)
	{
		switch (m_InputMode)
		{
			case InputMode_Touch:
				m_TouchMgr->EndAllTouches();
				break;
			case InputMode_Gamepad:
				m_GamepadIsEnabled = false;
				EventManager::I().AddEvent(Event(EVT_JOYDISCONNECT, 0));
				break;
		}

		m_InputMode = mode;

		switch (m_InputMode)
		{
			case InputMode_Touch:
				m_GameState->m_MenuMgr->CursorEnabled_set(false);
				m_GameState->m_GameSettings->SetControllerType(ControllerType_DualAnalog);
				break;
			case InputMode_Gamepad:
				m_GamepadIsEnabled = true;
				m_GameState->m_MenuMgr->CursorEnabled_set(true);
				m_GameState->m_GameSettings->SetControllerType(ControllerType_Gamepad);
				break;
		}
	}
}

int main(int argc, char* argv[])
{
	try
	{
		Calc::Initialize();
		Calc::Initialize_Color();

		gGameView = new GameView_Bbos();
		gGameView->Initialize();

		gGameView->Resume();

		gGameView->Run();

		gGameView->Shutdown();
		delete gGameView;
		gGameView = 0;

		LOG_INF("application exit", 0);

		return 0;
	}
	catch (std::exception& e)
	{
		printf("error: %s\n", e.what());

		return -1;
	}
}
