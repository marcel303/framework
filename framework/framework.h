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

#pragma once

#if USE_LEGACY_OPENGL
	#include <GL/glew.h>
	#include <SDL2/SDL_opengl.h>
#endif

#include "Debugging.h"
#include "framework-event.h" // SDL_Event and key codes
#include "framework-vr-hands.h"
#include "framework-vr-pointer.h"
#include "gx_texture.h" // GX_TEXTURE_FORMAT
#include "Mat4x4.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

#if FRAMEWORK_USE_SDL
	#include <SDL2/SDL_main.h>
	#include <SDL2/SDL_mutex.h>
	#include <SDL2/SDL_thread.h>
	#include <SDL2/SDL_timer.h>
#endif

#include <float.h> // FLT_MAX (sprite draw)
#include <math.h> // math functions
#include <string>
#include <vector>

#if defined(DEBUG)
	#define fassert(x) Assert(x)
#else
	#define fassert(x) do { } while (false)
#endif

// configuration

#if defined(DEBUG)
	#define ENABLE_LOGGING_DBG 1
	#define ENABLE_LOGGING 1

	#define FRAMEWORK_ENABLE_GL_ERROR_LOG 1
	#define FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT 1
#else
	#define ENABLE_LOGGING_DBG 0 // do not alter
	#define ENABLE_LOGGING 0 // do not alter

	#define FRAMEWORK_ENABLE_GL_ERROR_LOG 0 // do not alter
	#define FRAMEWORK_ENABLE_GL_DEBUG_CONTEXT 0 // do not alter
#endif

#if !defined(ENABLE_OPENGL) && !defined(ENABLE_METAL)
	#if (defined(MACOS) || defined(IPHONEOS)) && 0
		#define ENABLE_METAL 1
	#else
		#define ENABLE_OPENGL 1
	#endif
#endif

#if defined(IPHONEOS)
	#define ENABLE_DESKTOP_OPENGL 0
	#define ENABLE_OPENGL_ES3 1
#elif defined(ANDROID)
	#define ENABLE_DESKTOP_OPENGL 0
	#define ENABLE_OPENGL_ES3 1
#elif ENABLE_OPENGL
	#define ENABLE_DESKTOP_OPENGL 1 // do not alter
#else
	#define ENABLE_DESKTOP_OPENGL 0 // do not alter
#endif

#if ENABLE_DESKTOP_OPENGL
	#define ENABLE_COMPUTE_SHADER 1
#else
	#define ENABLE_COMPUTE_SHADER 0 // do not alter
#endif

#if ENABLE_OPENGL && !defined(LINUX) && !defined(IPHONEOS) && !defined(ANDROID)
	#define ENABLE_PROFILING 1
#else
	#define ENABLE_PROFILING 0
#endif

#if !defined(USE_LEGACY_OPENGL)
	#define USE_LEGACY_OPENGL 0
#endif

#if defined(MACOS)
    #define OPENGL_VERSION 410
#else
    #define OPENGL_VERSION 430
#endif

#if !defined(ENABLE_HQ_PRIMITIVES)
    #if (ENABLE_OPENGL && !USE_LEGACY_OPENGL) || ENABLE_METAL
        #define ENABLE_HQ_PRIMITIVES 1
    #else
        #define ENABLE_HQ_PRIMITIVES 0
    #endif
#endif

#if !defined(ENABLE_UTF8_SUPPORT)
	#define ENABLE_UTF8_SUPPORT 1
#endif

static const int MAX_GAMEPAD = 4;

// enumerations

enum BLEND_MODE // setBlend
{
	BLEND_OPAQUE,
	BLEND_ALPHA,                    // regular alpha blending
	BLEND_PREMULTIPLIED_ALPHA,      // blend a color with premultiplied alpha
	BLEND_PREMULTIPLIED_ALPHA_DRAW, // premultiply color with alpha and blend
	BLEND_ABSORBTION_MASK,          // generates a color mask, where colors are filtered away by the things drawn to a surface. the surface must be initialized to white with 100% opacity for this to work. the color blended with the mask MUST use premultiplied-alpha
	BLEND_ADD,                      // the color is multiplied with alpha before being added
	BLEND_ADD_OPAQUE,               // component-wise add. ignores alpha
	BLEND_SUBTRACT,
	BLEND_INVERT,
	BLEND_MUL,
	BLEND_MIN,
	BLEND_MAX
};

enum COLOR_MODE // setColorMode
{
	COLOR_MUL,
	COLOR_ADD,
	COLOR_SUB,
	COLOR_IGNORE
};

enum COLOR_POST // setColorPost
{
	POST_NONE,
	POST_RGB_MIX_ALPHA_TO_ZERO,
	POST_RGB_MIX_ALPHA_TO_ONE,
	POST_PREMULTIPLY_RGB_WITH_ALPHA = POST_RGB_MIX_ALPHA_TO_ZERO,
	POST_BLEND_MUL_FIX = POST_RGB_MIX_ALPHA_TO_ONE,
	POST_SET_ALPHA_TO_ONE,
	POST_RGB_TO_LUMI,
	POST_SET_RGB_TO_R
};

enum DEPTH_TEST // setDepthTest
{
	DEPTH_EQUAL,
	DEPTH_LESS,
	DEPTH_LEQUAL,
	DEPTH_GREATER,
	DEPTH_GEQUAL,
	DEPTH_ALWAYS
};

enum CULL_MODE // setCullMode
{
	CULL_NONE,
	CULL_FRONT,
	CULL_BACK
};

enum CULL_WINDING // setCullMode
{
	CULL_CCW,
	CULL_CW
};

enum FONT_MODE // setFontMode
{
	FONT_BITMAP,
	FONT_SDF
};

enum BUTTON
{
	BUTTON_LEFT,
	BUTTON_RIGHT,
	BUTTON_MAX
};

enum ANALOG
{
	ANALOG_X,
	ANALOG_Y,
	ANALOG_MAX
};

enum GAMEPAD
{
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
	GAMEPAD_MAX
};

enum TEXTURE_FILTER
{
	FILTER_POINT,
	FILTER_LINEAR,
	FILTER_MIPMAP
};

enum TRANSFORM
{
	TRANSFORM_SCREEN, // pixel based coordinate system
	TRANSFORM_2D,     // use transform set through setTransform2d
	TRANSFORM_3D      // use transform set through setTransform3d
};

enum SHADER_TYPE
{
	SHADER_VSPS,
	SHADER_CS
};

enum RESOURCE_CACHE
{
	CACHE_FONT = 1 << 0,
	CACHE_FONT_MSDF = 1 << 1,
	CACHE_SHADER = 1 << 2,
	CACHE_SOUND = 1 << 3,
	CACHE_SPRITE = 1 << 4,
	CACHE_SPRITER = 1 << 5,
	CACHE_TEXTURE = 1 << 6
};

enum INIT_ERROR
{
	INIT_ERROR_SDL,
	INIT_ERROR_VIDEO_MODE,
	INIT_ERROR_WINDOW,
	INIT_ERROR_OPENGL,
	INIT_ERROR_OPENGL_EXTENSIONS,
	INIT_ERROR_SOUND,
	INIT_ERROR_FREETYPE
};

// forward declations

class Color;
class Dictionary;
class Font;
class Framework;
class Gamepad;
class Keyboard;
class Model;
class Mouse;
class Music;
class Shader;
class ShaderBuffer;
class ShaderBufferRw;
class Sound;
class Sprite;
class Spriter;
struct SpriterState;
class Surface;
class Window;

namespace AnimModel
{
	class BoneTransform;
}

namespace spriter
{
	struct Drawable;
	class Scene;
}

class ColorTarget;
class DepthTarget;

// globals

extern Framework framework;
extern Mouse mouse;
extern Keyboard keyboard;
extern Gamepad gamepad[MAX_GAMEPAD];

extern VrHand vrHand[2];
extern VrPointer vrPointer[2];

// event handlers

