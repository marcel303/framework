/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#define NOMINMAX

#include <cmath>
#include <limits>
#include <map>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <vector>

#ifdef WIN32
	#include <direct.h>
	#include <GL/glew.h>
	#include <SDL2/SDL_opengl.h>
	#include <SDL2/SDL_syswm.h>
	#include <Windows.h>
	#include <Xinput.h>
	DWORD timeGetTime(void);
#else
	#include <dirent.h>
	#include <unistd.h>
	#define _chdir chdir
#endif

#include "audio.h"
#include "data/engine/ShaderCommon.txt"
#include "framework.h"
#include "image.h"
#include "internal.h"
#include "model.h"
#include "rte.h"
#include "shaders.h"
#include "spriter.h"

#if USE_GLYPH_ATLAS
	#include "textureatlas.h"
#endif

#include "StringEx.h"
#include "Timer.h"

// -----

#if ENABLE_UTF8_SUPPORT
#include "utf8rewind.h"
#endif

#if defined(MACOS)
    #define INDEX_TYPE GL_UNSIGNED_INT
#else
    #define INDEX_TYPE GL_UNSIGNED_SHORT
#endif

#define MAX_TEXT_LENGTH 2048

#if INDEX_TYPE == GL_UNSIGNED_INT
typedef unsigned int glindex_t;
#else
typedef unsigned short glindex_t;
#endif

extern bool initMidi(int deviceIndex);
extern void shutMidi();

// MIDI processing lock. required since MIDI events arrive asynchronously
extern void lockMidi();
extern void unlockMidi();

#if ENABLE_OPENGL && !USE_LEGACY_OPENGL
static void gxFlush(bool endOfBatch);
#endif

static float scale255(const float v)
{
	static const float m = 1.f / 255.f;
	return v * m;
}

// -----

Color colorBlackTranslucent(0, 0, 0, 0);
Color colorBlack(0, 0, 0, 255);
Color colorWhite(255, 255, 255, 255);
Color colorRed(255, 0, 0, 255);
Color colorGreen(0, 255, 0, 255);
Color colorBlue(0, 0, 255, 255);
Color colorYellow(255, 255, 0, 255);

//

Framework framework;
Dictionary settings;
Mouse mouse;
Keyboard keyboard;
Gamepad gamepad[MAX_GAMEPAD];
Midi midi;
Stage stage;
Ui ui;

// -----

Framework::Framework()
{
	waitForEvents = false;
	fullscreen = false;
	exclusiveFullscreen = true;
	useClosestDisplayMode = false;
	basicOpenGL = false;
	enableDepthBuffer = false;
	enableDrawTiming = true;
	enableProfiling = false;
	minification = 1;
	enableMidi = false;
	midiDeviceIndex = 0;
	reloadCachesOnActivate = false;
	cacheResourceData = false;
	enableRealTimeEditing = false;
	filedrop = false;
	windowX = -1;
	windowY = -1;
	windowBorder = true;
	windowTitle.clear();
	windowSx = 0;
	windowSy = 0;
	windowIsActive = false;
	numSoundSources = 32;
	actionHandler = 0;
	fillCachesCallback = 0;
	fillCachesUnknownResourceCallback = 0;
	realTimeEditCallback = 0;
	initErrorHandler = 0;
	
	quitRequested = false;
	time = 0.f;
	timeStep = 1.f / 60.f;

	m_sprites = 0;
}

Framework::~Framework()
{
}

bool Framework::init(int argc, const char * argv[], int sx, int sy)
{
#ifdef WIN32
	SetProcessDPIAware();
#endif

	// initialize SDL
	
	const int initFlags = SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_VIDEO;
	
	if (SDL_Init(initFlags) < 0)
	{
		logError("failed to initialize SDL: %s", SDL_GetError());
		if (initErrorHandler)
			initErrorHandler(INIT_ERROR_SDL);
		return false;
	}

	if (filedrop)
	{
		SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	}
	
	int flags = 0;

#if ENABLE_OPENGL
#if USE_LEGACY_OPENGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
#if OPENGL_VERSION == 430
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#endif
#if OPENGL_VERSION == 410
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#endif
#endif
	
#if 1
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
#endif
	
	if (enableDepthBuffer)
	{
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	}
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	
	// todo: ensure VSYNC is enabled
	
	flags |= SDL_WINDOW_OPENGL;
#endif
	
	if (fullscreen && minification == 1)
	{
		flags |= SDL_WINDOW_FULLSCREEN;
		
	#if ENABLE_OPENGL
		SDL_GL_SetSwapInterval(1);
	#endif
	}

	int actualSx = sx / minification;
	int actualSy = sy / minification;

	bool foundMode = false;

#ifndef DEBUG
	if (false)
	{
		SDL_DisplayMode desired;
		foundMode = SDL_GetCurrentDisplayMode(0, &desired) == 0;
		actualSx = desired.w;
		actualSy = desired.h;
		sx = actualSx;
		sy = actualSy;
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	else
#endif
	if (fullscreen && useClosestDisplayMode)
	{
		int start = exclusiveFullscreen ? 0 : 1;
		int end = exclusiveFullscreen ? 2 : 1;

		for (int i = start; i <= end; ++i)
		{
			if (i == 2)
			{
				actualSx /= 2;
				actualSy /= 2;
			}

			SDL_DisplayMode desired;
			SDL_DisplayMode closest;

			desired.w = actualSx;
			desired.h = actualSy;
			desired.format = SDL_PIXELFORMAT_UNKNOWN;
			desired.refresh_rate = 60;
			desired.driverdata = 0;

			if (i == 0)
				foundMode = SDL_GetClosestDisplayMode(0, &desired, &closest) != NULL;
			else if (i == 1)
				foundMode = SDL_GetCurrentDisplayMode(0, &closest) == 0;
			else if (i == 2)
				foundMode = SDL_GetClosestDisplayMode(0, &desired, &closest) != NULL;

			if (foundMode)
			{
				logInfo("found suitable display mode: %d x %d @ %d Hz", closest.w, closest.h, closest.refresh_rate);
				actualSx = closest.w;
				actualSy = closest.h;
				if (i == 1)
					flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
				break;
			}
		}

		if (!foundMode)
		{
			logError("unable to find suitable display mode");
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_VIDEO_MODE);
			return false;
		}
	}

	if (!windowBorder)
		flags |= SDL_WINDOW_BORDERLESS;
	
	// todo : enable flag by default. make automatic DPI upscaling optional
	//flags |= SDL_WINDOW_ALLOW_HIGHDPI;

	globals.window = SDL_CreateWindow(
		windowTitle.c_str(),
		windowX == -1 ? SDL_WINDOWPOS_CENTERED : windowX,
		windowY == -1 ? SDL_WINDOWPOS_CENTERED : windowY,
		actualSx,
		actualSy,
		flags);

	if (!globals.window)
	{
		logError("failed to set video mode (%dx%d @ %dbpp): %s", sx / minification, sy / minification, 32, SDL_GetError());
		if (initErrorHandler)
			initErrorHandler(INIT_ERROR_WINDOW);
		return false;
	}
	
	windowSx = sx;
	windowSy = sy;
	
#if ENABLE_OPENGL
	globals.glContext = SDL_GL_CreateContext(globals.window);
	checkErrorGL();
	
	if (!globals.glContext)
	{
		logError("failed to create OpenGL context: %s", SDL_GetError());
		if (initErrorHandler)
			initErrorHandler(INIT_ERROR_OPENGL);
		return false;
	}
	
	if (!basicOpenGL)
	{
		glewExperimental = GL_TRUE; // force GLEW to resolve all supported extension methods
	
		const int glewStatus = glewInit();
		glGetError(); // fixme : GLEW generates an error code and we're not interested in trapping it..
		checkErrorGL();

		if (glewStatus != GLEW_OK)
		{
			logError("failed to initialize GLEW: %s", glewGetErrorString(glewStatus));
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_OPENGL_EXTENSIONS);
			return false;
		}

		logInfo("using OpenGL %s, %s, GLEW %s", glGetString(GL_VERSION), glGetString(GL_VENDOR), glewGetString(GLEW_VERSION));
	
		if (!GLEW_VERSION_3_2)
		{
			logWarning("OpenGL 3.2 not supported");
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_OPENGL_EXTENSIONS);
			return false;
		}
	}

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
	if (GLEW_ARB_debug_output)
	{
		logInfo("using OpenGL debug output");
		glDebugMessageCallbackARB(debugOutputGL, stderr);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	}
#endif
#endif
	
	globals.displaySize[0] = sx;
	globals.displaySize[1] = sy;

	globals.actualDisplaySize[0] = actualSx * minification;
	globals.actualDisplaySize[1] = actualSy * minification;
	
#if 0 // invalid using non-legacy mode
	glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
	checkErrorGL();
#endif

	gxInitialize();

#if ENABLE_PROFILING
	if (framework.enableProfiling)
	{
		if (rmt_CreateGlobalInstance(&globals.rmt) != RMT_ERROR_NONE)
			return false;
		rmt_BindOpenGL();
	}
#endif

	globals.builtinShaders = new BuiltinShaders();

	if (enableMidi)
	{
		if (!initMidi(midiDeviceIndex))
		{
			logWarning("MIDI intialisation failed");
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_MIDI);
			//return false;
		}
	}

	// initialize FreeType
	
	if (FT_Init_FreeType(&globals.freeType) != 0)
	{
		logError("failed to initialize FreeType");
		if (initErrorHandler)
			initErrorHandler(INIT_ERROR_FREETYPE);
		return false;
	}
	
	// initialize sound player
	
	if (!g_soundPlayer.init(numSoundSources))
	{
		logError("failed to initialize sound player");
		//if (initErrorHandler)
		//	initErrorHandler(INIT_ERROR_SOUND);
		//return false;
	}

	// initialize real time editing

	if (enableRealTimeEditing)
	{
		initRealTimeEditing();
	}

	// load settings

	settings.load("settings.txt");
	
	// initialize UI
	
	ui.load("default_ui");

	// make sure we are focused

	SDL_RaiseWindow(globals.window);

	if (!windowIcon.empty())
	{
		ImageData * iconData = loadImage(windowIcon.c_str());
		SDL_Surface * surface = SDL_CreateRGBSurfaceFrom(iconData->imageData, iconData->sx, iconData->sy, 32, iconData->sx * sizeof(ImageData::Pixel), 0xff << 0, 0xff << 8, 0xff << 16, 0xff << 24);
		SDL_SetWindowIcon(globals.window, surface);
		SDL_FreeSurface(surface);
		delete iconData;
	}

	SDL_DisableScreenSaver();

	return true;
}

bool Framework::shutdown()
{
	bool result = true;
	
	// shut down real time editing

	shutRealTimeEditing();

	// shut down sound player
	
	if (!g_soundPlayer.shutdown())
	{
		logError("failed to shut down sound player");
		result = false;
	}
	
	// free resources
	
	g_textureCache.clear();
	g_shaderCache.clear();
	g_animCache.clear();
	g_spriterCache.clear();
	g_modelCache.clear();
	g_soundCache.clear();
	g_fontCache.clear();
	g_fontCacheMSDF.clear();
	g_glyphCache.clear();
	g_uiCache.clear();
	
	// shut down FreeType
	
	if (globals.freeType && FT_Done_FreeType(globals.freeType) != 0)
	{
		logError("failed to shut down FreeType");
		result = false;
	}
	globals.freeType = 0;
	
	if (enableMidi)
	{
		shutMidi();
	}

	delete globals.builtinShaders;
	globals.builtinShaders = nullptr;

#if ENABLE_PROFILING
	if (framework.enableProfiling)
	{
		rmt_UnbindOpenGL();
		rmt_DestroyGlobalInstance(globals.rmt);
		globals.rmt = 0;
	}
#endif

	gxShutdown();
	
	m_shaderSources.clear();

	glBlendEquation = 0;
	glClampColor = 0;
	
	// destroy SDL OpenGL context
	
	if (globals.glContext)
	{
		SDL_GL_DeleteContext(globals.glContext);
		globals.glContext = 0;
	}
	
	// destroy SDL window
	
	if (globals.window)
	{
		SDL_DestroyWindow(globals.window);
		globals.window = 0;
	}
	
	// shut down SDL
	
	SDL_Quit();
	
	// clear globals
	
	globals = Globals();
	
	// reset self
	
	quitRequested = false;
	time = 0.f;

	waitForEvents = false;
	fullscreen = false;
	exclusiveFullscreen = true;
	useClosestDisplayMode = false;
	basicOpenGL = false;
	enableDepthBuffer = false;
	enableDrawTiming = true;
	enableProfiling = false;
	minification = 1;
	enableMidi = false;
	midiDeviceIndex = 0;
	reloadCachesOnActivate = false;
	cacheResourceData = false;
	enableRealTimeEditing = false;
	filedrop = false;
	numSoundSources = 32;
	windowX = -1;
	windowY = -1;
	windowBorder = true;
	windowTitle.clear();
	windowSx = 0;
	windowSy = 0;
	windowIsActive = false;
	actionHandler = 0;
	fillCachesCallback = 0;
	fillCachesUnknownResourceCallback = 0;
	realTimeEditCallback = 0;
	initErrorHandler = 0;

	return result;
}

