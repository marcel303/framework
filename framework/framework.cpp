/*
	Copyright (C) 2020 Marcel Smit
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

#ifndef NOMINMAX
	#define NOMINMAX
#endif

#include <algorithm>
#include <cmath>
#include <inttypes.h>
#include <limits>
#include <map>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <vector>

#ifdef WIN32
	#include <Windows.h>
	#include <direct.h>
	#include <GL/glew.h>
	#include <SDL2/SDL_opengl.h>
	#include <SDL2/SDL_syswm.h>
	#include <Xinput.h>
	#define PATH_MAX MAX_PATH
#else
	#include <dirent.h>
	#include <unistd.h>
	#define _chdir chdir
#endif

#if defined(IPHONEOS)
	#include <OpenGLES/ES3/gl.h>
	#include <OpenGLES/ES3/glext.h>
#elif defined(ANDROID)
	#include "android-assetcopy.h"
	#include "framework-android-app.h"
	#include <android_native_app_glue.h>
	#include <GLES3/gl3.h>
#else
	#include <GL/glew.h>
#endif

#include "audio.h"
#include "framework.h"
#include "gx_render.h" // push/popRenderPass
#include "image.h" // for window icon
#include "internal.h"
#include "model.h" // updateAnimation
#include "rte.h"

#define ENABLE_DISPLAY_SIZE_SCALING 0 // todo : make this work with resizable windows

#include "Base64.h" // for decoding chibi resource paths
#include "Csv.h" // for decoding chibi resource paths
#include "StringEx.h"
#include "Timer.h"

#if ENABLE_METAL
	#include "gx-metal/metal.h"
#endif

#if FRAMEWORK_USE_OVR_MOBILE
	#include "framework-ovr.h"

	// todo : remove these include once controller input is handled more nicely
	#include <VrApi_Helpers.h>
	#include <VrApi_Input.h>
#endif

// -----

#if defined(_MSC_VER) && _MSC_VER >= 1900
	#pragma comment(lib, "legacy_stdio_definitions.lib")
#endif

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT && ENABLE_DESKTOP_OPENGL
#if defined(WIN32)
	void __stdcall debugOutputGL(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const GLvoid*);
#else
	void debugOutputGL(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar*, const GLvoid*);
#endif
#endif

#ifndef HAS_SCALE255
#define HAS_SCALE255

static const float rcp255 = 1.f / 255.f;

static inline float scale255(const float v)
{
	return v * rcp255;
}

#endif

static int getCurrentBackingScale();
static void getCurrentBackingSize(int & sx, int & sy);
static void getCurrentViewportSize(int & sx, int & sy);

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
Mouse mouse;
Keyboard keyboard;
Gamepad gamepad[MAX_GAMEPAD];

// -----

int s_backingScale = 1; // global backing scale multiplier. a bit of a hack as it assumed the scale never changes, but works well for most apps in most situations for now..

static Stack<COLOR_MODE, 32> colorModeStack(COLOR_MUL);
static Stack<COLOR_POST, 32> colorPostStack(POST_NONE);

static std::map<std::string, std::string> s_shaderSources;

//

Framework::Framework()
{
	waitForEvents = false;
	fullscreen = false;
	exclusiveFullscreen = true;
	useClosestDisplayMode = false;
	enableVsync = true;
	msaaLevel = 0;
	basicOpenGL = false;
	enableDepthBuffer = false;
	enableDrawTiming = true;
	enableProfiling = false;
	allowHighDpi = true;
	minification = 1;
	reloadCachesOnActivate = false;
	cacheResourceData = false;
#if defined(DEBUG) // todo : make this distribution. let libgg also set DISTRIBUTION define when config is Distribution
	enableRealTimeEditing = true;
#else
	enableRealTimeEditing = false;
#endif
	manualVrMode = false;
	filedrop = false;
	windowX = -1;
	windowY = -1;
	windowBorder = true;
	windowIsResizable = false;
	windowTitle.clear();
	windowSx = 0;
	windowSy = 0;
	windowIsActive = false;
	enableSound = true;
	numSoundSources = 32;
	actionHandler = 0;
	fillCachesCallback = 0;
	fillCachesUnknownResourceCallback = 0;
	realTimeEditCallback = 0;
	initErrorHandler = 0;
	
	events.clear();
	changedFiles.clear();
	droppedFiles.clear();
	
	quitRequested = false;
	time = 0.f;
	timeStep = 1.f / 60.f;
	
	m_lastTick = -1;

	m_sprites = 0;
	m_models = 0;
	m_windows = 0;
}

Framework::~Framework()
{
}

bool Framework::init(int sx, int sy)
{
#ifdef WIN32
	//SetProcessDPIAware();

	/*
	todo : high dpi support on Windows

	Either support will need to be added to SDL, or, framework will need to implement it.
	Idea:
	1) Set process dpi awareness flag.
	2) Detect DPI for default display.
	3) Compare DPI with 'standard' dpi. If high (almost 2x more), assume a high dpi display.
	4) If a high dpi display is detected, set backing scale to 2. This will ensure additional pixel density for surfaces.
	5) Multiply window size times backing scale, divide mouse coordinates times backing scale. This will ensure additioal pixel density for the back buffers.
	*/
#endif

#if FRAMEWORK_USE_SDL
	// initialize SDL
	
	const int initFlags = SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK;
	
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

#if defined(IPHONEOS)
// todo : add framework option to select orientation, instead of hard-coding it here
	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
#endif

#if ENABLE_OPENGL
	#if defined(IPHONEOS)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#elif USE_LEGACY_OPENGL
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	#elif FRAMEWORK_USE_OPENGL_ES
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#else
		#if OPENGL_VERSION == 430
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		#endif
		#if OPENGL_VERSION == 410
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		#endif
	#endif
	
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, msaaLevel >= 2 ? 1 : 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, msaaLevel >= 2 ? msaaLevel : 0);
	
	if (enableDepthBuffer)
	{
	#if USE_LEGACY_OPENGL
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	#elif FRAMEWORK_USE_OPENGL_ES
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	#else
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	#endif
	}
	else
	{
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
	}
	
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT && ENABLE_DESKTOP_OPENGL
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	
	flags |= SDL_WINDOW_OPENGL;
#endif

	if (fullscreen && minification == 1)
	{
		flags |= SDL_WINDOW_FULLSCREEN;
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
	
	if (windowIsResizable)
		flags |= SDL_WINDOW_RESIZABLE;
	
	if (allowHighDpi)
		flags |= SDL_WINDOW_ALLOW_HIGHDPI;
	
	fassert(globals.mainWindow == nullptr);
	SDL_Window * mainWindow = SDL_CreateWindow(
		windowTitle.c_str(),
		windowX == -1 ? SDL_WINDOWPOS_CENTERED : windowX,
		windowY == -1 ? SDL_WINDOWPOS_CENTERED : windowY,
		actualSx,
		actualSy,
		flags);

	if (!mainWindow)
	{
		logError("failed to set video mode (%dx%d @ %dbpp): %s", sx / minification, sy / minification, 32, SDL_GetError());
		if (initErrorHandler)
			initErrorHandler(INIT_ERROR_WINDOW);
		return false;
	}

	globals.mainWindow = new Window(mainWindow);
	
	fassert(globals.currentWindow == nullptr);
	globals.currentWindow = globals.mainWindow;

#if ENABLE_OPENGL
	int drawableSx;
	int drawableSy;
	SDL_GL_GetDrawableSize(globals.currentWindow->getWindow(), &drawableSx, &drawableSy);
	s_backingScale = (int)roundf(fmaxf(drawableSx / float(actualSx), drawableSy / float(actualSy)));
	if (s_backingScale < 1)
		s_backingScale = 1;
	
#if defined(IPHONEOS)
	// fixme : the backing scale is incorrect on iOS. SDL_GL_GetDrawableSize returns an invalid size
	s_backingScale = 1;
#endif
	
	fassert(globals.glContext == nullptr);
	globals.glContext = SDL_GL_CreateContext(globals.mainWindow->m_window);
	checkErrorGL();
	
	if (!globals.glContext)
	{
		logError("failed to create OpenGL context: %s", SDL_GetError());
		if (initErrorHandler)
			initErrorHandler(INIT_ERROR_OPENGL);
		return false;
	}
	
	{
		const char * renderer = (char*)glGetString(GL_RENDERER);
		const char * version = (char*)glGetString(GL_VERSION);
		const char * vendor = (char*)glGetString(GL_VENDOR);
		const char * glsl_version = (char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
		(void)renderer;
		(void)version;
		(void)vendor;
		(void)glsl_version;
		
		logInfo("OpenGL renderer: %s", renderer ? renderer : "unknown");
		logInfo("OpenGL version: %s", version ? version : "unknown");
		logInfo("OpenGL vendor: %s", vendor ? vendor : "unknown");
		logInfo("OpenGL GLSL version: %s", glsl_version ? glsl_version : "unknown");
	}
	
#if ENABLE_DESKTOP_OPENGL
	if (!basicOpenGL)
	{
		const int glewStatus = glewInit();
		checkErrorGL();

		if (glewStatus != GLEW_OK)
		{
			logError("failed to initialize GLEW: %s", glewGetErrorString(glewStatus));
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_OPENGL_EXTENSIONS);
			return false;
		}

		logInfo("using OpenGL %s, %s, GLEW %s", glGetString(GL_VERSION), glGetString(GL_VENDOR), glewGetString(GLEW_VERSION));
	
	#if !USE_LEGACY_OPENGL
		if (!GLEW_VERSION_3_2)
		{
			logWarning("OpenGL 3.2 not supported");
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_OPENGL_EXTENSIONS);
			return false;
		}
	#else
		if (!GLEW_VERSION_2_1)
		{
			logWarning("OpenGL 2.1 not supported");
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_OPENGL_EXTENSIONS);
			return false;
		}
		
		if (glBlendEquation == nullptr)
			logWarning("OpenGL extension glBlendEquation not found");
		if (glClampColor == nullptr)
			logWarning("OpenGL extension glClampColor not found");
	#endif
	}
#endif

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT && ENABLE_DESKTOP_OPENGL
	if (GLEW_ARB_debug_output)
	{
		logInfo("using OpenGL debug output");
		glDebugMessageCallbackARB((GLDEBUGPROCARB)debugOutputGL, stderr);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
	}
#endif

	SDL_GL_SetSwapInterval(enableVsync ? 1 : 0);
#endif

#if ENABLE_METAL
	metal_init();
	
	metal_attach(globals.mainWindow->getWindow());
	
	metal_make_active(globals.mainWindow->getWindow());
#endif
#else // !FRAMEWORK_USE_SDL
#if FRAMEWORK_USE_OVR_MOBILE
	globals.egl.createContext();
#endif

	globals.mainWindow = new Window("Framework", sx, sy, windowIsResizable);

	fassert(globals.currentWindow == nullptr);
	globals.currentWindow = globals.mainWindow;

	windowSx = sx;
	windowSy = sy;

	s_backingScale = 1;
#endif
	
	globals.displaySize[0] = sx;
	globals.displaySize[1] = sy;

	registerShaderOutput('c', "vec4", "shader_fragColor");
	registerShaderOutput('n', "vec4", "shader_fragNormal");
	
	gxInitialize();
	
#ifdef __WIN32__
	// we use XInput on Windows
#elif FRAMEWORK_USE_SDL
	// initialize SDL joysticks
	
	const int numJoysticks = SDL_NumJoysticks();
	
	for (int i = 0; i < numJoysticks && i < MAX_GAMEPAD; ++i)
	{
		fassert(globals.joystick[i] == nullptr);
		globals.joystick[i] = SDL_JoystickOpen(i);
	}
#endif

#if ENABLE_PROFILING
	if (framework.enableProfiling)
	{
		fassert(globals.rmt == nullptr);
		if (rmt_CreateGlobalInstance(&globals.rmt) != RMT_ERROR_NONE)
			return false;
		rmt_BindOpenGL();
	}
#endif
	
#if FRAMEWORK_USE_OVR_MOBILE
	if (!frameworkOvr.init())
	{
		logError("failed to initialize ovr mobile");
		return false;
	}

	// process once (to ensure we are in vr mode) and show a loading screen
	frameworkOvr.process();
	frameworkOvr.showLoadingScreen();
#endif
	