typedef void (*ActionHandler)(const std::string & action, const Dictionary & args);
typedef void (*FillCachesCallback)(float filePercentage);
typedef void (*FillCachesUnknownResourceCallback)(const char * filename);
typedef void (*RealTimeEditCallback)(const std::string & filename);
typedef void (*InitErrorHandler)(INIT_ERROR error);

// GX types

typedef int32_t GxImmediateIndex;

#if USE_LEGACY_OPENGL

// these must match the OpenGL definition for things to work when mapping GX calls to legacy OpenGL

enum GX_PRIMITIVE_TYPE
{
	GX_INVALID_PRIM   = -1,
	GX_POINTS         = GL_POINTS,
	GX_LINES          = GL_LINES,
	GX_LINE_LOOP      = GL_LINE_LOOP,
	GX_LINE_STRIP     = GL_LINE_STRIP,
	GX_TRIANGLES      = GL_TRIANGLES,
	GX_TRIANGLE_FAN   = GL_TRIANGLE_FAN,
	GX_TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
	GX_QUADS          = GL_QUADS
};

enum GX_MATRIX
{
	GX_MODELVIEW  = GL_MODELVIEW,
	GX_PROJECTION = GL_PROJECTION
};

enum GX_SAMPLE_FILTER
{
	GX_SAMPLE_NEAREST = GL_NEAREST,
	GX_SAMPLE_LINEAR  = GL_LINEAR,
	GX_SAMPLE_MIPMAP  = GL_LINEAR_MIPMAP_LINEAR
};

#else

enum GX_PRIMITIVE_TYPE
{
	GX_INVALID_PRIM = -1,
	GX_POINTS,
	GX_LINES,
	GX_LINE_LOOP,
	GX_LINE_STRIP,
	GX_TRIANGLES,
	GX_TRIANGLE_FAN,
	GX_TRIANGLE_STRIP,
	GX_QUADS
};

enum GX_MATRIX
{
	GX_MODELVIEW,
	GX_PROJECTION
};

enum GX_SAMPLE_FILTER
{
	GX_SAMPLE_NEAREST,
	GX_SAMPLE_LINEAR,
	GX_SAMPLE_MIPMAP
};

#endif

enum GX_STENCIL_FACE
{
	GX_STENCIL_FACE_FRONT = 0x1,
	GX_STENCIL_FACE_BACK  = 0x2,
	GX_STENCIL_FACE_BOTH  = 0x3
};

enum GX_STENCIL_FUNC
{
	GX_STENCIL_FUNC_NEVER,
	GX_STENCIL_FUNC_LESS,
	GX_STENCIL_FUNC_LEQUAL,
	GX_STENCIL_FUNC_GREATER,
	GX_STENCIL_FUNC_GEQUAL,
	GX_STENCIL_FUNC_EQUAL,
	GX_STENCIL_FUNC_NOTEQUAL,
	GX_STENCIL_FUNC_ALWAYS
};

enum GX_STENCIL_OP
{
	GX_STENCIL_OP_KEEP,
	GX_STENCIL_OP_REPLACE,
	GX_STENCIL_OP_ZERO,
	GX_STENCIL_OP_INC,
	GX_STENCIL_OP_DEC,
	GX_STENCIL_OP_INC_WRAP,
	GX_STENCIL_OP_DEC_WRAP
};

//

class Framework
{
public:
	friend class Sprite;
	friend class Model;
	friend class Window;
	
	Framework();
	~Framework();
	
	bool init(int sx, int sy);
	bool shutdown();
	
	void process();
	void processAction(const std::string & action, const Dictionary & args);
	void processActions(const std::string & actions, const Dictionary & args);
	
	void reloadCaches();
	void fillCachesWithPath(const char * path, bool recurse);
	void fillCaches(bool recurse);
	
	Window & getMainWindow() const;
	Window & getCurrentWindow() const;
	std::vector<Window*> getAllWindows() const;
	
	void setFullscreen(bool fullscreen);
	
	void getCurrentViewportSize(int & sx, int & sy) const;
	int getCurrentBackingScale() const;

	void beginDraw(int r, int g, int b, int a, float depth = 1.f);
	void endDraw();
	void present(); // note : currently only needed for manual vr mode. but may come in handy later, for correctly vsync'ed multi-window drawing
	
	// stereoscopic vr rendering
	bool isStereoVr() const;
	int getEyeCount() const;
	void beginEye(const int eyeIndex, const Color & clearColor);
	void beginEye(const int eyeIndex, float r, float g, float b, float a);
	void endEye();
	Mat4x4 getHeadTransform() const;
	
	void beginScreenshot(int r, int g, int b, int a, int scale);
	void endScreenshot(const char * name, int index = -1, bool omitAlpha = true);
	void screenshot(const char * name, int index = -1, bool omitAlpha = true);

	void registerShaderSource(const char * name, const char * text);
	void unregisterShaderSource(const char * name);
	bool tryGetShaderSource(const char * name, const char *& text) const;
	
	void registerShaderOutput(const char name, const char * outputType, const char * outputName);
	void unregisterShaderOutput(const char name);
	
	bool fileHasChanged(const char * filename) const;
	
	void registerResourcePath(const char * path);
	bool registerChibiResourcePaths(const char * text);
	const char * resolveResourcePath(const char * path) const;
	
	void registerResourceCache(class ResourceCacheBase * resourceCache);
	
	void blinkTaskbarIcon(int count);

	bool tickVirtualDesktop(const Mat4x4 & pointerTransform, const bool pointerTransformIsValid, const int buttonMask, const bool isHand);
	void drawVirtualDesktop();
	void drawVrPointers();
	
	void setClipboardText(const char * text);
	std::string getClipboardText();
	bool hasClipboardText();

	bool quitRequested;
	double time;
	float timeStep;
	
	bool waitForEvents;
	
	bool fullscreen;
	bool exclusiveFullscreen;
	bool useClosestDisplayMode;
	bool enableVsync;
	int msaaLevel;
	bool basicOpenGL;
	bool enableDepthBuffer;
	bool enableDrawTiming;
	bool enableProfiling;
	bool allowHighDpi;
	int minification;
	bool reloadCachesOnActivate;
	bool cacheResourceData;
	bool enableRealTimeEditing;
	bool vrMode;
	bool filedrop;
	bool enableSound;
	int numSoundSources;
	int windowX;
	int windowY;
	bool windowBorder;
	bool windowIsResizable;
	std::string windowTitle;
	std::string windowIcon;
	int windowSx;
	int windowSy;
	bool windowIsActive;
	bool portraitMode;
	
	ActionHandler actionHandler;
	FillCachesCallback fillCachesCallback;
	FillCachesUnknownResourceCallback fillCachesUnknownResourceCallback;
	RealTimeEditCallback realTimeEditCallback;
	InitErrorHandler initErrorHandler;
	
	std::vector<std::string> resourcePaths;
	
	std::vector<class ResourceCacheBase*> resourceCaches;
	
	std::vector<SDL_Event> events;       // all events captured during the last process() call
	std::vector<SDL_Event> windowEvents; // events for the current window captured during the last process() call
	
	std::vector<std::string> changedFiles;
	std::vector<std::string> droppedFiles;
	
	Vec3 vrOrigin;
	bool enableVrMovement;
	
	bool backgrounded;
	
private:
	uint64_t m_lastTick;
	
	Sprite * m_sprites;
	Model * m_models;
	Window * m_windows;

	void registerSprite(Sprite * sprite);
	void unregisterSprite(Sprite * sprite);
	
	void registerModel(Model * model);
	void unregisterModel(Model * model);
	
	void registerWindow(Window * window);
	void unregisterWindow(Window * window);
	class WindowData * findWindowDataById(const int id);
};

// -----

#if defined(FRAMEWORK_USE_OVR_MOBILE)
	#define FRAMEWORK_IS_NATIVE_VR 1
#else
	#define FRAMEWORK_IS_NATIVE_VR 0
#endif