void Framework::process()
{
	cpuTimingBlock(frameworkProcess);

	static int tstamp1 = SDL_GetTicks();
	const int tstamp2 = SDL_GetTicks();
	int delta = tstamp2 - tstamp1;
	tstamp1 = tstamp2;
	//if (delta == 0)
	//	delta = 1;
	timeStep = delta / 1000.f;

	time += timeStep;
	
	if (enableRealTimeEditing)
	{
		tickRealTimeEditing();
	}

	g_soundPlayer.process();
	
	bool doReload = false;
	
	// poll SDL event queue
	
	globals.keyChangeCount = 0;
	globals.keyRepeatCount = 0;
	memset(globals.mouseChange, 0, sizeof(globals.mouseChange));
	
	keyboard.events.clear();

	lockMidi();
	{
		memcpy(globals.midiIsSet, globals.midiIsSetAsync, sizeof(globals.midiIsSet));
		memcpy(globals.midiDown, globals.midiDownAsync, sizeof(globals.midiDown));
		memcpy(globals.midiChange, globals.midiChangeAsync, sizeof(globals.midiChange));
		memset(globals.midiChangeAsync, 0, sizeof(globals.midiChangeAsync));
	}
	unlockMidi();
	
	const int oldMouseX = mouse.x;
	const int oldMouseY = mouse.y;
	
	events.clear();

	SDL_Event e;
	
	bool hasWaited = false;

	for (;;)
	{
		if (waitForEvents)
		{
			if (!hasWaited)
			{
				if (!SDL_WaitEvent(&e))
					break;
				hasWaited = true;
			}
			else
			{
				if (!SDL_PollEvent(&e))
					break;
			}
		}
		else
		{
			if (!SDL_PollEvent(&e))
				break;
		}

		events.push_back(e);
		
		if (e.type == SDL_KEYDOWN)
		{
			keyboard.events.push_back(e);
			
			bool isRepeat = false;
			for (int i = 0; i < globals.keyDownCount; ++i)
				if (globals.keyDown[i] == e.key.keysym.sym)
					isRepeat = true;
			if (isRepeat)
			{
				globals.keyRepeat[globals.keyRepeatCount++] = e.key.keysym.sym;
			}
			else
			{
				globals.keyDown[globals.keyDownCount++] = e.key.keysym.sym;
				globals.keyChange[globals.keyChangeCount++] = e.key.keysym.sym;
			}
		}
		else if (e.type == SDL_KEYUP)
		{
			keyboard.events.push_back(e);
			
			for (int i = 0; i < globals.keyDownCount; ++i)
			{
				if (globals.keyDown[i] == e.key.keysym.sym)
				{
					for (int j = i + 1; j < globals.keyDownCount; ++j)
						globals.keyDown[j - 1] = globals.keyDown[j];
					globals.keyDownCount--;
				}
			}
			globals.keyChange[globals.keyChangeCount++] = e.key.keysym.sym;
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
		{
			//logDebug("mouse %d / %d", e.type, e.button.button);

			const int index = e.button.button == SDL_BUTTON_LEFT ? 0 : e.button.button == SDL_BUTTON_RIGHT ? 1 : -1;
			if (index >= 0)
			{
				globals.mouseDown[index] = e.button.state == SDL_PRESSED;
				globals.mouseChange[index] = true;
			}
		}
		else if (e.type == SDL_MOUSEMOTION)
		{
			mouse.x = e.motion.x * minification * globals.displaySize[0] / globals.actualDisplaySize[0];
			mouse.y = e.motion.y * minification * globals.displaySize[1] / globals.actualDisplaySize[1];
		}
		else if (e.type == SDL_WINDOWEVENT)
		{
			if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				windowIsActive = true;
			else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
				windowIsActive = false;

			if (reloadCachesOnActivate && e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				doReload |= true;
		}
		else if (e.type == SDL_DROPFILE)
		{
			Dictionary args;
			args.setString("file", e.drop.file);
			processAction("filedrop", args);
		}
		else if (e.type == SDL_QUIT)
		{
			quitRequested = true;
		}
	}

	if (globals.hasOldMousePosition)
	{
		mouse.dx = mouse.x - oldMouseX;
		mouse.dy = mouse.y - oldMouseY;
	}
	else
	{
		globals.hasOldMousePosition = true;
	}

#ifdef __WIN32__
	// use XInput to poll gamepad state
	for (int i = 0; i < MAX_GAMEPAD; ++i)
	{
		cpuTimingBlock(XInputGetState);

		// this is such a lame hack.. XInputGetState takes up a lot of time when the controller isn't connected. the
		// total amount of time spent here is usually over 2ms. slightly over 500us for each controller.. madness!
		// so instead of checking all disabled controllers.. only check one disabled controller per process call
		// ..maybe we should be checking WM_DEVICECHANGE instead.. and update the list of connected controllers
		// whenever a device is (dis)connected

		if (!gamepad[i].isConnected && i != (globals.xinputGamepadIdx % MAX_GAMEPAD))
			continue;

		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		DWORD result = XInputGetState(i, &state);

		if (result == ERROR_SUCCESS)
		{
			gamepad[i].isConnected = true;
			
			memset(gamepad[i].m_wentDown, 0, sizeof(gamepad[i].m_wentDown));
			memset(gamepad[i].m_wentUp, 0, sizeof(gamepad[i].m_wentUp));

			const XINPUT_GAMEPAD & g = state.Gamepad;
			
		#define APPLY_DEADZONE(v, t) (std::abs(v) <= t ? 0.f : clamp((std::abs(v) - t) * (v < 0.f ? -1.f : +1.f) / float(32767 - t), -1.f, +1.f))

			gamepad[i].m_analog[0][ANALOG_X] = APPLY_DEADZONE(+g.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad[i].m_analog[0][ANALOG_Y] = APPLY_DEADZONE(-g.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad[i].m_analog[1][ANALOG_X] = APPLY_DEADZONE(+g.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			gamepad[i].m_analog[1][ANALOG_Y] = APPLY_DEADZONE(-g.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			
		#undef APPLY_DEADZONE
			
			const int buttons = g.wButtons;

			bool * isDown = gamepad[i].m_isDown;

			bool wasDown[GAMEPAD_MAX];
			memcpy(wasDown, isDown, sizeof(wasDown));
			
			isDown[DPAD_LEFT]     = 0 != (buttons & XINPUT_GAMEPAD_DPAD_LEFT);
			isDown[DPAD_RIGHT]    = 0 != (buttons & XINPUT_GAMEPAD_DPAD_RIGHT);
			isDown[DPAD_UP]       = 0 != (buttons & XINPUT_GAMEPAD_DPAD_UP);
			isDown[DPAD_DOWN]     = 0 != (buttons & XINPUT_GAMEPAD_DPAD_DOWN);
			isDown[GAMEPAD_A]     = 0 != (buttons & XINPUT_GAMEPAD_A);
			isDown[GAMEPAD_B]     = 0 != (buttons & XINPUT_GAMEPAD_B);
			isDown[GAMEPAD_X]     = 0 != (buttons & XINPUT_GAMEPAD_X);
			isDown[GAMEPAD_Y]     = 0 != (buttons & XINPUT_GAMEPAD_Y);
			isDown[GAMEPAD_L1]    = 0 != (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			isDown[GAMEPAD_R1]    = 0 != (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
			isDown[GAMEPAD_L2]    = g.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
			isDown[GAMEPAD_R2]    = g.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
			isDown[GAMEPAD_START] = 0 != (buttons & XINPUT_GAMEPAD_START);
			isDown[GAMEPAD_BACK]  = 0 != (buttons & XINPUT_GAMEPAD_BACK);

			bool * wentDown = gamepad[i].m_wentDown;
			bool * wentUp = gamepad[i].m_wentUp;

			for (int j = 0; j < GAMEPAD_MAX; ++j)
			{
				if (!wasDown[j] && isDown[j])
					wentDown[j] = true;
				if (wasDown[j] && !isDown[j])
					wentUp[j] = true;
			}

			// update vibration

			if (gamepad[i].m_vibrationDuration > 0.f)
			{
				gamepad[i].m_vibrationDuration -= timeStep;
				if (gamepad[i].m_vibrationDuration <= 0.f)
				{
					gamepad[i].m_vibrationDuration = 0.f;
					gamepad[i].m_vibrationStrength = 0.f;
				}
			}

			if (gamepad[i].m_vibrationStrength != gamepad[i].m_lastVibrationStrength)
			{
				gamepad[i].m_lastVibrationStrength = gamepad[i].m_vibrationStrength;

				XINPUT_VIBRATION v;
				v.wLeftMotorSpeed = 65535 * gamepad[i].m_vibrationStrength;
				v.wRightMotorSpeed = 65535 * gamepad[i].m_vibrationStrength;
				XInputSetState(i, &v);
			}
		}
		else
		{
			memset(&gamepad[i], 0, sizeof(Gamepad));
		}
	}

	globals.xinputGamepadIdx++;
#endif

	if (doReload)
	{
		reloadCaches();
	}

	tickRealTimeEditing();

	for (Sprite * sprite = m_sprites; sprite; sprite = sprite->m_next)
	{
		sprite->updateAnimation(timeStep);
	}
	
	for (ModelSet::iterator i = m_models.begin(); i != m_models.end(); ++i)
	{
		(*i)->updateAnimation(timeStep);
	}
}

void Framework::processAction(const std::string & action, const Dictionary & args)
{
	if (action == "sound")
	{
		Sound(args.getString("file", "").c_str()).play(args.getInt("volume", 100));
	}
	
	if (actionHandler)
	{
		actionHandler(action, args);
	}
}

void Framework::processActions(const std::string & actions, const Dictionary & args)
{
	std::vector<std::string> vec;
	splitString(actions, vec, ',');
	for (size_t i = 0; i < vec.size(); ++i)
		processAction(vec[i], args);
}

void Framework::reloadCaches()
{
	settings.load("settings.txt");

	g_textureCache.reload();
	g_shaderCache.reload();
	g_animCache.reload();
	g_spriterCache.reload();
	g_modelCache.reload();
	g_soundCache.reload();
	g_fontCache.reload();
	g_fontCacheMSDF.reload();
	g_glyphCache.clear();
	g_uiCache.reload();
	
	globals.resourceVersion++;
	
	for (Sprite * sprite = m_sprites; sprite; sprite = sprite->m_next)
	{
		sprite->updateAnimationSegment();
	}
	
	for (ModelSet::iterator i = m_models.begin(); i != m_models.end(); ++i)
	{
		(*i)->updateAnimationSegment();
	}
}

//

std::vector<std::string> listFiles(const char * path, bool recurse)
{
#ifdef WIN32
	std::vector<std::string> result;
	WIN32_FIND_DATAA ffd;
	char wildcard[MAX_PATH];
	sprintf_s(wildcard, sizeof(wildcard), "%s\\*", path);
	HANDLE find = FindFirstFileA(wildcard, &ffd);
	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			char fullPath[MAX_PATH];
			if (strcmp(path, "."))
				sprintf_s(fullPath, sizeof(fullPath), "%s/%s", path, ffd.cFileName);
			else
				strcpy_s(fullPath, sizeof(fullPath), ffd.cFileName);

			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				if (recurse && strcmp(ffd.cFileName, ".") && strcmp(ffd.cFileName, ".."))
				{
					std::vector<std::string> subResult = listFiles(fullPath, recurse);
					result.insert(result.end(), subResult.begin(), subResult.end());
				}
			}
			else
			{
				result.push_back(fullPath);
			}
		} while (FindNextFileA(find, &ffd) != 0);

		FindClose(find);
	}
	return result;
#else
	std::vector<std::string> result;
	
	std::vector<DIR*> dirs;
	{
		DIR * dir = opendir(path);
		if (dir)
			dirs.push_back(dir);
	}
	
	while (!dirs.empty())
	{
		DIR * dir = dirs.back();
		dirs.pop_back();
		
		dirent * ent;
		
		while ((ent = readdir(dir)) != 0)
		{
			char fullPath[PATH_MAX];
			if (strcmp(path, "."))
				sprintf_s(fullPath, sizeof(fullPath), "%s/%s", path, ent->d_name);
			else
				strcpy_s(fullPath, sizeof(fullPath), ent->d_name);
			
			if (ent->d_type == DT_DIR)
			{
				if (recurse && strcmp(ent->d_name, ".") && strcmp(ent->d_name, ".."))
				{
					std::vector<std::string> subResult = listFiles(fullPath, recurse);
					result.insert(result.end(), subResult.begin(), subResult.end());
				}
			}
			else
			{
				result.push_back(fullPath);
			}
		}
		
		closedir(dir);
	}
	return result;
#endif
}

void showErrorMessage(const char * caption, const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, globals.window);
}

void Framework::fillCachesWithPath(const char * path, bool recurse)
{
	std::vector<std::string> files = listFiles(path, recurse);

	uint64_t lastProcessTime = g_TimerRT.TimeMS_get();

	for (size_t i = 0; i < files.size(); ++i)
	{
		const std::string & fs = files[i];
		auto ep = fs.rfind('.');
		if (ep == std::string::npos)
			continue;
		std::string e = fs.substr(ep + 1);
		std::transform(e.begin(), e.end(), e.begin(), ::tolower);
		const char * f = fs.c_str();
		const size_t fl = strlen(f);
		if (e == "png" || e == "bmp")
			Sprite s(f);
		else if (e == "scml" && !strstr(f, "autosave"))
			g_spriterCache.findOrCreate(f);
		else if (e == "wav")
			g_soundCache.findOrCreate(f);
		else if (e == "ogg")
		{
			FILE * file;
			if (fopen_s(&file, f, "rb") == 0)
			{
				fseek(file, 0, SEEK_END);
				const int size = ftell(file);
				fclose(file);
				if (size <= 512*1024)
					g_soundCache.findOrCreate(f);
			}
		}
		else if (e == "ttf")
		{
			g_fontCache.findOrCreate(f);
			g_fontCacheMSDF.findOrCreate(f);
		}
		else if (e == "txt")
		{
			FileReader r;
			std::string line;
			if (r.open(f, true) && r.read(line) && strstr(line.c_str(), "#ui"))
				g_uiCache.findOrCreate(f);
		}
		else if (strstr(f, ".vs") == f + fl - 3 || strstr(f, ".ps") == f + fl - 3)
		{
			std::string name = f;
			name = name.substr(0, name.rfind('.'));
			Shader(name.c_str());
		}
		else if (strstr(f, ".cs") == f + fl - 3)
		{
			std::string name = f;
			name = name.substr(0, name.rfind('.'));
			ComputeShader(name.c_str());
		}
		else
		{
			if (fillCachesUnknownResourceCallback)
				fillCachesUnknownResourceCallback(f);
		}

		const uint64_t currentTime = g_TimerRT.TimeMS_get();
		if (currentTime - lastProcessTime >= 20)
		{
			lastProcessTime = currentTime;
			if (fillCachesCallback)
				fillCachesCallback((i + 1.f) / files.size());
			else
				process();
		}
	}

	if (fillCachesCallback)
		fillCachesCallback(1.f);
}

void Framework::setFullscreen(bool fullscreen)
{
	if (fullscreen)
		SDL_SetWindowFullscreen(globals.window, SDL_WINDOW_FULLSCREEN);
	else
		SDL_SetWindowFullscreen(globals.window, 0);
}

void Framework::beginDraw(int r, int g, int b, int a)
{
#if ENABLE_OPENGL
	if (enableDrawTiming)
		gpuTimingBegin(frameworkDraw);

	// clear back buffer
	
	glClearColor(scale255(r), scale255(g), scale255(b), scale255(a));
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// initialize viewport and OpenGL matrices
	
	int windowSx;
	int windowSy;
	SDL_GL_GetDrawableSize(globals.window, &windowSx, &windowSy);
	globals.drawableOffset[0] = 0;
	globals.drawableOffset[1] = windowSy - (globals.actualDisplaySize[1] / minification);
	
	glViewport(
		globals.drawableOffset[0],
		globals.drawableOffset[1],
		globals.actualDisplaySize[0] / minification,
		globals.actualDisplaySize[1] / minification);
	
	applyTransform();
	setBlend(BLEND_ALPHA);
#endif
}

void Framework::endDraw()
{
#if ENABLE_OPENGL
	// process debug draw

	setTransform(TRANSFORM_SCREEN);
	setBlend(BLEND_ALPHA);
	
	for (int i = 0; i < globals.debugDraw.numLines; ++i)
	{
		globals.font = globals.debugDraw.lines[i].font;
		setColor(globals.debugDraw.lines[i].color);
		
		drawText(
			globals.debugDraw.lines[i].x,
			globals.debugDraw.lines[i].y,
			globals.debugDraw.lines[i].size,
			globals.debugDraw.lines[i].alignX,
			globals.debugDraw.lines[i].alignY,
			globals.debugDraw.lines[i].text);
	}
	
	globals.debugDraw.numLines = 0;
	
	if (enableDrawTiming)
		gpuTimingEnd();

	// check for errors
	
	checkErrorGL();
	
	// flip back buffers
	
	SDL_GL_SwapWindow(globals.window);
#else
	
#endif
}

void Framework::registerShaderSource(const char * name, const char * text)
{
	m_shaderSources[name] = text;
}

void Framework::unregisterShaderSource(const char * name)
{
	auto i = m_shaderSources.find(name);

	fassert(i != m_shaderSources.end());
	if (i != m_shaderSources.end())
		m_shaderSources.erase(i);
}

bool Framework::tryGetShaderSource(const char * name, const char *& text) const
{
	auto i = m_shaderSources.find(name);

	if (i != m_shaderSources.end())
	{
		text = i->second.c_str();
		return true;
	}
	else
	{
		return false;
	}
}

void Framework::blinkTaskbarIcon(int count)
{
#ifdef WIN32
	SDL_SysWMinfo info;
	memset(&info, 0, sizeof(info));
	SDL_VERSION(&info.version);

	if (SDL_GetWindowWMInfo(globals.window, &info))
	{
		FLASHWINFO flashInfo;
		memset(&flashInfo, 0, sizeof(flashInfo));
		flashInfo.cbSize = sizeof(flashInfo);
		flashInfo.hwnd = info.info.win.window;
		flashInfo.dwFlags = FLASHW_TRAY;
		flashInfo.uCount = count;
		FlashWindowEx(&flashInfo);
	}
#endif
}

void Framework::registerSprite(Sprite * sprite)
{
	fassert(sprite->m_prev == 0);
	fassert(sprite->m_next == 0);

	if (m_sprites)
	{
		m_sprites->m_prev = sprite;
		sprite->m_next = m_sprites;
	}

	m_sprites = sprite;
}

void Framework::unregisterSprite(Sprite * sprite)
{
	if (sprite == m_sprites)
		m_sprites = sprite->m_next;

	if (sprite->m_prev)
		sprite->m_prev->m_next = sprite->m_next;
	if (sprite->m_next)
		sprite->m_next->m_prev = sprite->m_prev;

	sprite->m_prev = 0;
	sprite->m_next = 0;
}

void Framework::registerModel(Model * model)
{
	m_models.insert(model);
}

void Framework::unregisterModel(Model * model)
{
	m_models.erase(m_models.find(model));
}

// -----

void Surface::construct()
{
	m_size[0] = 0;
	m_size[1] = 0;
	
	m_bufferId = 0;
	
	m_doubleBuffered = false;

	m_buffer[0] = 0;
	m_buffer[1] = 0;
	m_texture[0] = 0;
	m_texture[1] = 0;
	m_depthTexture = 0;
}

void Surface::destruct()
{
	m_size[0] = 0;
	m_size[1] = 0;
	
	m_bufferId = 0;
	
	for (int i = 0; i < (m_doubleBuffered ? 2 : 1); ++i)
	{
		if (m_buffer[i])
		{
			glDeleteFramebuffers(1, &m_buffer[i]);
			m_buffer[i] = 0;
			checkErrorGL();
		}
		
		if (m_texture[i])
		{
			glDeleteTextures(1, &m_texture[i]);
			m_texture[i] = 0;
			checkErrorGL();
		}
	}

	if (m_depthTexture)
	{
		glDeleteTextures(1, &m_depthTexture);
		m_depthTexture = 0;
		checkErrorGL();
	}

	m_doubleBuffered = false;
}

void Surface::swapBuffers()
{
	fassert(m_doubleBuffered);

	m_bufferId = (m_bufferId + 1) % 2;
}

Surface::Surface()
{
	construct();
}

Surface::Surface(int sx, int sy, bool highPrecision, bool withDepthBuffer, bool doubleBuffered)
{
	construct();
	
	init(sx, sy, highPrecision ? SURFACE_RGBA16F : SURFACE_RGBA8, withDepthBuffer, doubleBuffered);
}

Surface::Surface(int sx, int sy, bool withDepthBuffer, bool doubleBuffered, SURFACE_FORMAT format)
{
	construct();

	init(sx, sy, format, withDepthBuffer, doubleBuffered);
}

Surface::~Surface()
{
	destruct();
}

bool Surface::init(int sx, int sy, SURFACE_FORMAT format, bool withDepthBuffer, bool doubleBuffered)
{
	fassert(m_buffer[0] == 0);
	
	sx /= framework.minification;
	sy /= framework.minification;
	
	GLuint oldBuffer = 0;
	GLuint oldTexture = 0;
	
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&oldTexture);
	
	//
	
	bool result = true;
	
	m_size[0] = sx * framework.minification;
	m_size[1] = sy * framework.minification;
	
	m_doubleBuffered = doubleBuffered;

	if (result && withDepthBuffer)
	{
		fassert(m_depthTexture == 0);
		glGenTextures(1, &m_depthTexture);
		result &= m_depthTexture != 0;
		checkErrorGL();

		if (result)
		{
			glBindTexture(GL_TEXTURE_2D, m_depthTexture);
			checkErrorGL();

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			checkErrorGL();

			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, sx, sy, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
			checkErrorGL();

			// set filtering

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			checkErrorGL();
		}
	}

	for (int i = 0; result && i < (doubleBuffered ? 2 : 1); ++i)
	{
		// allocate storage
		
		glGenTextures(1, &m_texture[i]);
		result &= m_texture[i] != 0;
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, m_texture[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		GLenum glFormat = GL_INVALID_ENUM;

		if (format == SURFACE_RGBA8)
			glFormat = GL_RGBA8;
		if (format == SURFACE_RGBA16F)
			glFormat = GL_RGBA16F;
		if (format == SURFACE_R8)
			glFormat = GL_R8;
		if (format == SURFACE_R16F)
			glFormat = GL_R16F;
		if (format == SURFACE_R32F)
			glFormat = GL_R32F;

		glTexStorage2D(GL_TEXTURE_2D, 1, glFormat, sx, sy);
		checkErrorGL();

		// set filtering
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		checkErrorGL();
		
		// create attachment
		
		glGenFramebuffers(1, &m_buffer[i]);
		result &= m_buffer[i] != 0;
		checkErrorGL();
		
		glBindFramebuffer(GL_FRAMEBUFFER, m_buffer[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture[i], 0);
		checkErrorGL();
		
		if (withDepthBuffer)
		{
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
			checkErrorGL();
		}
		
		// check if all went well
		
		const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			logError("failed to init surface. status: %d", status);
			
			result = false;
		}
	}

	if (result && doubleBuffered == false)
	{
		m_texture[1] = m_texture[0];
		m_buffer[1] = m_buffer[0];
	}
	
	if (!result)
	{
		logError("failed to init surface. calling destruct()");
		
		destruct();
	}
	
	// restore the old framebuffer and texture bindings
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	glBindTexture(GL_TEXTURE_2D, oldTexture);
	checkErrorGL();
	
	return result;
}

GLuint Surface::getFramebuffer() const
{
	return m_buffer[m_bufferId];
}

GLuint Surface::getTexture() const
{
	return m_texture[m_bufferId];
}

GLuint Surface::getDepthTexture() const
{
	return m_depthTexture;
}

int Surface::getWidth() const
{
	return m_size[0];
}

int Surface::getHeight() const
{
	return m_size[1];
}

void Surface::clear(int r, int g, int b, int a)
{
	clearf(scale255(r), scale255(g), scale255(b), scale255(a));
}

void Surface::clearf(float r, float g, float b, float a)
{
	pushSurface(this);
	{
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	popSurface();
}

void Surface::clearDepth(float d)
{
	pushSurface(this);
	{
		glClearDepth(d);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	popSurface();
}

void Surface::clearAlpha()
{
	setAlphaf(0.f);
}

void Surface::setAlpha(int a)
{
	setAlphaf(scale255(a));
}

void Surface::setAlphaf(float a)
{
	pushSurface(this);
	{
		pushBlend(BLEND_OPAQUE);
		setColorf(1.f, 1.f, 1.f, a);
		glColorMask(0, 0, 0, 1);
		{
			drawRect(0.f, 0.f, m_size[0], m_size[1]);
		}
		glColorMask(1, 1, 1, 1);
		popBlend();
	}
	popSurface();
}

void Surface::mulf(float r, float g, float b, float a)
{
	pushSurface(this);
	{
		pushBlend(BLEND_MUL);
		setColorf(r, g, b, a);
		drawRect(0.f, 0.f, m_size[0], m_size[1]);
		popBlend();
	}
	popSurface();
}

void Surface::postprocess()
{
	swapBuffers();

	pushSurface(this);
	{
		drawRect(0.f, 0.f, m_size[0], m_size[1]);
	}
	popSurface();	
}

void Surface::postprocess(Shader & shader)
{
	swapBuffers();
	
	pushSurface(this);
	{
		setShader(shader);
		{
			drawRect(0.f, 0.f, m_size[0], m_size[1]);
		}
		clearShader();
	}
	popSurface();
}

void Surface::invert()
{
	pushSurface(this);
	{
		pushBlend(BLEND_INVERT);
		setColorf(1.f, 1.f, 1.f, 1.f);
		drawRect(0.f, 0.f, m_size[0], m_size[1]);
		popBlend();
	}
	popSurface();
}

void Surface::invertColor()
{
	glColorMask(1, 1, 1, 0);
	{
		invert();
	}
	glColorMask(1, 1, 1, 1);
}

void Surface::invertAlpha()
{
	glColorMask(0, 0, 0, 1);
	{
		invert();
	}
	glColorMask(1, 1, 1, 1);
}

void Surface::gaussianBlur(const float strengthH, const float strengthV, const int _kernelSize)
{
	const int kernelSize = _kernelSize < 0 ? int(std::ceilf(std::max(strengthH, strengthV))) : _kernelSize;
	
	if (kernelSize == 0)
		return;
	
	if (strengthH > 0.f)
	{
		pushBlend(BLEND_OPAQUE);
		setShader_GaussianBlurH(getTexture(), kernelSize, strengthH);
		postprocess();
		clearShader();
		popBlend();
	}
	
	if (strengthV > 0.f)
	{
		pushBlend(BLEND_OPAQUE);
		setShader_GaussianBlurV(getTexture(), kernelSize, strengthV);
		postprocess();
		clearShader();
		popBlend();
	}
}

void Surface::blitTo(Surface * surface) const
{
	int oldReadBuffer = 0;
	int oldDrawBuffer = 0;

	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &oldReadBuffer);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawBuffer);
	checkErrorGL();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, getFramebuffer());
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surface->getFramebuffer());
	checkErrorGL();

	glBlitFramebuffer(
		0, 0, getWidth(), getHeight(),
		0, 0, surface->getWidth(), surface->getHeight(),
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
	checkErrorGL();

	glBindFramebuffer(GL_READ_FRAMEBUFFER, oldReadBuffer);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawBuffer);
	checkErrorGL();
}

void Surface::blit(BLEND_MODE blendMode) const
{
	pushBlend(blendMode);
	{
		gxSetTexture(getTexture());
		drawRect(0, 0, getWidth(), getHeight());
		gxSetTexture(0);
	}
	popBlend();
}

void blitBackBufferToSurface(Surface * surface)
{
	int oldDrawBuffer = 0;

	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &oldDrawBuffer);
	checkErrorGL();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, surface->getFramebuffer());
	checkErrorGL();

	glBlitFramebuffer(
		// fixme
		//0, 0, globals.actualDisplaySize[0] / framework.minification, globals.actualDisplaySize[1] / framework.minification,
		0, 0, globals.actualDisplaySize[0], globals.actualDisplaySize[1],
		0, 0, surface->getWidth(), surface->getHeight(),
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST);
	checkErrorGL();

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, oldDrawBuffer);
	checkErrorGL();
}

// -----

Shader::Shader()
{
	m_shader = 0;
}

Shader::Shader(const char * filename)
{
	m_shader = 0;

	const std::string vs = std::string(filename) + ".vs";
	const std::string ps = std::string(filename) + ".ps";

	load(filename, vs.c_str(), ps.c_str());
}

Shader::Shader(const char * name, const char * filenameVs, const char * filenamePs)
{
	m_shader = 0;

	load(name, filenameVs, filenamePs);
}

Shader::~Shader()
{
	if (globals.shader == this)
		clearShader();
}

void Shader::load(const char * name, const char * filenameVs, const char * filenamePs)
{
	m_shader = &g_shaderCache.findOrCreate(name, filenameVs, filenamePs);
}

bool Shader::isValid() const
{
	return m_shader != 0 && m_shader->program != 0;
}

GLuint Shader::getProgram() const
{
	return m_shader ? m_shader->program : 0;
}

GLint Shader::getImmediate(const char * name)
{
	return glGetUniformLocation(getProgram(), name);
}

GLint Shader::getAttribute(const char * name)
{
	return glGetAttribLocation(getProgram(), name);
}

#define SET_UNIFORM(name, op) \
	if (getProgram()) \
	{ \
		setShader(*this); \
		const GLint index = glGetUniformLocation(getProgram(), name); \
		if (index == -1) \
		{ \
			/*logDebug("couldn't find shader uniform %s", name);*/ \
		} \
		else \
		{ \
			op; \
		} \
	}
		
void Shader::setImmediate(const char * name, float x)
{
	SET_UNIFORM(name, glUniform1f(index, x));
	checkErrorGL();
}

void Shader::setImmediate(const char * name, float x, float y)
{
	SET_UNIFORM(name, glUniform2f(index, x, y));
	checkErrorGL();
}

void Shader::setImmediate(const char * name, float x, float y, float z)
{
	SET_UNIFORM(name, glUniform3f(index, x, y, z));
	checkErrorGL();
}

void Shader::setImmediate(const char * name, float x, float y, float z, float w)
{
	SET_UNIFORM(name, glUniform4f(index, x, y, z, w));
	checkErrorGL();
}

void Shader::setImmediate(GLint index, float x, float y, float z, float w)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform4f(index, x, y, z, w);
	checkErrorGL();
}

void Shader::setImmediateMatrix4x4(const char * name, const float * matrix)
{
	SET_UNIFORM(name, glUniformMatrix4fv(index, 1, GL_FALSE, matrix));
	checkErrorGL();
}

void Shader::setImmediateMatrix4x4(GLint index, const float * matrix)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniformMatrix4fv(index, 1, GL_FALSE, matrix);
	checkErrorGL();
}

void Shader::setTextureUnit(const char * name, int unit)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();
}

void Shader::setTextureUnit(GLint index, int unit)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform1i(index, unit);
	checkErrorGL();
}