#if defined(ANDROID)
	{
		android_app * app = get_android_app();
		const double t1 = GetSystemTime();
		const bool copied_files =
			chdir(app->activity->internalDataPath) == 0 &&
			assetcopy::recursively_copy_assets_to_filesystem(
				app->activity->vm,
				app->activity->clazz,
				app->activity->assetManager,
				"") &&
			chdir(app->activity->internalDataPath) == 0;
		const double t2 = GetSystemTime();
		logInfo("asset copying took %.2f seconds", (t2 - t1));
		if (copied_files == false)
		{
			logError("failed to copy assets to filesystem. destination path: %s", app->activity->internalDataPath);
			return false;
		}
	}
#endif

	fassert(globals.builtinShaders == nullptr);
	globals.builtinShaders = new BuiltinShaders();

#if USE_FREETYPE
	// initialize FreeType
	
	fassert(globals.freeType == nullptr);
	if (FT_Init_FreeType(&globals.freeType) != 0)
	{
		logError("failed to initialize FreeType");
		if (initErrorHandler)
			initErrorHandler(INIT_ERROR_FREETYPE);
		return false;
	}
#endif
	
	// initialize sound player
	
#if !defined(LINUX) // todo : make sure PortAudio sound player works correctly on the Raspberry Pi
	if (enableSound)
	{
		if (!g_soundPlayer.init(numSoundSources))
		{
		// todo : check if this now works on Linux
			logError("failed to initialize sound player");
			if (initErrorHandler)
				initErrorHandler(INIT_ERROR_SOUND);
			return false;
		}
	}
#endif

	// initialize real time editing

	if (enableRealTimeEditing)
	{
		initRealTimeEditing();
	}

#if FRAMEWORK_USE_SDL
	// set icon

	if (!windowIcon.empty())
	{
		ImageData * iconData = loadImage(windowIcon.c_str());
		SDL_Surface * surface = SDL_CreateRGBSurfaceFrom(iconData->imageData, iconData->sx, iconData->sy, 32, iconData->sx * sizeof(ImageData::Pixel), 0xff << 0, 0xff << 8, 0xff << 16, 0xff << 24);
		SDL_SetWindowIcon(globals.mainWindow->m_window, surface);
		SDL_FreeSurface(surface);
		delete iconData;
	}
	
	// make sure we are focused
	
	SDL_RaiseWindow(globals.currentWindow->getWindow());

	SDL_DisableScreenSaver();
#endif

	// set the default font
	
	setFont("engine/fonts/Roboto-Regular.ttf");

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
#if ENABLE_MSDF_FONTS
	g_fontCacheMSDF.clear();
#endif
	g_glyphCache.clear();
	
#if USE_FREETYPE
	// shut down FreeType
	
	if (globals.freeType && FT_Done_FreeType(globals.freeType) != 0)
	{
		logError("failed to shut down FreeType");
		result = false;
	}
	globals.freeType = 0;
#endif
	
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
	
	s_shaderSources.clear();
	
#if ENABLE_METAL
	// destroy metal context
	
	metal_detach(globals.mainWindow->getWindow());
#endif

#if ENABLE_OPENGL
#if ENABLE_DESKTOP_OPENGL
	glBlendEquation = 0;
	glClampColor = 0;
#endif
	
#if FRAMEWORK_USE_SDL
	// destroy SDL OpenGL context
	
	if (globals.glContext)
	{
		SDL_GL_DeleteContext(globals.glContext);
		globals.glContext = 0;
	}
#endif
#endif
	
	// destroy SDL window
	
	if (globals.mainWindow)
	{
		fassert(globals.currentWindow == globals.mainWindow);
		
		delete globals.mainWindow;
		globals.mainWindow = nullptr;
		
		globals.currentWindow = nullptr;
	}
	
#if FRAMEWORK_USE_SDL
	// shut down SDL
	
	SDL_Quit();
#endif

#if FRAMEWORK_USE_OVR_MOBILE
	frameworkOvr.shutdown();
	globals.egl.destroyContext();
#endif
	
	// clear globals
	
	globals = Globals();
	
	// reset self
	
	quitRequested = false;
	time = 0.f;

	waitForEvents = false;
	fullscreen = false;
	exclusiveFullscreen = true;
	useClosestDisplayMode = false;
	enableVsync = true;
	msaaLevel = 0;
	basicOpenGL = false;
	enableDepthBuffer = false;
	enableDrawTiming = true;
	enableProfiling = false;
	allowHighDpi = true;
	minification = 1;
	reloadCachesOnActivate = false;
	cacheResourceData = false;
#if defined(DEBUG) // todo : make this distribution. let libgg also set DISTRIBUTION define when config is Distribution
	enableRealTimeEditing = true;
#else
	enableRealTimeEditing = false;
#endif
	manualVrMode = false;
	filedrop = false;
	enableSound = true;
	numSoundSources = 32;
	windowX = -1;
	windowY = -1;
	windowBorder = true;
	windowIsResizable = false;
	windowTitle.clear();
	windowSx = 0;
	windowSy = 0;
	windowIsActive = false;
	actionHandler = 0;
	fillCachesCallback = 0;
	fillCachesUnknownResourceCallback = 0;
	realTimeEditCallback = 0;
	initErrorHandler = 0;
	
	events.clear();
	changedFiles.clear();
	droppedFiles.clear();
	
	m_lastTick = -1;
	
	return result;
}

struct XMap
{
	enum Source
	{
		kSource_None,
		kSource_Axis,
		kSource_Button,
		kSource_Hat
	};
	
	struct Elem
	{
		Source source;
		int sourceId;
		int sourceChildId;
	};
	
	const char * deviceName;
	
	Elem buttons[GAMEPAD_MAX];
	Elem axis[2][ANALOG_MAX];
};

/*
	DPAD_LEFT,
	DPAD_RIGHT,
	DPAD_UP,
	DPAD_DOWN,
	GAMEPAD_A,
	GAMEPAD_B,
	GAMEPAD_X,
	GAMEPAD_Y,
	GAMEPAD_L1,
	GAMEPAD_L2,
	GAMEPAD_R1,
	GAMEPAD_R2,
	GAMEPAD_START,
	GAMEPAD_BACK,
*/

#if FRAMEWORK_USE_SDL

static XMap zeemoteSteelseriesFree =
{
	"Zeemote: SteelSeries FREE",
	{
		{ XMap::kSource_Hat, 0, SDL_HAT_LEFT },
		{ XMap::kSource_Hat, 0, SDL_HAT_RIGHT },
		{ XMap::kSource_Hat, 0, SDL_HAT_UP },
		{ XMap::kSource_Hat, 0, SDL_HAT_DOWN },
		{ XMap::kSource_Button, 0 },
		{ XMap::kSource_Button, 1 },
		{ XMap::kSource_Button, 3 },
		{ XMap::kSource_Button, 4 },
		{ XMap::kSource_Button, 6 },
		{ XMap::kSource_None },
		{ XMap::kSource_Button, 7 },
		{ XMap::kSource_None },
		{ XMap::kSource_Button, 12 },
		{ XMap::kSource_Button, 11 }
	},
	{
		{
			{ XMap::kSource_Axis, 0 },
			{ XMap::kSource_Axis, 1 }
		},
		{
			{ XMap::kSource_Axis, 2 },
			{ XMap::kSource_Axis, 3 }
		}
	}
};

static XMap logitechDualAction =
{
	"Logitech Dual Action",
	{
		{ XMap::kSource_Hat, 0, SDL_HAT_LEFT },
		{ XMap::kSource_Hat, 0, SDL_HAT_RIGHT },
		{ XMap::kSource_Hat, 0, SDL_HAT_UP },
		{ XMap::kSource_Hat, 0, SDL_HAT_DOWN },
		{ XMap::kSource_Button, 1 },
		{ XMap::kSource_Button, 2 },
		{ XMap::kSource_Button, 0 },
		{ XMap::kSource_Button, 3 },
		{ XMap::kSource_Button, 4 },
		{ XMap::kSource_Button, 6 },
		{ XMap::kSource_Button, 5 },
		{ XMap::kSource_Button, 7 },
		{ XMap::kSource_Button, 8 },
		{ XMap::kSource_Button, 9 }
	},
	{
		{
			{ XMap::kSource_Axis, 0 },
			{ XMap::kSource_Axis, 1 }
		},
		{
			{ XMap::kSource_Axis, 2 },
			{ XMap::kSource_Axis, 3 }
		}
	}
};

static XMap ps3Controller =
{
	"PLAYSTATION(R)3 Controller",
	{
		{ XMap::kSource_Button, 7 }, // dpad lrud
		{ XMap::kSource_Button, 5 },
		{ XMap::kSource_Button, 4 },
		{ XMap::kSource_Button, 6 },
		{ XMap::kSource_Button, 14 }, // abxy
		{ XMap::kSource_Button, 13 },
		{ XMap::kSource_Button, 15 },
		{ XMap::kSource_Button, 12 },
		{ XMap::kSource_Button, 10 }, // l1/l2 r1/r2
		{ XMap::kSource_Button, 8 },
		{ XMap::kSource_Button, 11 },
		{ XMap::kSource_Button, 9 },
		{ XMap::kSource_Button, 0 }, // start, back
		{ XMap::kSource_Button, 3 }
	},
	{
		{
			{ XMap::kSource_Axis, 0 },
			{ XMap::kSource_Axis, 1 }
		},
		{
			{ XMap::kSource_Axis, 2 },
			{ XMap::kSource_Axis, 3 }
		}
	}
};

static const XMap * getXMap(const char * deviceName)
{
	const XMap * xmaps[3] =
	{
		&zeemoteSteelseriesFree,
		&logitechDualAction,
		&ps3Controller
	};
	
	for (int i = 0; i < 3; ++i)
	{
		if (strcmp(xmaps[i]->deviceName, deviceName) == 0)
		{
			return xmaps[i];
		}
	}
	
	return nullptr;
}

#endif

void Framework::process()
{
	cpuTimingBlock(frameworkProcess);
	
	g_soundPlayer.process();
	
#if FRAMEWORK_USE_SDL
	// poll SDL event queue

	bool doReload = false;

	keyboard.events.clear();

	for (Window * window = m_windows; window != nullptr; window = window->m_next)
		window->m_windowData->beginProcess();

	events.clear();

	changedFiles.clear();
	droppedFiles.clear();

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
			WindowData * windowData = findWindowDataById(e.key.windowID);
			
			if (windowData != nullptr)
			{
				windowData->keyEvents.push_back(e);
			}
		}
		else if (e.type == SDL_KEYUP)
		{
			WindowData * windowData = findWindowDataById(e.key.windowID);
			
			if (windowData != nullptr)
			{
				windowData->keyEvents.push_back(e);
			}
		}
		else if (e.type == SDL_TEXTINPUT)
		{
			WindowData * windowData = findWindowDataById(e.key.windowID);
			
			if (windowData != nullptr)
			{
				windowData->keyEvents.push_back(e);
			}
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
		{
			WindowData * windowData = findWindowDataById(e.button.windowID);
			
			if (windowData != nullptr)
			{
				//logDebug("mouse %d / %d", e.type, e.button.button);
				
				windowData->mouseData.addEvent(e);
			}
		}
		else if (e.type == SDL_MOUSEMOTION)
		{
			WindowData * windowData = findWindowDataById(e.motion.windowID);
			
			if (windowData != nullptr)
			{
				SDL_Window * window = SDL_GetWindowFromID(e.motion.windowID);
				
				if (window == globals.mainWindow->m_window)
				{
					int windowSx;
					int windowSy;
					SDL_GetWindowSize(window, &windowSx, &windowSy);
					
				#if ENABLE_DISPLAY_SIZE_SCALING
					SDL_Event scaledEvent = e;
					scaledEvent.motion.x = e.motion.x * globals.displaySize[0] / windowSx;
					scaledEvent.motion.y = e.motion.y * globals.displaySize[1] / windowSy;
					
					windowData->mouseData.addEvent(scaledEvent);
				#else
					windowData->mouseData.addEvent(e);
				#endif
					
					//logDebug("motion event: %d, %d -> %d, %d", e.motion.x, e.motion.y, windowData->mouseX, windowData->mouseY);
				}
				else
				{
					windowData->mouseData.addEvent(e);
				}
			}
		}
		else if (e.type == SDL_MOUSEWHEEL)
		{
			WindowData * windowData = findWindowDataById(e.wheel.windowID);
			
			if (windowData != nullptr)
			{
				if (e.wheel.which != SDL_TOUCH_MOUSEID)
				{
					windowData->mouseData.mouseScrollY += e.wheel.y * (e.wheel.direction == SDL_MOUSEWHEEL_NORMAL ? 1 : -1);
				}
			}
		}
		else if (e.type == SDL_WINDOWEVENT)
		{
			WindowData * windowData = findWindowDataById(e.window.windowID);
			
			if (windowData != nullptr)
			{
				if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
					windowData->isActive = true;
				else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
					windowData->isActive = false;
				else if (e.window.event == SDL_WINDOWEVENT_CLOSE)
					windowData->quitRequested = true;
				
				if (windowData == globals.currentWindow->getWindowData())
					windowIsActive = windowData->isActive;
				
				if (reloadCachesOnActivate && e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED && windowData == globals.mainWindow->m_windowData)
				{
					doReload |= true;
				}
			}
		}
		else if (e.type == SDL_DROPFILE)
		{
			droppedFiles.push_back(e.drop.file);
			
			Dictionary args;
			args.setString("file", e.drop.file);
			processAction("filedrop", args);
		}
		else if (e.type == SDL_QUIT)
		{
			quitRequested = true;
		}
	}

	for (Window * window = m_windows; window != nullptr; window = window->m_next)
		window->m_windowData->endProcess();

	globals.currentWindow->getWindowData()->makeActive();

	if (doReload)
	{
		reloadCaches();
	}