#if FRAMEWORK_IS_NATIVE_VR
	#define WINDOW_HAS_A_SURFACE 1
	#define WINDOW_IS_3D 1
#else
	#define WINDOW_HAS_A_SURFACE 0
	#define WINDOW_IS_3D 0
#endif

class Window
{
private:
	friend class Framework;

#if FRAMEWORK_USE_SDL
	Window(SDL_Window * window);
#endif
	
public:
	Window(const char * title, const int sx, const int sy, const bool resizable = false);
	~Window();
	
	void setPosition(const int x, const int y);
	void setPositionCentered();
	void setSize(const int sx, const int sy);
	void setFullscreen(const bool fullscreen);
	void setTitle(const char * title);
	
	void show();
	void hide();
	
	bool isHidden() const;
	bool hasFocus() const;
	
	void raise();
	
	void getPosition(int & x, int & y) const;
	int getWidth() const;
	int getHeight() const;
	const char * getTitle() const;
	
	bool isFullscreen() const;
	
	bool getQuitRequested() const;
	
	bool getMousePosition(float & x, float & y) const;
	
	bool hasSurface() const;
	
	class ColorTarget * getColorTarget() const;
	class DepthTarget * getDepthTarget() const;

#if FRAMEWORK_USE_SDL
	SDL_Window * getWindow() const;
#else
	void setHasFocus(const bool hasFocus);
#endif

#if WINDOW_IS_3D
	void setTransform(const Mat4x4 & transform);
	void setPixelsPerMeter(const float ppm);
	bool intersectRay(Vec3Arg rayOrigin, Vec3Arg rayDirection, const float depthThreshold, Vec2 & out_pixelPos, float & out_distance) const;
	void draw3d() const;
	void draw3dCursor() const;
	const Mat4x4 & getTransform() const;
	Mat4x4 getTransformForDraw() const;
#endif

	class WindowData * getWindowData() const;
	
private:
	// book keeping
	Window * m_prev;
	Window * m_next;
	
#if WINDOW_HAS_A_SURFACE
	// render targets
	class ColorTarget * m_colorTarget;
	class DepthTarget * m_depthTarget;
#endif

#if FRAMEWORK_USE_SDL
	// SDL window
	SDL_Window * m_window;
#else
	// these properties are normally managed by SDL
	std::string m_title;
	bool m_isVisible;
	bool m_hasFocus;
#endif
	
#if WINDOW_IS_3D
	Mat4x4 m_transform;
	float m_pixelsPerMeter;
	bool m_isInteracting; // updated by tickVirtualDesktop
#endif

	// keyboard and mouse data
	class WindowData * m_windowData;
};

void pushWindow(Window & window);
void popWindow();

//

enum SURFACE_FORMAT
{
	SURFACE_RGBA8,
	SURFACE_RGBA8_SRGB, // perform srgb to linear conversion when sampling the texture
	SURFACE_RGBA16F,
	SURFACE_RGBA32F,
	SURFACE_R8,
	SURFACE_R16F,
	SURFACE_R32F,
	SURFACE_RG8,
	SURFACE_RG16F,
	SURFACE_RG32F
};

enum DEPTH_FORMAT
{
	DEPTH_UNORM16,
	DEPTH_FLOAT32
};

class SurfaceProperties
{
public:
	struct
	{
		int width = 0;
		int height = 0;
		int backingScale = 0; // the backing scale is a multiplier applied to the size of the surface. 0 = automatically select the backing scale, any other value will be used to multiply the width and height of the storage used to back the surface
		
		void init(const int sx, const int sy)
		{
			width = sx;
			height = sy;
		}
		
		void setBackingScale(const int in_backingScale)
		{
			backingScale = in_backingScale;
		}
	} dimensions;
	
	struct
	{
		bool enabled = false;
		SURFACE_FORMAT format = SURFACE_RGBA8;
		int swizzle[4] = { 0, 1, 2, 3 };
		bool doubleBuffered = false;
		
		void init(const SURFACE_FORMAT in_format, const bool in_doubleBuffered)
		{
			enabled = true;
			format = in_format;
			doubleBuffered = in_doubleBuffered;
		}
		
		void setSwizzle(const int r, const int g, const int b, const int a)
		{
			swizzle[0] = r;
			swizzle[1] = g;
			swizzle[2] = b;
			swizzle[3] = a;
		}
	} colorTarget;
	
	struct
	{
		bool enabled = false;
		DEPTH_FORMAT format = DEPTH_FLOAT32;
		bool doubleBuffered = false;
		
		void init(const DEPTH_FORMAT in_format, const bool in_doubleBuffered)
		{
			enabled = true;
			format = in_format;
			doubleBuffered = in_doubleBuffered;
		}
	} depthTarget;
};

class Surface
{
	SurfaceProperties m_properties;
	
	int m_backingScale; // backing scale puts a multiplier on the physical size (in pixels) of the surface. it's like MSAA, but fully super-sampled. it's used t orender to retina screens, where the 'resolve' operation just copies pixels 1:1, where a resolve onto a non-retina screen would downsample the surface instead
	
	int m_bufferId;
	
	ColorTarget * m_colorTarget[2];
	DepthTarget * m_depthTarget[2];
	
	std::string m_name;

	void construct();
	void destruct();
	
public:
	Surface();
	explicit Surface(int sx, int sy, bool highPrecision, bool withDepthBuffer = false, bool doubleBuffered = true, const int backingScale = 0);
	explicit Surface(int sx, int sy, bool withDepthBuffer, bool doubleBuffered, SURFACE_FORMAT format, const int backingScale = 0);
	~Surface();
	
	void swapBuffers();

	bool init(const SurfaceProperties & properties);
	bool init(int sx, int sy, SURFACE_FORMAT format, bool withDepthBuffer, bool doubleBuffered, int backingScale = 0);
	void free();
	void setSwizzle(int r, int g, int b, int a);
	void setClearColor(int r, int g, int b, int a);
	void setClearColorf(float r, float g, float b, float a);
	void setClearDepth(float d);
	
	void setName(const char * name);
	const char * getName() const;
	
	ColorTarget * getColorTarget();
	DepthTarget * getDepthTarget();
	GxTextureId getTexture() const;
	bool hasDepthTexture() const;
	GxTextureId getDepthTexture() const;
	int getWidth() const;
	int getHeight() const;
	int getBackingScale() const;
	SURFACE_FORMAT getFormat() const;
	
	void clear(int r = 0, int g = 0, int b = 0, int a = 0);
	void clearf(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.f);
	void clearDepth(float d);
	void clearAlpha();
	void setAlpha(int a);
	void setAlphaf(float a);
	void mulf(float r, float g, float b, float a = 1.f);
	
	void postprocess();
	void postprocess(Shader & shader);
	
	void invert();
	void invertColor();
	void invertAlpha();
	void gaussianBlur(const float strengthH, const float strengthV, const int kernelSize = -1);
	
	void blitTo(Surface * surface) const;
	void blit(BLEND_MODE blendMode) const;
	
	static SURFACE_FORMAT toSurfaceFormat(const GX_TEXTURE_FORMAT textureFormat);
};

void blitBackBufferToSurface(Surface * surface);

//

class ShaderBase
{
public:
	virtual ~ShaderBase() { }
	
	virtual bool isValid() const = 0;
	virtual SHADER_TYPE getType() const = 0;
	virtual int getVersion() const = 0;
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const = 0;
	
#if ENABLE_OPENGL
	virtual uint32_t getOpenglProgram() const = 0;
#endif
};

//

enum GX_IMMEDIATE_TYPE
{
	GX_IMMEDIATE_FLOAT,
	GX_IMMEDIATE_VEC2,
	GX_IMMEDIATE_VEC3,
	GX_IMMEDIATE_VEC4,
	GX_IMMEDIATE_TEXTURE_2D
};

struct GxImmediateInfo
{
	GX_IMMEDIATE_TYPE type;
	std::string name;
	GxImmediateIndex index = -1;
};

