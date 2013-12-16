#include <SDL/SDL.h>
#ifdef WIN32
#include <windows.h>
#endif
#include "AnimTimer.h"
#include "Arguments.h"
//#include "AudioSession.h"
#include "Benchmark.h"
#include "Calc.h"
#include "Camera.h"
#include "ColHashMap.h"
#include "CompiledShape.h"
#include "Display_Sdl.h"
#include "EventHandler_PlayerController_Gamepad.h"
#include "EventHandler_PlayerController_Keyboard.h"
#include "EventManager.h"
#include "FileStream.h"
#include "GameSettings.h"
#include "GameState.h"
#include "GameView_Win32.h"
#include "LogicTimer.h"
#include "MenuMgr.h"
#include "OpenALState.h"
#include "OpenGLCompat.h"
#include "OpenGLUtil.h"
#include "Parse.h"
#include "Path.h"
#include "PerfCount.h"
#include "ResIO.h"
#include "ResMgr.h"
#include "SelectionBuffer_Test.h"
#include "SpriteGfx.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringBuilder.h"
#include "StringEx.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "TexturePVR.h"
#include "TextureRGBA.h"
#include "Textures.h"
#include "TouchMgr_Win32.h"
#include "Types.h"
#include "UsgResources.h"
#include "Util_Color.h"
#include "Util_ColorEx.h"
#include "WorldGrid.h"

#include "Game/GameRound.h" // music player hack

#define Sprite Sprute
//#include "QuickTimeEncoder.h"
#undef Sprite

#define SAVE_VIDEO 0
#define USE_VIDEO_BIN 1
#define SPLIT_BIN USE_VIDEO_BIN && 1
#define SAVE_QUICKTIME 0

#define DEFAULT_FULLSCREEN false
#define DEFAULT_DISPLAY_SCALE 2

#define DEFAULT_DISPLAY_SX (VIEW_SX * DEFAULT_DISPLAY_SCALE)
#define DEFAULT_DISPLAY_SY (VIEW_SY * DEFAULT_DISPLAY_SCALE)

//#define DEFAULT_DISPLAY_SX 1920
//#define DEFAULT_DISPLAY_SY 1080

#define LOGIC_FPS 60.0f

#define INCLUDE_KEY 0

#define FADEIN_FRAMES 60

#if INCLUDE_KEY
static int TranslateKey(int key);
#endif
static void SaveTGA(const char* fileName, char* bytes, int sx, int sy);
#if SAVE_VIDEO
static void SaveGL(const char* fileName);
#endif
#if SAVE_QUICKTIME
static void QtSaveGL();
static QuickTimeEncoder sQtEncoder;
static MacImage sQtImage;
#endif

#if 1
#include "AudioManager.h"
#include "System.h"
AudioManager* sAudioManager = 0;
static void TestAudioManager()
{
	if (sAudioManager == 0)
	{
		sAudioManager = new AudioManager();
		sAudioManager->Initialize();
		AudioSheet* sheet = new AudioSheet();
		for (int i = 0; i < 44100 * 60 * 10; i += 44100)
		{
			sheet->mFadeIn.push_back(i);
			sheet->mFadeOut.push_back(i);
		}
		sAudioManager->AudioLayerData_set(0, g_System.GetResourcePath("bgm_main_02.ogg").c_str(), sheet);
		sAudioManager->AudioLayerData_set(1, g_System.GetResourcePath("bgm_main_03.ogg").c_str(), sheet);
		sAudioManager->AudioLayerData_set(2, g_System.GetResourcePath("bgm_main_04.ogg").c_str(), sheet);
		sAudioManager->AudioLayerData_set(3, g_System.GetResourcePath("bgm_main_05.ogg").c_str(), sheet);
		sAudioManager->AudioLayerIsActive_set(0, true);
		sAudioManager->AudioLayerIsActive_set(1, true);
		sAudioManager->AudioLayerIsActive_set(2, true);
		sAudioManager->AudioLayerIsActive_set(3, false);
	}
	sAudioManager->Update();
	
#if 0
	static int frame = 0;
	if ((frame % (60 * 4)) == 0)
	{
		int channelIndex = rand() % 3;
		bool isActive = !sAudioManager->AudioLayerIsActive_get(channelIndex);
		sAudioManager->AudioLayerIsActive_set(channelIndex, isActive);
	}
	frame++;
#endif
}
#endif

GameView_Win32* gGameView = 0;