#endif

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
			
			sprintf_s(gamepad[i].name, "XInput %d", i + 1);
		}
		else
		{
			memset(&gamepad[i], 0, sizeof(Gamepad));
		}
	}

	globals.xinputGamepadIdx++;
#elif FRAMEWORK_USE_SDL
	SDL_JoystickUpdate();
	
	for (int i = 0; i < MAX_GAMEPAD; ++i)
	{
		SDL_Joystick * joy = globals.joystick[i];
		
		gamepad[i].isConnected = joy != nullptr && SDL_JoystickGetAttached(joy);
		
		const XMap * xmap = nullptr;
		
		if (gamepad[i].isConnected)
		{
			const char * deviceName = SDL_JoystickNameForIndex(i);
			
			xmap = getXMap(deviceName);
			
		#if 0
			const int numButtons = SDL_JoystickNumButtons(joy);
			const int numHats = SDL_JoystickNumHats(joy);
			const int numBalls = SDL_JoystickNumBalls(joy);
			
			for (int j = 0; j < numButtons; ++j)
			{
				auto value = SDL_JoystickGetButton(joy, j);
				
				if (value)
					printf("joy button %d\n", j);
			}
			
			for (int j = 0; j < numHats; ++j)
			{
				auto value = SDL_JoystickGetHat(joy, j);
				
				if (value)
					printf("joy hat %d: %x\n", j, value);
			}
			
			for (int j = 0; j < numBalls; ++j)
			{
				int dx;
				int dy;
				
				if (SDL_JoystickGetBall(joy, j, &dx, &dy) == 0 && (dx || dy))
				{
					printf("joy ball %d: %d, %d\n", j, dx, dy);
				}
			}
		#endif
		}
		
		if (gamepad[i].isConnected && xmap != nullptr)
		{
			strcpy_s(gamepad[i].name, sizeof(gamepad[i].name), SDL_JoystickNameForIndex(i));
			
			memset(gamepad[i].m_wentDown, 0, sizeof(gamepad[i].m_wentDown));
			memset(gamepad[i].m_wentUp, 0, sizeof(gamepad[i].m_wentUp));
			
		#define APPLY_DEADZONE(v, t) (std::abs(v) <= t ? 0.f : clamp<float>((std::abs(v) - t) * (v < 0.f ? -1.f : +1.f) / float(32767 - t), -1.f, +1.f))
		#define DEADZONE 1024
			
			bool * isDown = gamepad[i].m_isDown;

			bool wasDown[GAMEPAD_MAX];
			memcpy(wasDown, isDown, sizeof(wasDown));
			
			const int numAxes = SDL_JoystickNumAxes(joy);
			const int numButtons = SDL_JoystickNumButtons(joy);
			const int numHats = SDL_JoystickNumHats(joy);
			
			for (int j = 0; j < GAMEPAD_MAX; ++j)
			{
				auto & b = xmap->buttons[j];
				
				if (b.source == XMap::kSource_Button)
				{
					if (b.sourceId < numButtons)
						isDown[j] = SDL_JoystickGetButton(joy, b.sourceId) != 0;
				}
				else if (b.source == XMap::kSource_Hat)
				{
					if (b.sourceId < numHats)
						isDown[j] = SDL_JoystickGetHat(joy, b.sourceId) & b.sourceChildId;
				}
			}
			
			for (int j = 0; j < 2; ++j)
			{
				for (int k = 0; k < ANALOG_MAX; ++k)
				{
					auto & a = xmap->axis[j][k];
					
					if (a.source == XMap::kSource_Axis)
					{
						if (a.sourceId < numAxes)
							gamepad[i].m_analog[j][k] = APPLY_DEADZONE(SDL_JoystickGetAxis(joy, a.sourceId), DEADZONE);
					}
				}
			}
			
			bool * wentDown = gamepad[i].m_wentDown;
			bool * wentUp = gamepad[i].m_wentUp;

			for (int j = 0; j < GAMEPAD_MAX; ++j)
			{
				if (!wasDown[j] && isDown[j])
					wentDown[j] = true;
				if (wasDown[j] && !isDown[j])
					wentUp[j] = true;
			}
			
		#undef DEADZONE
		#undef APPLY_DEADZONE
		}
		else
		{
			memset(&gamepad[i], 0, sizeof(Gamepad));
		}
	}
#endif
	
	if (enableRealTimeEditing)
	{
		tickRealTimeEditing();
	}
	
	// begin time step for the next frame
	
#if FRAMEWORK_USE_SDL
#if 1
	// high accuracy time steps using SDL_GetPerformanceCounter/SDL_GetPerformanceFrequency
	const uint64_t tickCount = SDL_GetPerformanceCounter();
	if (m_lastTick == -1)
		m_lastTick = tickCount;
	const uint64_t delta = tickCount - m_lastTick;
	m_lastTick = tickCount;

	timeStep = delta / (double)SDL_GetPerformanceFrequency();
	
	time += timeStep;
#else
	// lower precision time steps using SDL_GetTicks. leaving this code here as the performance
	// counter version is still in testing
	const uint32_t tickCount = SDL_GetTicks();
	if (m_lastTick == -1)
		m_lastTick = tickCount;
	const uint32_t delta = tickCount - m_lastTick;
	m_lastTick = tickCount;

	timeStep = delta / 1000.f;

	time += timeStep;
#endif
#elif FRAMEWORK_USE_OVR_MOBILE
	if (manualVrMode == false)
	{
		// Render the eye images.
		for (int eyeIndex = 0; eyeIndex < frameworkOvr.getEyeCount(); ++eyeIndex)
		{
			frameworkOvr.beginEye(eyeIndex, colorBlack);
			{
				gxPushMatrix();
				gxTranslatef(0, 0, 0);
				{
					pushDepthTest(true, DEPTH_LESS);
					pushBlend(BLEND_OPAQUE);
					{
						framework.drawVirtualDesktop();
						for (auto * window = m_windows; window != nullptr; window = window->m_next)
							if (window->isHidden() == false && window->hasFocus())
								window->draw3dCursor();
					}
					popBlend();
					popDepthTest();
				}
				gxPopMatrix();
			}
			frameworkOvr.endEye();
		}
		
		frameworkOvr.submitFrameAndPresent();
	}
	
	// process events and begin the next frame
	frameworkOvr.process();

	timeStep = float(frameworkOvr.TimeStep);
	time = frameworkOvr.PredictedDisplayTime;
	
	if (manualVrMode == false)
	{
		// update virtual desktop. but first, find the pointer transform to use

		// todo : remove this global
		// todo : does ovr api provide a way to determine the active hand?
	static int activeController = 0;

		Mat4x4 pointerTransform[2] =
			{
				Mat4x4(true),
				Mat4x4(true)
			};

		bool pointerTransformIsValid = false;
		int buttonMasks[2] = { 0, 0 };

	#if true
		auto * ovr = frameworkOvr.Ovr;

		uint32_t index = 0;

		for (;;)
		{
			ovrInputCapabilityHeader header;

			if (vrapi_EnumerateInputDevices(ovr, index++, &header) < 0)
				break;

			if (header.Type != ovrControllerType_TrackedRemote)
				continue;

			ovrTracking tracking;
			if (vrapi_GetInputTrackingState(ovr, header.DeviceID, frameworkOvr.PredictedDisplayTime, &tracking) != ovrSuccess)
				tracking.Status = 0;

			ovrInputStateTrackedRemote state;
			state.Header.ControllerType = ovrControllerType_TrackedRemote;
			if (vrapi_GetCurrentInputState(ovr, header.DeviceID, &state.Header ) >= 0)
			{
				int index = -1;

				ovrInputTrackedRemoteCapabilities remoteCaps;
				remoteCaps.Header.Type = ovrControllerType_TrackedRemote;
				remoteCaps.Header.DeviceID = header.DeviceID;
				if (vrapi_GetInputDeviceCapabilities(ovr, &remoteCaps.Header) == ovrSuccess)
				{
					if (remoteCaps.ControllerCapabilities & ovrControllerCaps_LeftHand)
						index = 0;
					if (remoteCaps.ControllerCapabilities & ovrControllerCaps_RightHand)
						index = 1;
				}

				if (index != -1)
				{
					if (tracking.Status & VRAPI_TRACKING_STATUS_POSITION_VALID)
					{
						ovrMatrix4f transform = vrapi_GetTransformFromPose(&tracking.HeadPose.Pose);
						transform = ovrMatrix4f_Transpose(&transform);

						memcpy(&pointerTransform[index], (float*)transform.M, sizeof(Mat4x4));
						pointerTransformIsValid = true;
					}

					if (state.Buttons & ovrButton_Trigger)
					{
						activeController = index;
						buttonMasks[index] |= 1 << 0;
					}
					if (state.Buttons & ovrButton_GripTrigger)
					{
						activeController = index;
						buttonMasks[index] |= 1 << 1;
					}
				}
			}
		}
	#endif

		// todo : use controller as a pointing device, not the direction of the head pose
		framework.tickVirtualDesktop(pointerTransform[activeController], buttonMasks[activeController], false);

		globals.currentWindow->getWindowData()->makeActive();
	}
#else
	#error