void Shader::setTexture(const char * name, int unit, GLuint texture)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void Shader::setTexture(const char * name, int unit, GLuint texture, bool filtered, bool clamped)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void Shader::setTextureArray(const char * name, int unit, GLuint texture)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void Shader::setTextureArray(const char * name, int unit, GLuint texture, bool filtered, bool clamped)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

#undef SET_UNIFORM

void Shader::setBuffer(const char * name, const ShaderBuffer & buffer)
{
	const GLuint program = getProgram();
	if (!program)
		return;
	const GLuint index = glGetUniformBlockIndex(program, name);
	checkErrorGL();

	if (index == -1) // todo : index is -1 on failure to find it ?
		logWarning("unable to find block index for %s", name);
	else
		setBuffer(index, buffer);
}

void Shader::setBuffer(GLint index, const ShaderBuffer & buffer)
{
	fassert(globals.shader == this);

	glUniformBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
}

void Shader::setBufferRw(const char * name, const ShaderBufferRw & buffer)
{
	const GLuint program = getProgram();
	if (!program)
		return;
	const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
	checkErrorGL();

	if (index == -1) // todo : index is -1 on failure to find it ?
		logWarning("unable to find block index for %s", name);
	else
		setBufferRw(index, buffer);
}

void Shader::setBufferRw(GLint index, const ShaderBufferRw & buffer)
{
	fassert(globals.shader == this);

	glShaderStorageBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
}

void Shader::reload()
{
	m_shader->reload();
}

// -----

ComputeShader::ComputeShader()
{
	m_shader = 0;
}

ComputeShader::ComputeShader(const char * filename, const int groupSx, const int groupSy, const int groupSz)
{
	m_shader = 0;

	load(filename, groupSx, groupSy, groupSz);
}

ComputeShader::~ComputeShader()
{
	if (globals.shader == this)
		clearShader();
}

void ComputeShader::load(const char * filename, const int groupSx, const int groupSy, const int groupSz)
{
	m_shader = &g_computeShaderCache.findOrCreate(filename, groupSx, groupSy, groupSz);
}

GLuint ComputeShader::getProgram() const
{
	return m_shader ? m_shader->program : 0;
}

int ComputeShader::getGroupSx() const
{
	return m_shader ? m_shader->groupSx : 0;
}

int ComputeShader::getGroupSy() const
{
	return m_shader ? m_shader->groupSy : 0;
}

int ComputeShader::getGroupSz() const
{
	return m_shader ? m_shader->groupSz : 0;
}

static int calcThreadSize(const int dispatchSize, const int groupSize)
{
	if (groupSize > 0)
		return (dispatchSize + groupSize - 1) / groupSize;
	else
		return 0;
}

int ComputeShader::toThreadSx(const int sx) const
{
	return calcThreadSize(sx, getGroupSx());
}

int ComputeShader::toThreadSy(const int sy) const
{
	return calcThreadSize(sy, getGroupSy());
}

int ComputeShader::toThreadSz(const int sz) const
{
	return calcThreadSize(sz, getGroupSz());
}

GLint ComputeShader::getImmediate(const char * name)
{
	return glGetUniformLocation(getProgram(), name);
}

GLint ComputeShader::getAttribute(const char * name)
{
	return glGetAttribLocation(getProgram(), name);
}

#define SET_UNIFORM(name, op) \
	if (getProgram()) \
	{ \
		setShader(*this); \
		const GLint index = glGetUniformLocation(getProgram(), name); \
		if (index == -1) \
		{ \
			/*logDebug("couldn't find shader uniform %s", name);*/ \
		} \
		else \
		{ \
			op; \
		} \
	}

void ComputeShader::setImmediate(const char * name, float x)
{
	SET_UNIFORM(name, glUniform1f(index, x));
	checkErrorGL();
}

void ComputeShader::setImmediate(const char * name, float x, float y)
{
	SET_UNIFORM(name, glUniform2f(index, x, y));
	checkErrorGL();
}

void ComputeShader::setImmediate(const char * name, float x, float y, float z)
{
	SET_UNIFORM(name, glUniform3f(index, x, y, z));
	checkErrorGL();
}

void ComputeShader::setImmediate(const char * name, float x, float y, float z, float w)
{
	SET_UNIFORM(name, glUniform4f(index, x, y, z, w));
	checkErrorGL();
}

void ComputeShader::setImmediate(GLint index, float x, float y, float z, float w)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniform4f(index, x, y, z, w);
	checkErrorGL();
}

void ComputeShader::setImmediateMatrix4x4(const char * name, const float * matrix)
{
	SET_UNIFORM(name, glUniformMatrix4fv(index, 1, GL_FALSE, matrix));
	checkErrorGL();
}

void ComputeShader::setImmediateMatrix4x4(GLint index, const float * matrix)
{
	fassert(index != -1);
	fassert(globals.shader == this);
	glUniformMatrix4fv(index, 1, GL_FALSE, matrix);
	checkErrorGL();
}

void ComputeShader::setTexture(const char * name, int unit, GLuint texture, bool filtered, bool clamped)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void ComputeShader::setTextureArray(const char * name, int unit, GLuint texture, bool filtered, bool clamped)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, filtered ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, clamped ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glActiveTexture(GL_TEXTURE0);
	checkErrorGL();
}

void ComputeShader::setTextureRw(const char * name, int unit, GLuint texture, GLuint format, bool filtered, bool clamp)
{
	SET_UNIFORM(name, glUniform1i(index, unit));
	checkErrorGL();

	glBindImageTexture(unit, texture, 0, false, 0, GL_READ_WRITE, format);
	checkErrorGL();
}

#undef SET_UNIFORM

void ComputeShader::setBuffer(const char * name, const ShaderBuffer & buffer)
{
	const GLuint program = getProgram();
	if (!program)
		return;
	const GLuint index = glGetUniformBlockIndex(program, name);
	checkErrorGL();

	if (index == -1) // todo : index is -1 on failure to find it ?
		logWarning("unable to find block index for %s", name);
	else
		setBuffer(index, buffer);
}

void ComputeShader::setBuffer(GLint index, const ShaderBuffer & buffer)
{
	fassert(globals.shader == this);

	glUniformBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
}

void ComputeShader::setBufferRw(const char * name, const ShaderBufferRw & buffer)
{
	const GLuint program = getProgram();
	if (!program)
		return;
	const GLuint index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, name);
	checkErrorGL();

	if (index == -1) // todo : index is -1 on failure to find it ?
		logWarning("unable to find block index for %s", name);
	else
		setBufferRw(index, buffer);
}

void ComputeShader::setBufferRw(GLint index, const ShaderBufferRw & buffer)
{
	fassert(globals.shader == this);

	glShaderStorageBlockBinding(getProgram(), index, index);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.getBuffer());

	checkErrorGL();
}

void ComputeShader::dispatch(const int dispatchSx, const int dispatchSy, const int dispatchSz)
{
	fassert(globals.shader == this);

	const int threadSx = toThreadSx(dispatchSx);
	const int threadSy = toThreadSy(dispatchSy);
	const int threadSz = toThreadSz(dispatchSz);

	glDispatchCompute(threadSx, threadSy, threadSz);
	checkErrorGL();

	// todo : let application insert barriers
	//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glMemoryBarrier(GL_ALL_BARRIER_BITS);
	checkErrorGL();
}

void ComputeShader::reload()
{
	m_shader->reload();
}

// -----

static std::vector<GLuint> s_bufferPool;
static bool s_useBufferPool = false;

ShaderBuffer::ShaderBuffer()
	: m_buffer(0)
{
	if (!s_useBufferPool || s_bufferPool.empty())
	{
		glGenBuffers(1, &m_buffer);
		checkErrorGL();
	}
	else
	{
		m_buffer = s_bufferPool.back();
		s_bufferPool.pop_back();
	}
}

ShaderBuffer::~ShaderBuffer()
{
	if (m_buffer)
	{
		if (s_useBufferPool)
		{
			s_bufferPool.push_back(m_buffer);
			m_buffer = 0;
		}
		else
		{
			glDeleteBuffers(1, &m_buffer);
			m_buffer = 0;
			checkErrorGL();
		}
	}
}

GLuint ShaderBuffer::getBuffer() const
{
	return m_buffer;
}

void ShaderBuffer::setData(const void * bytes, int numBytes)
{
	fassert(m_buffer);

	if (m_buffer)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
		checkErrorGL();

		glBufferData(GL_UNIFORM_BUFFER, numBytes, bytes, GL_DYNAMIC_DRAW);
		checkErrorGL();
	}
}

//

ShaderBufferRw::ShaderBufferRw()
	: m_buffer(0)
{
	glGenBuffers(1, &m_buffer);
	checkErrorGL();
}

ShaderBufferRw::~ShaderBufferRw()
{
	if (m_buffer)
	{
		glDeleteBuffers(1, &m_buffer);
		m_buffer = 0;
		checkErrorGL();
	}
}

GLuint ShaderBufferRw::getBuffer() const
{
	return m_buffer;
}

void ShaderBufferRw::setDataRaw(const void * bytes, int numBytes)
{
	fassert(m_buffer);

	if (m_buffer)
	{
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_buffer);
		checkErrorGL();

		glBufferData(GL_SHADER_STORAGE_BUFFER, numBytes, bytes, GL_DYNAMIC_DRAW);
		checkErrorGL();
	}
}

// -----

Color::Color()
{
	r = g = b = a = 0.f;
}

Color::Color(int r, int g, int b, int a)
{
	this->r = scale255(r);
	this->g = scale255(g);
	this->b = scale255(b);
	this->a = scale255(a);
}