class AppSettings
{
public:
	AppSettings()
	{
		displayScale = DEFAULT_DISPLAY_SCALE;
		displaySx = DEFAULT_DISPLAY_SX;
		displaySy = DEFAULT_DISPLAY_SY;
		fullscreen = DEFAULT_FULLSCREEN;
#if defined(MACOS)
		controllerType = ControllerType_Gamepad;
#else
		controllerType = ControllerType_Keyboard;
#endif
	}

	int displayScale;
	int displaySx;
	int displaySy;
	bool fullscreen;
	ControllerType controllerType;
};

static AppSettings sAppSettings;

static int sFadeinFrame = FADEIN_FRAMES;

#include "DdsLoader.h"

int main(int argc, char* argv[])
{
	try
	{	
#ifdef DEBUG
#if 0
#if defined(WIN32)
#pragma warning("warning: DDS loader test enabled")
		DdsLoader ddsLoader;
		FileStream ddsStream("C:/test.dds", OpenMode_Read);
		StreamReader ddsStreamReader(&ddsStream, false);
		ddsLoader.LoadHeader(ddsStreamReader);
		LOG_DBG("DDS file position: %d", ddsStream.Position_get());
		if (ddsLoader.IsFourCC("DXT1"))
		{
			LOG_DBG("DXT1 file");
			const int sx = ddsLoader.mHeader.dwWidth;
			const int sy = ddsLoader.mHeader.dwHeight;
			const int bpp = 4;
			const int byteCount = sx * sy * bpp / 8;
			uint8_t* bytes = new uint8_t[byteCount];
			if (ddsStream.Read(bytes, byteCount) != byteCount)
				throw ExceptionVA("file read error");
			for (int i = 0; i < byteCount; ++i)
				printf("%d ", (int)bytes[i]);
		}
		ddsStream.Close();
#endif
#endif
#endif

#if defined(MACOS) || defined(LINUX)
		std::string path = Path::GetDirectory(argv[0]);
		
		if (!path.empty() && path != ".")
		{
			if (path[0] != '/')
				path = "/" + path;
				
			printf("%s\n", argv[0]);
			printf("%s\n", path.c_str());
			chdir(path.c_str());
		}
#endif
		
		Calc::Initialize();
		Calc::Initialize_Color();

		for (int i = 1; i < argc;)
		{
			if (!strcmp(argv[i], "-s"))
			{
				ARGS_CHECKPARAM(1);

				int scale = Parse::Int32(argv[i + 1]);

				if (scale <= 0 || scale > 4)
				{
					throw ExceptionVA("invalid display scale: %d", scale);
				}

				sAppSettings.displayScale = scale;
				sAppSettings.displaySx = VIEW_SX * scale;
				sAppSettings.displaySy = VIEW_SY * scale;

				i += 2;
			}
			else if (!strcmp(argv[i], "-f"))
			{
				ARGS_CHECKPARAM(1);

				sAppSettings.fullscreen = Parse::Bool(argv[i + 1]);

				i += 2;
			}
			else if (!strcmp(argv[i], "-p"))
			{
				ARGS_CHECKPARAM(1);

				std::string mode = argv[i + 1];

				if (mode == "gamepad")
					sAppSettings.controllerType = ControllerType_Gamepad;
				else if (mode == "keyboard")
					sAppSettings.controllerType = ControllerType_Keyboard;
				else
					throw ExceptionVA("unknown controller type: %s", mode.c_str());

				i += 2;
			}
			else
			{
				throw ExceptionVA("unknown argument: %s", argv[i]);
			}
		}
		
#if SAVE_QUICKTIME
		sQtImage.Size_set(sAppSettings.displaySx, sAppSettings.displaySy, true);
		sQtEncoder.Initialize("C:/critwave.mov", QtVideoCodec_H264, QtVideoQuality_High, &sQtImage);
#endif

		gGameView = new GameView_Win32();
		gGameView->Initialize();

		if (sAppSettings.controllerType != ControllerType_Undefined)
		{
			g_GameState->m_GameSettings->Load();
			g_GameState->m_GameSettings->SetControllerType(sAppSettings.controllerType);
			g_GameState->m_GameSettings->Save();
		}
		
		gGameView->Run();
		
		gGameView->Shutdown();
		delete gGameView;
		gGameView = 0;
		
#if SAVE_QUICKTIME
		sQtEncoder.Shutdown();
#endif

		DBG_PrintAllocState();

		return 0;
	}
	catch (std::exception& e)
	{
#ifdef WIN32
		MessageBoxA(0, String::Format("error: %s", e.what()).c_str(), "error", MB_OK);
#endif

		printf("error: %s\n", e.what());

		return -1;
	}
}