#endif
	
	if (manualVrMode)
	{
		bool inputIsCaptured = !mouse.isDown(BUTTON_LEFT);
		globals.emulatedVrCamera.mode = Camera::kMode_FirstPerson;
		globals.emulatedVrCamera.tick(framework.timeStep, inputIsCaptured, false);
	}
	
	// time step sprites and models

	for (Sprite * sprite = m_sprites; sprite; sprite = sprite->m_next)
	{
		sprite->updateAnimation(timeStep);
	}
	
	for (Model * model = m_models; model; model = model->m_next)
	{
		model->updateAnimation(timeStep);
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
	g_textureCache.reload();
	g_shaderCache.reload();
	g_animCache.reload();
	g_spriterCache.reload();
	g_modelCache.reload();
	g_soundCache.reload();
	g_fontCache.reload();
#if ENABLE_MSDF_FONTS
	g_fontCacheMSDF.reload();
#endif
	g_glyphCache.clear();
	
	globals.resourceVersion++;
	
	for (Sprite * sprite = m_sprites; sprite; sprite = sprite->m_next)
	{
		sprite->updateAnimationSegment();
	}
	
	for (Model * model = m_models; model; model = model->m_next)
	{
		model->updateAnimationSegment();
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

#if FRAMEWORK_USE_SDL
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, caption, text, globals.currentWindow->getWindow());
#else
	logError("%s", text);
#endif
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
		if (e == "png" || e == "bmp" || e == "jpg" || e == "jpeg")
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
		else if (e == "ttf" || e == "otf")
		{
			g_fontCache.findOrCreate(f);
		#if ENABLE_MSDF_FONTS
			g_fontCacheMSDF.findOrCreate(f);
		#endif
		}
		else if (strstr(f, ".vs") == f + fl - 3 || strstr(f, ".ps") == f + fl - 3)
		{
			std::string name = f;
			name = name.substr(0, name.rfind('.'));
			Shader(name.c_str());
		}
	 #if ENABLE_COMPUTE_SHADER
		else if (strstr(f, ".cs") == f + fl - 3)
		{
			std::string name = f;
			name = name.substr(0, name.rfind('.'));
			ComputeShader(name.c_str());
		}
	#endif
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

void Framework::fillCaches(bool recurse)
{
	for (auto & resourcePath : resourcePaths)
		fillCachesWithPath(resourcePath.c_str(), recurse);
	
	fillCachesWithPath(".", recurse);
}

Window & Framework::getMainWindow() const
{
	return *globals.mainWindow;
}

Window & Framework::getCurrentWindow() const
{
	for (Window * window = m_windows; window != nullptr; window = window->m_next)
		if (window == globals.currentWindow)
			return *window;
	
	logError("failed to find current window. this should not be possible unless framework failed to initialize!");
	return *globals.mainWindow;
}

std::vector<Window*> Framework::getAllWindows() const
{
	std::vector<Window*> result;
	for (Window * window = m_windows; window != nullptr; window = window->m_next)
		result.push_back(window);
	return result;
}

void Framework::setFullscreen(bool fullscreen)
{
	globals.mainWindow->setFullscreen(fullscreen);
}

void Framework::getCurrentViewportSize(int & sx, int & sy) const
{
	::getCurrentViewportSize(sx, sy);
}

int Framework::getCurrentBackingScale() const
{
	return ::getCurrentBackingScale();
}

void Framework::beginDraw(int r, int g, int b, int a, float depth)
{
	if (globals.currentWindow->hasSurface())
	{
		globals.currentWindow->getColorTarget()->setClearColor(
			scale255(r),
			scale255(g),
			scale255(b),
			scale255(a));
		globals.currentWindow->getDepthTarget()->setClearDepth(1.f);
		
		pushRenderPass(
			globals.currentWindow->getColorTarget(), true,
			globals.currentWindow->getDepthTarget(), true,
			globals.currentWindow->getTitle());
	}
	else
	{
	#if FRAMEWORK_USE_SDL
	#if ENABLE_OPENGL
		if (enableDrawTiming)
			gpuTimingBegin(frameworkDraw);
		
		const Color color(
			scale255(r),
			scale255(g),
			scale255(b),
			scale255(a));
		
		pushBackbufferRenderPass(true, color, enableDepthBuffer, depth, "Backbuffer");
	#endif

	#if ENABLE_METAL
	// todo : replace with beginBackbufferRenderPass
		metal_draw_begin(scale255(r), scale255(g), scale255(b), scale255(a), depth);
	#endif
	#endif
	}
	
	applyTransform();
	
	setBlend(BLEND_ALPHA);
}

void Framework::endDraw()
{
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

	if (globals.currentWindow->hasSurface())
	{
		popRenderPass();
	}
	else
	{
	#if FRAMEWORK_USE_SDL
	#if ENABLE_OPENGL
		popRenderPass();
		
		// check for errors
		
		checkErrorGL();
		
		// flip back buffers
		
		SDL_GL_SwapWindow(globals.currentWindow->getWindow());
	#endif

	#if ENABLE_METAL
	// todo : replace with endRenderPass
		metal_draw_end();

	// todo : add metal_present function
	#endif
	#endif
	}
}

void Framework::present()
{
#if FRAMEWORK_USE_OVR_MOBILE
	// fixme : present() calls may appear for regular, properly vsync'ed apps.
	//         In this case, manualVrMode may not be set and this assert will trigger.
	//         We should probably call present(..) at the start of process, which
	//         will present any pending frames, and wait for frame pacing (if vsync is enabled).
	Assert(manualVrMode);
	frameworkOvr.submitFrameAndPresent();
#endif
}

int Framework::getEyeCount() const
{
#if FRAMEWORK_USE_OVR_MOBILE
	return frameworkOvr.getEyeCount();
#else
	return 1;
#endif
}

void Framework::beginEye(const int eyeIndex, const Color & clearColor)
{
#if FRAMEWORK_USE_OVR_MOBILE
	frameworkOvr.beginEye(eyeIndex, clearColor);
#else
	Assert(eyeIndex == 0);
	beginDraw(
		scale255(clearColor.r),
		scale255(clearColor.g),
		scale255(clearColor.b),
		scale255(clearColor.a));
	
	globals.emulatedVrCamera.pushProjectionMatrix();
	globals.emulatedVrCamera.pushViewMatrix();
#endif
}

void Framework::endEye()
{
#if FRAMEWORK_USE_OVR_MOBILE
	frameworkOvr.endEye();
#else
	globals.emulatedVrCamera.popViewMatrix();
	globals.emulatedVrCamera.popProjectionMatrix();
	
	endDraw();
#endif
}

Mat4x4 Framework::getHeadTransform() const
{
#if FRAMEWORK_USE_OVR_MOBILE
	return frameworkOvr.HeadTransform;
#else
	Mat4x4 result;
	globals.emulatedVrCamera.calculateWorldMatrix(result);
	
	return result;
#endif
}

void Framework::tickVirtualDesktop(const Mat4x4 & transform, const int in_buttonMask, const bool isHand)
{
#if WINDOW_IS_3D
	const Vec3 pointerOrigin = transform.GetTranslation();
	const Vec3 pointerDirection = transform.GetAxis(2).CalcNormalized();

	const float depthThreshold = .1f;

	Window * hoverWindow = nullptr;
	Vec2 hoverPos;
	float hoverDistance = FLT_MAX;

	for (Window * window = m_windows; window != nullptr; window = window->m_next)
	{
		if (window->isHidden())
			continue;

		Vec2 pixelPos;
		float distance;
		if (window->intersectRay(pointerOrigin, pointerDirection, depthThreshold, pixelPos, distance))
		{
			if (distance < hoverDistance)
			{
				hoverWindow = window;
				hoverPos = pixelPos;
				hoverDistance = distance;
			}
		}
	}

	for (Window * window = m_windows; window != nullptr; window = window->m_next)
	{
		// give focus or remove it

		const bool hasFocus = (window == hoverWindow);

		window->setHasFocus(hasFocus);

		// update mouse movement and button presses

		WindowData * windowData = window->getWindowData();

		windowData->beginProcess();
		{
			if (window == hoverWindow)
			{
				int buttonMask = 0;

				if (isHand)
				{
					if (window->m_isInteracting)
					{
						// end interacting when,
						//     - hand pushes too far through the surface
						//     - hand moves outside of the window
						//     - hand is pulled back

						if (hoverDistance < -.1f ||
							hoverDistance > .005f ||
							hoverPos[0] < 0 || hoverPos[0] > window->getWidth() ||
							hoverPos[1] < 0 || hoverPos[1] > window->getHeight())
						{
							window->m_isInteracting = false;
						}
					}
					else
					{
						// begin interacting when,
						//     - hand is inside the window
						//     - hand is touching the window

						if (hoverDistance > -.05f &&
							hoverDistance < 0.f &&
							hoverPos[0] >= 0 && hoverPos[0] < window->getWidth() &&
							hoverPos[1] >= 0 && hoverPos[1] < window->getHeight())
						{
							window->m_isInteracting = true;
						}
					}

					if (window->m_isInteracting)
					{
						buttonMask = in_buttonMask;
					}
				}
				else
				{
					buttonMask = in_buttonMask;
				}

				windowData->mouseData.mouseX = hoverPos[0];
				windowData->mouseData.mouseY = hoverPos[1];

				for (int i = 0; i < 2; ++i)
				{
					const bool isDown = (buttonMask & (1 << i)) != 0;

					if (windowData->mouseData.mouseDown[i] != isDown)
					{
						windowData->mouseData.mouseDown[i] = isDown;
						windowData->mouseData.mouseChange[i] = true;
					}
				}
			}
			else
			{
				for (int i = 0; i < 2; ++i)
				{
					if (windowData->mouseData.mouseDown[i])
					{
						windowData->mouseData.mouseDown[i] = false;
						windowData->mouseData.mouseChange[i] = true;
					}
				}
			}
		}
		windowData->endProcess();
	}
#endif
}

void Framework::drawVirtualDesktop()
{
#if WINDOW_IS_3D
	for (Window * window = m_windows; window != nullptr; window = window->m_next)
	{
		if (window->hasSurface() && !window->isHidden())
		{
			window->draw3d();

			if (window->hasFocus())
				window->draw3dCursor();
		}
	}
#endif
}

// -----

#define ENABLE_SCREENSHOTS 0

#if ENABLE_SCREENSHOTS

#include "FileStream.h"
#include "ImageData.h"
#include "ImageLoader_Tga.h"

static Stack<Surface*, 32> s_screenshotSurfaceStack;

#endif

void Framework::beginScreenshot(int r, int g, int b, int a, int scale)
{
#if ENABLE_SCREENSHOTS
	Assert(scale >= 1);
	
	int sx;
	int sy;
	getCurrentViewportSize(sx, sy);
	
	sx *= scale;
	sy *= scale;
	
	Surface * surface = new Surface(sx, sy, true, false, SURFACE_RGBA8);
	
	pushSurface(surface);
	surface->clear(r, g, b, a);
	gxPushMatrix();
	gxScalef(scale, scale, 1);
	
	s_screenshotSurfaceStack.push(surface);
#endif
}

void Framework::endScreenshot(const char * name, const int index, const bool omitAlpha)
{
#if ENABLE_SCREENSHOTS
	Surface * surface = s_screenshotSurfaceStack.popValue();
	
	gxPopMatrix();
	popSurface();
	
	// save the image
	
	pushSurface(surface);
	{
		screenshot(name, index, omitAlpha);
	}
	popSurface();

	// draw the captured image to the current surface, as if the temporary surface never happened
	
	int sx;
	int sy;
	getCurrentViewportSize(sx, sy);
	
	pushBlend(BLEND_OPAQUE);
	gxSetTexture(surface->getTexture());
	drawRect(0, 0, sx, sy);
	gxSetTexture(0);
	popBlend();
	
	//
	
	delete surface;
	surface = nullptr;
#endif
}

void Framework::screenshot(const char * name, int index, bool omitAlpha)
{
#if ENABLE_SCREENSHOTS
	int sx;
	int sy;
	getCurrentBackingSize(sx, sy);

	// fetch the pixel data
	
	uint8_t * bytes = new uint8_t[sx * sy * 4];
	
	glReadPixels(
		0, 0,
		sx,
		sy,
		GL_BGRA, GL_UNSIGNED_BYTE,
		bytes);
	checkErrorGL();
	
	// force alpha to one, if desired
	
	if (omitAlpha)
	{
		const int numPixels = sx * sy;
		
		for (int i = 0; i < numPixels; ++i)
		{
			bytes[i * 4 + 3] = 255;
		}
	}
	
	// flip image along the Y axis (TGA stores the image upside-down)
	
	const int lineSize = sx * 4;
	uint8_t * temp = (uint8_t*)alloca(lineSize);
	
	for (int y = 0; y < sy/2; ++y)
	{
		const int y1 = y;
		const int y2 = sy - 1 - y;
		
		uint8_t * line1 = bytes + lineSize * y1;
		uint8_t * line2 = bytes + lineSize * y2;
		
		memcpy(temp, line1, lineSize);
		memcpy(line1, line2, lineSize);
		memcpy(line2, temp, lineSize);
	}
	
#if WINDOWS
	char filename[MAX_PATH];
#else
	char filename[PATH_MAX];
#endif

	if (index < 0)
	{
		// search for a filename for which the file doesn't exist yet
		
		int currentIndex = 0;
		
		do
		{
		#if WINDOWS
			char temp[MAX_PATH];
		#else
			char temp[PATH_MAX];
		#endif
			sprintf_s(temp, sizeof(temp), name, currentIndex);
			sprintf_s(filename, sizeof(filename), "%s.tga", temp);
			currentIndex++;
		} while (FileStream::Exists(filename));
	}
	else
	{
		sprintf_s(filename, sizeof(filename), name, index);
	}
	
	try
	{
		ImageLoader_Tga loader;
		loader.SaveBGRA_vflipped(bytes, sx, sy, filename, true);
	}
	catch (std::exception & e)
	{
		logError("failed to write image: %s", e.what());
		(void)e;
	}
	
	delete [] bytes;
	bytes = nullptr;
#endif
}

// -----

void Framework::registerShaderSource(const char * name, const char * text)
{
	s_shaderSources[name] = text;

	// refresh shaders which are using this source
	
	g_shaderCache.handleSourceChanged(name);
}

void Framework::unregisterShaderSource(const char * name)
{
	auto i = s_shaderSources.find(name);

	fassert(i != s_shaderSources.end());
	if (i != s_shaderSources.end())
		s_shaderSources.erase(i);
}

bool Framework::tryGetShaderSource(const char * name, const char *& text) const
{
	auto i = s_shaderSources.find(name);

	if (i != s_shaderSources.end())
	{
		text = i->second.c_str();
		return true;
	}
	else
	{
		return false;
	}
}

void Framework::registerShaderOutput(const char name, const char * outputType, const char * outputName)
{
	for (auto & output : g_shaderOutputs)
	{
		Assert(output.name != name);
		if (output.name == name)
			return;
	}
	
	ShaderOutput output;
	output.name = name;
	output.outputType = outputType;
	output.outputName = outputName;
	g_shaderOutputs.push_back(output);
}

void Framework::unregisterShaderOutput(const char name)
{
	for (auto i = g_shaderOutputs.begin(); i != g_shaderOutputs.end(); ++i)
	{
		auto & output = *i;
		
		if (output.name == name)
		{
			g_shaderOutputs.erase(i);
			break;
		}
	}
}

bool Framework::fileHasChanged(const char * filename) const
{
	for (auto & file : changedFiles)
		if (file == filename)
			return true;
	
	return false;
}

void Framework::registerResourcePath(const char * path)
{
	logInfo("registerResourcePath: %s", path);
	
	resourcePaths.push_back(path);
}

bool Framework::registerChibiResourcePaths(const char * encoded_text)
{
	bool result = false;
	
	const ByteString bytes = Base64::Decode(encoded_text);
	const std::string text = bytes.ToString();
	
	ReadOnlyCsvDocument doc;
	if (doc.loadText(text.c_str(), true, ',') == false)
		logError("failed to parse chibi resource paths");
	else
	{
		const int type_index = doc.getColumnIndex("type");
		const int path_index = doc.getColumnIndex("path");
		
		if (path_index < 0)
			logError("unexpected chibi resource paths format");
		else
		{
			for (auto i = doc.firstRow(); i != doc.lastRow(); i = doc.nextRow(i))
			{
				const char * path = i[path_index];
				
				logDebug("resource path: %s", path);
				
				registerResourcePath(path);
				
				if (type_index != -1)
				{
					const char * type = i[type_index];
					
					if (strcmp(type, "app") == 0)
					{
						logInfo("changeDirectory to app path: %s", path);
						
						changeDirectory(path);
					}
				}
			}
			
			result = true;
		}
	}
	
	return result;
}

char s_resourcePath[PATH_MAX];

const char * Framework::resolveResourcePath(const char * path)
{
	for (auto & resourcePath : resourcePaths)
	{
		auto * end = s_resourcePath + PATH_MAX - 1;
		auto * dst = s_resourcePath;
		for (auto * src = resourcePath.c_str(); *src != 0 && dst < end; )
			*dst++ = *src++;
		if (dst < end)
			*dst++ = '/';
		for (auto * src = path; *src != 0 && dst < end; )
			*dst++ = *src++;
		*dst = 0;
		
		if (dst == end)
			continue;
		
		FILE * file = nullptr;
		fopen_s(&file, s_resourcePath, "rb");
		
		if (file != nullptr)
		{
			fclose(file);
			file = nullptr;
			
			return s_resourcePath;
		}
	}
	
	return path;
}

void Framework::blinkTaskbarIcon(int count)
{
#ifdef WIN32
	SDL_SysWMinfo info;
	memset(&info, 0, sizeof(info));
	SDL_VERSION(&info.version);

	if (SDL_GetWindowWMInfo(globals.currentWindow->getWindow(), &info))
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
	fassert(model->m_prev == 0);
	fassert(model->m_next == 0);

	if (m_models)
	{
		m_models->m_prev = model;
		model->m_next = m_models;
	}

	m_models = model;
}

void Framework::unregisterModel(Model * model)
{
	if (model == m_models)
		m_models = model->m_next;

	if (model->m_prev)
		model->m_prev->m_next = model->m_next;
	if (model->m_next)
		model->m_next->m_prev = model->m_prev;

	model->m_prev = 0;
	model->m_next = 0;
}

void Framework::registerWindow(Window * window)
{
	fassert(window->m_prev == 0);
	fassert(window->m_next == 0);

	if (m_windows)
	{
		m_windows->m_prev = window;
		window->m_next = m_windows;
	}

	m_windows = window;
}

void Framework::unregisterWindow(Window * window)
{
	if (window == m_windows)
		m_windows = window->m_next;

	if (window->m_prev)
		window->m_prev->m_next = window->m_next;
	if (window->m_next)
		window->m_next->m_prev = window->m_prev;

	window->m_prev = 0;
	window->m_next = 0;
}

WindowData * Framework::findWindowDataById(const int id)
{
#if FRAMEWORK_USE_SDL
	for (Window * window = m_windows; window != nullptr; window = window->m_next)
		if (window->m_window)
			if (SDL_GetWindowID(window->m_window) == id)
				return window->m_windowData;
#endif

	return nullptr;
}

// -----

static Stack<std::string, 32> s_shaderOutputsStack;

// -----

class DictionaryStorage
{
public:
	typedef std::map<std::string, std::string> Map;
	Map m_map;
};

Dictionary::Dictionary()
	: m_storage(nullptr)
{
	m_storage = new DictionaryStorage();
}

Dictionary::Dictionary(const Dictionary & other)
	: m_storage(nullptr)
{
	m_storage = new DictionaryStorage();
	
	*m_storage = *other.m_storage;
}

Dictionary::~Dictionary()
{
	delete m_storage;
	m_storage = nullptr;
}

bool Dictionary::load(const char * filename)
{
	bool result = true;

	m_storage->m_map.clear();

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
		for (auto & i : m_storage->m_map)
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
		m_storage->m_map.clear();
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
	return m_storage->m_map.count(name) != 0;
}

void Dictionary::setString(const char * name, const char * value)
{
	m_storage->m_map[name] = value;
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
    sprintf_s(text, sizeof(text), "%" PRId64, value);
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
	fassert(sizeof(intptr_t) <= sizeof(int64_t));

	setInt64(name, reinterpret_cast<intptr_t>(value));
}

std::string Dictionary::getString(const char * name, const char * _default) const
{
	DictionaryStorage::Map::const_iterator i = m_storage->m_map.find(name);
	if (i != m_storage->m_map.end())
		return i->second;
	else
		return _default;
}

int Dictionary::getInt(const char * name, int _default) const
{
	DictionaryStorage::Map::const_iterator i = m_storage->m_map.find(name);
	if (i != m_storage->m_map.end())
		return atoi(i->second.c_str());
	else
		return _default;
}

int64_t Dictionary::getInt64(const char * name, int64_t _default) const
{
#if defined(WINDOWS)
    DictionaryStorage::Map::const_iterator i = m_storage->m_map.find(name);
    if (i != m_storage->m_map.end())
		return _atoi64(i->second.c_str());
    else
        return _default;
#else
    DictionaryStorage::Map::const_iterator i = m_storage->m_map.find(name);
    if (i != m_storage->m_map.end())
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
	DictionaryStorage::Map::const_iterator i = m_storage->m_map.find(name);
	if (i != m_storage->m_map.end())
		return atof(i->second.c_str());
	else
		return _default;
}

void * Dictionary::getPtr(const char * name, void * _default) const
{
	return reinterpret_cast<void*>(getInt64(name, (int)(reinterpret_cast<intptr_t>(_default))));
}

std::string & Dictionary::operator[](const char * name)
{
	return m_storage->m_map[name];
}

Dictionary & Dictionary::operator=(const Dictionary & other)
{
	*m_storage = *other.m_storage;
	
	return *this;
}

// -----

GxTextureId getTexture(const char * filename)
{
	const TextureCacheElem & elem = g_textureCache.findOrCreate(filename, 1, 1, true);

	if (elem.textures)
		return elem.textures[0].id;
	else
		return 0;
}

// -----

Sound::Sound(const char * filename)
{
	m_sound = &g_soundCache.findOrCreate(filename);
	m_playId = -1;
	m_volume = 100;
}

void Sound::play(int volume)
{
	if (volume == -1)
		volume = m_volume;
	
	volume = clamp(volume, 0, 100);
	
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
	
#if ENABLE_MSDF_FONTS
	m_fontMSDF = &g_fontCacheMSDF.findOrCreate(filename);
#endif
}

bool Font::saveCache(const char * _filename) const
{
#if ENABLE_MSDF_FONTS
	const std::string filename = _filename ? _filename : (m_fontMSDF->m_filename + ".cache");
	
	return m_fontMSDF->m_glyphCache->saveCache(filename.c_str());
#else
	return true;
#endif
}

bool Font::loadCache(const char * _filename)
{
#if ENABLE_MSDF_FONTS
	const std::string filename = _filename ? _filename : (m_fontMSDF->m_filename + ".cache");
	
	return m_fontMSDF->m_glyphCache->loadCache(filename.c_str());
#else
	return true;
#endif
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
	const float flatnessEps = .03f;

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

		if (t1 != 0.f || t2 != 1.f)
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
	case BUTTON_MAX:
		break;
	}
	fassert(false);
	return -1;
}