Color::Color(float r, float g, float b, float a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color Color::fromHex(const char * str)
{
	const size_t len = strlen(str);
	
	if (len == 0)
	{
		return Color(0.f, 0.f, 0.f, 0.f);
	}
	else if (len == 6)
	{
		const uint32_t hex = std::stoul(str, 0, 16);
		const float r = scale255((hex >> 16) & 0xff);
		const float g = scale255((hex >>  8) & 0xff);
		const float b = scale255((hex >>  0) & 0xff);
		const float a = 1.f;
		return Color(r, g, b, a);
	}
	else if (len == 8)
	{
		const uint32_t hex = std::stoul(str, 0, 16);
		const float r = scale255((hex >> 24) & 0xff);
		const float g = scale255((hex >> 16) & 0xff);
		const float b = scale255((hex >>  8) & 0xff);
		const float a = scale255((hex >>  0) & 0xff);
		return Color(r, g, b, a);
	}
	else
	{
		return colorBlack;
	}
}

Color Color::fromHSL(float hue, float sat, float lum)
{
	hue = fmod(hue, 1.f) * 6.f;
	sat = sat < 0.f ? 0.f : sat > 1.f ? 1.f : sat;
	lum = lum < 0.f ? 0.f : lum > 1.f ? 1.f : lum;
	
	//
	
	float r, g, b;

	float m2 = (lum <= .5f) ? (lum + (lum * sat)) : (lum + sat - lum * sat);
	float m1 = lum + lum - m2;

	if (hue < 0.f)
	{
		hue += 6.f;
	}

	if (hue < 3.0f)
	{
		if (hue < 2.0f)
		{
			if (hue < 1.0f)
			{
				r = m2;
				g = m1 + (m2 - m1) * hue;
				b = m1;
			}
			else
			{
				r = (m1 + (m2 - m1) * (2.f - hue));
				g = m2;
				b = m1;
			}
		}
		else
		{
			r = m1;
			g = m2;
			b = (m1 + (m2 - m1) * (hue - 2.f));
		}
	}
	else
	{
		if (hue < 5.0f)
		{
			if (hue < 4.0f)
			{
				r = m1;
				g = (m1 + (m2 - m1) * (4.f - hue));
				b = m2;
			}
			else
			{
				r = (m1 + (m2 - m1) * (hue - 4.f));
				g = m1;
				b = m2;
			}
		}
		else
		{
			r = m2;
			g = m1;
			b = (m1 + (m2 - m1) * (6.f - hue));
		}
	}

	return Color(r, g, b);
}

void Color::toHSL(float & hue, float & sat, float & lum) const
{
	float max = std::max(r, std::max(g, b));
	float min = std::min(r, std::min(g, b));

	lum = (max + min) / 2.0f;

	float delta = max - min;

	if (delta < FLT_EPSILON)
	{
		sat = 0.f;
		hue = 0.f;
	}
	else
	{
		sat = (lum <= .5f) ? (delta / (max + min)) : (delta / (2.f - (max + min)));

		if (r == max)
			hue = (g - b) / delta;
		else if (g == max)
			hue = 2.f + (b - r) / delta;
		else
			hue = 4.f + (r - g) / delta;

		if (hue < 0.f)
			hue += 6.0f;

		hue /= 6.f;
	}
}

Color Color::interp(const Color & other, float t) const
{
	const float t1 = 1.f - t;
	const float t2 = t;
	
	return Color(
		r * t1 + other.r * t2,
		g * t1 + other.g * t2,
		b * t1 + other.b * t2,
		a * t1 + other.a * t2);
}

Color Color::hueShift(float shift) const
{
	float hue, sat, lum;
	toHSL(hue, sat, lum);
	return Color::fromHSL(hue + shift, sat, lum);
}

uint32_t Color::toRGBA() const
{
	const int ir = r < 0.f ? 0 : r > 1.f ? 255 : int(r * 255.f);
	const int ig = g < 0.f ? 0 : g > 1.f ? 255 : int(g * 255.f);
	const int ib = b < 0.f ? 0 : b > 1.f ? 255 : int(b * 255.f);
	const int ia = a < 0.f ? 0 : a > 1.f ? 255 : int(a * 255.f);
	return (ir << 24) | (ig << 16) | (ib << 8) | (ia << 0);
}

std::string Color::toHexString(const bool withAlpha) const
{
	const int ir = r < 0.f ? 0 : r > 1.f ? 255 : int(r * 255.f);
	const int ig = g < 0.f ? 0 : g > 1.f ? 255 : int(g * 255.f);
	const int ib = b < 0.f ? 0 : b > 1.f ? 255 : int(b * 255.f);
	const int ia = a < 0.f ? 0 : a > 1.f ? 255 : int(a * 255.f);
	
	char text[64];
	
	if (withAlpha)
		sprintf_s(text, sizeof(text), "%02x%02x%02x%02x", ir, ig, ib, ia);
	else
		sprintf_s(text, sizeof(text), "%02x%02x%02x", ir, ig, ib);
	
	return text;
}

void Color::set(const float r, const float g, const float b, const float a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color Color::addRGB(const Color & other) const
{
	return Color(r + other.r, g + other.g, b + other.b, a);
}

Color Color::mulRGBA(const Color & other) const
{
	return Color(r * other.r, g * other.g, b * other.b, a * other.a);
}

Color Color::mulRGB(float t) const
{
	return Color(r * t, g * t, b * t, a);
}

// -----

Gradient::Gradient()
{
	x1 = y1 = 0.f;
	x2 = y2 = 0.f;
}

Gradient::Gradient(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2)
{
	set(x1, y1, color1, x2, y2, color2);
}

void Gradient::set(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2)
{
	this->x1 = x1;
	this->y1 = y1;
	this->color1 = color1;
	this->x2 = x2;
	this->y2 = y2;
	this->color2 = color2;
}

Color Gradient::eval(float x, float y) const
{
	const float dx = x2 - x1;
	const float dy = y2 - y1;
	const float ds = dx * dx + dy * dy;
	const float pd = dx * x1 + dy * y1;
	const float t = (dx * x + dy * y - pd) / ds;
	return color1.interp(color2, t);
}

// -----

bool Dictionary::load(const char * filename)
{
	bool result = true;

	m_map.clear();

	FileReader reader;

	if (!reader.open(filename, true))
	{
		result = false;
	}
	else
	{
		std::string line;

		while (reader.read(line))
		{
			if (!parse(line, false))
			{
				result = false;
			}
		}
	}

	return result;
}

bool Dictionary::save(const char * filename)
{
	bool result = true;
	
	FILE * file = nullptr;
	
	fopen_s(&file, filename, "wt");
	
	if (file == nullptr)
	{
		result = false;
	}
	else
	{
		for (auto i : m_map)
		{
			const char * key = i.first.c_str();
			const char * value = i.second.c_str();
			
			fprintf(file, "%s:%s\n", key, value);
		}
		
		fclose(file);
		file = nullptr;
	}
	
	return result;
}

bool Dictionary::parse(const std::string & line, bool clear)
{
	bool result = true;
	
	if (clear)
	{
		m_map.clear();
	}
	
	std::vector<std::string> parts;
	splitString(line, parts);
	
	for (size_t i = 0; i < parts.size(); ++i)
	{
		const size_t separator = parts[i].find(':');
		
		if (separator == std::string::npos)
		{
			logError("%s: incorrect key:value syntax: %s (%s)", __FUNCTION__, line.c_str(), parts[i].c_str());
			result = false;
			continue;
		}
		
		const std::string key = parts[i].substr(0, separator);
		const std::string value = parts[i].substr(separator + 1, parts[i].size() - separator - 1);
		
		if (key.size() == 0 || value.size() == 0)
		{
			logError("%s: incorrect key:value syntax: %s (%s)", __FUNCTION__, line.c_str(), parts[i].c_str());
			result = false;
			continue;
		}
		
		if (contains(key.c_str()))
		{
			logError("%s: duplicate key: %s (%s)", __FUNCTION__, line.c_str(), key.c_str());
			result = false;
			continue;
		}
		
		setString(key.c_str(), value.c_str());
	}
	
	return result;
}

bool Dictionary::contains(const char * name) const
{
	return m_map.count(name) != 0;
}

void Dictionary::setString(const char * name, const char * value)
{
	m_map[name] = value;
}

void Dictionary::setInt(const char * name, int value)
{
	char text[32];
	sprintf_s(text, sizeof(text), "%d", value);
	setString(name, text);
}

void Dictionary::setInt64(const char * name, int64_t value)
{
    char text[32];
    sprintf_s(text, sizeof(text), "%lld", value);
    setString(name, text);
}

void Dictionary::setBool(const char * name, bool value)
{
	setInt(name, value ? 1 : 0);
}

void Dictionary::setFloat(const char * name, float value)
{
	char text[64];
	sprintf_s(text, sizeof(text), "%f", value);
	setString(name, text);
}

void Dictionary::setPtr(const char * name, void * value)
{
	// fixme : right now this only works with 32 or 64 bit pointers

	fassert(sizeof(intptr_t) <= sizeof(int64_t));

	setInt64(name, reinterpret_cast<intptr_t>(value));
}

std::string Dictionary::getString(const char * name, const char * _default) const
{
	Map::const_iterator i = m_map.find(name);
	if (i != m_map.end())
		return i->second;
	else
		return _default;
}

int Dictionary::getInt(const char * name, int _default) const
{
	Map::const_iterator i = m_map.find(name);
	if (i != m_map.end())
		return atoi(i->second.c_str());
	else
		return _default;
}

int64_t Dictionary::getInt64(const char * name, int64_t _default) const
{
#if defined(WINDOWS)
    Map::const_iterator i = m_map.find(name);
    if (i != m_map.end())
		return _atoi64(i->second.c_str());
    else
        return _default;
#else
    Map::const_iterator i = m_map.find(name);
    if (i != m_map.end())
        return atoll(i->second.c_str());
    else
        return _default;
#endif
}

bool Dictionary::getBool(const char * name, bool _default) const
{
	return getInt(name, _default) != 0;
}

float Dictionary::getFloat(const char * name, float _default) const
{
	Map::const_iterator i = m_map.find(name);
	if (i != m_map.end())
		return atof(i->second.c_str());
	else
		return _default;
}

void * Dictionary::getPtr(const char * name, void * _default) const
{
	// fixme : right now this only works with 32 or 64 bit bit pointers

	return reinterpret_cast<void*>(getInt64(name, (int)(reinterpret_cast<intptr_t>(_default))));
}

std::string & Dictionary::operator[](const char * name)
{
	return m_map[name];
}

// -----

GLuint getTexture(const char * filename)
{
	const TextureCacheElem & elem = g_textureCache.findOrCreate(filename, 1, 1);

	if (elem.textures)
		return elem.textures[0];
	else
		return 0;
}

// -----

static AnimCacheElem s_dummyAnimCacheElem;

Sprite::Sprite(const char * filename, float pivotX, float pivotY, const char * spritesheet, bool autoUpdate, bool hasSpriteSheet)
	: m_autoUpdate(autoUpdate)
{
	// drawing
	this->pivotX = pivotX;
	this->pivotY = pivotY;
	x = 0.f;
	y = 0.f;
	angle = 0.f;
	separateScale = false;
	scale = 1.f;
	scaleX = 1.f;
	scaleY = 1.f;
	flipX = false;
	flipY = false;
	pixelpos = true;
	filter = FILTER_POINT;
	
	// animation
	const char * sheetFilename = 0;
	std::string sheetFilenameStr;
	if (hasSpriteSheet)
	{
		if (spritesheet)
		{
			sheetFilename = spritesheet;
		}
		else
		{
			sheetFilenameStr = filename;
			size_t dot = sheetFilenameStr.rfind('.');
			if (dot != std::string::npos)
				sheetFilenameStr = sheetFilenameStr.substr(0, dot) + ".txt";
			sheetFilename = sheetFilenameStr.c_str();
		}
	}
	
	if (sheetFilename)
		m_anim = &g_animCache.findOrCreate(sheetFilename);
	else
		m_anim = &s_dummyAnimCacheElem;
	m_animVersion = m_anim->getVersion();
	m_animSegment = 0;
	animIsActive = false;
	animIsPaused = false;
	m_isAnimStarted = false;
	m_animFramef = 0.f;
	m_animFrame = 0;
	animSpeed = 1.f;
	animActionHandler = 0;
	animActionHandlerObj = 0;

	if (m_anim->m_hasSheet)
	{
		this->pivotX = (float)m_anim->m_pivot[0];
		this->pivotY = (float)m_anim->m_pivot[1];
		this->scale = (float)m_anim->m_scale;
	}
	
	// texture
	m_texture = &g_textureCache.findOrCreate(filename, m_anim->m_gridSize[0], m_anim->m_gridSize[1]);
	
	m_prev = 0;
	m_next = 0;
	if (m_autoUpdate)
		framework.registerSprite(this);
}

Sprite::~Sprite()
{
	if (m_autoUpdate)
		framework.unregisterSprite(this);
}

void Sprite::reload()
{
	m_texture->reload();
}

void Sprite::update(float dt)
{
	updateAnimation(dt);
}

void Sprite::draw()
{
	drawEx(x, y, angle, separateScale ? scaleX : scale, separateScale ? scaleY : scale, pixelpos, filter);
}

void Sprite::drawEx(float x, float y, float angle, float scaleX, float scaleY, bool pixelpos, TEXTURE_FILTER filter)
{
	if (scaleY == FLT_MAX)
		scaleY = scaleX;

	if (m_texture->textures)
	{
		gxPushMatrix();
		{
			if (pixelpos)
			{
				x = std::floor(x / framework.minification) * framework.minification;
				y = std::floor(y / framework.minification) * framework.minification;
			}
			
			gxTranslatef(x, y, 0.f);
			
			if (angle != 0.f)
				gxRotatef(angle, 0.f, 0.f, 1.f);
			if (scaleX != 1.f || scaleY != 1.f)
				gxScalef(scaleX, scaleY, 1.f);
			if (flipX || flipY)
				gxScalef(flipX ? -1.f : +1.f, flipY ? -1.f : +1.f, 1.f);
			if (pivotX != 0.f || pivotY != 0.f)
				gxTranslatef(-pivotX, -pivotY, 0.f);
			
			int cellIndex;
			
			if (m_isAnimStarted && m_animSegment)
			{
				AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
				
				cellIndex = getAnimFrame() + anim->firstCell;
			}
			else
			{
				cellIndex = 0;
			}
			
			fassert(cellIndex >= 0 && cellIndex < (m_anim->m_gridSize[0] * m_anim->m_gridSize[1]));
			
			gxSetTexture(m_texture->textures[cellIndex]);
			
			if (filter == FILTER_POINT)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
		#if 1
			else if (filter == FILTER_LINEAR || filter == FILTER_MIPMAP)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
		#else
			else if (filter == FILTER_LINEAR)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else if (filter == FILTER_MIPMAP)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
		#endif
			else
			{
				fassert(false);
			}
			
			checkErrorGL();

			const float rsx = float(m_texture->sx / m_anim->m_gridSize[0]);
			const float rsy = float(m_texture->sy / m_anim->m_gridSize[1]);
			
		#if 0
			const float verts[16] =
			{
				0.f, 0.f, 0.f, 1.f,
				rsx, 0.f, 0.f, 1.f,
				rsx, rsy, 0.f, 1.f,
				0.f, rsy, 0.f, 1.f
			};
			
			static const float texs[8] =
			{
				0.f, 1.f,
				1.f, 1.f,
				1.f, 0.f,
				0.f, 0.f
			};
			
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(4, GL_FLOAT, 0, verts);
			glTexCoordPointer(2, GL_FLOAT, 0, texs);
			glDrawArrays(GL_QUADS, 0, 4);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		#else
			gxBegin(GL_QUADS);
			{
				gxTexCoord2f(0.f, 0.f); gxVertex2f(0.f, 0.f);
				gxTexCoord2f(1.f, 0.f); gxVertex2f(rsx, 0.f);
				gxTexCoord2f(1.f, 1.f); gxVertex2f(rsx, rsy);
				gxTexCoord2f(0.f, 1.f); gxVertex2f(0.f, rsy);
			}
			gxEnd();
		#endif

			checkErrorGL();
		}
		gxPopMatrix();

		gxSetTexture(0);
	}
}

void Sprite::startAnim(const char * name, int frame)
{
	animIsPaused = false;
	m_animSegmentName = name;
	m_isAnimStarted = true;
	m_animFramef = (float)frame;
	m_animFrame = frame;

	m_animVersion = -1;
	updateAnimationSegment();
	
	if (animIsActive)
	{
		processAnimationTriggersForFrame(m_animFrame, AnimCacheElem::AnimTrigger::OnEnter);
	}
}

void Sprite::stopAnim()
{
	animIsActive = false;
	m_isAnimStarted = false;
	m_animSegment = 0;
}

const std::string & Sprite::getAnim() const
{
	return m_animSegmentName;
}

void Sprite::setAnimFrame(int frame)
{
	fassert(frame >= 0);
	
	if (m_animSegment)
	{
		const int frame1 = m_animFrame;
		{
			m_animFrame = calculateLoopedFrameIndex(frame);
		}
		const int frame2 = m_animFrame;
		
		m_animFramef = (float)m_animFrame;

		processAnimationFrameChange(frame1, frame2);
	}
}

int Sprite::getAnimFrame() const
{
	return m_animFrame;
}

std::vector<std::string> Sprite::getAnimList() const
{
	std::vector<std::string> result;

	if (m_anim)
	{
		for (auto i = m_anim->m_animMap.begin(); i != m_anim->m_animMap.end(); ++i)
			result.push_back(i->first);
	}

	return result;
}

void Sprite::updateAnimationSegment()
{
	if (m_isAnimStarted && m_animVersion != m_anim->getVersion() && !m_animSegmentName.empty())
	{
		m_animVersion = m_anim->getVersion();
		
		if (m_anim->m_animMap.count(m_animSegmentName) != 0)
			m_animSegment = &m_anim->m_animMap[m_animSegmentName];
		else
			m_animSegment = 0;
		
		if (!m_animSegment)
		{
			logInfo("unable to find animation: %s", m_animSegmentName.c_str());
			animIsActive = false;
			m_animFramef = 0.f;
			m_animFrame = 0;
		}
		else
		{
			AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
			
			this->pivotX = (float)anim->pivot[0];
			this->pivotY = (float)anim->pivot[1];
			
			animIsActive = true;
		}
		
		// recache texture, since the animation grid size may have changed
		m_texture = &g_textureCache.findOrCreate(m_texture->name.c_str(), m_anim->m_gridSize[0], m_anim->m_gridSize[1]);
	}
}

void Sprite::updateAnimation(float timeStep)
{
	if (m_isAnimStarted && m_animSegment && !animIsPaused)
	{
		AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
		
		const int frame1 = m_animFrame;
		{
			const float step = animSpeed * anim->frameRate * timeStep;
			
			m_animFramef += step;
			
			if (!anim->loop)
				m_animFrame = std::min<int>((int)m_animFramef, anim->numFrames - 1);
			else
				m_animFrame = (int)m_animFramef;
		}
		const int frame2 = m_animFrame;
		
		for (int frame = frame1; frame < frame2; frame++)
		{
			const int oldFrame = calculateLoopedFrameIndex(frame + 0);
			const int newFrame = calculateLoopedFrameIndex(frame + 1);
			processAnimationFrameChange(oldFrame, newFrame);
		}
		
		if (anim->loop)
		{
			while (m_animFramef >= anim->numFrames)
				m_animFramef -= anim->numFrames - anim->loopStart;
			m_animFrame = (int)m_animFramef;
		}
		else
		{
			if (m_animFramef >= anim->numFrames)
				animIsActive = false;
		}
		
		//if (m_animSegmentName == "default")
		//	logInfo("%d (%d)", m_animFrame, anim->numFrames);
	}
}

void Sprite::processAnimationFrameChange(int frame1, int frame2)
{
	fassert(animIsActive);
	
	if (frame1 != frame2)
	{
		// process frame triggers
		processAnimationTriggersForFrame(frame1, AnimCacheElem::AnimTrigger::OnLeave);
		processAnimationTriggersForFrame(frame2, AnimCacheElem::AnimTrigger::OnEnter);
	}
}

void Sprite::processAnimationTriggersForFrame(int frame, int event)
{
	fassert(animIsActive);
	
	AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
		
	for (size_t i = 0; i < anim->frameTriggers[frame].size(); ++i)
	{
		const AnimCacheElem::AnimTrigger & trigger = anim->frameTriggers[frame][i];
		
		if (trigger.event == event)
		{
			//logInfo("event == this->event");
			
			Dictionary args = trigger.args;
			args.setPtr("obj", animActionHandlerObj);
			args.setInt("x", args.getInt("x", 0) + (int)this->x);
			args.setInt("y", args.getInt("y", 0) + (int)this->y);
			
			if (animActionHandler)
				animActionHandler(trigger.action, args);
			else
				framework.processActions(trigger.action, args);
		}
	}
}

int Sprite::calculateLoopedFrameIndex(int frame) const
{
	AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);

	if (!anim->loop)
		frame = std::max<int>(0, std::min<int>(anim->numFrames - 1, frame));
	else
	{
		while (frame < 0)
			frame += anim->numFrames;
		while (frame >= anim->numFrames)
			frame -= anim->numFrames - anim->loopStart;
	}

	return frame;
}

int Sprite::getWidth() const
{
	return m_texture->sx / m_anim->m_gridSize[0];
}

int Sprite::getHeight() const
{
	return m_texture->sy / m_anim->m_gridSize[1];
}

GLuint Sprite::getTexture() const
{
	if (m_texture->textures)
		return m_texture->textures[0];
	else
		return 0;
}

// -----

SpriterState::SpriterState()
{
	memset(this, 0, sizeof(SpriterState));

	scale = 1.f;
	scaleX = FLT_MAX;
	scaleY = FLT_MAX;

	animIndex = -1;
	animSpeed = 1.f;
}

bool SpriterState::startAnim(const Spriter & spriter, int index)
{
	animIsActive = true;
	animIndex = index;
	animTime = 0.f;

	return animIndex >= 0;
}

bool SpriterState::startAnim(const Spriter & spriter, const char * name)
{
	return startAnim(spriter, spriter.getAnimIndexByName(name));
}

void SpriterState::stopAnim(const Spriter & spriter)
{
	animIsActive = false;
}

bool SpriterState::updateAnim(const Spriter & spriter, float dt)
{
	if (animIsActive)
		animTime += animSpeed * dt;

	const bool isDone =
		!animIsActive ||
		spriter.isAnimDoneAtTime(animIndex, animTime);

	if (isDone)
		stopAnim(spriter);

	return isDone;
}

void SpriterState::setCharacterMap(const Spriter & spriter, int index)
{
	//fassert(index >= 0 && index < spriter.m_spriter->m_scene->m_fileCaches.size());
	if (index >= 0 && index < (int)spriter.m_spriter->m_scene->m_fileCaches.size())
		characterMap = index;
	else
	{
		logError("character map does not exist: index=%d", index);
		characterMap = 0;
	}
}

void SpriterState::setCharacterMap(const Spriter & spriter, const char * name)
{
	const int index = spriter.m_spriter->m_scene->getCharacterMapIndexByName(name);
	fassert(index != -1);
	if (index == -1)
		logError("character map not found: %s", name);

	characterMap = index < 0 ? 0 : characterMap;
}

// -----

Spriter::Spriter(const char * filename)
{
	m_spriter = &g_spriterCache.findOrCreate(filename);
}

void Spriter::getDrawableListAtTime(const SpriterState & state, spriter::Drawable * drawables, int & numDrawables)
{
	if (m_spriter->m_scene->m_entities.empty())
		numDrawables = 0;
	else
	{
		m_spriter->m_scene->m_entities[0]->getDrawableListAtTime(
			state.animIndex,
			state.characterMap,
			state.animTime * 1000.f,
			drawables, numDrawables);
	}
}

void Spriter::draw(const SpriterState & state)
{
	if (m_spriter->m_scene->m_entities.empty())
	{
		return;
	}

	const int kMaxDrawables = 64;
	spriter::Drawable drawables[kMaxDrawables];
	int numDrawables = kMaxDrawables;

	m_spriter->m_scene->m_entities[0]->getDrawableListAtTime(
		state.animIndex,
		state.characterMap,
		state.animTime * 1000.f,
		drawables, numDrawables);

	draw(state, drawables, numDrawables);
}

void Spriter::draw(const SpriterState & state, const spriter::Drawable * drawables, int numDrawables)
{
	gxPushMatrix();
	gxTranslatef(state.x, state.y, 0.f);
	gxRotatef(state.angle, 0.f, 0.f, 1.f);
	const float scaleX = state.scaleX != FLT_MAX ? state.scaleX : state.scale;
	const float scaleY = state.scaleY != FLT_MAX ? state.scaleY : state.scale;
	if (scaleX != 1.f || scaleY != 1.f)
		gxScalef(scaleX, scaleY, 1.f);
	if (state.flipX || state.flipY)
		gxScalef(state.flipX ? -1.f : +1.f, state.flipY ? -1.f : +1.f, 1.f);

	const Color oldColor = globals.color;

	for (int i = 0; i < numDrawables; ++i)
	{
		const spriter::Drawable & d = drawables[i];

		setColorf(
			globals.color.r,
			globals.color.g,
			globals.color.b,
			d.a * oldColor.a);

		Sprite sprite(d.filename, d.pivotX, d.pivotY, 0, false, false);
		sprite.x = d.x;
		sprite.y = d.y;
		sprite.angle = d.angle;
		sprite.separateScale = true;
		sprite.scaleX = d.scaleX;
		sprite.scaleY = d.scaleY;
		sprite.pixelpos = false;
		sprite.filter = FILTER_MIPMAP;
		sprite.draw();

	#if 0
		if (k == 0)
		{
			printf("transform @ t=%+04.2f: (%+04.2f, %+04.2f) @ %+04.2f x (%+04.2f, %+04.2f) * %+04.2f\n",
				t,
				tf.x, tf.y,
				tf.angle,
				tf.scaleX,
				tf.scaleY,
				tf.a);
		}
	#endif
	}

	setColor(oldColor);

	gxPopMatrix();
}

int Spriter::getAnimCount() const
{
	if (m_spriter->m_scene->m_entities.empty())
		return 0;
	return (int)m_spriter->m_scene->m_entities[0]->m_animations.size();
}

int Spriter::getAnimIndexByName(const char * name) const
{
	if (m_spriter->m_scene->m_entities.empty())
		return -1;
	return m_spriter->m_scene->m_entities[0]->getAnimIndexByName(name);
}

float Spriter::getAnimLength(int animIndex) const
{
	if (animIndex == -1)
		return 0.f;
	fassert(!m_spriter->m_scene->m_entities.empty());
	return m_spriter->m_scene->m_entities[0]->getAnimLength(animIndex);
}

bool Spriter::isAnimDoneAtTime(int animIndex, float time) const
{
	if (animIndex == -1)
		return true;
	fassert(!m_spriter->m_scene->m_entities.empty());
	if (m_spriter->m_scene->m_entities.empty())
		return true;
	if (m_spriter->m_scene->m_entities[0]->isAnimLooped(animIndex))
		return false;
	return time >= getAnimLength(animIndex) / 1000.f;
}

bool Spriter::getHitboxAtTime(int animIndex, const char * name, float time, Vec2 * points)
{
	if (animIndex == -1)
		return true;
	fassert(!m_spriter->m_scene->m_entities.empty());
	spriter::Hitbox hitbox;
	if (!m_spriter->m_scene->m_entities[0]->getHitboxAtTime(animIndex, name, time * 1000.f, hitbox))
		return false;

	const float x1 = 0.f;
	const float x2 = +hitbox.sx;
	const float y1 = 0.f;
	const float y2 = +hitbox.sy;

	Vec3 p1(x1, y1, 0.f);
	Vec3 p2(x2, y1, 0.f);
	Vec3 p3(x2, y2, 0.f);
	Vec3 p4(x1, y2, 0.f);

	Mat4x4 matT;
	Mat4x4 matR;
	Mat4x4 matS;
	
	matT.MakeTranslation(hitbox.x, hitbox.y, 0.f);
	matR.MakeRotationZ(-hitbox.angle / 180.f * M_PI);
	matS.MakeScaling(hitbox.scaleX, hitbox.scaleY, 1.f);

	Mat4x4 mat = matT * matR * matS;

	p1 = mat * p1;
	p2 = mat * p2;
	p3 = mat * p3;
	p4 = mat * p4;

	points[0] = Vec2(p1[0], p1[1]);
	points[1] = Vec2(p2[0], p2[1]);
	points[2] = Vec2(p3[0], p3[1]);
	points[3] = Vec2(p4[0], p4[1]);

	return true;
}

bool Spriter::hasCharacterMap(int index) const
{
	if (m_spriter->m_scene->m_entities.empty())
		return false;

	return m_spriter->m_scene->hasCharacterMap(index);
}

spriter::Scene * Spriter::getSpriterScene() const
{
	return m_spriter->m_scene;
}

// -----

Sound::Sound(const char * filename)
{
	m_sound = &g_soundCache.findOrCreate(filename);
	m_playId = -1;
	m_volume = 100;
	m_speed = 100;
}

void Sound::play(int volume, int speed)
{
	if (volume == -1)
		volume = m_volume;
	if (speed == -1)
		speed = m_speed;
	
	volume = clamp(volume, 0, 100);
	speed = std::max(0, speed);
	
	stop();
	
	if (m_sound->buffer != 0)
	{
		m_playId = g_soundPlayer.playSound(m_sound->buffer, volume / 100.f, false);
	}
}

void Sound::stop()
{
	if (m_playId != -1)
	{
		g_soundPlayer.stopSound(m_playId);
		m_playId = -1;
	}
}

void Sound::setVolume(int volume)
{
	m_volume = volume;
	
	if (m_playId != -1)
	{
		g_soundPlayer.setSoundVolume(m_playId, volume / 100.f);
	}
}

void Sound::setSpeed(int speed)
{
	m_speed = speed;
	
	if (m_playId != -1)
	{
		// todo
	}
}

void Sound::stopAll()
{
	g_soundPlayer.stopAllSounds();
}

// -----

Music::Music(const char * filename)
{
	m_filename = filename;
}

void Music::play(bool loop)
{
	g_soundPlayer.playMusic(m_filename.c_str(), loop);
}

void Music::stop()
{
	g_soundPlayer.stopMusic();
}

void Music::setVolume(int volume)
{
	g_soundPlayer.setMusicVolume(volume / 100.f);
}

// -----

Font::Font(const char * filename)
{
	m_font = &g_fontCache.findOrCreate(filename);
	
	m_fontMSDF = &g_fontCacheMSDF.findOrCreate(filename);
}

bool Font::saveCache(const char * _filename) const
{
	const std::string filename = _filename ? _filename : (m_fontMSDF->m_filename + ".cache");
	
	return m_fontMSDF->m_glyphCache->saveCache(filename.c_str());
}

bool Font::loadCache(const char * _filename)
{
	const std::string filename = _filename ? _filename : (m_fontMSDF->m_filename + ".cache");
	
	return m_fontMSDF->m_glyphCache->loadCache(filename.c_str());
}

// -----

void Path2d::PathElem::lineHeading(float & x, float & y) const
{
	x = v2.x - v1.x;
	y = v2.y - v1.y;
}

void Path2d::PathElem::curveHeading(float & x, float & y, const float t) const
{
	// taken from libcinder

	const float t1 = 1.f - t;
	const float t2 = t;

	const float w0 = -3.f * t1 * t1;
	const float w1 = +3.f * t1 * t1 - 6.f * t2 * t1;
	const float w2 = -3.f * t2 * t2 + 6.f * t2 * t1;
	const float w3 = +3.f * t2 * t2;

	x = w0 * v1.x + w1 * v2.x + w2 * v3.x + w3 * v4.x;
	y = w0 * v1.y + w1 * v2.y + w2 * v3.y + w3 * v4.y;
}

void Path2d::PathElem::curveEval(float & x, float & y, const float t) const
{
	const float t1 = 1.f - t;
	const float t2 =       t;

	const float a = t1 * t1 * t1;
	const float b = t1 * t1 * t2 * 3.f;
	const float c = t1 * t2 * t2 * 3.f;
	const float d = t2 * t2 * t2;

	x =
		v1.x * a +
		v2.x * b +
		v3.x * c +
		v4.x * d;

	y =
		v1.y * a +
		v2.y * b +
		v3.y * c +
		v4.y * d;
}

void Path2d::PathElem::curveSubdiv(const float t1, const float t2, float *& xy, float *& hxy, int & numPoints) const
{
	const float eps = .1f;
	const float flatnessEps = .1f;

	const float tm = (t1 + t2) * .5f;
	
	float px1;
	float py1;
	float px2;
	float py2;
	float px3;
	float py3;

	curveEval(px1, py1, t1);
	curveEval(px2, py2, tm);
	curveEval(px3, py3, t2);

	const float dx = px3 - px1;
	const float dy = py3 - py1;
	const float ds = std::hypot(dx, dy);

	bool needsSubdiv = true;

	if (ds <= eps)
		needsSubdiv = false;

	if (needsSubdiv)
	{
		const float nx = +dy / ds;
		const float ny = -dx / ds;

		const float d1 = px1 * nx + py1 * ny;
		const float d2 = px2 * nx + py2 * ny;
		//const float d3 = px3 * nx + py3 * ny;

		const float dd = d2 - d1;

		if (std::abs(dd) <= flatnessEps)
			needsSubdiv = false;
	}

	if (needsSubdiv)
	{
		curveSubdiv(t1, tm, xy, hxy, numPoints);

		if (numPoints >= 1)
		{
			*xy++ = px2;
			*xy++ = py2;

			if (hxy != nullptr)
			{
				curveHeading(hxy[0], hxy[1], tm);
				hxy += 2;
			}

			numPoints -= 1;
		}

		curveSubdiv(tm, t2, xy, hxy, numPoints);
	}
	else
	{
		if (numPoints >= 1)
		{
			*xy++ = px3;
			*xy++ = py3;

			if (hxy != nullptr)
			{
				curveHeading(hxy[0], hxy[1], t2);
				hxy += 2;
			}

			numPoints -= 1;
		}
	}
}

void Path2d::PathElem::curveSubdiv(const Path2d::Vertex & v1, const Path2d::Vertex & v2, const Path2d::Vertex & v3, float *& xy, float *& hxy, int & numPoints) const
{
#if 1
	const float eps = 1.f;

	Vertex v12 = (v1 + v2) * .5f;
	Vertex v23 = (v2 + v3) * .5f;

	Vertex vm = (v12 + v23) * .5f;

	Vertex d = v3 - v1;

	const float ds = d.len();

	bool needsSubdiv = true;

	if (ds <= eps)
		needsSubdiv = false;

	if (needsSubdiv)
	{
		Vertex n = d / ds;

		const float d1 = n.dot(v1);
		const float d2 = n.dot(v2);
		//const float d3 = n.dot(v3);

		const float dd = d2 - d1;

		if (std::abs(dd) <= eps)
			needsSubdiv = false;
	}

	if (needsSubdiv)
	{
		curveSubdiv(v1, v12, vm, xy, hxy, numPoints);
		curveSubdiv(vm, v23, v3, xy, hxy, numPoints);
	}
	else
	{
		if (numPoints >= 1)
		{
			*xy++ = vm.x;
			*xy++ = vm.y;

			if (hxy != nullptr)
			{
				hxy[0] = 0.f;
				hxy[1] = 0.f;

				hxy += 2;
			}

			numPoints -= 1;
		}
	}
#endif
}

//

void Path2d::moveTo(const float x, const float y)
{
	fassert(hasMove == false);

	this->x = x;
	this->y = y;
	this->hasMove = true;
}

void Path2d::lineTo(const float x, const float y)
{
	fassert(hasMove);

	PathElem & elem = allocElem();
	elem.type = ELEM_LINE;

	elem.v1.x = this->x;
	elem.v1.y = this->y;

	this->x = x;
	this->y = y;

	elem.v2.x = this->x;
	elem.v2.y = this->y;
}

void Path2d::line(const float dx, const float dy)
{
	fassert(hasMove);

	PathElem & elem = allocElem();
	elem.type = ELEM_LINE;

	elem.v1.x = this->x;
	elem.v1.y = this->y;

	this->x += dx;
	this->y += dy;

	elem.v2.x = this->x;
	elem.v2.y = this->y;
}

void Path2d::curveTo(const float x, const float y, const float tx1, const float ty1, const float tx2, const float ty2)
{
	fassert(hasMove);

	PathElem & elem = allocElem();
	elem.type = ELEM_CURVE;

	const float x1 = this->x;
	const float y1 = this->y;
	const float x2 = x;
	const float y2 = y;

	elem.v1.x = x1;
	elem.v1.y = y1;

	elem.v2.x = x1 + tx1;
	elem.v2.y = y1 + ty1;

	elem.v3.x = x2 + tx2;
	elem.v3.y = y2 + ty2;

	elem.v4.x = x2;
	elem.v4.y = y2;

	this->x = x2;
	this->y = y2;
}

void Path2d::curveToAbs(const float x, const float y, const float cx1, const float cy1, const float cx2, const float cy2)
{
	fassert(hasMove);

	PathElem & elem = allocElem();
	elem.type = ELEM_CURVE;

	const float x1 = this->x;
	const float y1 = this->y;
	const float x2 = x;
	const float y2 = y;

	elem.v1.x = x1;
	elem.v1.y = y1;

	elem.v2.x = cx1;
	elem.v2.y = cy1;

	elem.v3.x = cx2;
	elem.v3.y = cy2;

	elem.v4.x = x2;
	elem.v4.y = y2;

	this->x = x2;
	this->y = y2;
}

void Path2d::curve(const float dx, const float dy, const float tx1, const float ty1, const float tx2, const float ty2)
{
	curveTo(x + dx, y + dy, tx1, ty1, tx2, ty2);
	/*
	fassert(hasMove);

	PathElem & elem = allocElem();
	elem.type = ELEM_CURVE;

	const float x1 = this->x;
	const float y1 = this->y;
	const float x2 = x1 + dx;
	const float y2 = y1 + dy;

	elem.x1 = x1;
	elem.y1 = y1;

	elem.x2 = x1 + tx1;
	elem.y2 = y1 + ty1;

	elem.x3 = x2 + tx2;
	elem.y3 = y2 + ty2;

	elem.x4 = x2;
	elem.y4 = y2;

	this->x = x2;
	this->y = y2;
	*/
}

void Path2d::arc(const float angle, const float radius)
{
}

void Path2d::close()
{
	if (elems.size() > 0)
	{
		const PathElem & e = elems.front();

		lineTo(e.v1.x, e.v1.y);
	}
}

void Path2d::generatePoints(float * xy, float * hxy, const int maxPoints, const float curveFlatness, int & numPoints) const
{
	numPoints = 0;

	for (const PathElem & elem : elems)
	{
		if (elem.type == ELEM_LINE)
		{
			if (numPoints + 2 <= maxPoints)
			{
				*xy++ = elem.v1.x;
				*xy++ = elem.v1.y;
				*xy++ = elem.v2.x;
				*xy++ = elem.v2.y;

				if (hxy != nullptr)
				{
					elem.lineHeading(hxy[0], hxy[1]);
					hxy += 2;

					elem.lineHeading(hxy[0], hxy[1]);
					hxy += 2;
				}

				numPoints += 2;
			}
			else
			{
				break;
			}
		}
		else if (elem.type == ELEM_CURVE)
		{
#if 1
			int todoPoints = maxPoints - numPoints;

			if (todoPoints >= 1)
			{
				elem.curveEval(xy[0], xy[1], 0.f);
				xy += 2;
				
				elem.curveHeading(hxy[0], hxy[1], 0.f);
				hxy += 2;
				
				todoPoints--;
			}

			elem.curveSubdiv(0.f, 1.f, xy, hxy, todoPoints);

			numPoints = maxPoints - todoPoints;
#elif 0
			float t = 0.f;

			while (t < 1.f && numPoints < maxPoints)
			{
				elem.curveEval(xy[0], xy[1], t);
				xy += 2;

				float hx;
				float hy;
				elem.curveHeading(hx, hy, t);

				if (hxy != nullptr)
				{
					hxy[0] = hx;
					hxy[1] = hy;
					hxy += 2;
				}

				numPoints++;

				const float hs = std::sqrt(hx * hx + hy * hy);
				const float dt = std::sqrt(hs) / 1000.f;

				t += dt;
			}
#else
			const int kNumPoints = 100;

			const Vertex v1 = (elem.v1 + elem.v2) * .5f;
			const Vertex v2 = (elem.v3 + elem.v4) * .5f;
			const Vertex vm = (v1 + v2) * .5f;
			const Vertex d1 = vm - elem.v1;
			const Vertex d2 = vm - elem.v4;
			const float l1 = d1.len();
			const float l2 = d2.len();
			const float lengthEstimate = l1 + l2;

			if (numPoints + kNumPoints <= maxPoints)
			{
				for (int i = 0; i < kNumPoints; ++i)
				{
					const float t = i / (kNumPoints - 1.f);

					elem.curveEval(xy[0], xy[1], t);
					xy += 2;

					if (hxy != nullptr)
					{
						elem.curveHeading(hxy[0], hxy[1], t);
						hxy += 2;
					}
				}

				numPoints += kNumPoints;
			}
			else
			{
				break;
			}
#endif
		}
		else if (elem.type == ELEM_ARC)
		{
		}
	}
}

// -----

static int getButtonIndex(BUTTON button)
{
	switch (button)
	{
	case BUTTON_LEFT:
		return 0;
	case BUTTON_RIGHT:
		return 1;
	default:
		fassert(false);
	}
	return -1;
}

bool Mouse::isDown(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	return globals.mouseDown[index];
}

bool Mouse::wentDown(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	return isDown(button) && globals.mouseChange[index];
}

bool Mouse::wentUp(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	return !isDown(button) && globals.mouseChange[index];
}

void Mouse::showCursor(bool enabled)
{
	SDL_ShowCursor(enabled ? 1 : 0);
}

void Mouse::setRelative(bool isRelative)
{
	SDL_SetRelativeMouseMode(isRelative ? SDL_TRUE : SDL_FALSE);
	SDL_CaptureMouse(isRelative ? SDL_TRUE : SDL_FALSE);
}

bool Mouse::isIdle() const
{
	return dx == 0 && dy == 0 && !globals.mouseChange[0] && !globals.mouseChange[1];
}

// -----

bool Keyboard::isDown(SDLKey key) const
{
	for (int i = 0; i < globals.keyDownCount; ++i)
		if (globals.keyDown[i] == key)
			return true;
	return false;
}

static bool keyChange(SDLKey key)
{
	for (int i = 0; i < globals.keyChangeCount; ++i)
		if (globals.keyChange[i] == key)
			return true;
	return false;
}

bool Keyboard::wentDown(SDLKey key, bool allowRepeat) const
{
	return (isDown(key) && keyChange(key)) || (allowRepeat && keyRepeat(key));
}

bool Keyboard::wentUp(SDLKey key) const
{
	return !isDown(key) && keyChange(key);
}

bool Keyboard::keyRepeat(SDLKey key) const
{
	for (int i = 0; i < globals.keyRepeatCount; ++i)
		if (globals.keyRepeat[i] == key)
			return true;
	return false;
}

bool Keyboard::isIdle() const
{
	return
		globals.keyDownCount == 0 &&
		globals.keyChangeCount == 0;
}

// -----

Gamepad::Gamepad()
{
	memset(this, 0, sizeof(Gamepad));
}

bool Gamepad::isDown(GAMEPAD button) const
{
	return m_isDown[button];
}

bool Gamepad::wentDown(GAMEPAD button) const
{
	return m_wentDown[button];
}

bool Gamepad::wentUp(GAMEPAD button) const
{
	return m_wentUp[button];
}

float Gamepad::getAnalog(int stick, ANALOG analog, float scale) const
{
	fassert(stick >= 0 && stick <= 1);
	if (stick >= 0 && stick <= 1)
		return m_analog[stick][analog] * scale;
	else
		return 0.f;
}

void Gamepad::vibrate(float duration, float strength)
{
	fassert(strength >= 0.f && strength <= 1.f);
	if (strength < 0.f)
		strength = 0.f;
	if (strength > 1.f)
		strength = 1.f;
	m_vibrationDuration = duration;
	m_vibrationStrength = strength;
}

// -----

Midi::Midi()
	: isConnected(false)
{
}

bool Midi::isDown(int key) const
{
	if (key >= 0 && key < 256)
		return globals.midiDown[key];
	else
		return false;
}

bool Midi::wentDown(int key) const
{
	if (key >= 0 && key < 256)
		return globals.midiDown[key] && globals.midiChange[key];
	else
		return false;
}

bool Midi::wentUp(int key) const
{
	if (key >= 0 && key < 256)
		return !globals.midiDown[key] && globals.midiChange[key];
	else
		return false;
}

float Midi::getValue(int key, float _default) const
{
	if (globals.midiIsSet[key] && isDown(key))
		return globals.midiValue[key];
	else
		return _default;
}

// -----

StageObject::StageObject()
{
	isDead = false;
	sprite = 0;
}

StageObject::~StageObject()
{
	delete sprite;
	sprite = 0;
}

void StageObject::process(float timeStep)
{
}

void StageObject::draw()
{
	if (sprite)
	{
		sprite->draw();
	}
}

// -----

StageObject_SpriteAnim::StageObject_SpriteAnim(const char * name, const char * anim, const char * sheet)
{
	sprite = new Sprite(name, 0.f, 0.f, sheet);
	
	sprite->startAnim(anim);
}

void StageObject_SpriteAnim::process(float timeStep)
{
	StageObject::process(timeStep);
	
	if (!sprite->animIsActive)
	{
		isDead = true;
	}
}

// -----

Stage::Stage()
{
	m_objectId = 0;
}

void Stage::process(float timeStep)
{
	for (ObjectList::iterator i = m_objects.begin(); i != m_objects.end(); )
	{
		StageObject * obj = i->second;
		
		ObjectList::iterator next = i;
		++next;
		
		fassert(!obj->isDead);
		obj->process(timeStep);
		
		if (obj->isDead)
		{
			delete obj;
			m_objects.erase(i);
		}
		
		i = next;
	}
}

void Stage::draw()
{
	for (ObjectList::iterator i = m_objects.begin(); i != m_objects.end(); ++i)
	{
		StageObject * obj = i->second;
		
		obj->draw();
	}
}

int Stage::addObject(StageObject * object)
{
	const int objectId = m_objectId++;
	
	m_objects.insert(ObjectList::value_type(objectId, object));
	
	return objectId;
}

void Stage::removeObject(int objectId)
{
	ObjectList::iterator i = m_objects.find(objectId);
	
	if (i != m_objects.end())
	{
		logInfo("removing object with object ID %d", objectId);
		StageObject * obj = i->second;
		delete obj;
		m_objects.erase(i);
	}
	else
	{
		logInfo("removing object with object ID %d (but already dead)", objectId);
	}
}

// -----

Ui::Ui()
	: m_ui(0)
	, m_ownsUi(false)
{
	m_ui = new UiCacheElem();
	m_ownsUi = true;
}

Ui::Ui(const char * filename)
	: m_ui(0)
	, m_ownsUi(false)
{
	load(filename);
}

Ui::~Ui()
{
	if (m_ownsUi)
	{
		m_ownsUi = false;

		delete m_ui;
		m_ui = 0;
	}
}

void Ui::load(const char * filename)
{
	if (m_ownsUi)
	{
		m_ownsUi = false;

		delete m_ui;
		m_ui = 0;
	}

	m_ui = &g_uiCache.findOrCreate(filename);

	m_over.clear();
	m_down.clear();
}

std::string Ui::getImage(Dictionary & d)
{
	const std::string name = d.getString("name", "");
	const std::string type = d.getString("type", "");
	const bool isOver = name == m_over;
	const bool isDown = name == m_down && type == "button";
	const std::string filenameDefault = d.getString("image", "");
	const std::string filenameOver = d.getString("image_over", filenameDefault.c_str());
	const std::string filenameDown = d.getString("image_down", filenameDefault.c_str());
	const std::string & filename = (isDown && m_over == m_down) ? filenameDown : isOver ? filenameOver : filenameDefault;
	return filename;
}

bool Ui::getArea(Dictionary & d, int & x, int & y, int & sx, int & sy)
{
	const std::string type = d.getString("type", "");
	
	if (type == "image" || type == "button")
	{
		const Sprite sprite(getImage(d).c_str());
		const float scale = d.getFloat("scale", 1.f);
		x = d.getInt("x", 0);
		y = d.getInt("y", 0);
		sx = int(d.getInt("w", sprite.getWidth()) * scale);
		sy = int(d.getInt("h", sprite.getHeight()) * scale);
		return true;
	}
	else
	{
		return false;
	}
}

std::string Ui::findMouseOver()
{
	for (UiCacheElem::Map::iterator i = m_ui->map.begin(); i != m_ui->map.end(); ++i)
	{
		Dictionary & d = i->second;
		
		int x, y, sx, sy;
		
		if (getArea(d, x, y, sx, sy))
		{
			if (mouse.x >= x && mouse.x < x + sx && mouse.y >= y && mouse.y < y + sy)
			{
				return i->first;
			}
		}
	}
	
	return "";
}

void Ui::process()
{
	// make sure the names are good..
	
	for (UiCacheElem::Map::iterator i = m_ui->map.begin(); i != m_ui->map.end(); ++i)
	{
		Dictionary & d = i->second;
		
		d.setString("name", i->first.c_str());
	}
	
	// update mouse over
	
	std::string over = findMouseOver();
	
	m_over = over;
	
	// update mouse down
	
	if (mouse.wentDown(BUTTON_LEFT))
	{
		m_down = m_over;
	}
	else if (mouse.wentUp(BUTTON_LEFT))
	{
		if (!m_down.empty())
		{
			if (m_down == m_over)
			{
				// trigger action, if set
				Dictionary & d = m_ui->map[m_down];
				const std::string action = d.getString("action", "");
				if (!action.empty())
					framework.processActions(action, d);
			}
			m_down.clear();
		}
	}
}

void Ui::draw()
{
	for (UiCacheElem::Map::iterator i = m_ui->map.begin(); i != m_ui->map.end(); ++i)
	{
		Dictionary & d = i->second;
		
		const bool isVisible = d.getBool("visible", true);
		
		if (isVisible)
		{
			const std::string type = d.getString("type", "");
			
			if (type == "image" || type == "button")
			{
				Sprite(getImage(d).c_str()).drawEx(
					float(d.getInt("x", 0)),
					float(d.getInt("y", 0)),
					0.f,
					d.getFloat("scale", 1.f));

				const std::string text = d.getString("text", "");

				if (!text.empty())
				{
					int x, y, sx, sy;

					if (getArea(d, x, y, sx, sy))
					{
						Font font(d.getString("font", "" ).c_str());
						setFont(font);

						// todo : set text color

						drawText(x + sx/2.f, y + sy/2.f, d.getInt("font_size", 10), 0.f, 0.f, "%s", text.c_str());
					}
				}
			}
		}
	}
}

void Ui::remove(const char * name)
{
	m_ui->map.erase(name);
}

void Ui::clear()
{
	m_ui->map.clear();
}

Dictionary & Ui::operator[](const char * name)
{
	UiCacheElem::Map::iterator i = m_ui->map.find(name);

	if (i == m_ui->map.end())
	{
		Dictionary d;
		d.setString("name", name);

		i = m_ui->map.insert(UiCacheElem::Map::value_type(name, d)).first;
	}

	return i->second;
}

// -----

void clearCaches(int caches)
{
	if (caches & CACHE_FONT)
	{
		g_fontCache.clear();
		g_glyphCache.clear();
	}
	
	if (caches & CACHE_FONT_MSDF)
		g_fontCacheMSDF.clear();

	if (caches & CACHE_SHADER)
		g_shaderCache.clear();

	if (caches & CACHE_SOUND)
		g_soundCache.clear();

	if (caches & CACHE_SPRITE)
		g_animCache.clear();

	if (caches & CACHE_SPRITER)
		g_spriterCache.clear();

	if (caches & CACHE_TEXTURE)
		g_textureCache.clear();
}

// -----

static const int kMaxSurfaceStackSize = 32;
static Surface * surfaceStack[kMaxSurfaceStackSize] = { };
static int surfaceStackSize = 0;

void setTransform(TRANSFORM transform)
{
	globals.transform = transform;
	
	applyTransform();
}

void applyTransform()
{
	Surface * surface = surfaceStackSize ? surfaceStack[surfaceStackSize - 1] : 0;
	const float sx = surface ? surface->getWidth() : globals.displaySize[0];
	const float sy = surface ? surface->getHeight() : globals.displaySize[1];
	applyTransformWithViewportSize(sx, sy);
}

void applyTransformWithViewportSize(const float sx, const float sy)
{
	// calculate screen matrix (we need it to transform vertices to screen space)
	{
		gxMatrixMode(GL_PROJECTION);
		gxPushMatrix();
		{
			gxLoadIdentity();
			
			if (surfaceStackSize == 0)
			{
				// flip Y axis so the vertical axis runs top to bottom
				gxScalef(1.f, -1.f, 1.f);
			}
		
			// convert from (0,0),(1,1) to (-1,-1),(+1+1)
			gxTranslatef(-1.f, -1.f, 0.f);
			gxScalef(2.f, 2.f, 1.f);
			
			// convert from (0,0),(sx,sy) to (0,0),(1,1)
			gxScalef(1.f / sx, 1.f / sy, 1.f);
			
			// capture transform
			gxGetMatrixf(GL_PROJECTION, globals.transformScreen.m_v);
			checkErrorGL();
		}
		gxPopMatrix();
	}
	
	// apply current transform
	
	gxMatrixMode(GL_PROJECTION);
	
	switch (globals.transform)
	{
		case TRANSFORM_SCREEN:
		{
			gxLoadMatrixf(globals.transformScreen.m_v);
			break;
		}
		case TRANSFORM_2D:
		{
			gxLoadMatrixf(globals.transform2d.m_v);
			break;
		}
		case TRANSFORM_3D:
		{
			gxLoadMatrixf(globals.transform3d.m_v);
			break;
		}
		default:
		{
			fassert(false);
			gxLoadIdentity();
			break;
		}
	}
	
	gxMatrixMode(GL_MODELVIEW);
	gxLoadIdentity();
}

void setTransform2d(const Mat4x4 & transform)
{
	globals.transform2d = transform;
}

void setTransform3d(const Mat4x4 & transform)
{
	globals.transform3d = transform;
}

Vec2 transformToScreen(const Vec3 & v)
{
	Mat4x4 matP;
	Mat4x4 matM;
	
	gxGetMatrixf(GL_PROJECTION, matP.m_v);
	gxGetMatrixf(GL_MODELVIEW, matM.m_v);
	checkErrorGL();
	
	// from current transfor to view
	
	Vec4 t = matP * matM * Vec4(v[0], v[1], v[2], 1.f);
	
	// perspective divide
	
	if (t[3] != 0.f)
		t /= t[3];
	
	// and back to screen coordinates
	
	Mat4x4 viewToScreen = globals.transformScreen.CalcInv();
	
	Vec3 s = viewToScreen * Vec3(t[0], t[1], t[2]);
	
	return Vec2(s[0], s[1]);
}

static void setSurface(Surface * surface)
{
	const GLuint framebuffer = surface ? surface->getFramebuffer() : 0;
	const GLsizei sx = surface ? surface->getWidth() : globals.actualDisplaySize[0];
	const GLsizei sy = surface ? surface->getHeight() : globals.actualDisplaySize[1];
	
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glViewport(0, 0, sx / framework.minification, sy / framework.minification);
	
	applyTransform();
}

void pushSurface(Surface * surface)
{
	gxMatrixMode(GL_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GL_MODELVIEW);
	gxPushMatrix();

	fassert(surfaceStackSize < kMaxSurfaceStackSize);
	surfaceStack[surfaceStackSize++] = surface;
	setSurface(surface);
	checkErrorGL();
}

void popSurface()
{
	fassert(surfaceStackSize > 0);
	surfaceStack[--surfaceStackSize] = 0;
	Surface * surface = surfaceStackSize ? surfaceStack[surfaceStackSize - 1] : 0;
	setSurface(surface);
	checkErrorGL();

	gxMatrixMode(GL_PROJECTION);
	gxPopMatrix();
	gxMatrixMode(GL_MODELVIEW);
	gxPopMatrix();
}

void setDrawRect(int x, int y, int sx, int sy)
{
	y = globals.displaySize[1] - y - sy;
	
	x /= framework.minification;
	y /= framework.minification;
	sx /= framework.minification;
	sy /= framework.minification;
	
	x += globals.drawableOffset[0];
	y += globals.drawableOffset[1];

	glScissor(x, y, sx, sy);
	glEnable(GL_SCISSOR_TEST);
}

void clearDrawRect()
{
	glDisable(GL_SCISSOR_TEST);
}

void setBlend(BLEND_MODE blendMode)
{
	globals.blendMode = blendMode;
	
	switch (blendMode)
	{
	case BLEND_OPAQUE:
		glDisable(GL_BLEND);
		break;
	case BLEND_ALPHA:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_PREMULTIPLIED_ALPHA:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		if (glBlendFuncSeparate)
			glBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_PREMULTIPLIED_ALPHA_DRAW:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		if (glBlendFuncSeparate)
			glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_ADD:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BLEND_ADD_OPAQUE:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case BLEND_SUBTRACT:
		glEnable(GL_BLEND);
		fassert(glBlendEquation);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case BLEND_INVERT:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		break;
	case BLEND_MUL:
		glEnable(GL_BLEND);
		if (glBlendEquation)
			glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		break;
	default:
		fassert(false);
		break;
	}
}

static const int kMaxBlendStackSize = 32;
static BLEND_MODE blendStack[kMaxBlendStackSize] = { BLEND_ALPHA };
static int blendStackSize = 0;

void pushBlend(BLEND_MODE blendMode)
{
	fassert(blendStackSize < kMaxBlendStackSize);
	blendStack[blendStackSize++] = globals.blendMode;
	setBlend(blendMode);
}

void popBlend()
{
	fassert(blendStackSize > 0);
	--blendStackSize;
	const BLEND_MODE blendMode = blendStack[blendStackSize];
	blendStack[blendStackSize] = BLEND_ALPHA;
	setBlend(blendMode);
}

void setColorMode(COLOR_MODE colorMode)
{
	globals.colorMode = colorMode;
	
#if USE_LEGACY_OPENGL
	switch (colorMode)
	{
	case COLOR_MUL:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case COLOR_ADD:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		break;
	case COLOR_SUB:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_SUBTRACT);
		break;
	case COLOR_IGNORE:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	default:
		fassert(false);
		break;
	}
#endif
}