struct GxTextureInfo
{
	GX_IMMEDIATE_TYPE type;
	std::string name;
	GxImmediateIndex index = -1;
};

class ShaderCacheElem;

class Shader : public ShaderBase
{
public:
	Shader();
	Shader(const char * name, const char * outputs = nullptr);
	Shader(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs = nullptr);
	virtual ~Shader();
	
	void load(const char * name, const char * filenameVs, const char * filenamePs, const char * outputs = nullptr);
	void reload();
	
	virtual bool isValid() const override;
	virtual SHADER_TYPE getType() const override { return SHADER_VSPS; }
	virtual int getVersion() const override;
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const override;

	GxImmediateIndex getImmediateIndex(const char * name) const;
	void getImmediateValuef(const GxImmediateIndex index, float * value) const;
	
	std::vector<GxImmediateInfo> getImmediateInfos() const;
	std::vector<GxTextureInfo> getTextureInfos() const;
	
	void setImmediate(const char * name, float x);
	void setImmediate(const char * name, float x, float y);
	void setImmediate(const char * name, float x, float y, float z);
	void setImmediate(const char * name, float x, float y, float z, float w);
	void setImmediate(GxImmediateIndex index, float x);
	void setImmediate(GxImmediateIndex index, float x, float y);
	void setImmediate(GxImmediateIndex index, float x, float y, float z);
	void setImmediate(GxImmediateIndex index, float x, float y, float z, float w);
	void setImmediateFloatArray(const char * name, const float * values, const int numValues);
	void setImmediateFloatArray(GxImmediateIndex index, const float * values, const int numValues);
	void setImmediateMatrix4x4(const char * name, const float * matrix);
	void setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix);
	void setImmediateMatrix4x4Array(const char * name, const float * matrices, const int numMatrices);
	void setImmediateMatrix4x4Array(GxImmediateIndex index, const float * matrices, const int numMatrices);
	
	void setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTexture(GxImmediateIndex index, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureCube(const char * name, int unit, GxTextureId texture, bool filtered = true);
	void setTexture3d(const char * name, int unit, GxTextureId texture, bool filtered = true, bool clamp = true);
	void setBuffer(const char * name, const ShaderBuffer & buffer);
	void setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer);
	void setBufferRw(const char * name, const ShaderBufferRw & buffer);
	void setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer);

#if ENABLE_OPENGL
	virtual uint32_t getOpenglProgram() const override final { return getProgram(); }
#endif

#if ENABLE_METAL
	const ShaderCacheElem & getCacheElem() const;
#else
	const ShaderCacheElem & getCacheElem() const { return *m_cacheElem; }
#endif

private:
#if ENABLE_METAL
	class ShaderCacheElem_Metal * m_cacheElem = nullptr;
#else
	ShaderCacheElem * m_cacheElem = nullptr;
#endif

#if ENABLE_OPENGL
	uint32_t getProgram() const;
#endif
};

//

#if ENABLE_OPENGL && ENABLE_COMPUTE_SHADER

class ComputeShader : public ShaderBase
{
public:
	class ComputeShaderCacheElem * m_shader;

	uint32_t getProgram() const;
	
public:
	// assuming a 64-lane wavefront, 8x8x1 is good thread distribution for common shaders
	static const int kDefaultGroupSx = 8;
	static const int kDefaultGroupSy = 8;
	static const int kDefaultGroupSz = 1;

	ComputeShader();
	ComputeShader(const char * filename, const int groupSx = kDefaultGroupSx, const int groupSy = kDefaultGroupSy, const int groupSz = kDefaultGroupSz);
	virtual ~ComputeShader();

	void load(const char * filename, const int groupSx = kDefaultGroupSx, const int groupSy = kDefaultGroupSy, const int groupSz = kDefaultGroupSz);
	virtual bool isValid() const override { return m_shader != 0; }
	virtual SHADER_TYPE getType() const override { return SHADER_CS; }
	virtual int getVersion() const override;
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const override;

	int getGroupSx() const;
	int getGroupSy() const;
	int getGroupSz() const;
	int toThreadSx(const int sx) const;
	int toThreadSy(const int sy) const;
	int toThreadSz(const int sz) const;

	GxImmediateIndex getImmediateIndex(const char * name) const;

	void setImmediate(const char * name, float x);	
	void setImmediate(const char * name, float x, float y);
	void setImmediate(const char * name, float x, float y, float z);
	void setImmediate(const char * name, float x, float y, float z, float w);
	void setImmediate(GxImmediateIndex index, float x);
	void setImmediate(GxImmediateIndex index, float x, float y);
	void setImmediate(GxImmediateIndex index, float x, float y, float z);
	void setImmediate(GxImmediateIndex index, float x, float y, float z, float w);
	void setImmediateMatrix4x4(const char * name, const float * matrix);
	void setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix);
	void setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureRw(const char * name, int unit, GxTextureId texture, GX_TEXTURE_FORMAT format, bool filtered, bool clamp = true);
	void setBuffer(const char * name, const ShaderBuffer & buffer);
	void setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer);
	void setBufferRw(const char * name, const ShaderBufferRw & buffer);
	void setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer);

	void dispatch(const int dispatchSx, const int dispatchSy, const int dispatchSz);

	const ComputeShaderCacheElem & getCacheElem() const { return *m_shader; }
	void reload();
	
	virtual uint32_t getOpenglProgram() const override final { return getProgram(); }
};

#endif

//

class ShaderBuffer
{
#if ENABLE_METAL
	mutable class DynamicBufferPool * m_bufferPool;
	mutable class DynamicBufferPoolElem * m_bufferPoolElem;
	mutable bool m_bufferPoolElemIsUsed; // if true, setData is required to allocate a new buffer
	uint32_t m_bufferSize;
#endif

#if ENABLE_OPENGL
	uint32_t m_bufferId;
	uint32_t m_bufferSize;
#endif
	
public:
	ShaderBuffer();
	~ShaderBuffer();
	
	ShaderBuffer(const ShaderBuffer & other) = delete; // copying shader buffers by value is not allowed

	void alloc(int numBytes);
	void free();
	
	void setData(const void * bytes, int numBytes);
	
#if ENABLE_OPENGL
	uint32_t getOpenglBufferId() const { return m_bufferId; }
	int getBufferSize() const { return m_bufferSize; }
#endif
#if ENABLE_METAL
	void markMetalBufferIsUsed() const;
	void * getMetalBuffer() const;
	int getBufferSize() const { return m_bufferSize; }
#endif
};

//

class ShaderBufferRw
{
	uint32_t m_bufferId;

public:
	ShaderBufferRw();
	~ShaderBufferRw();

	void setDataRaw(const void * bytes, int numBytes);

	template <typename T>
	void setData(const T * elements, int numElements)
	{
		setDataRaw(elements, sizeof(T) * numElements);
	}
	
#if ENABLE_OPENGL
	uint32_t getOpenglBufferId() const { return m_bufferId; }
#endif
#if ENABLE_METAL
	uint32_t getMetalBufferId() const { return m_bufferId; }
#endif
};

class Color
{
public:
	Color();
	explicit Color(int r, int g, int b, int a = 255);
	explicit Color(float r, float g, float b, float a = 1.f);
	
	static Color fromHex(const char * str, bool * success = nullptr);
	static Color fromHSL(float hue, float sat, float lum);
	
	void toHSL(float & hue, float & sat, float & lum) const;
	Color interp(const Color & other, float t) const;
	Color hueShift(float shift) const;
	Color desaturate(float amount) const;
	Color invertRGB(float amount) const;

	uint32_t toRGBA() const;
	std::string toHexString(const bool withAlpha) const;

	void set(const float r, const float g, const float b, const float a);
	Color addRGB(const Color & other) const;
	Color mulRGBA(const Color & other) const;
	Color mulRGB(float t) const;
	
	float r, g, b, a;
};

//

class DictionaryStorage;