class ViewListener : public EventHandler
{
public:
	float mMouseX;
	float mMouseY;
	float mMoveTime;
	bool mJoyOverride;
	Vec2F mJoyDir;
	bool mStop;

	ViewListener()
	{
		mMouseX = 0;
		mMouseY = 0;
		mJoyOverride = false;
		mStop = false;
	}
	
	void TranslateMousePos(Vec2F& out_PosV, Vec2F& out_PosW)
	{
		out_PosV = Vec2F(mMouseX, mMouseY);
		out_PosW = g_GameState->m_Camera->ViewToWorld(out_PosV);
	}
	
	virtual bool OnEvent(Event& event)
	{
		switch (event.type)
		{
		case EVT_MOUSEBUTTON:
			{
				mMoveTime = g_TimerRT.Time_get();

				switch (event.mouse_button.button)
				{
				case 0:
					if (event.mouse_button.state)
					{
						//mMouseX = event.mouse_button.x;
						//mMouseY = event.mouse_button.y;

						Vec2F posV;
						Vec2F posW;
						TranslateMousePos(posV, posW);
						gGameView->m_TouchMgr->Mouse_Begin(posV, posW, posW);
					}
					else
					{
						gGameView->m_TouchMgr->Mouse_End();
					}

					return true;

				case 1:
					if (event.mouse_button.state)
					{
						EventManager::I().AddEvent(Event(EVT_MENU_BACK));
						EventManager::I().AddEvent(Event(EVT_PAUSE));
						return true;
					}
					break;

				default:
					return false;
				}
				break;
			}
		case EVT_MOUSEMOVE:
			{
				mMoveTime = g_TimerRT.Time_get();

				if (event.mouse_move.axis == 0)
					mMouseX += event.mouse_move.position / (float)sAppSettings.displayScale;
				if (event.mouse_move.axis == 1)
					mMouseY += event.mouse_move.position / (float)sAppSettings.displayScale;
				
				if (mMouseX < 0.0f)
					mMouseX = 0.0f;
				if (mMouseY < 0.0f)
					mMouseY = 0.0f;
				if (mMouseX > VIEW_SX - 1.0f)
					mMouseX = VIEW_SX - 1.0f;
				if (mMouseY > VIEW_SY - 1.0f)
					mMouseY = VIEW_SY - 1.0f;

				Vec2F posV;
				Vec2F posW;
				TranslateMousePos(posV, posW);
				gGameView->m_TouchMgr->Mouse_Move(posV, posW, posW);

				return true;
			}
			break;

		case EVT_JOYMOVE_ABS:
			{			
				const float scale = 32700.0f;

				if (event.joy_move.axis == g_GameState->m_GameSettings->m_Axis_MoveX)
				{
					mJoyDir[0] = event.joy_move.value / scale;
				}
				if (event.joy_move.axis == g_GameState->m_GameSettings->m_Axis_MoveY)
				{
					mJoyDir[1] = event.joy_move.value / scale;
				}

				if (mJoyOverride)
				{
					return true;
				}
			}
			break;

		case EVT_JOYBUTTON:
			{
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_NavigateL)
				{
					if (event.joy_button.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_LEFT));
					return true;
				}
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_NavigateR)
				{
					if (event.joy_button.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_RIGHT));
					return true;
				}
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_NavigateU)
				{
					if (event.joy_button.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_UP));
					return true;
				}
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_NavigateD)
				{
					if (event.joy_button.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_DOWN));
					return true;
				}
				
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_MenuButtonSelect)
				{
					if (event.joy_button.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_SELECT));
					return true;
				}
				
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_Dismiss)
				{
					if (event.joy_button.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_BACK));
					return true;
				}
				
				if (event.joy_button.button == g_GameState->m_GameSettings->m_Button_Select)
				{
					if (event.joy_button.state == 1)
						EventManager::I().AddEvent(Event(EVT_PAUSE));
					return true;
				}
			}
			break;

		case EVT_KEY:
			{
				if (event.key.key == IK_ESCAPE)
				{
					mStop = true;
					return true;
				}

				if (event.key.key == IK_BACKSPACE)
				{
					if (event.key.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_BACK));
					return true;
				}

				if (event.key.key == IK_ENTER)
				{
					if (event.key.state == 1)
						EventManager::I().AddEvent(Event(EVT_MENU_SELECT));
					return true;
				}

				if (event.key.key == IK_p)
				{
					if (event.key.state == 1)
						EventManager::I().AddEvent(Event(EVT_PAUSE));
					return true;
				}
				