bool Mouse::isDown(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	auto * currentWindowData = globals.currentWindow->getWindowData();
	return currentWindowData->mouseData.mouseDown[index];
}

bool Mouse::wentDown(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	auto * currentWindowData = globals.currentWindow->getWindowData();
	return isDown(button) && currentWindowData->mouseData.mouseChange[index];
}

bool Mouse::wentUp(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	auto * currentWindowData = globals.currentWindow->getWindowData();
	return !isDown(button) && currentWindowData->mouseData.mouseChange[index];
}

void Mouse::showCursor(bool enabled)
{
#if FRAMEWORK_USE_SDL
	SDL_ShowCursor(enabled ? 1 : 0);
#endif
}

void Mouse::setRelative(bool isRelative)
{
#if FRAMEWORK_USE_SDL
	SDL_SetRelativeMouseMode(isRelative ? SDL_TRUE : SDL_FALSE);
	SDL_CaptureMouse(isRelative ? SDL_TRUE : SDL_FALSE);
#endif
}

bool Mouse::isIdle() const
{
	auto * currentWindowData = globals.currentWindow->getWindowData();
	return
		dx == 0 &&
		dy == 0 &&
		!currentWindowData->mouseData.mouseChange[0] &&
		!currentWindowData->mouseData.mouseChange[1];
}

// -----

bool Keyboard::isDown(int key) const
{
	auto * currentWindowData = globals.currentWindow->getWindowData();
	for (int i = 0; i < currentWindowData->keyDownCount; ++i)
		if (currentWindowData->keyDown[i] == key)
			return true;
	return false;
}

static bool keyChange(int key)
{
	auto * currentWindowData = globals.currentWindow->getWindowData();
	for (int i = 0; i < currentWindowData->keyChangeCount; ++i)
		if (currentWindowData->keyChange[i] == key)
			return true;
	return false;
}

bool Keyboard::wentDown(int key, bool allowRepeat) const
{
	return (isDown(key) && keyChange(key)) || (allowRepeat && keyRepeat(key));
}

bool Keyboard::wentUp(int key) const
{
	return !isDown(key) && keyChange(key);
}

bool Keyboard::keyRepeat(int key) const
{
	auto * currentWindowData = globals.currentWindow->getWindowData();
	for (int i = 0; i < currentWindowData->keyRepeatCount; ++i)
		if (currentWindowData->keyRepeat[i] == key)
			return true;
	return false;
}