class Dictionary
{
	DictionaryStorage * m_storage;
	
public:
	Dictionary();
	Dictionary(const Dictionary & other);
	~Dictionary();
	
	bool load(const char * filename);
	bool save(const char * filename);
	bool parse(const std::string & line, bool clear = true); // line = key1:value1 key2:value2 key3:value3 ..
	
	bool contains(const char * name) const;
	
	void setString(const char * name, const char * value);
	void setInt(const char * name, int value);
    void setInt64(const char * name, int64_t value);
	void setBool(const char * name, bool value);
	void setFloat(const char * name, float value);
	void setPtr(const char * name, void * value);
	
	std::string getString(const char * name, const char * _default) const;
	int getInt(const char * name, int _default) const;
	int64_t getInt64(const char * name, int64_t _default) const;
	bool getBool(const char * name, bool _default) const;
	float getFloat(const char * name, float _default) const;
	void * getPtr(const char * name, void * _default) const;
	template <typename T> T * getPtrType(const char * name, T * _default) const { return (T*)getPtr(name, _default); }
	
	std::string & operator[](const char * name);
	
	Dictionary & operator=(const Dictionary & other);
};

//

GxTextureId getTexture(const char * filename);
const GxTexture3d & getTexture3d(const char * filename);

//

class Sprite
{
public:
	friend class Framework;
	
	Sprite(const char * filename, float pivotX = 0.f, float pivotY = 0.f, const char * spritesheet = 0, bool autoUpdate = false, bool hasSpriteSheet = true);
	~Sprite();
	
	void reload();

	void update(float dt); // only needs to be called if autoUpdate is false!
	void draw();
	void drawEx(float x, float y, float angle = 0.f, float scaleX = 1.f, float scaleY = FLT_MAX, bool pixelpos = true, TEXTURE_FILTER filter = FILTER_POINT);
	
	// animation
	void startAnim(const char * anim, int frame = 0);
	void stopAnim();
	void pauseAnim() { animIsPaused = true; }
	void resumeAnim() { animIsPaused = false; }
	const std::string & getAnim() const;
	void setAnimFrame(int frame);
	int getAnimFrame() const;
	std::vector<std::string> getAnimList() const;
	
	// drawing
	float pivotX;
	float pivotY;
	float x;
	float y;
	float angle;
	bool separateScale;
	float scale;
	float scaleX;
	float scaleY;
	bool flipX;
	bool flipY;
	bool pixelpos;
	TEXTURE_FILTER filter;
	
	int getWidth() const;
	int getHeight() const;
	GxTextureId getTexture() const;
	
	// animation
	float animSpeed;
	bool animIsActive;
	bool animIsPaused;
	ActionHandler animActionHandler;
	void * animActionHandlerObj;
	
private:
	// book keeping
	Sprite * m_prev;
	Sprite * m_next;

	// drawing
	class TextureCacheElem * m_texture;
	
	// animation
	class AnimCacheElem * m_anim;
	int m_animVersion;
	std::string m_animSegmentName;
	void * m_animSegment;
	bool m_isAnimStarted;
	float m_animFramef;
	int m_animFrame;

	bool m_autoUpdate;

	void updateAnimationSegment();
	void updateAnimation(float timeStep);
	void processAnimationFrameChange(int frame1, int frame2);
	void processAnimationTriggersForFrame(int frame, int event);
	int calculateLoopedFrameIndex(int frame) const;
};

//

enum ModelDrawFlags
{
	DrawMesh               = 0x0001,
	DrawBones              = 0x0002,
	DrawNormals            = 0x0004,
	DrawPoseMatrices       = 0x0008,
	DrawColorNormals       = 0x0010,
	DrawColorBlendIndices  = 0x0020,
	DrawColorBlendWeights  = 0x0040,
	DrawColorTexCoords     = 0x0080,
	DrawBoundingBox        = 0x0100,
	DrawUnSkinned          = 0x0200,
	DrawHardSkinned        = 0x0400
};

class Model
{
public:
	friend class Framework;
	
	float x;
	float y;
	float z;
	Vec3 axis;
	float angle;
	float scale;
	Shader * overrideShader;
	
	bool animIsActive;
	bool animIsPaused;
	float animTime;
	int animLoop;
	int animLoopCount;
	float animSpeed;
	Vec3 animRootMotion;
	bool animRootMotionEnabled;
	
	float drawNormalsScale = 1.f;
	
	Model(const char * filename, bool autoUpdate = false);
	Model(class ModelCacheElem & cacheElem, bool autoUpdate);
	~Model();
	
	void startAnim(const char * name, int loop = 1);
	void stopAnim();
	void pauseAnim() { animIsPaused = true; }
	void resumeAnim() { animIsPaused = false; }
	std::vector<std::string> getAnimList() const;
	const char * getAnimName() const;
	
	void tick(const float dt);
	
	void draw(const int drawFlags = DrawMesh) const;
	void drawEx(Vec3Arg position, Vec3Arg axis, const float angle = 0.f, const float scale = 1.f, const int drawFlags = DrawMesh) const;
	void drawEx(const Mat4x4 & matrix, const int drawFlags = DrawMesh) const;

	void calculateTransform(Mat4x4 & matrix) const;
	static void calculateTransform(Vec3Arg position, Vec3Arg axis, const float angle, const float scale, Mat4x4 & matrix);
	
	int calculateBoneMatrices(const Mat4x4 & matrix, Mat4x4 * localMatrices, Mat4x4 * worldMatrices, Mat4x4 * globalMatrices, const int numMatrices) const;
	
	int softBlend(const Mat4x4 & matrix, Mat4x4 * localMatrices, Mat4x4 * worldMatrices, Mat4x4 * globalMatrices, const int numMatrices,
		const bool wantsPosition,
		float * __restrict positionX,
		float * __restrict positionY,
		float * __restrict positionZ,
		const bool wantsNormal,
		float * __restrict normalX,
		float * __restrict normalY,
		float * __restrict normalZ,
		const int numVertices) const;
	
	bool calculateAABB(Vec3 & min, Vec3 & max, const bool applyAnimation) const;
	
private:
	void ctor();
	void ctorEnd();
	
	// book keeping
	Model * m_prev;
	Model * m_next;
	
	// drawing
	class ModelCacheElem * m_model;
	
	// animation
	std::string m_animSegmentName;
	void * m_currentAnim;
	void * m_animSegment;
	bool m_isAnimStarted;
	AnimModel::BoneTransform * m_boneTransforms;
	
	bool m_autoUpdate;

	void updateAnimationSegment();
	void updateAnimation(float timeStep);
};

//

#pragma pack(push)
#pragma pack(1)

struct SpriterState
{
	SpriterState();

	float x;
	float y;
	float angle;
	float scale;
	float scaleX;
	float scaleY;
	bool flipX;
	bool flipY;

	bool animIsActive;
	int animIndex;
	float animTime;
	float animSpeed;
	bool animLoop;

	int characterMap;

	bool startAnim(const Spriter & spriter, const char * name);
	bool startAnim(const Spriter & spriter, int index);
	void stopAnim(const Spriter & spriter);
	bool updateAnim(const Spriter & spriter, float dt);

	void setCharacterMap(const Spriter & spriter, int index);
	void setCharacterMap(const Spriter & spriter, const char * name);
};

#pragma pack(pop)

class Spriter
{
public:
	friend struct SpriterState;

	Spriter(const char * filename);

	void getDrawableListAtTime(const SpriterState & state, spriter::Drawable * drawables, int & numDrawables);

	void draw(const SpriterState & state);
	void draw(const SpriterState & state, const spriter::Drawable * drawables, int numDrawables);

	int getAnimCount() const;
	const char * getAnimName(const int animIndex) const;
	int getAnimIndexByName(const char * name) const;
	float getAnimLength(int animIndex) const;
	bool isAnimDoneAtTime(int animIndex, float time) const;

	bool getHitboxAtTime(int animIndex, const char * name, float time, Vec2 * points);

	bool hasCharacterMap(int index) const;