void setColor(const Color & color)
{
	setColorf(color.r, color.g, color.b, color.a);
}

void setColor(int r, int g, int b, int a, int rgbMul)
{
	setColorf(scale255(r), scale255(g), scale255(b), scale255(a), scale255(rgbMul));
}

void setColorf(float r, float g, float b, float a, float rgbMul)
{
	r *= rgbMul;
	g *= rgbMul;
	b *= rgbMul;
	
#if 1
	r = clamp(r, 0.f, 1.f);
	g = clamp(g, 0.f, 1.f);
	b = clamp(b, 0.f, 1.f);
	a = clamp(a, 0.f, 1.f);
#endif

	globals.color.r = r;
	globals.color.g = g;
	globals.color.b = b;
	globals.color.a = a;
	
	#if 0
	glColor4f(r, g, b, a);
	#else
	gxColor4f(r, g, b, a);
	#endif
}

void setAlpha(int a)
{
	globals.color.a = scale255(a);
}

void setAlphaf(float a)
{
	globals.color.a = a;
}

void setGradientf(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2)
{
	globals.gradient.set(x1, y1, color1, x2, y2, color2);
}

void setGradientf(float x1, float y1, float r1, float g1, float b1, float a1, float x2, float y2, float r2, float g2, float b2, float a2)
{
	setGradientf(x1, y1, Color(r1, g1, b1, a1), x2, y2, Color(r2, g2, b2, a2));
}