bool Keyboard::isIdle() const
{
	auto * currentWindowData = globals.currentWindow->getWindowData();
	return
		currentWindowData->keyDownCount == 0 &&
		currentWindowData->keyChangeCount == 0;
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

const char * Gamepad::getName() const
{
	return name;
}

// -----

Camera3d::Camera3d()
	: mouseDx(0.0)
	, mouseDy(0.0)
	, position(0.f, 0.f, 0.f)
	, yaw(0.f)
	, pitch(0.f)
	, roll(0.f)
	, mouseSmooth(0.75)
	, mouseRotationSpeed(1.f)
	, maxForwardSpeed(1.f)
	, maxStrafeSpeed(1.f)
	, maxUpSpeed(1.f)
	, gamepadIndex(-1)
{
}

void Camera3d::tick(float dt, bool enableInput)
{
	float forwardSpeed = 0.f;
	float strafeSpeed = 0.f;
	float upSpeed = 0.f;
	
	if (enableInput)
	{
		// keyboard
		
		if (keyboard.isDown(SDLK_DOWN) || keyboard.isDown(SDLK_s))
			forwardSpeed -= 1.f;
		if (keyboard.isDown(SDLK_UP) || keyboard.isDown(SDLK_w))
			forwardSpeed += 1.f;
		if (keyboard.isDown(SDLK_LEFT) || keyboard.isDown(SDLK_a))
			strafeSpeed -= 1.f;
		if (keyboard.isDown(SDLK_RIGHT) || keyboard.isDown(SDLK_d))
			strafeSpeed += 1.f;
		
		// gamepad
		
		if (gamepadIndex >= 0 && gamepadIndex < MAX_GAMEPAD && gamepad[gamepadIndex].isConnected)
		{
			strafeSpeed += gamepad[gamepadIndex].getAnalog(0, ANALOG_X);
			forwardSpeed -= gamepad[gamepadIndex].getAnalog(0, ANALOG_Y);
		
			yaw -= gamepad[gamepadIndex].getAnalog(1, ANALOG_X);
			pitch -= gamepad[gamepadIndex].getAnalog(1, ANALOG_Y);
		}
		
		// mouse + mouse smoothing
		
		mouseDx += mouse.dx;
		mouseDy += mouse.dy;
		
		const double retain = std::pow(mouseSmooth, dt * 100.0);
		
		const double newDx = mouseDx * retain;
		const double newDy = mouseDy * retain;
		
		const double thisDx = mouseDx - newDx;
		const double thisDy = mouseDy - newDy;
		
		mouseDx = newDx;
		mouseDy = newDy;
		
		yaw -= thisDx * mouseRotationSpeed;
		pitch -= thisDy * mouseRotationSpeed;
	}
	else
	{
		mouseDx = 0.f;
		mouseDy = 0.f;
	}
	
	// go from normalized input values to values directly used to add to position
	
	forwardSpeed *= maxForwardSpeed * dt;
	strafeSpeed *= maxStrafeSpeed * dt;
	upSpeed *= maxUpSpeed * dt;
	
	const Mat4x4 worldMatrix = getWorldMatrix();
	
	const Vec3 xAxis = worldMatrix.GetAxis(0);
	const Vec3 yAxis = worldMatrix.GetAxis(1);
	const Vec3 zAxis = worldMatrix.GetAxis(2);
	
	position = position + xAxis * strafeSpeed + yAxis * upSpeed + zAxis * forwardSpeed;
}

Mat4x4 Camera3d::getWorldMatrix() const
{
	return Mat4x4(true).Translate(position).RotateZ(roll / 180.f * M_PI).RotateY(yaw / 180.f * M_PI).RotateX(pitch / 180.f * M_PI);
}

Mat4x4 Camera3d::getViewMatrix() const
{
	return getWorldMatrix().CalcInv();
}

void Camera3d::pushViewMatrix() const
{
	const Mat4x4 matrix = getViewMatrix();
	
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_MODELVIEW);
		gxPushMatrix();
        gxMultMatrixf(matrix.m_v);
	}
	gxMatrixMode(restoreMatrixMode);
}

void Camera3d::popViewMatrix() const
{
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_MODELVIEW);
		gxPopMatrix();
	}
	gxMatrixMode(restoreMatrixMode);
}

// -----

void clearCaches(int caches)
{
	if (caches & CACHE_FONT)
	{
		g_fontCache.clear();
		g_glyphCache.clear();
	}
	
#if ENABLE_MSDF_FONTS
	if (caches & CACHE_FONT_MSDF)
		g_fontCacheMSDF.clear();
#endif

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

#if ENABLE_OPENGL
bool s_renderPassIsBackbufferPass = false; // todo : unify surfaces and render passes. currently it's too difficult to figure out viewport size and whether to flip the clip space Y axis or not
#endif

static int getCurrentBackingScale()
{
	int sx, sy;
	int backingScale;
	
	if (getCurrentRenderTargetSize(sx, sy, backingScale))
		return backingScale;
	else
		return s_backingScale;
}

static void getCurrentBackingSize(int & sx, int & sy)
{
	int backingScale;
	
	if (getCurrentRenderTargetSize(sx, sy, backingScale))
		return;
	else
	{
		sx = globals.currentWindow->getWidth();
		sy = globals.currentWindow->getHeight();
		
		sx *= s_backingScale;
		sy *= s_backingScale;
	}
}

static void getCurrentViewportSize(int & sx, int & sy)
{
	// return the size of the current render target
	
	int backingScale;
	
	if (getCurrentRenderTargetSize(sx, sy, backingScale))
	{
		sx /= backingScale;
		sy /= backingScale;
	}
	else
	{
		// or when no render target is active, the size of the current window
	
		// todo : fix for case with fullscreen desktop mode
		// fixme : add specific code for setting screen matrix
		if (globals.currentWindow == globals.mainWindow && false)
		{
			sx = globals.displaySize[0];
			sy = globals.displaySize[1];
		}
		else
		{
			sx = globals.currentWindow->getWidth() * framework.minification;
			sy = globals.currentWindow->getHeight() * framework.minification;
		}
	}
}

void setTransform(TRANSFORM transform)
{
	globals.transform = transform;
	
	applyTransform();
}

TRANSFORM getTransform()
{
	return globals.transform;
}

void applyTransform()
{
	int sx;
	int sy;
	getCurrentViewportSize(sx, sy);
	
	applyTransformWithViewportSize(sx, sy);
}

void applyTransformWithViewportSize(const int sx, const int sy)
{
	// calculate screen matrix (we need it to transform vertices to screen space)
	{
		gxMatrixMode(GX_PROJECTION);
		gxPushMatrix();
		{
			gxLoadIdentity();
		
		#if ENABLE_METAL
			// in Metal clip-space, (-1, -1) is the bottom-left corner, (+1, +1) is top-right
			// flip Y axis so the vertical axis runs top to bottom
			gxScalef(1.f, -1.f, 1.f);
		#endif
		
		#if ENABLE_OPENGL
			if (s_renderPassIsBackbufferPass)
			{
				// flip Y axis so the vertical axis runs top to bottom
				gxScalef(1.f, -1.f, 1.f);
			}
		#endif
		
			// convert from (0,0),(1,1) to (-1,-1),(+1+1)
			gxTranslatef(-1.f, -1.f, 0.f);
			gxScalef(2.f, 2.f, 1.f);
			
			// convert from (0,0),(sx,sy) to (0,0),(1,1)
			gxScalef(1.f / sx, 1.f / sy, 1.f);
			
			// capture transform
			gxGetMatrixf(GX_PROJECTION, globals.transformScreen.m_v);
		}
		gxPopMatrix();
	}
	
	// apply current transform
	
	gxMatrixMode(GX_PROJECTION);
	
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
		
		#if ENABLE_OPENGL
			if (s_renderPassIsBackbufferPass == false)
			{
				// flip Y axis so the vertical axis runs bottom to top
				gxScalef(1.f, -1.f, 1.f);
			}
		#endif
			
			break;
		}
		default:
		{
			fassert(false);
			gxLoadIdentity();
			break;
		}
	}
	
	gxMatrixMode(GX_MODELVIEW);
	gxLoadIdentity();
}

void setTransform2d(const Mat4x4 & transform)
{
	globals.transform2d = transform;
}

void setTransform3d(const Mat4x4 & transform)
{
	globals.transform3d = transform;
	
	setTransform(TRANSFORM_3D);
}

struct TransformData
{
	TRANSFORM transform;
	Mat4x4 projection;
	Mat4x4 modelView;
};

static Stack<TransformData, 32> s_transformStack;

void pushTransform()
{
	TransformData t;
	
	t.transform = globals.transform;
	gxGetMatrixf(GX_PROJECTION, t.projection.m_v);
	gxGetMatrixf(GX_MODELVIEW, t.modelView.m_v);
	
	s_transformStack.push(t);
}

void popTransform()
{
	const TransformData t = s_transformStack.popValue();
	
	setTransform(t.transform);
	gxSetMatrixf(GX_PROJECTION, t.projection.m_v);
	gxSetMatrixf(GX_MODELVIEW, t.modelView.m_v);
}

struct ScrollData
{
	int scrollX = 0;;
	int scrollY = 0;
};

static Stack<ScrollData, 32> s_scrollStack;

void pushScroll(const int scrollX, const int scrollY)
{
	ScrollData s;
	s.scrollX = scrollX;
	s.scrollY = scrollY;
	s_scrollStack.push(s);
	
	mouse.x -= scrollX;
	mouse.y -= scrollY;
	
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_PROJECTION);
		gxPushMatrix();
		gxTranslatef(scrollX, scrollY, 0);
	}
	gxMatrixMode(restoreMatrixMode);
}

void popScroll()
{
	ScrollData s = s_scrollStack.popValue();
	
	mouse.x += s.scrollX;
	mouse.y += s.scrollY;
	
	const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
	{
		gxMatrixMode(GX_PROJECTION);
		gxPopMatrix();
	}
	gxMatrixMode(restoreMatrixMode);
}

void projectScreen2d()
{
	setTransform(TRANSFORM_SCREEN);
}

void projectPerspective3d(const float fov, const float nearZ, const float farZ)
{
	Mat4x4 transform;
	
	int sx;
	int sy;
	getCurrentViewportSize(sx, sy);
	
#if ENABLE_OPENGL
	transform.MakePerspectiveGL(fov / 180.f * M_PI, sy / float(sx), nearZ, farZ);
#else
	transform.MakePerspectiveLH(fov / 180.f * M_PI, sy / float(sx), nearZ, farZ);
#endif
	
	globals.transform3d = transform;
	
	setTransform(TRANSFORM_3D);
}

void viewLookat3d(const float originX, const float originY, const float originZ, const float targetX, const float targetY, const float targetZ, const float upX, const float upY, const float upZ)
{
	Mat4x4 transform;
	
	transform.MakeLookat(Vec3(originX, originY, originZ), Vec3(targetX, targetY, targetZ), Vec3(upX, upY, upZ));
	
	globals.transform3d = globals.transform3d * transform;
	
	setTransform(TRANSFORM_3D);
}

Vec4 transformToWorld(const Vec4 & v)
{
	Mat4x4 matM;
	
	gxGetMatrixf(GX_MODELVIEW, matM.m_v);
	
	// from current transform to world
	
	Vec4 t = matM * Vec4(v[0], v[1], v[2], v[3]);
	
	return t;
}

Vec2 transformToScreen(const Mat4x4 & modelViewProjection, const Vec3 & v, float & w)
{
	// from current transform to view
	
	Vec4 t = modelViewProjection * Vec4(v[0], v[1], v[2], 1.f);
	
	// perspective divide
	
	w = t[3];
	
	if (t[3] != 0.f)
		t /= t[3];
	
	// and back to screen coordinates
	
	Mat4x4 viewToScreen = globals.transformScreen.CalcInv();
	
	Vec3 s = viewToScreen * Vec3(t[0], t[1], t[2]);
	
	return Vec2(s[0], s[1]);
}

Vec2 transformToScreen(const Vec3 & v, float & w)
{
	Mat4x4 matP;
	Mat4x4 matM;
	
	gxGetMatrixf(GX_PROJECTION, matP.m_v);
	gxGetMatrixf(GX_MODELVIEW, matM.m_v);
	
	const Mat4x4 modelViewProjection = matP * matM;
	
	return transformToScreen(modelViewProjection, v, w);
}

void pushSurface(Surface * newSurface, const bool clearSurface)
{
#if ENABLE_SCREENSHOTS
	const bool screenshotMode = surface == nullptr && s_screenshotSurfaceStack.stackSize > 0;
	
	if (screenshotMode)
		surface = s_screenshotSurfaceStack.stack[s_screenshotSurfaceStack.stackSize - 1];
#endif

	// record the new surface inside the surface stack

	fassert(surfaceStackSize < kMaxSurfaceStackSize);
	surfaceStack[surfaceStackSize++] = newSurface;
	
	// start a render pass for the new surface
	
	if (newSurface == nullptr)
	{
		pushBackbufferRenderPass(
			clearSurface,
			colorBlackTranslucent,
			clearSurface,
			0.f,
			"Backbuffer",
			s_backingScale);
	}
	else
	{
		pushRenderPass(
			newSurface->getColorTarget(),
			clearSurface,
			newSurface->getDepthTarget(),
			clearSurface,
			newSurface->getName(),
			newSurface->getBackingScale());
	}

	//

#if ENABLE_SCREENSHOTS
	if (screenshotMode)
	{
		int sx;
		int sy;
		SDL_GetWindowSize(globals.currentWindow->getWindow(), &sx, &sy);
		const float scaleX = surface->getWidth() / float(sx);
		const float scaleY = surface->getHeight() / float(sy);
		gxScalef(scaleX, scaleY, 1);
	}
#endif
}

void popSurface()
{
	fassert(surfaceStackSize > 0);
	surfaceStack[--surfaceStackSize] = 0;
	
	popRenderPass();
}