	spriter::Scene * getSpriterScene() const;

private:
	class SpriterCacheElem * m_spriter;
};


//

class Sound
{
public:
	Sound(const char * filename);
	
	void play(int volume = -1);
	void stop();
	void setVolume(int volume);
	
	static void stopAll();
	
private:
	class SoundCacheElem * m_sound;
	int m_playId;
	int m_volume;
	int m_speed;
};

//

class Music
{
	std::string m_filename;
	
public:
	Music(const char * filename);
	
	void play(bool loop = true);
	void stop();
	void setVolume(int volume);
};

//

class Font
{
public:
	Font(const char * filename);
	
	class FontCacheElem * getFont()
	{
		return m_font;
	}
	
	class MsdfFontCacheElem * getFontMSDF()
	{
		return m_fontMSDF;
	}
	
	bool saveCache(const char * filename = nullptr) const;
	bool loadCache(const char * filename = nullptr);
	
private:
	class FontCacheElem * m_font;
	
	class MsdfFontCacheElem * m_fontMSDF;
};

//

class Mouse
{
public:
	int x, y;
	int dx, dy;
	
	int scrollY;
	
	void * mouseCaptureObject;
	
	Mouse()
	{
		x = y = 0;
		dx = dy = 0;
		scrollY = 0;
		mouseCaptureObject = 0;
	}
	
	bool isDown(BUTTON button) const;
	bool wentDown(BUTTON button) const;
	bool wentUp(BUTTON button) const;
	void showCursor(bool enabled);
	void setRelative(bool isRelative);
	
	bool isIdle() const; // return true when there is no mouse movement and there are no buttons being pressed
	
	void capture(void * object);
	void release(void * object);
};

class Keyboard
{
public:
	std::vector<SDL_Event> events;
	
	bool isDown(int key) const;
	bool wentDown(int key, bool allowRepeat = false) const;
	bool wentUp(int key) const;
	bool keyRepeat(int key) const;

	bool isIdle() const; // return true when there are no buttons being pressed
};

class Gamepad
{
	friend class Framework;

	bool m_isDown[GAMEPAD_MAX];
	bool m_wentDown[GAMEPAD_MAX];
	bool m_wentUp[GAMEPAD_MAX];
	float m_analog[2][ANALOG_MAX];
	float m_vibrationDuration;
	float m_vibrationStrength;
#ifdef __WIN32__
	float m_lastVibrationStrength;
#endif
	char name[64];

public:
	Gamepad();
	
	bool isConnected;
	bool isDown(GAMEPAD button) const;
	bool wentDown(GAMEPAD button) const;
	bool wentUp(GAMEPAD button) const;
	float getAnalog(int stick, ANALOG analog, float scale = 1.f) const;

	void vibrate(float duration, float strength);
	
	const char * getName() const;
};

class Camera3d
{
	double mouseDx;
	double mouseDy;
	
public:
	Vec3 position;
	
	float yaw;
	float pitch;
	float roll;
	
	double mouseSmooth;
	float mouseRotationSpeed;
	float maxForwardSpeed;
	float maxStrafeSpeed;
	float maxUpSpeed;
	
	int gamepadIndex;
	
	Camera3d();
	
	void tick(float dt, bool enableInput);
	
	Mat4x4 getWorldMatrix() const;
	Mat4x4 getViewMatrix() const;
	
	void pushViewMatrix() const;
	void popViewMatrix() const;
};

void clearCaches(int caches);

// drawing

void setTransform(TRANSFORM transform);
TRANSFORM getTransform();
void applyTransform();
void applyTransformWithViewportSize(const int sx, const int sy);
void setTransform2d(const Mat4x4 & transform);
void setTransform3d(const Mat4x4 & transform);
void pushTransform();
void popTransform();
void pushScroll(const int scrollX, const int scrollY);
void popScroll();

void projectScreen2d();
void projectPerspective3d(const float fov, const float nearZ, const float farZ);
void viewLookat3d(const float originX, const float originY, const float originZ, const float targetX, const float targetY, const float targetZ, const float upX, const float upY, const float upZ);
Vec4 transformToView(const Vec4 & v);
Vec2 transformToScreen(const Mat4x4 & modelViewProjection, const Vec3 & v, float & w);
Vec2 transformToScreen(const Vec3 & v, float & w);

void pushSurface(Surface * surface, const bool clearSurface = false);
void popSurface();

void setDrawRect(int x, int y, int sx, int sy);
void clearDrawRect();

void setBlend(BLEND_MODE blendMode);
void pushBlend(BLEND_MODE blendMode);
void popBlend();

void setColorMode(COLOR_MODE colorMode);
void pushColorMode(COLOR_MODE colorMode);
void popColorMode();

void setColorPost(COLOR_POST colorPost);
void pushColorPost(COLOR_POST colorPost);
void popColorPost();

void setLineSmooth(bool enabled);
void pushLineSmooth(bool enabled);
void popLineSmooth();

void setWireframe(bool enabled);
void pushWireframe(bool enabled);
void popWireframe();

void setColorWriteMask(int r, int g, int b, int a);
void setColorWriteMaskAll();
void pushColorWriteMask(int r, int g, int b, int a);
void popColorWriteMask();

void setDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled = true);
void pushDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled = true);
void popDepthTest();

void pushDepthWrite(bool enabled);
void popDepthWrite();

void setDepthBias(float depthBias, float slopeScale);
void pushDepthBias(float depthBias, float slopeScale);
void popDepthBias();

void setAlphaToCoverage(bool enabled);
void pushAlphaToCoverage(bool enabled);
void popAlphaToCoverage();

struct StencilState
{
	GX_STENCIL_FUNC compareFunc = GX_STENCIL_FUNC_ALWAYS;
	uint8_t compareRef = 0x00;
	uint8_t compareMask = 0xff;
	
	GX_STENCIL_OP onStencilFail = GX_STENCIL_OP_KEEP;
	GX_STENCIL_OP onDepthFail = GX_STENCIL_OP_KEEP;
	GX_STENCIL_OP onDepthStencilPass = GX_STENCIL_OP_KEEP;
	
	uint8_t writeMask = 0xff;
};

void clearStencil(uint8_t value, uint32_t writeMask);

void setStencilTest(const StencilState & front, const StencilState & back);
void clearStencilTest();

class StencilSetter
{
public:
	StencilSetter();
	~StencilSetter();
	
	StencilSetter & op(
		GX_STENCIL_FACE face,
		GX_STENCIL_OP onStencilFail,
		GX_STENCIL_OP onDepthFail,
		GX_STENCIL_OP onDepthStencilPass);
	StencilSetter & writeMask(GX_STENCIL_FACE face, uint8_t mask);
	StencilSetter & comparison(GX_STENCIL_FACE face, GX_STENCIL_FUNC func, uint8_t ref, uint8_t mask);
	
	StencilSetter & op(
		GX_STENCIL_OP onStencilFail,
		GX_STENCIL_OP onDepthFail,
		GX_STENCIL_OP onDepthStencilPass);
	StencilSetter & writeMask(uint8_t mask);
	StencilSetter & comparison(GX_STENCIL_FUNC func, uint8_t ref, uint8_t mask);
};

inline StencilSetter setStencilTest() { return StencilSetter(); }

void setCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding);
void pushCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding);
void popCullMode();

void pushCullFlip();
void popCullFlip();
void updateCullFlip();

void setColor(const Color & color);
void setColor(const Color & color, float rgbMul);
void setColor(int r, int g, int b, int a = 255, int rgbMul = 255);
void setColorf(float r, float g, float b, float a = 1.f, float rgbMul = 1.f);
void setColorClamp(bool clamp);
void setAlpha(int a);
void setAlphaf(float a);
void setLumi(int l);
void setLumif(float l);
void pushColor();
void popColor();

void setFont(const Font & font);
void setFont(const char * font);
void pushFontMode(FONT_MODE fontMode);
void popFontMode();