void setFont(const Font & font)
{
	globals.font = const_cast<Font&>(font).getFont();
	
	globals.fontMSDF = const_cast<Font&>(font).getFontMSDF();
}

void setFont(const char * font)
{
	setFont(Font(font));
}

static void setFontMode(FONT_MODE fontMode)
{
	globals.fontMode = fontMode;
}

static const int kMaxFontModeStackSize = 32;
static FONT_MODE fontModeStack[kMaxFontModeStackSize] = { FONT_BITMAP };
static int fontModeStackSize = 0;

void pushFontMode(FONT_MODE fontMode)
{
	fassert(fontModeStackSize < kMaxFontModeStackSize);
	fontModeStack[fontModeStackSize++] = globals.fontMode;
	setFontMode(fontMode);
}

void popFontMode()
{
	fassert(fontModeStackSize > 0);
	--fontModeStackSize;
	const FONT_MODE fontMode = fontModeStack[fontModeStackSize];
	fontModeStack[fontModeStackSize] = FONT_BITMAP;
	setFontMode(fontMode);
}

void setShader(const ShaderBase & shader)
{
	if (&shader != globals.shader)
	{
		globals.shader = const_cast<ShaderBase*>(&shader);
	
		glUseProgram(shader.getProgram());
		
		globals.gxShaderIsDirty = true;
	}
}

void clearShader()
{
	if (globals.shader)
	{
		globals.shader = 0;
	
		glUseProgram(0);
	}
}

void shaderSource(const char * filename, const char * text)
{
	framework.registerShaderSource(filename, text);
}

void drawPoint(float x, float y)
{
	gxBegin(GL_POINTS);
	{
		gxVertex2f(x, y);
	}
	gxEnd();
}

void drawLine(float x1, float y1, float x2, float y2)
{
	gxBegin(GL_LINES);
	{
		gxVertex2f(x1, y1);
		gxVertex2f(x2, y2);
	}
	gxEnd();
}

void drawRect(float x1, float y1, float x2, float y2)
{
	gxBegin(GL_QUADS);
	{
		gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
		gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
		gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
		gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	}
	gxEnd();
}

void drawRectLine(float x1, float y1, float x2, float y2)
{
	gxBegin(GL_LINE_LOOP);
	{
		gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
		gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
		gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
		gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	}
	gxEnd();
}

void drawRectGradient(float x1, float y1, float x2, float y2)
{
	float oldColor[4];
	glGetFloatv(GL_CURRENT_COLOR, oldColor);

	const Color color[4] =
	{
		globals.gradient.eval(x1, y1),
		globals.gradient.eval(x2, y1),
		globals.gradient.eval(x2, y2),
		globals.gradient.eval(x1, y2)
	};
	
	gxBegin(GL_QUADS);
	{
		gxColor4f(color[0].r, color[0].g, color[0].b, color[0].a);
		gxVertex2f(x1, y1);
		gxColor4f(color[1].r, color[1].g, color[1].b, color[1].a);
		gxVertex2f(x2, y1);
		gxColor4f(color[2].r, color[2].g, color[2].b, color[2].a);
		gxVertex2f(x2, y2);
		gxColor4f(color[3].r, color[3].g, color[3].b, color[3].a);
		gxVertex2f(x1, y2);
	}
	gxEnd();
	
	setColorf(oldColor[0], oldColor[1], oldColor[2], oldColor[3]);
}

void drawCircle(float x, float y, float radius, int numSegments)
{
	gxBegin(GL_LINE_LOOP);
	{
		for (int i = 0; i < numSegments; ++i)
		{
			const float angle = i * (M_PI * 2.f / numSegments);

			gxVertex2f(
				x + std::cosf(angle) * radius,
				y + std::sinf(angle) * radius);
		}
	}
	gxEnd();
}

void fillCircle(float x, float y, float radius, int numSegments)
{
	gxBegin(GL_TRIANGLES);
	{
		for (int i = 0; i < numSegments; ++i)
		{
			const float angle1 = (i + 0) * (M_PI * 2.f / numSegments);
			const float angle2 = (i + 1) * (M_PI * 2.f / numSegments);

			gxVertex2f(
				x,
				y);

			gxVertex2f(
				x + std::cosf(angle1) * radius,
				y + std::sinf(angle1) * radius);
			gxVertex2f(
				x + std::cosf(angle2) * radius,
				y + std::sinf(angle2) * radius);
		}
	}
	gxEnd();
}

static void measureText(FT_Face face, int size, const GlyphCacheElem ** glyphs, const int numGlyphs, float & sx, float & sy, float & yTop)
{
	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float maxY = std::numeric_limits<float>::min();
	
	float x = 0.f;
	float y = 0.f;
	
	//y += size;
	
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCacheElem & elem = *glyphs[i];
		
	#if USE_GLYPH_ATLAS
		if (elem.textureAtlasElem != nullptr)
	#else
		if (elem.texture != 0)
	#endif
		{
			const float bsx = float(elem.g.bitmap.width);
			const float bsy = float(elem.g.bitmap.rows);
			const float x1 = x + elem.g.bitmap_left;
			const float y1 = y - elem.g.bitmap_top;
			const float x2 = x1 + bsx;
			const float y2 = y1 + bsy;
			
			minX = std::min(minX, std::min(x1, x2));
			minY = std::min(minY, std::min(y1, y2));
			maxX = std::max(maxX, std::max(x1, x2));
			maxY = std::max(maxY, std::max(y1, y2));

			x += (elem.g.advance.x / float(1 << 6));
			y += (elem.g.advance.y / float(1 << 6));
		}
	}

	if (maxX < minX)
	{
		sx = 0.f;
		sy = 0.f;
	}
	else
	{
		sx = maxX - minX;
		sy = maxY - minY;
	}
	
	yTop = minY;
}

static void drawTextInternal(FT_Face face, int size, const GlyphCacheElem ** glyphs, const int numGlyphs, float x, float y)
{
	// the (0,0) coordinate represents the lower left corner of a glyph
	// we want to render the glyph using its top left corner at (0,0)
	
	//y += size;
	
#if USE_GLYPH_ATLAS
	if (globals.isInTextBatch == false)
	{
		gxSetTexture(globals.font->textureAtlas->texture);
		
		gxBegin(GL_QUADS);
	}
	
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		const GlyphCacheElem & elem = *glyphs[i];
		
		// skip current character if the element is invalid
		
		if (elem.textureAtlasElem != nullptr)
		{
			const float bsx = float(elem.g.bitmap.width);
			const float bsy = float(elem.g.bitmap.rows);
			const float x1 = x + elem.g.bitmap_left;
			const float y1 = y - elem.g.bitmap_top;
			const float x2 = x1 + bsx;
			const float y2 = y1 + bsy;
			
			const int iu1 = elem.textureAtlasElem->x + GLYPH_ATLAS_BORDER;
			const int iu2 = elem.textureAtlasElem->x - GLYPH_ATLAS_BORDER + elem.textureAtlasElem->sx;
			const int iv1 = elem.textureAtlasElem->y + GLYPH_ATLAS_BORDER;
			const int iv2 = elem.textureAtlasElem->y - GLYPH_ATLAS_BORDER + elem.textureAtlasElem->sy;
			const float u1 = iu1 / float(globals.font->textureAtlas->a.sx);
			const float u2 = iu2 / float(globals.font->textureAtlas->a.sx);
			const float v1 = iv1 / float(globals.font->textureAtlas->a.sy);
			const float v2 = iv2 / float(globals.font->textureAtlas->a.sy);
			
			gxTexCoord2f(u1, v1); gxVertex2f(x1, y1);
			gxTexCoord2f(u2, v1); gxVertex2f(x2, y1);
			gxTexCoord2f(u2, v2); gxVertex2f(x2, y2);
			gxTexCoord2f(u1, v2); gxVertex2f(x1, y2);
		}
		
		x += (elem.g.advance.x / float(1 << 6));
		y += (elem.g.advance.y / float(1 << 6));
	}
	
	if (globals.isInTextBatch == false)
	{
		gxEnd();
		
		gxSetTexture(0);
	}
#else
	for (size_t i = 0; i < numGlyphs; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCacheElem & elem = *glyphs[i];
		
		if (elem.texture != 0)
		{
			gxSetTexture(elem.texture);
			
			gxBegin(GL_QUADS);
			{
				const float bsx = float(elem.g.bitmap.width);
				const float bsy = float(elem.g.bitmap.rows);
				const float x1 = x + elem.g.bitmap_left;
				const float y1 = y - elem.g.bitmap_top;
				const float x2 = x1 + bsx;
				const float y2 = y1 + bsy;
				
				gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
				gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
				gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
				gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
			}
			gxEnd();
			
			x += (elem.g.advance.x / float(1 << 6));
			y += (elem.g.advance.y / float(1 << 6));
		}
	}

	gxSetTexture(0);
#endif
}

//

#if ENABLE_MSDF_FONTS

// fixme : settle on one method
static int sampleMethod = 3;
static bool useSuperSampling = true;

static void measureTextMSDFInternal(const stbtt_fontinfo & fontInfo, const float size, const MsdfGlyphCode * codepoints, const MsdfGlyphCacheElem ** glyphs, const int numGlyphs, float & sx, float & sy, float & yTop)

{
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	
	//
	
	int lastCodepoint = -1;
	
	int x = 0;
	
	bool isFirst = true;
	
	for (int i = 0; i < numGlyphs; ++i)
	{
		const int codepoint = codepoints[i];
		const MsdfGlyphCacheElem & glyph = *glyphs[i];
		
		//
		
		if (glyph.textureAtlasElem != nullptr)
		{
			int gsx = glyph.sx;
			int gsy = glyph.sy;
			
			int dx1 = x - MSDF_GLYPH_PADDING_INNER;
			int dy1 = 0 + MSDF_GLYPH_PADDING_INNER;
			int dx2 = dx1 + gsx;
			int dy2 = dy1 - gsy;
			
			dy1 -= glyph.y;
			dy2 -= glyph.y;
			
			// note: as natively the Y axis runs up, not down for STB/TrueType fonts we need to swap these. note also that while drawing we don't have to do this, as it neatly compensates for the glyphs being drawn 'upside down' as well
			
			std::swap(dy1, dy2);
			
			if (isFirst)
			{
				isFirst = false;
				
				x1 = dx1;
				y1 = dy1;
				x2 = dx2;
				y2 = dy2;
			}
			else
			{
				x1 = std::min(x1, dx1);
				y1 = std::min(y1, dy1);
				x2 = std::max(x2, dx2);
				y2 = std::max(y2, dy2);
			}
		}
	
		const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&fontInfo, lastCodepoint, codepoint);
		
		x += advance;
		
		lastCodepoint = codepoint;
	}
	
	//
	
	const float scale = stbtt_ScaleForPixelHeight(&fontInfo, size);
	
	sx = (x2 - x1) * scale;
	sy = (y2 - y1) * scale;
	yTop = y1 * scale;
}

static void drawTextMSDFInternal(MsdfGlyphCache & glyphCache, const float _x, const float _y, const float size, const MsdfGlyphCode * codepoints, const MsdfGlyphCacheElem ** glyphs, const int numGlyphs)
{
	if (globals.isInTextBatchMSDF == false)
	{
		Shader & shader = globals.builtinShaders->msdfText.get();
		setShader(shader);
		
		shader.setTexture("msdf", 0, glyphCache.m_textureAtlas->texture);
		shader.setImmediate("sampleMethod", sampleMethod);
		shader.setImmediate("useSuperSampling", useSuperSampling);
		
		gxBegin(GL_QUADS);
	}
	
	const float scale = stbtt_ScaleForPixelHeight(&glyphCache.m_font.fontInfo, size);
	
	int lastCodepoint = -1;
	
	float x = _x / scale;
	float y = _y / scale;
	
	const int atlasSx = glyphCache.m_textureAtlas->a.sx;
	const int atlasSy = glyphCache.m_textureAtlas->a.sy;
	
	for (int i = 0; i < numGlyphs; ++i)
	{
		const int codepoint = codepoints[i];
		const MsdfGlyphCacheElem & glyph = *glyphs[i];
		
		//
		
		if (glyph.textureAtlasElem != nullptr)
		{
			const int sx = glyph.textureAtlasElem->sx;
			const int sy = glyph.textureAtlasElem->sy;
			
			const int u1i = glyph.textureAtlasElem->x + MSDF_GLYPH_PADDING_INNER;
			const int v1i = glyph.textureAtlasElem->y + MSDF_GLYPH_PADDING_INNER;
			const int u2i = glyph.textureAtlasElem->x + sx - MSDF_GLYPH_PADDING_INNER;
			const int v2i = glyph.textureAtlasElem->y + sy - MSDF_GLYPH_PADDING_INNER;
			
			const float u1 = u1i / float(atlasSx);
			const float v1 = v1i / float(atlasSy);
			const float u2 = u2i / float(atlasSx);
			const float v2 = v2i / float(atlasSy);
			
			int gsx = glyph.sx;
			int gsy = glyph.sy;
			
			int dx1 = x - MSDF_GLYPH_PADDING_INNER;
			int dy1 = 0 + MSDF_GLYPH_PADDING_INNER;
			int dx2 = dx1 + gsx;
			int dy2 = dy1 - gsy;
			
			dy1 += y - glyph.y;
			dy2 += y - glyph.y;
			
			gxTexCoord2f(u1, v1); gxVertex2f(dx1 * scale, dy1 * scale);
			gxTexCoord2f(u2, v1); gxVertex2f(dx2 * scale, dy1 * scale);
			gxTexCoord2f(u2, v2); gxVertex2f(dx2 * scale, dy2 * scale);
			gxTexCoord2f(u1, v2); gxVertex2f(dx1 * scale, dy2 * scale);
		}
	
		const int advance = glyph.advance + stbtt_GetCodepointKernAdvance(&glyphCache.m_font.fontInfo, lastCodepoint, codepoint);
		
		x += advance;
		
		lastCodepoint = codepoint;
	}
	
	if (globals.isInTextBatchMSDF == false)
	{
		gxEnd();
		
		clearShader();
	}
}

#endif

//

void measureText(float size, float & sx, float & sy, const char * format, ...)
{
	char _text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(_text, sizeof(_text), format, args);
	va_end(args);
	
#if ENABLE_UTF8_SUPPORT
	unicode_t text[MAX_TEXT_LENGTH];
	const size_t textLength = utf8toutf32(_text, strlen(_text), text, MAX_TEXT_LENGTH * 4, 0) / 4;
#else
	const char * text = _text;
	const size_t textLength = strlen(_text);
#endif

	if (globals.fontMode == FONT_BITMAP)
	{
		const int sizei = int(std::ceilf(size));
		
		auto face = globals.font->face;
		
		const GlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &g_glyphCache.findOrCreate(face, sizei, text[i]);
		}
		
		float yTop;

		measureText(face, size, glyphs, textLength, sx, sy, yTop);
	}
	else if (globals.fontMode == FONT_SDF)
	{
		MsdfGlyphCache & glyphCache = *globals.fontMSDF->m_glyphCache;
		
		const MsdfGlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &glyphCache.findOrCreate(text[i]);
		}
		
		float yTop;
		
		measureTextMSDFInternal(glyphCache.m_font.fontInfo, size, text, glyphs, textLength, sx, sy, yTop);
	}
}

void beginTextBatch()
{
	if (globals.fontMode == FONT_BITMAP)
	{
	#if USE_GLYPH_ATLAS
		Assert(!globals.isInTextBatch);
		globals.isInTextBatch = true;
		
		gxSetTexture(globals.font->textureAtlas->texture);
		gxBegin(GL_QUADS);
	#endif
	}
	else if (globals.fontMode == FONT_SDF)
	{
		fassert(globals.isInTextBatchMSDF == false);
		if (globals.isInTextBatchMSDF == true)
			return;
		
		globals.isInTextBatchMSDF = true;
		
		Shader & shader = globals.builtinShaders->msdfText.get();
		setShader(shader);
		
		shader.setTexture("msdf", 0, globals.fontMSDF->m_glyphCache->m_textureAtlas->texture);
		shader.setImmediate("sampleMethod", sampleMethod);
		shader.setImmediate("useSuperSampling", useSuperSampling);
		
		gxBegin(GL_QUADS);
	}
}

void endTextBatch()
{
	if (globals.fontMode == FONT_BITMAP)
	{
	#if USE_GLYPH_ATLAS
		Assert(globals.isInTextBatch);
		globals.isInTextBatch = false;
		
		gxEnd();
		gxSetTexture(0);
	#endif
	}
	else if (globals.fontMode == FONT_SDF)
	{
		fassert(globals.isInTextBatchMSDF == true);
		if (globals.isInTextBatchMSDF == false)
			return;
		
		globals.isInTextBatchMSDF = false;

		gxEnd();
		
		clearShader();
	}
}

void drawText(float x, float y, float size, float alignX, float alignY, const char * format, ...)
{
	char _text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(_text, sizeof(_text), format, args);
	va_end(args);
	
#if ENABLE_UTF8_SUPPORT
	unicode_t text[MAX_TEXT_LENGTH];
	const size_t textLength = utf8toutf32(_text, strlen(_text), text, MAX_TEXT_LENGTH * 4, 0) / 4;
#else
	const char * text = _text;
	const size_t textLength = strlen(_text);
#endif
	
	if (globals.fontMode == FONT_BITMAP)
	{
		const int sizei = int(std::ceilf(size));
		
		auto face = globals.font->face;
		
		const GlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &g_glyphCache.findOrCreate(face, size, text[i]);
		}
		
		float sx, sy, yTop;
		measureText(face, sizei, glyphs, textLength, sx, sy, yTop);
		
	#if USE_GLYPH_ATLAS
		x += sx * (alignX - 1.f) / 2.f;
		y += sy * (alignY - 1.f) / 2.f;
		
		y -= yTop;
		
		drawTextInternal(face, sizei, glyphs, textLength, x, y);
	#else
		gxMatrixMode(GL_MODELVIEW);
		gxPushMatrix();
		{
			x += sx * (alignX - 1.f) / 2.f;
			y += sy * (alignY - 1.f) / 2.f;
			
			y -= yTop;

			gxTranslatef(x, y, 0.f);
			
			drawTextInternal(globals.font->face, sizei, glyphs, textLength, 0.f, 0.f);
		}
		gxPopMatrix();
	#endif
	}
	else
	{
		MsdfGlyphCache & glyphCache = *globals.fontMSDF->m_glyphCache;
		
		const MsdfGlyphCacheElem * glyphs[MAX_TEXT_LENGTH];
		
		for (size_t i = 0; i < textLength; ++i)
		{
			glyphs[i] = &glyphCache.findOrCreate(text[i]);
		}
		
		float sx, sy, yTop;
		measureTextMSDFInternal(glyphCache.m_font.fontInfo, size, text, glyphs, textLength, sx, sy, yTop);
		
		x += sx * (alignX - 1.f) / 2.f;
		y += sy * (alignY - 1.f) / 2.f;
		
		y -= yTop;

		drawTextMSDFInternal(glyphCache, x, y, size, text, glyphs, textLength);
	}
}