void setDrawRect(int x, int y, int sx, int sy)
{
	int surfaceSx;
	int surfaceSy;
	getCurrentViewportSize(surfaceSx, surfaceSy);
	
	int backingSx;
	int backingSy;
	getCurrentBackingSize(backingSx, backingSy);
	
	#define ScaleX(x) x = ((x) * backingSx / (surfaceSx / framework.minification))
	#define ScaleY(y) y = ((y) * backingSy / (surfaceSy / framework.minification))
	
#if ENABLE_OPENGL
	if (s_renderPassIsBackbufferPass)
		y = surfaceSy - y - sy;
#endif

	ScaleX(x);
	ScaleY(y);
	ScaleX(sx);
	ScaleY(sy);

#if ENABLE_METAL
	metal_set_scissor(x, y, sx, sy);
#endif

#if ENABLE_OPENGL
	glScissor(x, y, sx, sy);
	checkErrorGL();

	glEnable(GL_SCISSOR_TEST);
	checkErrorGL();
#endif
}

void clearDrawRect()
{
#if ENABLE_METAL
	metal_clear_scissor();
#endif

#if ENABLE_OPENGL
	glDisable(GL_SCISSOR_TEST);
#endif
}

//

static Stack<BLEND_MODE, 32> blendModeStack(BLEND_ALPHA);
static Stack<bool, 32> lineSmoothStack(false);
static Stack<bool, 32> wireframeStack(false);
static Stack<DepthTestInfo, 32> depthTestStack(DepthTestInfo { false, DEPTH_LESS, true });
static Stack<DepthBiasInfo, 32> depthBiasStack(DepthBiasInfo { 0.f, 0.f });
static Stack<AlphaToCoverageInfo, 32> alphaToCoverageStack(AlphaToCoverageInfo { false });
static Stack<CullModeInfo, 32> cullModeStack(CullModeInfo { CULL_NONE, CULL_CCW });

void pushBlend(BLEND_MODE blendMode)
{
	blendModeStack.push(globals.blendMode);
	
	setBlend(blendMode);
}

void popBlend()
{
	const BLEND_MODE blendMode = blendModeStack.popValue();
	
	setBlend(blendMode);
}

void setColorMode(COLOR_MODE colorMode)
{
	globals.colorMode = colorMode;

#if ENABLE_OPENGL && USE_LEGACY_OPENGL
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

void pushColorMode(COLOR_MODE colorMode)
{
	colorModeStack.push(globals.colorMode);
	
	setColorMode(colorMode);
}

void popColorMode()
{
	const COLOR_MODE value = colorModeStack.popValue();
	
	setColorMode(value);
}

void setColorPost(COLOR_POST colorPost)
{
	globals.colorPost = colorPost;
}

void pushColorPost(COLOR_POST colorPost)
{
	colorPostStack.push(globals.colorPost);
	
	setColorPost(colorPost);
}

void popColorPost()
{
	const COLOR_POST value = colorPostStack.popValue();
	
	setColorPost(value);
}

void pushLineSmooth(bool enabled)
{
	lineSmoothStack.push(globals.lineSmoothEnabled);
	
	setLineSmooth(enabled);
}

void popLineSmooth()
{
	const bool value = lineSmoothStack.popValue();
	
	setLineSmooth(value);
}

void pushWireframe(bool enabled)
{
	wireframeStack.push(globals.wireframeEnabled);
	
	setWireframe(enabled);
}

void popWireframe()
{
	const bool value = wireframeStack.popValue();
	
	setWireframe(value);
}

void pushDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled)
{
	const DepthTestInfo info =
	{
		globals.depthTestEnabled,
		globals.depthTest,
		globals.depthTestWriteEnabled
	};
	
	depthTestStack.push(info);
	
	setDepthTest(enabled, test, writeEnabled);
}

void popDepthTest()
{
	const DepthTestInfo depthTestInfo = depthTestStack.popValue();
	
	setDepthTest(depthTestInfo.testEnabled, depthTestInfo.test, depthTestInfo.writeEnabled);
}

void pushDepthWrite(bool enabled)
{
	pushDepthTest(globals.depthTestEnabled, globals.depthTest, enabled);
}

void popDepthWrite()
{
	popDepthTest();
}

void pushDepthBias(float depthBias, float slopeScale)
{
	const DepthBiasInfo info =
	{
		globals.depthBias,
		globals.depthBiasSlopeScale
	};
	
	depthBiasStack.push(info);
	
	setDepthBias(depthBias, slopeScale);
}

void popDepthBias()
{
	const DepthBiasInfo info = depthBiasStack.popValue();
	
	setDepthBias(info.depthBias, info.slopeScale);
}

void pushAlphaToCoverage(bool enabled)
{
	const AlphaToCoverageInfo info =
	{
		globals.alphaToCoverageEnabled
	};
	
	alphaToCoverageStack.push(info);
	
	setAlphaToCoverage(enabled);
}

void popAlphaToCoverage()
{
	const AlphaToCoverageInfo info = alphaToCoverageStack.popValue();
	
	setAlphaToCoverage(info.enabled);
}

//

static StencilState & getStencilStateForFace(GX_STENCIL_FACE face)
{
	return
		face == GX_STENCIL_FACE_FRONT
		? globals.frontStencilState
		: globals.backStencilState;
}

StencilSetter::StencilSetter()
{
	globals.frontStencilState = StencilState();
	globals.backStencilState = StencilState();
}

StencilSetter::~StencilSetter()
{
	setStencilTest(globals.frontStencilState, globals.backStencilState);
}

StencilSetter & StencilSetter::comparison(GX_STENCIL_FACE face, GX_STENCIL_FUNC func, uint8_t ref, uint8_t mask)
{
	auto & state = getStencilStateForFace(face);
	state.compareFunc = func;
	state.compareRef = ref;
	state.compareMask = mask;
	return *this;
}

StencilSetter & StencilSetter::op(GX_STENCIL_FACE face, GX_STENCIL_OP onStencilFail, GX_STENCIL_OP onDepthFail, GX_STENCIL_OP onDepthStencilPass)
{
	auto & state = getStencilStateForFace(face);
	state.onStencilFail = onStencilFail;
	state.onDepthFail = onDepthFail;
	state.onDepthStencilPass = onDepthStencilPass;
	return *this;
}

StencilSetter & StencilSetter::writeMask(GX_STENCIL_FACE face, uint8_t mask)
{
	auto & state = getStencilStateForFace(face);
	state.writeMask = mask;
	return *this;
}

StencilSetter & StencilSetter::comparison(GX_STENCIL_FUNC func, uint8_t ref, uint8_t mask)
{
	comparison(GX_STENCIL_FACE_FRONT, func, ref, mask);
	comparison(GX_STENCIL_FACE_BACK,  func, ref, mask);
	return *this;
}

StencilSetter & StencilSetter::op(GX_STENCIL_OP onStencilFail, GX_STENCIL_OP onDepthFail, GX_STENCIL_OP onDepthStencilPass)
{
	op(GX_STENCIL_FACE_FRONT, onStencilFail, onDepthFail, onDepthStencilPass);
	op(GX_STENCIL_FACE_BACK,  onStencilFail, onDepthFail, onDepthStencilPass);
	return *this;
}

StencilSetter & StencilSetter::writeMask(uint8_t mask)
{
	writeMask(GX_STENCIL_FACE_FRONT, mask);
	writeMask(GX_STENCIL_FACE_BACK,  mask);
	return *this;
}

//

void pushCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding)
{
	const CullModeInfo info =
	{
		globals.cullMode,
		globals.cullWinding
	};
	
	cullModeStack.push(info);
	
	setCullMode(mode, frontFaceWinding);
}

void popCullMode()
{
	const CullModeInfo cullMode = cullModeStack.popValue();
	
	setCullMode(cullMode.mode, cullMode.winding);
}

//

void setColor(const Color & color)
{
	setColorf(color.r, color.g, color.b, color.a);
}

void setColor(const Color & color, float rgbMul)
{
	setColorf(color.r * rgbMul, color.g * rgbMul, color.b * rgbMul, color.a);
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
	
	globals.color.r = r;
	globals.color.g = g;
	globals.color.b = b;
	globals.color.a = a;
	
	gxColor4f(r, g, b, a);
}

void setColorClamp(bool clamp)
{
	globals.colorClamp = clamp;
	
#if ENABLE_OPENGL && USE_LEGACY_OPENGL
	if (glClampColor != nullptr)
	{
		glClampColor(GL_CLAMP_VERTEX_COLOR, clamp ? GL_TRUE : GL_FALSE);
		checkErrorGL();
	}
#endif
}

void setAlpha(int a)
{
	globals.color.a = scale255(a);
	
	gxColor4f(globals.color.r, globals.color.g, globals.color.b, globals.color.a);
}

void setAlphaf(float a)
{
	globals.color.a = a;
	
	gxColor4f(globals.color.r, globals.color.g, globals.color.b, globals.color.a);
}

void setLumi(int l)
{
	const float lf = scale255(l);
	
	setLumif(lf);
}

void setLumif(float l)
{
	globals.color.r = l;
	globals.color.g = l;
	globals.color.b = l;
	
	gxColor4f(globals.color.r, globals.color.g, globals.color.b, globals.color.a);
}

static Stack<Color, 32> colorStack;

void pushColor()
{
	colorStack.push(globals.color);
}

void popColor()
{
	const Color color = colorStack.popValue();
	
	setColor(color);
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

static Stack<FONT_MODE, 32> fontModeStack(FONT_BITMAP);

void pushFontMode(FONT_MODE fontMode)
{
#if !ENABLE_MSDF_FONTS
	if (fontMode == FONT_SDF)
		fontMode = FONT_BITMAP;
#endif

	fontModeStack.push(globals.fontMode);
	
	setFontMode(fontMode);
}

void popFontMode()
{
	const FONT_MODE fontMode = fontModeStack.popValue();
	
	setFontMode(fontMode);
}

void setShader(const ShaderBase & shader)
{
	if (&shader != globals.shader)
	{
		globals.shader = const_cast<ShaderBase*>(&shader);
	
	#if ENABLE_OPENGL
		glUseProgram(shader.getOpenglProgram());
	#endif
		
		globals.gxShaderIsDirty = true;
	}
}

void clearShader()
{
	if (globals.shader)
	{
		globals.shader = 0;
	
	#if ENABLE_OPENGL
		glUseProgram(0);
	#endif
	}
}

void shaderSource(const char * filename, const char * text)
{
	framework.registerShaderSource(filename, text);
}

void pushShaderOutputs(const char * outputs)
{
	s_shaderOutputsStack.push(globals.shaderOutputs);
	strcpy_s(globals.shaderOutputs, sizeof(globals.shaderOutputs), outputs);
}

void popShaderOutputs()
{
	auto value = s_shaderOutputsStack.popValue();
	strcpy_s(globals.shaderOutputs, sizeof(globals.shaderOutputs), value.c_str());
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

	gxBegin(GX_LINES);
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

	gxBegin(GX_LINES);
	{
		for (int i = 0; i < numPoints; ++i)
		{
			const float tx = hxy[0];
			const float ty = hxy[1];
			const float ts = hypotf(tx, ty);

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

	gxBegin(GX_POINTS);
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

void debugDrawText(const Vec3 & position, int size, float alignX, float alignY, const char * format, ...)
{
	float w;
	const Vec2 position_screen = transformToScreen(position, w);
	
	if (w > 0.f)
	{
		char text[1024];
		va_list args;
		va_start(args, format);
		vsprintf_s(text, sizeof(text), format, args);
		va_end(args);
		
		debugDrawText(position_screen[0], position_screen[1], size, alignX, alignY, "%s", text);
	}
}

#if !ENABLE_OPENGL

SDL_Window * getWindow()
{
	return globals.currentWindow->getWindow();
}

SDL_Surface * getWindowSurface()
{
	return SDL_GetWindowSurface(globals.currentWindow->getWindow());
}

#endif

//

#if ENABLE_HQ_PRIMITIVES

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

static void setShader_HqStrokedRoundedRects()
{
	setShader(globals.builtinShaders->hqStrokedRoundedRect.get());
}

static void applyHqShaderConstants()
{
	Shader & shader = *static_cast<Shader*>(globals.shader);
	
	fassert(shader.getType() == SHADER_VSPS);
	
	const ShaderCacheElem & shaderElem = shader.getCacheElem();
	
	if (shaderElem.params[ShaderCacheElem::kSp_ShadingParams].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_ShadingParams].index,
			globals.hqGradientType,
			globals.hqTextureEnabled,
			globals.hqUseScreenSize);
	}
	
	if (globals.hqGradientType != GRADIENT_NONE)
	{
		if (shaderElem.params[ShaderCacheElem::kSp_GradientMatrix].index != -1)
		{
			// note: we multiply the gradient matrix with the inverse of the modelView matrix here,
			// so we go back from screen-space coordinates inside the shader to local coordinates
			// for the gradient. this makes it much more easy to define gradients on elements,
			// since you can freely scale and translate them around without having to worry
			// about how this affects your gradient
			
			Mat4x4 modelView;
			gxGetMatrixf(GX_MODELVIEW, modelView.m_v);
			
			const Mat4x4 gmat = globals.hqGradientMatrix * modelView.CalcInv();
			
			shader.setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_GradientMatrix].index, gmat.m_v);
		}
		
		if (shaderElem.params[ShaderCacheElem::kSp_GradientInfo].index != -1)
		{
			Mat4x4 gradientInfo;
			gradientInfo(0, 0) = globals.hqGradientType;
			gradientInfo(0, 1) = globals.hqGradientBias;
			gradientInfo(0, 2) = globals.hqGradientScale;
			gradientInfo(0, 3) = globals.hqGradientColorMode;
			gradientInfo(1, 0) = globals.hqGradientColor1.r;
			gradientInfo(1, 1) = globals.hqGradientColor1.g;
			gradientInfo(1, 2) = globals.hqGradientColor1.b;
			gradientInfo(1, 3) = globals.hqGradientColor1.a;
			gradientInfo(2, 0) = globals.hqGradientColor2.r;
			gradientInfo(2, 1) = globals.hqGradientColor2.g;
			gradientInfo(2, 2) = globals.hqGradientColor2.b;
			gradientInfo(2, 3) = globals.hqGradientColor2.a;
			gradientInfo(3, 0) = 0.f;
			gradientInfo(3, 1) = 0.f;
			gradientInfo(3, 2) = 0.f;
			gradientInfo(3, 3) = 0.f;
			
			shader.setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_GradientInfo].index, gradientInfo.m_v);
		}
	}
	
	if (globals.hqTextureEnabled)
	{
		shader.setTexture("source", 0, globals.hqTexture, true, false);
		
		if (shaderElem.params[ShaderCacheElem::kSp_TextureMatrix].index != -1)
		{
			const Mat4x4 & tmat = globals.hqTextureMatrix;
			
			shader.setImmediateMatrix4x4(shaderElem.params[ShaderCacheElem::kSp_TextureMatrix].index, tmat.m_v);
		}
	}