#if defined(MACOS)
				if (event.key.key >= IK_1 && event.key.key <= IK_9)
				{
					if (event.key.state == 1)
					{
						int index = event.key.key - IK_1;
						int res = -1;
						if (g_GameState->m_GameRound->GameModeIsIntroScreen())
						{
#if 1
							if (index >= 0 && index <= 3)
							{
								sAudioManager->AudioLayerIsActive_set(index, !sAudioManager->AudioLayerIsActive_get(index));
								return true;
							}
#endif
							const int resList[] =
							{
								Resources::BGM_MAIN1,
								Resources::BGM_MAIN2,
								Resources::BGM_MAIN3,
								Resources::BGM_MAIN4,
								Resources::BGM_MAIN5
							};
							if (index >= 0 && index < int(sizeof(resList) / sizeof(int)))
								res = resList[index];
						}
						else
						{
							const int resList[] =
							{
								Resources::BGM_GAME1,
								Resources::BGM_GAME2,
								Resources::BGM_GAME3,
								Resources::BGM_GAME4,
								Resources::BGM_GAME5,
								Resources::BGM_GAME6,
								Resources::BGM_GAME7
							};
							if (index >= 0 && index < int(sizeof(resList) / sizeof(int)))
								res = resList[index];
						}
						if (res >= 0)
						{
							g_GameState->PlayMusic(g_GameState->m_ResMgr.Get(res));
						}
					}
					return true;
				}
#endif
			}
			
		default:
			break;
		}

		return g_GameState->m_MenuMgr->HandleEvent(event);
		//return false;
	}

	void Update(float dt)
	{
		float speed = 600.0f;

		if (mJoyOverride)
		{
			if (mJoyDir.Length_get() >= 0.3f)
			{
				//int clamp = 20000;
				// fixme: use float values..
				int value1 = (int)(mJoyDir[0] * speed * dt);
				int value2 = (int)(mJoyDir[1] * speed * dt);
				EventManager::I().AddEvent(Event(EVT_MOUSEMOVE, INPUT_AXIS_X, value1));
				EventManager::I().AddEvent(Event(EVT_MOUSEMOVE, INPUT_AXIS_Y, value2));
			}
		}
	}
};

static ViewListener g_ViewListener;

class VideoFrame
{
public:
	VideoFrame(int sx, int sy)
	{
		mSx = sx;
		mSy = sy;
		mByteCount = sx * sy * 4;
		mBytes = new char[mByteCount];
	}

	~VideoFrame()
	{
		delete[] mBytes;
	}

	int mSx;
	int mSy;
	char* mBytes;
	int mByteCount;
};

class VideoWriter
{
public:
	VideoWriter(const std::string& fileName)
	{
		mThread = 0;
		mMutex = SDL_CreateMutex();
		mStop = false;

		mFileName = fileName;

		Start();
	}

	~VideoWriter()
	{
		Stop();

		SDL_DestroyMutex(mMutex);
	}

	void Write(VideoFrame* frame, bool takeOwnership)
	{
		if (!takeOwnership)
			throw ExceptionVA("ownership must be true");

		Enqueue(frame);
	}

private:
	void Start()
	{
		mStream.Open(mFileName.c_str(), OpenMode_Write);

		mThread = SDL_CreateThread(Execute, this);
	}

	void Stop()
	{
		mStop = true;

		SDL_WaitThread(mThread, 0);

		while (mQueue.size() > 0)
		{
			VideoFrame* frame = Dequeue();

			Process(frame);
		}

		mStream.Close();

#if SPLIT_BIN
		mStream.Open(mFileName.c_str(), OpenMode_Read);

		Split();

		mStream.Close();
#endif
	}

	void Split()
	{
		LOG(LogLevel_Debug, "splitting bin to TGA files");

		StreamReader reader(&mStream, false);

		int index = 0;

		while (!reader.EOF_get())
		{
			int sx = reader.ReadInt32();
			int sy = reader.ReadInt32();

			int byteCount = sx * sy * 4;

			char* bytes = new char[byteCount];

			mStream.Read(bytes, byteCount);

			std::string fileName = String::Format("capture/%07d.tga", index);

			SaveTGA(fileName.c_str(), bytes, sx, sy);

			delete[] bytes;

			index++;
		}
	}