struct TextAreaData
{
	const static int kMaxLines = 64;
	
	char lines[kMaxLines][1024];
	int numLines;
	
	TextAreaData()
		: numLines(0)
	{
	}
};

static const char * eatWord(const char * str)
{
	while (*str && *str != ' ')
		str++;
	while (*str && *str == ' ')
		str++;
	return str;
}

static void prepareTextArea(const float size, const char * text, const float maxSx, float & sx, float & sy, TextAreaData & data)
{
	const char * textend = text + strlen(text);
	const char * textptr = text;
	
	sx = 0.f;
	sy = 0.f;

	while (textptr != textend && data.numLines < TextAreaData::kMaxLines)
	{
		const char * nextptr = eatWord(textptr);
		while (*nextptr)
		{
			const char * tempptr = eatWord(nextptr);
			
			const char temp = *tempptr;
			*(char*)tempptr = 0;
			float _sx, _sy;
			measureText(size, _sx, _sy, "%s", textptr);
			*(char*)tempptr = temp;

			if (_sx >= maxSx)
			{
				break;
			}
			
			if (_sx > sx)
				sx = _sx;
			
			nextptr = tempptr;
		}

		const char temp = *nextptr;
		*(char*)nextptr = 0;
		strcpy_s(data.lines[data.numLines++], sizeof(data.lines[0]), textptr);
		*(char*)nextptr = temp;

		textptr = nextptr;
	}

	sy = size * data.numLines;
}

void measureTextArea(float size, float maxSx, float & sx, float & sy, const char * format, ...)
{
	char text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	TextAreaData data;
	prepareTextArea(size, text, maxSx, sx, sy, data);
}

void drawTextArea(float x, float y, float sx, float size, const char * format, ...)
{
	char text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	drawTextArea(x, y, sx, 0.f, size, +1.f, +1.f, text);
}

void drawTextArea(float x, float y, float sx, float sy, float size, float alignX, float alignY, const char * format, ...)
{
	char text[MAX_TEXT_LENGTH];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	float tsx;
	float tsy;
	
	TextAreaData data;
	prepareTextArea(size, text, sx, tsx, tsy, data);
	
	x += (sx      ) * (-alignX + 1.f) / 2.f;
	y += (sy - tsy) * (-alignY + 1.f) / 2.f;

	for (int i = 0; i < data.numLines; ++i)
	{
		drawText(x, y, size, alignX, 1, data.lines[i]);
		y += size;
	}
}


//

void drawPath(const Path2d & path)
{
	const int kMaxPoints = 16 * 1024;

	float pxyStorage[kMaxPoints * 2];
	float hxyStorage[kMaxPoints * 2];

	float * pxy = pxyStorage;
	float * hxy = hxyStorage;

	int numPoints = 0;
	path.generatePoints(pxy, hxy, kMaxPoints, 1.f, numPoints);

	gxBegin(GL_LINES);
	{
		for (int i = 0; i < numPoints - 1; ++i)
		{
			gxVertex2f(pxy[0], pxy[1]);
			gxVertex2f(pxy[2], pxy[3]);

			pxy += 2;
			hxy += 2;
		}
	}
	gxEnd();

#if 0
	pxy = pxyStorage;
	hxy = hxyStorage;

	glLineWidth(1.f);

	gxBegin(GL_LINES);
	{
		for (int i = 0; i < numPoints; ++i)
		{
			const float tx = hxy[0];
			const float ty = hxy[1];
			const float ts = std::hypot(tx, ty);

			gxVertex2f(pxy[0],          pxy[1]         );
			gxVertex2f(pxy[0] + hxy[0], pxy[1] + hxy[1]);

			pxy += 2;
			hxy += 2;
		}
	}
	gxEnd();
#endif

#if 0
	pxy = pxyStorage;

	glPointSize(5.f);
	setColor(255, 255, 255, 31);

	gxBegin(GL_POINTS);
	{
		for (int i = 0; i < numPoints; ++i)
		{
			gxVertex2f(pxy[0], pxy[1]);

			pxy += 2;
		}
	}
	gxEnd();

	glPointSize(1.f);
#endif
}

static GLuint createTexture(const void * source, int sx, int sy, bool filter, bool clamp, GLenum internalFormat, GLenum uploadFormat, GLenum uploadElementType)
{
	checkErrorGL();

	GLuint texture = 0;

	glGenTextures(1, &texture);

	if (texture)
	{
		GLuint restoreTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
		GLint restoreUnpackAlignment;
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpackAlignment);
		GLint restoreUnpackRowLength;
		glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restoreUnpackRowLength);

		// copy image data

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		checkErrorGL();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, sx);
		glTexImage2D(
			GL_TEXTURE_2D,
			0,
			internalFormat,
			sx,
			sy,
			0,
			uploadFormat,
			uploadElementType,
			source);

		// set filtering

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

		// restore previous OpenGL states

		glBindTexture(GL_TEXTURE_2D, restoreTexture);
		glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpackAlignment);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, restoreUnpackRowLength);
	}

	checkErrorGL();

	return texture;
}

GLuint createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
}

GLuint createTextureFromRGB8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE);
}

GLuint createTextureFromR8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
}

GLuint createTextureFromRGBF32(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_RGB32F, GL_RGB, GL_FLOAT);
}

GLuint createTextureFromR16(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_R16, GL_RED, GL_UNSIGNED_SHORT);
}

GLuint createTextureFromR32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, filter, clamp, GL_R32F, GL_RED, GL_FLOAT);
}

void debugDrawText(float x, float y, int size, float alignX, float alignY, const char * format, ...)
{
	if (globals.debugDraw.numLines < globals.debugDraw.kMaxLines)
	{
		globals.debugDraw.lines[globals.debugDraw.numLines].font = globals.font;
		globals.debugDraw.lines[globals.debugDraw.numLines].color = globals.color;
		
		globals.debugDraw.lines[globals.debugDraw.numLines].x = x;
		globals.debugDraw.lines[globals.debugDraw.numLines].y = y;
		globals.debugDraw.lines[globals.debugDraw.numLines].size = size;
		globals.debugDraw.lines[globals.debugDraw.numLines].alignX = alignX;
		globals.debugDraw.lines[globals.debugDraw.numLines].alignY = alignY;
		
		va_list args;
		va_start(args, format);
		vsprintf_s(
			globals.debugDraw.lines[globals.debugDraw.numLines].text,
			globals.debugDraw.kMaxLineSize,
			format, args);
		va_end(args);
		
		globals.debugDraw.numLines++;
	}
}

#if !ENABLE_OPENGL

	SDL_Window * getWindow()
	{
		return globals.window;
	}

	SDL_Surface * getWindowSurface()
	{
		return SDL_GetWindowSurface(globals.window);
	}

#elif !USE_LEGACY_OPENGL

class GxMatrixStack
{
public:
	static const int kSize = 32;
	Mat4x4 stack[kSize];
	int stackDepth;
	bool isDirty;
	
	GxMatrixStack()
	{
		stackDepth = 0;
		stack[0].MakeIdentity();
		
		isDirty = true;
	}
	
	void push()
	{
		fassert(stackDepth + 1 < kSize);
		stackDepth++;
		stack[stackDepth] = stack[stackDepth - 1];
	}
	
	void pop()
	{
		fassert(stackDepth > 0);
		stackDepth--;
		
		isDirty = true;
	}
	
	const Mat4x4 & get() const
	{
		return stack[stackDepth];
	}
	
	Mat4x4 & getRw()
	{
		isDirty = true;
		
		return stack[stackDepth];
	}
	
	void makeDirty()
	{
		isDirty = true;
	}
};

static GxMatrixStack s_gxModelView;
static GxMatrixStack s_gxProjection;
static GxMatrixStack * s_gxMatrixStack = &s_gxModelView;

void gxMatrixMode(GLenum mode)
{
	switch (mode)
	{
		case GL_MODELVIEW:
			s_gxMatrixStack = &s_gxModelView;
			break;
		case GL_PROJECTION:
			s_gxMatrixStack = &s_gxProjection;
			break;
		default:
			fassert(false);
			break;
	}
}

void gxPopMatrix()
{
	s_gxMatrixStack->pop();
}

void gxPushMatrix()
{
	s_gxMatrixStack->push();
}

void gxLoadIdentity()
{
	s_gxMatrixStack->getRw().MakeIdentity();
}

void gxLoadMatrixf(const float * m)
{
	memcpy(s_gxMatrixStack->getRw().m_v, m, sizeof(float) * 16);
}

void gxGetMatrixf(GLenum mode, float * m)
{
	switch (mode)
	{
		case GL_PROJECTION:
			memcpy(m, s_gxProjection.get().m_v, sizeof(float) * 16);
			break;
		case GL_MODELVIEW:
			memcpy(m, s_gxModelView.get().m_v, sizeof(float) * 16);
			break;
		default:
			fassert(false);
			break;
	}
}

void gxMultMatrixf(const float * _m)
{
	Mat4x4 m;
	memcpy(m.m_v, _m, sizeof(m.m_v));

	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxTranslatef(float x, float y, float z)
{
	Mat4x4 m;
	m.MakeTranslation(x, y, z);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxRotatef(float angle, float x, float y, float z)
{
	Quat q;
	q.fromAxisAngle(Vec3(x, y, z), angle * M_PI / 180.f);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * q.toMatrix();
}

void gxScalef(float x, float y, float z)
{
	Mat4x4 m;
	m.MakeScaling(x, y, z);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxValidateMatrices()
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);

	// todo: check if matrices are dirty
	
	//printf("validate1\n");
	
#if VS_USE_LEGACY_MATRICES
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(s_gxProjection.get().m_v);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(s_gxModelView.get().m_v);
#else
	if (globals.shader && globals.shader->getType() == SHADER_VSPS)
	{
		Shader * shader = static_cast<Shader*>(globals.shader);

		const ShaderCacheElem & shaderElem = shader->getCacheElem();
		
		if ((globals.gxShaderIsDirty || s_gxModelView.isDirty) && shaderElem.params[ShaderCacheElem::kSp_ModelViewMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_ModelViewMatrix].index, s_gxModelView.get().m_v);
			//printf("validate2\n");
		}
		if ((globals.gxShaderIsDirty || s_gxModelView.isDirty || s_gxProjection.isDirty) && shaderElem.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].index, (s_gxProjection.get() * s_gxModelView.get()).m_v);
			//printf("validate3\n");
		}
		if ((globals.gxShaderIsDirty || s_gxProjection.isDirty) && shaderElem.params[ShaderCacheElem::kSp_ProjectionMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_ProjectionMatrix].index, s_gxProjection.get().m_v);
			//printf("validate4\n");
		}
	}
#endif
	
	s_gxModelView.isDirty = false;
	s_gxProjection.isDirty = false;
}

struct GxVertex
{
	float px, py, pz, pw;
	float nx, ny, nz;
	float cx, cy, cz, cw;
	float tx, ty;
};

#define GX_USE_RINGBUFFER 0
#define GX_USE_UBERBUFFER 0
#define GX_USE_BUFFER_RENAMING 0
#define GX_BUFFER_DRAW_MODE GL_DYNAMIC_DRAW
//#define GX_BUFFER_DRAW_MODE GL_STREAM_DRAW
#if defined(MACOS)
    #define GX_USE_ELEMENT_ARRAY_BUFFER 1
#else
    #define GX_USE_ELEMENT_ARRAY_BUFFER 0
#endif
#define GX_VAO_COUNT 1

static Shader s_gxShader;
static GLuint s_gxVertexArrayObject[GX_VAO_COUNT] = { };
static GLuint s_gxVertexBufferObject[GX_VAO_COUNT] = { };
static GLuint s_gxIndexBufferObject[GX_VAO_COUNT] = { };
static GxVertex s_gxVertexBuffer[1024*16];

#if GX_USE_RINGBUFFER
static GLuint s_gxVertexBufferRing = 0;
#endif

#if GX_USE_UBERBUFFER
const int kUberVertexBufferSize = 1024 * 32;
const int kUberIndexBufferSize = 1024 * 64;
static int s_gxUberVertexBufferPosition = 0;
static int s_gxUberIndexBufferPosition = 0;
#endif

static int s_gxPrimitiveType = -1;
static GxVertex * s_gxVertices = 0;
static int s_gxVertexCount = 0;
static int s_gxMaxVertexCount = 0;
static int s_gxPrimitiveSize = 0;
static GxVertex s_gxVertex = { };
static bool s_gxTextureEnabled = false;

static const VsInput vsInputs[] =
{
	{ VS_POSITION, 4, GL_FLOAT, 0, offsetof(GxVertex, px) },
	{ VS_NORMAL,   3, GL_FLOAT, 0, offsetof(GxVertex, nx) },
	{ VS_COLOR,    4, GL_FLOAT, 0, offsetof(GxVertex, cx) },
	{ VS_TEXCOORD, 2, GL_FLOAT, 0, offsetof(GxVertex, tx) }
};
const int numVsInputs = sizeof(vsInputs) / sizeof(vsInputs[0]);

void gxEmitVertex();

void gxInitialize()
{
	registerBuiltinShaders();

	s_gxShader.load("engine/Generic", "engine/Generic.vs", "engine/Generic.ps");
	
	memset(&s_gxVertex, 0, sizeof(s_gxVertex));
	s_gxVertex.cx = 1.f;
	s_gxVertex.cy = 1.f;
	s_gxVertex.cz = 1.f;
	s_gxVertex.cw = 1.f;

	fassert(s_gxVertexBufferObject[0] == 0);
	glGenBuffers(GX_VAO_COUNT, s_gxVertexBufferObject);
#if GX_USE_UBERBUFFER
	glBindBuffer(GL_ARRAY_BUFFER, s_gxVertexBufferObject);
	glBufferData(GL_ARRAY_BUFFER, kUberVertexBufferSize * sizeof(GxVertex), 0, GL_STREAM_DRAW);
#endif
	
	fassert(s_gxIndexBufferObject[0] == 0);
	glGenBuffers(GX_VAO_COUNT, s_gxIndexBufferObject);
#if GX_USE_UBERBUFFER
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_gxIndexBufferObject);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, kUberIndexBufferSize * sizeof(glindex_t), 0, GL_STREAM_DRAW);
#endif
	
	// create vertex array
	fassert(s_gxVertexArrayObject[0] == 0);
	glGenVertexArrays(GX_VAO_COUNT, s_gxVertexArrayObject);
	checkErrorGL();

	for (int i = 0; i < GX_VAO_COUNT; ++i)
	{
		glBindVertexArray(s_gxVertexArrayObject[i]);
		checkErrorGL();
		{
			#if GX_USE_ELEMENT_ARRAY_BUFFER
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_gxIndexBufferObject[i]);
			checkErrorGL();
			#endif
			glBindBuffer(GL_ARRAY_BUFFER, s_gxVertexBufferObject[i]);
			checkErrorGL();
			bindVsInputs(vsInputs, numVsInputs, sizeof(GxVertex));
		}
	}

	glBindVertexArray(0);
	checkErrorGL();
	
	#if GX_USE_RINGBUFFER
	if (glBufferStorage)
	{
		const GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		const int bufferSize = sizeof(GxVertex) * 1024;
		glGenBuffers(1, &s_gxVertexBufferRing);
		glBindBuffer(GL_ARRAY_BUFFER, s_gxVertexBufferRing);
		glBufferStorage(GL_ARRAY_BUFFER, bufferSize, 0, flags);
		checkErrorGL();
		glMapBufferRange(GL_ARRAY_BUFFER, 0, bufferSize, flags);
		checkErrorGL();
	}
	#endif
}

void gxShutdown()
{
	if (s_gxVertexArrayObject[0] != 0)
	{
		glDeleteVertexArrays(GX_VAO_COUNT, s_gxVertexArrayObject);
		memset(s_gxVertexArrayObject, 0, sizeof(s_gxVertexArrayObject));
	}
	
	if (s_gxVertexBufferObject[0] != 0)
	{
		glDeleteBuffers(GX_VAO_COUNT, s_gxVertexBufferObject);
		memset(s_gxVertexBufferObject, 0, sizeof(s_gxVertexBufferObject));
	}

	if (s_gxIndexBufferObject[0] != 0)
	{
		glDeleteBuffers(GX_VAO_COUNT, s_gxIndexBufferObject);
		memset(s_gxIndexBufferObject, 0, sizeof(s_gxIndexBufferObject));
	}
}

static void gxFlush(bool endOfBatch)
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);

	if (s_gxVertexCount)
	{
		const int primitiveType = s_gxPrimitiveType;

		Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : s_gxShader;

		setShader(shader);
		
		gxValidateMatrices();
		
		static int vaoIndex = 0;
		vaoIndex = (vaoIndex + 1) % GX_VAO_COUNT;

	#if GX_USE_UBERBUFFER
		glBindBuffer(GL_ARRAY_BUFFER, s_gxVertexBufferObject);
		if (s_gxUberVertexBufferPosition + s_gxVertexCount > kUberVertexBufferSize)
		{
			glBufferData(GL_ARRAY_BUFFER, kUberVertexBufferSize * sizeof(GxVertex), 0, GX_BUFFER_DRAW_MODE);
			s_gxUberVertexBufferPosition = 0;
		}
		glBufferSubData(GL_ARRAY_BUFFER, s_gxUberVertexBufferPosition * sizeof(GxVertex), s_gxVertexCount * sizeof(GxVertex), s_gxVertices);
		checkErrorGL();
	#else
		glBindBuffer(GL_ARRAY_BUFFER, s_gxVertexBufferObject[vaoIndex]);
		#if GX_USE_BUFFER_RENAMING
		glBufferData(GL_ARRAY_BUFFER, sizeof(GxVertex) * s_gxVertexCount, 0, GX_BUFFER_DRAW_MODE);
		#endif
		glBufferData(GL_ARRAY_BUFFER, sizeof(GxVertex) * s_gxVertexCount, s_gxVertices, GX_BUFFER_DRAW_MODE);
		checkErrorGL();
	#endif
		
		bool indexed = false;
		glindex_t * indices = 0;
		int numElements = s_gxVertexCount;
		int numIndices = 0;

	#if !GX_USE_ELEMENT_ARRAY_BUFFER || GX_VAO_COUNT > 1
		bool needToRegenerateIndexBuffer = true;
	#else
		bool needToRegenerateIndexBuffer = false;
        
        static int lastPrimitiveType = -1;
        static int lastVertexCount = -1;
        
		if (s_gxPrimitiveType != lastPrimitiveType || s_gxVertexCount != lastVertexCount)
		{
			lastPrimitiveType = s_gxPrimitiveType;
			lastVertexCount = s_gxVertexCount;

			needToRegenerateIndexBuffer = true;
		}
	#endif

	#if 1
		if (s_gxPrimitiveType == GL_QUADS)
		{
			fassert(s_gxVertexCount < 65536);
			
			// todo: use triangle strip + compute index buffer once at init time
			
			const int numQuads = s_gxVertexCount / 4;
			numIndices = numQuads * 6;

			if (needToRegenerateIndexBuffer)
			{
				indices = (glindex_t*)alloca(sizeof(glindex_t) * numIndices);

				glindex_t * __restrict indexPtr = indices;
				glindex_t baseIndex = 0;
			
				for (int i = 0; i < numQuads; ++i)
				{
					*indexPtr++ = baseIndex + 0;
					*indexPtr++ = baseIndex + 1;
					*indexPtr++ = baseIndex + 2;
				
					*indexPtr++ = baseIndex + 0;
					*indexPtr++ = baseIndex + 2;
					*indexPtr++ = baseIndex + 3;
				
					baseIndex += 4;
				}
			
			#if GX_USE_ELEMENT_ARRAY_BUFFER
			#if GX_USE_UBERBUFFER
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_gxIndexBufferObject);
				if (s_gxUberIndexBufferPosition + numIndices > kUberIndexBufferSize)
				{
					glBufferData(GL_ELEMENT_ARRAY_BUFFER, kUberIndexBufferSize * sizeof(glindex_t), 0, GX_BUFFER_DRAW_MODE);
					s_gxUberIndexBufferPosition = 0;
				}
				glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, s_gxUberIndexBufferPosition * sizeof(glindex_t), numIndices * sizeof(glindex_t), indices);
				checkErrorGL();
			#else
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_gxIndexBufferObject[vaoIndex]);
				#if GX_USE_BUFFER_RENAMING
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glindex_t) * numIndices, 0, GX_BUFFER_DRAW_MODE);
				#endif
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glindex_t) * numIndices, indices, GX_BUFFER_DRAW_MODE);
				checkErrorGL();
			#endif
			#endif
			}
			
			s_gxPrimitiveType = GL_TRIANGLES;
			numElements = numIndices;
			
			indexed = true;
		}
	#endif
		
		glBindVertexArray(s_gxVertexArrayObject[vaoIndex]);
		checkErrorGL();
		
		const ShaderCacheElem & shaderElem = shader.getCacheElem();
		
		if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
		{
			shader.setImmediate(
				shaderElem.params[ShaderCacheElem::kSp_Params].index,
				s_gxTextureEnabled ? 1 : 0,
				globals.colorMode,
				0,
				0);
		}

		if (globals.gxShaderIsDirty)
		{
			if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
				shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
		}

		if (indexed)
		{
			#if GX_USE_UBERBUFFER
			glDrawElementsBaseVertex(s_gxPrimitiveType, numElements, INDEX_TYPE, (void*)(s_gxUberIndexBufferPosition * sizeof(glindex_t)), s_gxUberVertexBufferPosition);
			#elif GX_USE_ELEMENT_ARRAY_BUFFER
			glDrawElements(s_gxPrimitiveType, numElements, INDEX_TYPE, 0);
			#else
			glDrawElements(s_gxPrimitiveType, numElements, INDEX_TYPE, indices);
			#endif
			checkErrorGL();
		}
		else
		{
			#if GX_USE_UBERBUFFER
			glDrawArrays(s_gxPrimitiveType, s_gxUberVertexBufferPosition, numElements);
			#else
			glDrawArrays(s_gxPrimitiveType, 0, numElements);
			#endif
			checkErrorGL();
		}
		
	#if GX_USE_UBERBUFFER
		//logDebug("uber offsets: %d, %d", s_gxUberVertexBufferPosition, s_gxUberIndexBufferPosition);
		s_gxUberVertexBufferPosition += s_gxVertexCount;
		s_gxUberIndexBufferPosition += numIndices;
	#endif
		
		if (endOfBatch)
		{
			s_gxVertexCount = 0;
		}
		else
		{
			switch (s_gxPrimitiveType)
			{
				case GL_LINE_LOOP:
					s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 1;
					break;
				case GL_LINE_STRIP:
					s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 1;
					break;
				case GL_TRIANGLE_FAN:
					s_gxVertices[0] = s_gxVertices[0];
					s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 2;
					break;
				case GL_TRIANGLE_STRIP:
					s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 2];
					s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
					s_gxVertexCount = 2;
					break;
				default:
					s_gxVertexCount = 0;
			}
		}
		
		globals.gxShaderIsDirty = false;

		s_gxPrimitiveType = primitiveType;
	}
	
	if (endOfBatch)
		s_gxVertices = 0;
}