#if FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT
	//shader.setImmediate("disableOptimizations", cosf(framework.time * 6.28f) < 0.f ? 0.f : 1.f);
	//shader.setImmediate("disableAA", cosf(framework.time) < 0.f ? 0.f : 1.f);

	shader.setImmediate("disableOptimizations", 0.f);
	shader.setImmediate("disableAA", 0.f);
	shader.setImmediate("_debugHq", 0.f);
#endif
}

void hqBegin(HQ_TYPE type, bool useScreenSize)
{
	switch (type)
	{
	case HQ_LINES:
		setShader_HqLines();
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_TRIANGLES:
		setShader_HqFilledTriangles();
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_FILLED_CIRCLES:
		setShader_HqFilledCircles();
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_RECTS:
		setShader_HqFilledRects();
		gxBegin(GX_QUADS);
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		setShader_HqFilledRoundedRects();
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_TRIANGLES:
		setShader_HqStrokedTriangles();
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_STROKED_CIRCLES:
		setShader_HqStrokedCircles();
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_RECTS:
		setShader_HqStrokedRects();
		gxBegin(GX_QUADS);
		break;
	
	case HQ_STROKED_ROUNDED_RECTS:
		setShader_HqStrokedRoundedRects();
		gxBegin(GX_QUADS);
		break;

	default:
		fassert(false);
		break;
	}
	
	globals.hqUseScreenSize = useScreenSize;
	
	applyHqShaderConstants();
}

void hqBeginCustom(HQ_TYPE type, Shader & shader, bool useScreenSize)
{
	setShader(shader);
	
	switch (type)
	{
	case HQ_LINES:
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_TRIANGLES:
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_FILLED_CIRCLES:
		gxBegin(GX_QUADS);
		break;

	case HQ_FILLED_RECTS:
		gxBegin(GX_QUADS);
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_TRIANGLES:
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_STROKED_CIRCLES:
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_RECTS:
		gxBegin(GX_QUADS);
		break;
		
	case HQ_STROKED_ROUNDED_RECTS:
		gxBegin(GX_QUADS);
		break;

	default:
		fassert(false);
		break;
	}
	
	globals.hqUseScreenSize = useScreenSize;
	
	applyHqShaderConstants();
}

void hqEnd()
{
	gxEnd();

	clearShader();
}

#else

static HQ_TYPE s_hqType;

static float s_hqScale;

void hqBegin(HQ_TYPE type, bool useScreenSize)
{
	if (useScreenSize)
	{
		Mat4x4 matM;
		
		gxGetMatrixf(GX_MODELVIEW, matM.m_v);
		
		const float scale = matM.GetAxis(0).CalcSize();
		
		s_hqScale = 1.f / scale;
	}
	else
	{
		s_hqScale = 1.f;
	}
	
	//
	
	switch (type)
	{
	case HQ_LINES:
		gxBegin(GX_LINES);
		break;

	case HQ_FILLED_TRIANGLES:
		gxBegin(GX_TRIANGLES);
		break;

	case HQ_FILLED_CIRCLES:
		break;

	case HQ_FILLED_RECTS:
		gxBegin(GX_QUADS);
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxBegin(GX_QUADS);
		break;

	case HQ_STROKED_TRIANGLES:
		gxBegin(GX_LINES);
		break;

	case HQ_STROKED_CIRCLES:
		break;

	case HQ_STROKED_RECTS:
		gxBegin(GX_LINES);
		break;
	
	case HQ_STROKED_ROUNDED_RECTS:
		gxBegin(GX_LINES);
		break;

	default:
		fassert(false);
		break;
	}
	
	s_hqType = type;
	
	globals.hqUseScreenSize = useScreenSize;
}

void hqBeginCustom(HQ_TYPE type, Shader & shader, bool useScreenSize)
{
	setShader(shader);
	
	hqBegin(type, useScreenSize);
}

void hqEnd()
{
	switch (s_hqType)
	{
	case HQ_LINES:
		gxEnd();
		break;

	case HQ_FILLED_TRIANGLES:
		gxEnd();
		break;

	case HQ_FILLED_CIRCLES:
		break;

	case HQ_FILLED_RECTS:
		gxEnd();
		break;
	
	case HQ_FILLED_ROUNDED_RECTS:
		gxEnd();
		break;

	case HQ_STROKED_TRIANGLES:
		gxEnd();
		break;

	case HQ_STROKED_CIRCLES:
		break;

	case HQ_STROKED_RECTS:
		gxEnd();
		break;
	
	case HQ_STROKED_ROUNDED_RECTS:
		gxEnd();
		break;

	default:
		fassert(false);
		break;
	}
}

#endif

void hqSetGradient(GRADIENT_TYPE gradientType, const Mat4x4 & matrix, const Color & color1, const Color & color2, const COLOR_MODE colorMode, const float bias, const float scale)
{
	globals.hqGradientType = gradientType;
	globals.hqGradientMatrix = matrix;
	globals.hqGradientColor1 = color1;
	globals.hqGradientColor2 = color2;
	globals.hqGradientColorMode = colorMode;
	globals.hqGradientBias = bias;
	globals.hqGradientScale = scale;
}

void hqClearGradient()
{
	globals.hqGradientType = GRADIENT_NONE;
}

void hqSetTexture(const GxTextureId texture, const Mat4x4 & matrix)
{
	globals.hqTextureEnabled = true;
	globals.hqTexture = texture;
	globals.hqTextureMatrix = matrix;
}

void hqSetTextureScreen(const GxTextureId texture, float x1, float y1, float x2, float y2)
{
	const Mat4x4 matrix =
		Mat4x4(true)
			.Scale(1.f / fmaxf(x2 - x1, 1e-6f), 1.f / fmaxf(y2 - y1, 1e-6f), 1.f)
			.Translate(-x1, -y1, 0.f);
	
	hqSetTexture(texture, matrix);
}

void hqClearTexture()
{
	globals.hqTextureEnabled = false;
	globals.hqTexture = 0;
}

#if ENABLE_HQ_PRIMITIVES

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

void hqStrokeRoundedRect(float x1, float y1, float x2, float y2, float radius, float stroke)
{
	gxNormal3f(radius, stroke, 0.f);
	for (int i = 0; i < 4; ++i)
		gxVertex4f(x1, y1, x2, y2);
}

#else

// these are really shitty regular OpenGL approximations to the HQ primitives. don't expect much when using them!

void hqLine(float x1, float y1, float strokeSize1, float x2, float y2, float strokeSize2)
{
	gxVertex2f(x1, y1);
	gxVertex2f(x2, y2);
}

void hqFillTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
	gxVertex2f(x1, y1);
	gxVertex2f(x2, y2);
	gxVertex2f(x3, y3);
}

void hqFillCircle(float x, float y, float radius)
{
	radius *= s_hqScale;
	
	const int numSegments = radius * 6.f + 4.f;
	
	fillCircle(x, y, radius, numSegments);
}

void hqFillRect(float x1, float y1, float x2, float y2)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
}

void hqFillRoundedRect(float x1, float y1, float x2, float y2, float radius)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
}

void hqStrokeTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float stroke)
{
	gxVertex2f(x1, y1);
	gxVertex2f(x2, y2);
	
	gxVertex2f(x2, y2);
	gxVertex2f(x3, y3);
	
	gxVertex2f(x3, y3);
	gxVertex2f(x1, y1);
}

void hqStrokeCircle(float x, float y, float radius, float stroke)
{
	radius *= s_hqScale;
	
	const int numSegments = radius * 6.f + 4.f;
	
	drawCircle(x, y, radius, numSegments);
}

void hqStrokeRect(float x1, float y1, float x2, float y2, float stroke)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
}

void hqStrokeRoundedRect(float x1, float y1, float x2, float y2, float radius, float stroke)
{
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	
	gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	
	gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	
	gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
	gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
}

#endif

void hqDrawPath(const Path2d & path, float stroke)
{
	const int kMaxPoints = 16 * 1024;

	float pxyStorage[kMaxPoints * 2];
	float hxyStorage[kMaxPoints * 2];

	float * pxy = pxyStorage;
	float * hxy = hxyStorage;

	int numPoints = 0;
	path.generatePoints(pxy, hxy, kMaxPoints, 1.f, numPoints);

	hqBegin(HQ_LINES, true);
	{
		for (int i = 0; i < numPoints - 1; ++i)
		{
			hqLine(pxy[0], pxy[1], stroke, pxy[2], pxy[3], stroke);

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
			//const float ts = hypotf(tx, ty);

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
		const float strokeSize = (sinf(framework.time) + 1.f) / 2.f * 4.f;

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

void setupPaths(const char * chibiResourcePaths)
{
#if FRAMEWORK_USE_SDL
	changeDirectory(SDL_GetBasePath());
#endif
	
	if (chibiResourcePaths != nullptr)
		framework.registerChibiResourcePaths(chibiResourcePaths);
}

void changeDirectory(const char * path)
{
	const int error = _chdir(path);
	
	if (error != 0)
		logError("failed to changeDirectory. errno=%d", errno);
}

std::string getDirectory()
{
	char temp[1024];
	
#if WINDOWS
	_getcwd(temp, sizeof(temp));
#else
	const char * r;
	r = getcwd(temp, sizeof(temp));
	(void)r; // mute gcc warning about unused return value
#endif

	return temp;
}

#if ENABLE_LOGGING

#if defined(ANDROID)
	#include <android/log.h>
#endif

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

#if defined(ANDROID)
	__android_log_write(ANDROID_LOG_VERBOSE, "Framework", text);
#else
	fprintf(stderr, "[DD] %s\n", text);
#endif
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

#if defined(ANDROID)
	__android_log_write(ANDROID_LOG_INFO, "Framework", text);
#else
	fprintf(stderr, "[II] %s\n", text);
#endif
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

#if defined(ANDROID)
	__android_log_write(ANDROID_LOG_WARN, "Framework", text);
#else
	fprintf(stderr, "[WW] %s\n", text);
#endif
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

#if defined(ANDROID)
	__android_log_write(ANDROID_LOG_ERROR, "Framework", text);
#else
	fprintf(stderr, "[EE] %s\n", text);
#endif
}

#endif