	static int Execute(void* obj)
	{
		VideoWriter* self = (VideoWriter*)obj;

		self->Execute();

		return 0;
	}

	void Execute()
	{
		while (!mStop)
		{
			VideoFrame* frame = Dequeue();

			if (frame)
			{
				Process(frame);
			}

			SDL_Delay(1);
		}
	}

	void Process(VideoFrame* frame)
	{
#if USE_VIDEO_BIN
		StreamWriter writer(&mStream, false);

		writer.WriteInt32(frame->mSx);
		writer.WriteInt32(frame->mSy);

		mStream.Write(frame->mBytes, frame->mByteCount);
#else
		static int index = 0;

		std::string fileName = String::Format("capture/%07d.tga", index);

		{
			Benchmark bm(fileName.c_str());

			SaveTGA(fileName.c_str(), frame->mBytes, frame->mSx, frame->mSy);
		}

		index++;
#endif

		delete frame;
	}

	void Enqueue(VideoFrame* frame)
	{
		SDL_LockMutex(mMutex);

		mQueue.push_back(frame);

		SDL_UnlockMutex(mMutex);
	}

	VideoFrame* Dequeue()
	{
		VideoFrame* result;

		SDL_LockMutex(mMutex);

		if (mQueue.size() == 0)
			result = 0;
		else
		{
			result = mQueue.front();

			mQueue.pop_front();
		}

		SDL_UnlockMutex(mMutex);

		return result;
	}

	SDL_Thread* mThread;
	SDL_mutex* mMutex;
	bool mStop;

	std::string mFileName;
	FileStream mStream;

	std::deque<VideoFrame*> mQueue;
};

#if SAVE_VIDEO
static VideoWriter* sVideoWriter = 0;
#endif

GameView_Win32::GameView_Win32()
{
	m_TouchMgr = 0;
	m_Display = 0;
	m_OpenALState = 0;
	
	m_GameState = 0;
	m_LogicTimer = 0;
}

GameView_Win32::~GameView_Win32()
{
}