void setShader(const ShaderBase & shader);
void clearShader();
void shaderSource(const char * filename, const char * text);
void pushShaderOutputs(const char * outputs);
void popShaderOutputs();

void drawPoint(float x, float y);
void drawLine(float x1, float y1, float x2, float y2);
void drawRect(float x1, float y1, float x2, float y2);
void drawRectLine(float x1, float y1, float x2, float y2);
void drawCircle(float x, float y, float radius, int numSegments);
void fillCircle(float x, float y, float radius, int numSegments);
void measureText(float size, float & sx, float & sy, const char * format, ...);
void beginTextBatch(Shader * overrideShader = nullptr);
void endTextBatch();
void drawText(float x, float y, float size, float alignX, float alignY, const char * format, ...);
void measureTextArea(float size, float maxSx, float & sx, float & sy, const char * format, ...);
void drawTextArea(float x, float y, float sx, float size, const char * format, ...);
void drawTextArea(float x, float y, float sx, float sy, float size, float alignX, float alignY, const char * format, ...);

void drawLine3d(int axis = 0);
void drawLine3d(Vec3Arg from, Vec3Arg to);
void drawRect3d(int axis1 = 0, int axis2 = 1);
void drawGrid3d(int resolution1, int resolution2, int axis1 = 0, int axis2 = 1);
void drawGrid3dLine(int resolution1, int resolution2, int axis1 = 0, int axis2 = 1, bool optimized = false);
void lineCube(Vec3Arg position, Vec3Arg size);
void fillCube(Vec3Arg position, Vec3Arg size);
void fillCylinder(Vec3Arg position, const float radius, const float height, const int resolution, const float angleOffset = 0.f, const bool smoothNormals = false);
void fillHexagon(Vec3Arg position, const float radius, const float height, const float angleOffset = 0.f, const bool smoothNormals = false);
void beginCubeBatch();
void endCubeBatch();

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromRGB8(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromR8(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromRGBA32F(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromRGB32F(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromRG32F(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromR16(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromR32F(const void * source, int sx, int sy, bool filter, bool clamp);

GxTextureId copyTexture(const GxTextureId source);

void freeTexture(GxTextureId & textureId);

void debugDrawText(float x, float y, int size, float alignX, float alignY, const char * format, ...);
void debugDrawText(const Vec3 & position, int size, float alignX, float alignY, const char * format, ...);

// OpenGL legacy mode drawing

#if !ENABLE_OPENGL && !ENABLE_METAL

SDL_Surface * getWindowSurface();

static inline void gxMatrixMode(GX_MATRIX mode) { }
static inline GX_MATRIX gxGetMatrixMode() { return GX_MODELVIEW; }
static inline int gxGetMatrixParity() { return 1; }
static inline void gxPopMatrix() { }
static inline void gxPushMatrix() { }
static inline void gxLoadIdentity() { }
static inline void gxLoadMatrixf(const float * m) { }
static inline void gxGetMatrixf(GX_MATRIX mode, float * m) { }
static inline void gxSetMatrixf(GX_MATRIX mode, const float * m) { }
static inline void gxMultMatrixf(const float * m) { }
static inline void gxTranslatef(float x, float y, float z) { }
static inline void gxRotatef(float angle, float x, float y, float z) { }
static inline void gxScalef(float x, float y, float z) { }
static inline void gxValidateMatrices() { }

static inline void gxInitialize() { }
static inline void gxShutdown() { }
static inline void gxBegin(GX_PRIMITIVE_TYPE primitiveType) { }
static inline void gxEnd() { }
static inline void gxEmitVertices(GX_PRIMITIVE_TYPE primitiveType, int numVertices) { }
static inline void gxColor3f(float r, float g, float b) { }
static inline void gxColor3fv(const float * v) { }
static inline void gxColor4f(float r, float g, float b, float a) { }
static inline void gxColor4fv(const float * rgba) { }
static inline void gxColor3ub(int r, int g, int b) { }
static inline void gxColor4ub(int r, int g, int b, int a) { }
static inline void gxTexCoord2f(float u, float v) { }
static inline void gxNormal3f(float x, float y, float z) { }
static inline void gxNormal3fv(const float  * v) { }
static inline void gxVertex2f(float x, float y) { }
static inline void gxVertex3f(float x, float y, float z) { }
static inline void gxVertex3fv(const float * v) { }
static inline void gxVertex4f(float x, float y, float z, float w) { }
static inline void gxVertex4fv(const float * v) { }
static inline void gxSetTexture(GxTextureId texture) { }
static inline void gxSetTextureSampler(GX_SAMPLE_FILTER filter, bool clamp) { }

static inline void gxGetTextureSize(GxTextureId texture, int & width, int & height) { width = 0; height = 0; }
static inline GX_TEXTURE_FORMAT gxGetTextureFormat(GxTextureId texture) { return GX_UNKNOWN_FORMAT; }

#elif (ENABLE_OPENGL && !USE_LEGACY_OPENGL) || ENABLE_METAL

void gxMatrixMode(GX_MATRIX mode);
GX_MATRIX gxGetMatrixMode();
int gxGetMatrixParity();
void gxPopMatrix();
void gxPushMatrix();
void gxLoadIdentity();
void gxLoadMatrixf(const float * m);
void gxGetMatrixf(GX_MATRIX mode, float * m);
void gxSetMatrixf(GX_MATRIX mode, const float * m);
void gxMultMatrixf(const float * m);
void gxTranslatef(float x, float y, float z);
void gxRotatef(float angle, float x, float y, float z);
void gxScalef(float x, float y, float z);
void gxValidateMatrices();

void gxInitialize();
void gxShutdown();
void gxBegin(GX_PRIMITIVE_TYPE primitiveType);
void gxEnd();
void gxEmitVertices(GX_PRIMITIVE_TYPE primitiveType, int numVertices);
void gxColor3f(float r, float g, float b);
void gxColor3fv(const float * v);
void gxColor4f(float r, float g, float b, float a);
void gxColor4fv(const float * rgba);
void gxColor3ub(int r, int g, int b);
void gxColor4ub(int r, int g, int b, int a);
void gxTexCoord2f(float u, float v);
void gxNormal3f(float x, float y, float z);
void gxNormal3fv(const float * v);
void gxVertex2f(float x, float y);
void gxVertex3f(float x, float y, float z);
void gxVertex3fv(const float * v);
void gxVertex4f(float x, float y, float z, float w);
void gxVertex4fv(const float * v);
void gxSetTexture(GxTextureId texture);
void gxSetTextureSampler(GX_SAMPLE_FILTER filter, bool clamp);

void gxGetTextureSize(GxTextureId texture, int & width, int & height);
GX_TEXTURE_FORMAT gxGetTextureFormat(GxTextureId texture);

#else

#define gxMatrixMode glMatrixMode
GX_MATRIX gxGetMatrixMode();
int gxGetMatrixParity();
#define gxPopMatrix glPopMatrix
#define gxPushMatrix glPushMatrix
#define gxLoadIdentity glLoadIdentity
#define gxLoadMatrixf glLoadMatrixf
void gxGetMatrixf(GX_MATRIX mode, float * m);
void gxSetMatrixf(GX_MATRIX mode, const float * m);
#define gxMultMatrixf glMultMatrixf
#define gxTranslatef glTranslatef
#define gxRotatef glRotatef
#define gxScalef glScalef
static inline void gxValidateMatrices() { }

void gxInitialize();
void gxShutdown();
void gxBegin(GLenum type);
void gxEnd();
void gxEmitVertices(GLenum type, int numVertices);
void gxColor3f(float r, float g, float b);
void gxColor3fv(const float * v);
#define gxColor3f glColor3f
#define gxColor3fv glColor3fv
#define gxColor4f glColor4f
#define gxColor4fv glColor4fv
#define gxColor3ub glColor3ub
#define gxColor4ub glColor4ub
#define gxTexCoord2f glTexCoord2f
#define gxNormal3f glNormal3f
#define gxNormal3fv glNormal3fv
#define gxVertex2f glVertex2f
#define gxVertex2fv glVertex2fv
#define gxVertex3f glVertex3f
#define gxVertex3fv glVertex3fv
#define gxVertex4f glVertex4f
#define gxVertex4fv glVertex4fv
void gxSetTexture(GxTextureId texture);
void gxSetTextureSampler(GX_SAMPLE_FILTER filter, bool clamp);

void gxGetTextureSize(GxTextureId texture, int & width, int & height);
GX_TEXTURE_FORMAT gxGetTextureFormat(GxTextureId texture);

#endif

#if FRAMEWORK_ENABLE_GL_ERROR_LOG
	void checkErrorGL_internal(const char * function, int line);
	#define checkErrorGL() checkErrorGL_internal(__FUNCTION__, __LINE__)
#else
	#define checkErrorGL() do { } while (false)
#endif

// utility

void setupPaths(const char * chibiResourcePaths);
void changeDirectory(const char * path);
std::string getDirectory();
std::vector<std::string> listFiles(const char * path, bool recurse);
void showErrorMessage(const char * caption, const char * format, ...);

// builtin shaders

void makeGaussianKernel(int kernelSize, float * kernel, float sigma = 1.632f);
void makeGaussianKernel(int kernelSize, ShaderBuffer & kernel, float sigma = 1.632f);

void setShader_GaussianBlurH(const GxTextureId source, const int kernelSize, const float radius);
void setShader_GaussianBlurV(const GxTextureId source, const int kernelSize, const float radius);
void setShader_ThresholdLumi(const GxTextureId source, const float lumi, const Color & failColor, const Color & passColor, const float opacity);
void setShader_ThresholdLumiFail(const GxTextureId source, const float lumi, const Color & failColor, const float opacity);
void setShader_ThresholdLumiPass(const GxTextureId source, const float lumi, const Color & passColor, const float opacity);
void setShader_ThresholdValue(const GxTextureId source, const Color & value, const Color & failColor, const Color & passColor, const float opacity);
void setShader_GrayscaleLumi(const GxTextureId source, const float opacity);
void setShader_GrayscaleWeights(const GxTextureId source, const Vec3 & weights, const float opacity);
void setShader_Colorize(const GxTextureId source, const float hue, const float opacity);
void setShader_HueShift(const GxTextureId source, const float hue, const float opacity);
void setShader_ColorMultiply(const GxTextureId source, const Color & color, const float opacity);
void setShader_ColorTemperature(const GxTextureId source, const float temperature, const float opacity); // temperature in Kelvin. sensible range is 1000 to 12.000
void setShader_TextureSwizzle(const GxTextureId source, const int r, const int g, const int b, const int a);

// high quality rendering

enum HQ_TYPE
{
	HQ_LINES,
	HQ_FILLED_TRIANGLES,
	HQ_FILLED_CIRCLES,
	HQ_FILLED_RECTS,
	HQ_FILLED_ROUNDED_RECTS,
	HQ_STROKED_TRIANGLES,
	HQ_STROKED_CIRCLES,
	HQ_STROKED_RECTS,
	HQ_STROKED_ROUNDED_RECTS
};

enum GRADIENT_TYPE
{
	GRADIENT_NONE,
	GRADIENT_LINEAR,
	GRADIENT_RADIAL
};

void hqBegin(HQ_TYPE type, bool useScreenSize = false);
void hqBeginCustom(HQ_TYPE type, Shader & shader, bool useScreenSize = false);
void hqEnd();

void hqSetGradient(GRADIENT_TYPE gradientType, const Mat4x4 & matrix, const Color & color1, const Color & color2, const COLOR_MODE colorMode, const float bias = 0.f, const float scale = 1.f);
void hqClearGradient();

void hqSetTexture(const GxTextureId texture, const Mat4x4 & matrix);
void hqSetTextureScreen(const GxTextureId texture, float x1, float y1, float x2, float y2);
void hqClearTexture();

void hqLine(float x1, float y1, float strokeSize1, float x2, float y2, float strokeSize2);

void hqFillTriangle(float x1, float y1, float x2, float y2, float x3, float y3);
void hqFillCircle(float x, float y, float radius);
void hqFillRect(float x1, float y1, float x2, float y2);
void hqFillRoundedRect(float x1, float y1, float x2, float y2, float radius);

void hqStrokeTriangle(float x1, float y1, float x2, float y2, float x3, float y3, float stroke);
void hqStrokeCircle(float x, float y, float radius, float stroke);
void hqStrokeRect(float x1, float y1, float x2, float y2, float stroke);
void hqStrokeRoundedRect(float x1, float y1, float x2, float y2, float radius, float stroke);

// math

template <typename T>
inline T clamp(T v, T vmin, T vmax)
{
	return v < vmin ? vmin : v > vmax ? vmax : v;
}

template <typename T>
inline T saturate(T v)
{
	return clamp<T>(v, (T)0, (T)1);
}

template <typename T>
inline T lerp(T v1, T v2, float t)
{
	return v1 * (1.f - t) + v2 * t;
}

template <typename T>
inline T lerp(T v1, T v2, double t)
{
	return v1 * (1.f - t) + v2 * t;
}

template <typename T>
inline float inverseLerp(T v1, T v2, T v)
{
	return float(v - v1) / float(v2 - v1);
}

template <typename T>
inline T sine(T min, T max, float t)
{
	t = t * float(M_PI) / 180.f;
	return static_cast<T>(min + (max - min) * (sinf(t) + 1.f) / 2.f);
}

template <typename T>
inline T cosine(T min, T max, float t)
{
	t = t * float(M_PI) / 180.f;
	return static_cast<T>(min + (max - min) * (cosf(t) + 1.f) / 2.f);
}

template <typename T>
inline T getAngle(T dx, T dy)
{
	return static_cast<T>(atan2f(dy, dx) / M_PI * 180.f + 90.f);
}

template <typename T>
inline T random(T min, T max)
{
	return static_cast<T>(min + (max - min) * ((rand() % 1000) / 999.f));
}

template <>
inline int random(int in_min, int in_max)
{
	int min = in_min < in_max ? in_min : in_max;
	int max = in_max > in_min ? in_max : in_min;
	return min + (rand() % (max - min + 1));
}

// logging

#if ENABLE_LOGGING && ENABLE_LOGGING_DBG
	void logDebug(const char * format, ...);
#else
	#define logDebug(...) do { } while (false)
#endif

#if ENABLE_LOGGING
	void logInfo(const char * format, ...);
	void logWarning(const char * format, ...);
	void logError(const char * format, ...);
#else
	#define logInfo(...) do { } while (false)
	#define logWarning(...) do { } while (false)
	#define logError(...) do { } while (false)
#endif

// profiling

#if ENABLE_PROFILING
	#include "remotery.h"
	#define cpuTimingSetThreadName(name) rmt_SetCurrentThreadName(name)
	#define cpuTimingBlock(name) rmt_ScopedCPUSample(name)
	#define cpuTimingBegin(name) rmt_BeginCPUSample(name)
	#define cpuTimingEnd() rmt_EndCPUSample()
	#define gpuTimingBlock(name) rmt_ScopedOpenGLSample(name)
	#define gpuTimingBegin(name) rmt_BeginOpenGLSample(name)
	#define gpuTimingEnd() rmt_EndOpenGLSample()
#else
	#define cpuTimingSetThreadName(name) do { } while (false)
	#define cpuTimingBlock(name) do { } while (false)
	#define cpuTimingBegin(name) do { } while (false)
	#define cpuTimingEnd() do { } while (false)
	#define gpuTimingBlock(name) do { } while (false)
	#define gpuTimingBegin(name) do { } while (false)
	#define gpuTimingEnd() do { } while (false)
#endif

// constants

extern Color colorBlackTranslucent;
extern Color colorBlack;
extern Color colorWhite;
extern Color colorRed;
extern Color colorGreen;
extern Color colorBlue;
extern Color colorYellow;