void gxBegin(int primitiveType)
{
	s_gxPrimitiveType = primitiveType;
	s_gxVertices = s_gxVertexBuffer;
	s_gxMaxVertexCount = sizeof(s_gxVertexBuffer) / sizeof(s_gxVertexBuffer[0]);
	
	switch (primitiveType)
	{
		case GL_TRIANGLES:
			s_gxPrimitiveSize = 3;
			break;
		case GL_QUADS:
			s_gxPrimitiveSize = 4;
			break;
		case GL_LINES:
			s_gxPrimitiveSize = 2;
			break;
		case GL_LINE_LOOP:
			s_gxPrimitiveSize = 1;
			break;
		case GL_LINE_STRIP:
			s_gxPrimitiveSize = 1;
			break;
		case GL_POINTS:
			s_gxPrimitiveSize = 1;
			break;
		case GL_TRIANGLE_FAN:
			s_gxPrimitiveSize = 1;
			break;
		case GL_TRIANGLE_STRIP:
			s_gxPrimitiveSize = 1;
			break;
		default:
			fassert(false);
	}
	
	fassert(s_gxVertexCount == 0);
}

void gxEnd()
{
	gxFlush(true);
}

void gxEmitVertices(int primitiveType, int numVertices)
{
	Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : s_gxShader;

	setShader(shader);

	gxValidateMatrices();

	//

	const int vaoIndex = 0;
	glBindVertexArray(s_gxVertexArrayObject[vaoIndex]);
	checkErrorGL();

	//

	const ShaderCacheElem & shaderElem = shader.getCacheElem();

	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1 : 0,
			globals.colorMode,
			0,
			0);
	}

	if (globals.gxShaderIsDirty)
	{
		if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
			shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
	}

	//

	glDrawArrays(primitiveType, 0, numVertices);
	checkErrorGL();

	globals.gxShaderIsDirty = false;
}

void gxColor4f(float r, float g, float b, float a)
{
	s_gxVertex.cx = r;
	s_gxVertex.cy = g;
	s_gxVertex.cz = b;
	s_gxVertex.cw = a;
}

void gxColor4fv(const float * rgba)
{
	s_gxVertex.cx = rgba[0];
	s_gxVertex.cy = rgba[1];
	s_gxVertex.cz = rgba[2];
	s_gxVertex.cw = rgba[3];
}

void gxColor3ub(int r, int g, int b)
{
	gxColor4f(
		scale255(r),
		scale255(g),
		scale255(b),
		1.f);
}

void gxColor4ub(int r, int g, int b, int a)
{
	gxColor4f(
		scale255(r),
		scale255(g),
		scale255(b),
		scale255(a));
}

void gxTexCoord2f(float u, float v)
{
	s_gxVertex.tx = u;
	s_gxVertex.ty = v;
}

void gxNormal3f(float x, float y, float z)
{
	s_gxVertex.nx = x;
	s_gxVertex.ny = y;
	s_gxVertex.nz = z;
}

void gxVertex2f(float x, float y)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = 0.f;
	s_gxVertex.pw = 1.f;

	gxEmitVertex();
}

void gxVertex3f(float x, float y, float z)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = z;
	s_gxVertex.pw = 1.f;
	
	gxEmitVertex();
}

void gxVertex4f(float x, float y, float z, float w)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = z;
	s_gxVertex.pw = w;

	gxEmitVertex();
}

void gxVertex3fv(const float * v)
{
	gxVertex3f(v[0], v[1], v[2]);
}

void gxEmitVertex()
{
	s_gxVertices[s_gxVertexCount++] = s_gxVertex;
	
	if (s_gxVertexCount + s_gxPrimitiveSize > s_gxMaxVertexCount)
	{
		if (s_gxVertexCount % s_gxPrimitiveSize == 0)
		{
			gxFlush(false);
		}
	}
}

void gxSetTexture(GLuint texture)
{
	glActiveTexture(GL_TEXTURE0);
	
	if (texture)
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();
		
		s_gxTextureEnabled = true;
	}
	else
	{
		s_gxTextureEnabled = false;
	}
}

#else

void gxBegin(int primitiveType)
{
	//clearShader();

	glBegin(primitiveType);
}

void gxGetMatrixf(GLenum mode, float * m)
{
	switch (mode)
	{
	case GL_PROJECTION:
		glGetFloatv(GL_PROJECTION_MATRIX, m);
		checkErrorGL();
		break;

	case GL_MODELVIEW:
		glGetFloatv(GL_MODELVIEW_MATRIX, m);
		checkErrorGL();
		break;

	default:
		fassert(false);
		break;
	}
}

void gxSetTexture(GLuint texture)
{
	if (texture)
	{
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, texture);
		checkErrorGL();
	}
	else
	{
	#if USE_LEGACY_OPENGL
		glDisable(GL_TEXTURE_2D);
		checkErrorGL();
	#endif
	}
}

#endif

// builtin shaders

static void makeGaussianKernel(const int kernelSize, ShaderBuffer & kernel)
{
	const float s = 1.632f;
	//const float s = 1.8355f;
	
	auto dist = [](const float x) { return .5f * erfcf(-x); };
	
	float * values = (float*)alloca(sizeof(float) * kernelSize);
	
	if (kernelSize > 0)
	{
		for (int i = 0; i < kernelSize; ++i)
		{
			const float x1 = (i - .5f) / float(kernelSize - 1.f);
			const float x2 = (i + .5f) / float(kernelSize - 1.f);
			
			const float y1 = dist(x1 * s);
			const float y2 = dist(x2 * s);
			
			const float dy = y2 - y1;
			
			//printf("%02.2f - %02.2f : %02.4f\n", x1, x2, dy);
			
			values[i] = dy;
		}
		
		float total = values[0];
		
		for (int i = 1; i < kernelSize; ++i)
			total += values[i] * 2.f;
		
		if (total > 0.f)
		{
			for (int i = 0; i < kernelSize; ++i)
			{
				values[i] /= total;
			}
		}
	}
	
	kernel.setData(values, sizeof(float) * kernelSize);
}

void setShader_GaussianBlurH(const GLuint source, const int kernelSize, const float radius)
{
	Shader & shader = globals.builtinShaders->gaussianBlurH.get();
	setShader(shader);

	static ShaderBuffer kernel;
	makeGaussianKernel(kernelSize, kernel);
	
	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("kernelSize", kernelSize);
	shader.setImmediate("radius", radius);
	shader.setBuffer("kernel", kernel);
}

void setShader_GaussianBlurV(const GLuint source, const int kernelSize, const float radius)
{
	Shader & shader = globals.builtinShaders->gaussianBlurV.get();
	setShader(shader);
	
	static ShaderBuffer kernel;
	makeGaussianKernel(kernelSize, kernel);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("kernelSize", kernelSize);
	shader.setImmediate("radius", radius);
	shader.setBuffer("kernel", kernel);
}

// todo : move these shaders to BuiltinShaders and supply shader source

static void setShader_TresholdLumiEx(
	const GLuint source,
	const float treshold,
	const Vec4 weights,
	bool doFailReplacement,
	bool doPassReplacement,
	const Color & failColor,
	const Color & passColor)
{
	//Shader & shader = globals.builtinShaders->tresholdEx;
	static Shader shader("builtin-treshold-ex", "builtin-effect.vs", "builtin-treshold-ex.ps");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("settings", treshold, doFailReplacement, doPassReplacement);
	shader.setImmediate("weights", weights[0], weights[1], weights[2], weights[3]);
	shader.setImmediate("failValue", failColor.r, failColor.g, failColor.b, failColor.a);
	shader.setImmediate("passValue", passColor.r, passColor.g, passColor.b, passColor.a);
}

static const Vec4 lumiVec(.30f, .59f, .11f, 0.f);

void setShader_TresholdLumi(const GLuint source, const float lumi, const Color & failColor, const Color & passColor)
{
	//Shader & shader = globals.builtinShaders->tresholdLumi;
	static Shader shader("builtin-treshold-lumi", "builtin-effect.vs", "builtin-treshold-lumi.ps");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("treshold", lumi);
	shader.setImmediate("failColor", failColor.r, failColor.g, failColor.b);
	shader.setImmediate("passColor", passColor.r, passColor.g, passColor.b);
}

void setShader_TresholdLumiFail(const GLuint source, const float lumi, const Color & failColor)
{
	setShader_TresholdLumiEx(
		source,
		lumi,
		lumiVec,
		true,
		false,
		failColor,
		colorWhite);
}

void setShader_TresholdLumiPass(const GLuint source, const float lumi, const Color & passColor)
{
	setShader_TresholdLumiEx(
		source,
		lumi,
		lumiVec,
		false,
		true,
		colorWhite,
		passColor);
}

void setShader_TresholdValue(const GLuint source, const Color & value, const Color & failColor, const Color & passColor)
{
	//Shader & shader = globals.builtinShaders->tresholdValue;
	static Shader shader("builtin-treshold-value", "builtin-effect.vs", "builtin-treshold-value.ps");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("treshold", value.r, value.g, value.b, value.a);
	shader.setImmediate("failValue", failColor.r, failColor.g, failColor.b, failColor.a);
	shader.setImmediate("passValue", passColor.r, passColor.g, passColor.b, passColor.a);
}

void setShader_GrayscaleLumi(const GLuint source, const float opacity)
{
	//Shader & shader = globals.builtinShaders->grayscaleLumi;
	static Shader shader("builtin-grayscale-lumi", "builtin-effect.vs", "builtin-grayscale-lumi.ps");
	setShader(shader);
	shader.setImmediate("opacity", opacity);

	shader.setTexture("source", 0, source, true, true);
}

void setShader_GrayscaleWeights(const GLuint source, const Vec3 & weights, const float opacity)
{
	//Shader & shader = globals.builtinShaders->grayscaleWeights;
	static Shader shader("builtin-grayscale-weights", "builtin-effect.vs", "builtin-grayscale-weights.ps");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("weights", weights[0], weights[1], weights[2]);
	shader.setImmediate("opacity", opacity);
}

void setShader_Colorize(const GLuint source, const float hue, const float opacity)
{
	//Shader & shader = globals.builtinShaders->hueAssign;
	static Shader shader("builtin-hue-assign", "builtin-effect.vs", "builtin-hue-assign.ps");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("hue", hue);
	shader.setImmediate("opacity", opacity);
}

void setShader_HueShift(const GLuint source, const float hue, const float opacity)
{
	//Shader & shader = globals.builtinShaders->hueShift;
	static Shader shader("builtin-hue-shift", "builtin-effect.vs", "builtin-hue-shift.ps");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
	shader.setImmediate("hueShift", hue);
	shader.setImmediate("opacity", opacity);
}

void setShader_Compositie(const GLuint source1, const GLuint source2)
{
	//Shader & shader = globals.builtinShaders->compositeAlpha;
	Shader shader("builtin-composite-alpha");
	setShader(shader);

	shader.setTexture("source1", 0, source1, true, true);
	shader.setTexture("source2", 1, source2, true, true);
}

void setShader_CompositiePremultiplied(const GLuint source1, const GLuint source2)
{
	//Shader & shader = globals.builtinShaders->compositeAlphaPremultiplied;
	Shader shader("builtin-composite-alpha-premultiplied");
	setShader(shader);

	shader.setTexture("source1", 0, source1, true, true);
	shader.setTexture("source2", 1, source2, true, true);
}

void setShader_Premultiply(const GLuint source)
{
	//Shader & shader = globals.builtinShaders->premultiplyAlpha;
	Shader shader("builtin-premultiply-alpha");
	setShader(shader);

	shader.setTexture("source", 0, source, true, true);
}

void setShader_ColorMultiply(const GLuint source, const Color & color, const float opacity)
{
	Shader & shader = globals.builtinShaders->colorMultiply.get();
	setShader(shader);
	
	shader.setTexture("source", 0, source);
	shader.setImmediate("color", color.r, color.g, color.b, color.a);
	shader.setImmediate("opacity", opacity);

}

void setShader_ColorTemperature(const GLuint source, const float temperature, const float opacity)
{
	Shader & shader = globals.builtinShaders->colorTemperature.get();
	setShader(shader);
	
	shader.setTexture("source", 0, source);
	shader.setImmediate("temperature", temperature);
	shader.setImmediate("opacity", opacity);
}

//

static void setShader_HqLines()
{
	setShader(globals.builtinShaders->hqLine.get());
}

static void setShader_HqFilledTriangles()
{
	setShader(globals.builtinShaders->hqFilledTriangle.get());
}

static void setShader_HqFilledCircles()
{
	setShader(globals.builtinShaders->hqFilledCircle.get());
}

static void setShader_HqFilledRects()
{
	setShader(globals.builtinShaders->hqFilledRect.get());
}

static void setShader_HqFilledRoundedRects()
{
	setShader(globals.builtinShaders->hqFilledRoundedRect.get());
}

static void setShader_HqStrokedTriangles()
{
	setShader(globals.builtinShaders->hqStrokeTriangle.get());
}

static void setShader_HqStrokedCircles()
{
	setShader(globals.builtinShaders->hqStrokedCircle.get());
}

static void setShader_HqStrokedRects()
{
	setShader(globals.builtinShaders->hqStrokedRect.get());
}

void hqBegin(HQ_TYPE type, bool useScreenSize)
{
	switch (type)
	{
	case HQ_LINES:
		gxBegin(GL_QUADS);
		setShader_HqLines();
		break;

	case HQ_FILLED_TRIANGLES:
		gxBegin(GL_TRIANGLES);
		setShader_HqFilledTriangles();
		break;

	case HQ_FILLED_CIRCLES:
		gxBegin(GL_QUADS);
		setShader_HqFilledCircles();
		break;

	case HQ_FILLED_RECTS:
		gxBegin(GL_QUADS);
		setShader_HqFilledRects();
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxBegin(GL_QUADS);
		setShader_HqFilledRoundedRects();
		break;

	case HQ_STROKED_TRIANGLES:
		gxBegin(GL_TRIANGLES);
		setShader_HqStrokedTriangles();
		break;

	case HQ_STROKED_CIRCLES:
		gxBegin(GL_QUADS);
		setShader_HqStrokedCircles();
		break;

	case HQ_STROKED_RECTS:
		gxBegin(GL_QUADS);
		setShader_HqStrokedRects();
		break;

	default:
		fassert(false);
		break;
	}

	if (globals.shader != nullptr && globals.shader->getType() == SHADER_VSPS)
	{
		Shader * shader = static_cast<Shader*>(globals.shader);

		shader->setImmediate("useScreenSize", useScreenSize ? 1.f : 0.f);
		
		if (s_gxTextureEnabled)
			shader->setTextureUnit("source", 0);
		shader->setImmediate("sourceEnabled", s_gxTextureEnabled ? 1.f : 0.f);

		//shader->setImmediate("disableOptimizations", cos(framework.time * 6.28f) < 0.f ? 0.f : 1.f);
		//shader->setImmediate("disableAA", cos(framework.time) < 0.f ? 0.f : 1.f);

		shader->setImmediate("disableOptimizations", 0.f);
		shader->setImmediate("disableAA", 0.f);
		shader->setImmediate("_debugHq", 0.f);
	}
}

void hqBeginCustom(HQ_TYPE type, Shader & shader, bool useScreenSize)
{
	switch (type)
	{
	case HQ_LINES:
		gxBegin(GL_QUADS);
		break;

	case HQ_FILLED_TRIANGLES:
		gxBegin(GL_TRIANGLES);
		break;

	case HQ_FILLED_CIRCLES:
		gxBegin(GL_QUADS);
		break;

	case HQ_FILLED_RECTS:
		gxBegin(GL_QUADS);
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxBegin(GL_QUADS);
		break;

	case HQ_STROKED_TRIANGLES:
		gxBegin(GL_TRIANGLES);
		break;

	case HQ_STROKED_CIRCLES:
		gxBegin(GL_QUADS);
		break;

	case HQ_STROKED_RECTS:
		gxBegin(GL_QUADS);
		break;

	default:
		fassert(false);
		break;
	}
	
	setShader(shader);

	if (globals.shader != nullptr && globals.shader->getType() == SHADER_VSPS)
	{
		Shader * shader = static_cast<Shader*>(globals.shader);

		shader->setImmediate("useScreenSize", useScreenSize ? 1.f : 0.f);

		//shader->setImmediate("disableOptimizations", cos(framework.time * 6.28f) < 0.f ? 0.f : 1.f);
		//shader->setImmediate("disableAA", cos(framework.time) < 0.f ? 0.f : 1.f);

		shader->setImmediate("disableOptimizations", 0.f);
		shader->setImmediate("disableAA", 0.f);
		shader->setImmediate("_debugHq", 0.f);
	}
}

void hqEnd()
{
	gxEnd();

	clearShader();
}

void hqLine(float x1, float y1, float strokeSize1, float x2, float y2, float strokeSize2)
{
	gxNormal3f(strokeSize1, strokeSize2, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqFillTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
	gxNormal3f(x3, y3, 0.f);
	for (int i = 0; i < 3; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqFillCircle(float x, float y, float radius)
{
	gxNormal3f(radius, 0.f, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex2f(x, y);
}

void hqFillRect(float x1, float y1, float x2, float y2)
{
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqFillRoundedRect(float x1, float y1, float x2, float y2, float radius)
{
	gxNormal3f(radius, 0.f, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqStrokeTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float stroke)
{
	gxNormal3f(x3, y3, stroke);
	for (int i = 0; i < 3; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqStrokeCircle(float x, float y, float radius, float stroke)
{
	gxNormal3f(radius, stroke, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex2f(x, y);
}

void hqStrokeRect(float x1, float y1, float x2, float y2, float stroke)
{
	gxNormal3f(stroke, 0.f, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

void hqDrawPath(const Path2d & path)
{
	const int kMaxPoints = 16 * 1024;

	float pxyStorage[kMaxPoints * 2];
	float hxyStorage[kMaxPoints * 2];

	float * pxy = pxyStorage;
	float * hxy = hxyStorage;

	int numPoints = 0;
	path.generatePoints(pxy, hxy, kMaxPoints, 1.f, numPoints);

	hqBegin(HQ_LINES);
	{
		for (int i = 0; i < numPoints - 1; ++i)
		{
			hqLine(pxy[0], pxy[1], 1.f, pxy[2], pxy[3], 1.f);

			pxy += 2;
			hxy += 2;
		}
	}
	hqEnd();

#if 0
	pxy = pxyStorage;
	hxy = hxyStorage;

	hqBegin(HQ_LINES);
	{
		for (int i = 0; i < numPoints; ++i)
		{
			const float tx = hxy[0];
			const float ty = hxy[1];
			//const float ts = std::hypot(tx, ty);

			hqLine(
				pxy[0],          pxy[1],          1.f,
				pxy[0] + hxy[0], pxy[1] + hxy[1], 1.f);

			pxy += 2;
			hxy += 2;
		}
	}
	hqEnd();
#endif

#if 0
	pxy = pxyStorage;

	static bool useScreenSpace = false;
	static int mode = 0;

	if (keyboard.wentDown(SDLK_a))
		useScreenSpace = !useScreenSpace;
	if (keyboard.wentDown(SDLK_s))
		mode = (mode + 1) % 4;

	if (mode == 0)
		hqBegin(HQ_FILLED_RECTS, useScreenSpace);
	if (mode == 1)
		hqBegin(HQ_FILLED_CIRCLES, useScreenSpace);
	if (mode == 2)
		hqBegin(HQ_STROKED_RECTS, useScreenSpace);
	if (mode == 3)
		hqBegin(HQ_STROKED_CIRCLES, useScreenSpace);
	{
		const float strokeSize = (std::sin(framework.time) + 1.f) / 2.f * 4.f;

		for (int i = 0; i < numPoints; ++i)
		{
			if (mode == 0)
			{
				hqFillRect(
					pxy[0] - 5.f, pxy[1] - 5.f,
					pxy[0] + 5.f, pxy[1] + 5.f);
			}

			if (mode == 1)
			{
				hqFillCircle(pxy[0], pxy[1], 5.f);
			}

			if (mode == 2)
			{
				hqStrokeRect(
					pxy[0] - 5.f, pxy[1] - 5.f,
					pxy[0] + 5.f, pxy[1] + 5.f,
					strokeSize);
			}

			if (mode == 3)
			{
				hqStrokeCircle(pxy[0], pxy[1], 5.f, strokeSize);
			}

			pxy += 2;
		}
	}
	hqEnd();
#endif
}

//

void changeDirectory(const char * path)
{
	_chdir(path);
}

#if ENABLE_LOGGING

static int logLevel = 0;

#if ENABLE_LOGGING_DBG

void logDebug(const char * format, ...)
{
	if (logLevel > 0)
		return;

	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[DD] %s\n", text);
}

#endif

void logInfo(const char * format, ...)
{
	if (logLevel > 1)
		return;

	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[II] %s\n", text);
}

void logWarning(const char * format, ...)
{
	if (logLevel > 2)
		return;

	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[WW] %s\n", text);
}

void logError(const char * format, ...)
{
	if (logLevel > 3)
		return;

	char text[1 << 16];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[EE] %s\n", text);
}

#endif