void GameView_Win32::Initialize()
{
	UsingBegin (Benchmark gbm("GameView init"))
	{
		SDL_Init(SDL_INIT_VIDEO);
		
		gGameView = this;
		
		m_Log = LogCtx("GameView");

		// Create touch manager
		
		m_TouchMgr = new TouchMgr_Win32();
		
		UsingBegin (Benchmark bm("DisplaySDL"))
		{
			// Initialize SDL display & OpenGL
			
			m_Display = new DisplaySDL(0, 0, sAppSettings.displaySx, sAppSettings.displaySy, sAppSettings.fullscreen, true);
		}
		UsingEnd()

#if SAVE_VIDEO
		sVideoWriter = new VideoWriter("capture/video.bin");
#endif
		
		EventManager::I().AddEventHandler(&g_ViewListener, EVENT_PRIO_INTERFACE);
		EventManager::I().Enable(EVENT_PRIO_INTERFACE);
		
		// todo: try to redraw screen. does it work? can we use it to implement a simple loading screen?
		
		UsingBegin (Benchmark bm("OpenAL"))
		{
			// Initialize OpenAL
		
			m_OpenALState = new OpenALState();
		
			if (!m_OpenALState->Initialize())
			{
				LOG(LogLevel_Error, "Failed to initialize OpenAL");
			}
		}
		UsingEnd()
		
		UsingBegin (Benchmark bm("GameState"))
		{
			// Create game state
		
			m_GameState = new Application();
			m_GameState->Setup(m_OpenALState, (float)sAppSettings.displayScale);
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
		m_LogicTimer->Setup(new Timer(), true, LOGIC_FPS, 3);
		
		// Begin main loop
		
		m_Log.WriteLine(LogLevel_Debug, "Starting main loop");
		
		m_LogicTimer->Start();
	}
	UsingEnd();
}

void GameView_Win32::Shutdown()
{
	m_Log.WriteLine(LogLevel_Info, "Shutdown");
	
	// Free objects (in reverse order of creation)
	
	delete m_LogicTimer;
	m_LogicTimer = 0;
	
	delete m_GameState;
	m_GameState = 0;

	m_OpenALState->Shutdown();
	delete m_OpenALState;
	m_OpenALState = 0;
	
	EventManager::I().RemoveEventHandler(&g_ViewListener, EVENT_PRIO_INTERFACE);

#if SAVE_VIDEO
	delete sVideoWriter;
	sVideoWriter = 0;
#endif

	delete m_Display;
	m_Display = 0;
	
	delete m_TouchMgr;
	m_TouchMgr = 0;
}

Vec2F GameView_Win32::ScreenToWorld(Vec2F location)
{
	return m_GameState->m_Camera->ViewToWorld(location);
}

void GameView_Win32::Run()
{
	m_Log.WriteLine(LogLevel_Info, "Run");
	
	float step = 1.0f / (LOGIC_FPS + 2.0f);

	float next = g_TimerRT.Time_get();

	while (!g_ViewListener.mStop)
	{
#if 1
		while (g_TimerRT.Time_get() < next)
		{
			SDL_Delay(1);
		}
#endif

		next = g_TimerRT.Time_get() + step;

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
}

void GameView_Win32::Update()
{
	// update input

	m_Display->Update();

	EventManager::I().Purge();

	// update loop w/ time recovery
	
	//bool update = m_LogicTimer->BeginUpdate()
	bool update = true;

	if (update)
	{
#if 0
		while (m_LogicTimer->Tick())
		{
			m_GameState->Update();
		}

		m_LogicTimer->EndUpdate();
#else
		m_GameState->Update(1.0f);
#endif

		g_ViewListener.Update(1.0f / LOGIC_FPS);
	}
	
	//TestAudioManager();
}

void GameView_Win32::Render()
{
	g_PerfCount.Set_Count(PC_RENDER_FLUSH, 0);
	
	// clear surface
	
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	
	UsingBegin(PerfTimer timer(PC_RENDER_SETUP))
	{
		// prepare OpenGL transformation for landscape mode drawing with X axis right and Y axis down
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		if (!g_GameState->m_GameSettings->m_ScreenFlip)
			glOrtho(0.0f, VIEW_SX, VIEW_SY, 0.0f, -1000.0f, 1000.0f);
		else
			glOrtho(VIEW_SX, 0.0f, 0.0f, VIEW_SY, -1000.0f, 1000.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}
	UsingEnd()
	
	m_GameState->Render();
	
	OpenGLUtil::SetTexture(m_GameState->m_TextureAtlas[m_GameState->m_ActiveDataSet]->m_Texture);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	g_GameState->DrawMode_set(VectorShape::DrawMode_Texture);
	float at = 2.0f;
	float a = g_ViewListener.mMoveTime - g_TimerRT.Time_get() + at;
	if (a < 0.0f)
		a = 0.0f;
	if (a > 1.0f)
		a = 1.0f;
	g_GameState->Render(g_GameState->GetShape(Resources::MENU_CURSOR), Vec2F(g_ViewListener.mMouseX, g_ViewListener.mMouseY), 0.0f, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, a));
	g_GameState->m_SpriteGfx->Flush();

#if 1
	if (sFadeinFrame > 0)
	{
		float opacity = sFadeinFrame / float(FADEIN_FRAMES);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		TempRenderBegin(g_GameState->m_SpriteGfx);
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F(float(VIEW_SX), float(VIEW_SY)), 1.0f, 1.0f, 1.0f, opacity, g_GameState->GetTexture(Textures::COLOR_WHITE));
		g_GameState->m_SpriteGfx->Flush();
		sFadeinFrame--;
	}
#endif
	
#ifdef DEBUG
	TempRenderBegin(g_GameState->m_SpriteGfx);
	StringBuilder<32> sb;
	sb.AppendFormat("%d,%d", (int)g_ViewListener.mMouseX, (int)g_ViewListener.mMouseY);
	RenderText(Vec2F(1.0f, 1.0f), Vec2F(), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::Black, TextAlignment_Left, TextAlignment_Top, true, sb.ToString());
	RenderText(Vec2F(0.0f, 0.0f), Vec2F(), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Left, TextAlignment_Top, true, sb.ToString());
	g_GameState->m_SpriteGfx->Flush();
#endif

#if SAVE_VIDEO
	static int frame = 0;
	SaveGL(String::Format("capture/%07d.tga", frame).c_str());
	frame++;
#endif
#if SAVE_QUICKTIME
	QtSaveGL();
#endif

	UsingBegin(PerfTimer timer(PC_RENDER_PRESENT))
	{
		// present rendered output to screen
	
		SDL_GL_SwapBuffers();
	}
	UsingEnd()
}

#if INCLUDE_KEY
static int TranslateKey(int key)
{
	#define TRANSLATE(key1, key2) \
		if (key == key1) \
			return key2

	#define TRANSLATE_RANGE(begin, end, offset) \
		if (key >= begin && key <= end) \
			return offset + (key - begin)

	TRANSLATE_RANGE(SDLK_a, SDLK_z, IK_a);
	TRANSLATE_RANGE(SDLK_0, SDLK_9, IK_0);
	TRANSLATE_RANGE(SDLK_KP0, SDLK_KP9, IK_KP_0);
	TRANSLATE_RANGE(SDLK_F1, SDLK_F12, IK_F1);

	// symbols
	TRANSLATE(SDLK_PLUS, IK_PLUS);
	TRANSLATE(SDLK_MINUS, IK_MINUS);
	TRANSLATE(SDLK_ASTERISK, IK_ASTERISK);
	TRANSLATE(SDLK_SPACE, IK_SPACE);

	// special keys
	TRANSLATE(SDLK_ESCAPE, IK_ESCAPE);
	TRANSLATE(SDLK_RETURN, IK_ENTER);
	TRANSLATE(SDLK_DELETE, IK_DELETE);
	TRANSLATE(SDLK_LSHIFT, IK_SHIFTL);
	TRANSLATE(SDLK_RSHIFT, IK_SHIFTR);
	TRANSLATE(SDLK_LALT, IK_ALTL);
	TRANSLATE(SDLK_RALT, IK_ALTR);
	TRANSLATE(SDLK_LCTRL, IK_CONTROLL);
	TRANSLATE(SDLK_RCTRL, IK_CONTROLR);
	TRANSLATE(SDLK_HOME, IK_HOME);
	TRANSLATE(SDLK_END, IK_END);
	TRANSLATE(SDLK_PAGEUP, IK_PAGEUP);
	TRANSLATE(SDLK_PAGEDOWN, IK_PAGEDOWN);
	TRANSLATE(SDLK_UP, IK_UP);
	TRANSLATE(SDLK_DOWN, IK_DOWN);
	TRANSLATE(SDLK_LEFT, IK_LEFT);
	TRANSLATE(SDLK_RIGHT, IK_RIGHT);

	#undef TRANSLATE
	#undef TRANSLATE_RANGE

	return 0;
}
#endif

static void SaveTGA(const char* fileName, char* bytes, int sx, int sy)
{
	FileStream stream;

	stream.Open(fileName, OpenMode_Write);

	StreamWriter writer(&stream, false);

	writer.WriteInt8(0); // id length
	writer.WriteInt8(0); // colormap type
	writer.WriteUInt8(2); // image type
	writer.WriteInt16(0); // first color map entry
	writer.WriteInt16(0); // color map length
	writer.WriteInt8(0); // color map size
	writer.WriteInt16(0); // origin x
	writer.WriteInt16(0); // origin y
	writer.WriteInt16(sx); // sx
	writer.WriteInt16(sy); // sy
	writer.WriteUInt8(32); // bpp
	writer.WriteInt8(0); // image descriptor

	const int n = sx * sy;

	char* ptr = bytes;

	for (int i = 0; i < n; ++i)
	{
		const char temp = ptr[0];
		ptr[0] = ptr[2];
		ptr[2] = temp;
		ptr += 4;
	}

	stream.Write(bytes, n * 4);
}

#if SAVE_VIDEO
static void SaveGL(const char* fileName)
{
	const int sx = sAppSettings.displaySx;
	const int sy = sAppSettings.displaySy;

	VideoFrame* frame = new VideoFrame(sx, sy);
	
	glReadPixels(0, 0, sx, sy, GL_RGBA, GL_UNSIGNED_BYTE, frame->mBytes);

	sVideoWriter->Write(frame, true);
}
#endif

#if SAVE_QUICKTIME
static void QtSaveGL()
{
	const int sx = sAppSettings.displaySx;
	const int sy = sAppSettings.displaySy;
	
	glReadPixels(0, 0, sx, sy, GL_RGBA, GL_UNSIGNED_BYTE, sQtImage.Data_get());

	sQtEncoder.CommitVideoFrame(1000/60);
}
#endif
