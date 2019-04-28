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

#pragma once

// todo : remove SDL2 header file include ? add a new include for including low-level framework functions
//        which should hopefully rarely be needed
// for now we seem to depend mostly on: SDL_event, SDL_mutex, SDL_thread and SDL_timer

#if USE_LEGACY_OPENGL
	#include <GL/glew.h>
	#include <SDL2/SDL_opengl.h>
#endif

#include "Debugging.h"
#include "gx_texture.h"
#include "Mat4x4.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"
#include <float.h> // FLT_MAX (sprite draw)
#include <SDL2/SDL.h> // SDL_Event
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

#if defined(LINUX)
	#define ENABLE_PROFILING 0
#else
	#define ENABLE_PROFILING 1
#endif

#if !defined(ENABLE_OPENGL)
	#define ENABLE_OPENGL 1
#endif

/*
#ifdef DEBUG
	#define ENABLE_OPENGL 1
#else
	#define ENABLE_OPENGL 0
#endif
*/

#if !defined(USE_LEGACY_OPENGL)
	#define USE_LEGACY_OPENGL 0
#endif

#if defined(MACOS)
    #define OPENGL_VERSION 410
#else
    #define OPENGL_VERSION 430
#endif

#if !defined(ENABLE_HQ_PRIMITIVES)
	#define ENABLE_HQ_PRIMITIVES 1
#endif

#if !defined(ENABLE_UTF8_SUPPORT)
	#define ENABLE_UTF8_SUPPORT 1
#endif

static const int MAX_GAMEPAD = 4;

// enumerations

enum BLEND_MODE // setBlend
{
	BLEND_OPAQUE,
	BLEND_ALPHA,
	BLEND_PREMULTIPLIED_ALPHA,
	BLEND_PREMULTIPLIED_ALPHA_DRAW,
	BLEND_ADD,
	BLEND_ADD_OPAQUE,
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

enum COLOR_POST
{
	POST_NONE,
	POST_RGB_MIX_ALPHA_TO_ZERO,
	POST_RGB_MIX_ALPHA_TO_ONE,
	POST_PREMULTIPLY_RGB_WITH_ALPHA = POST_RGB_MIX_ALPHA_TO_ZERO,
	POST_BLEND_MUL_FIX = POST_RGB_MIX_ALPHA_TO_ONE,
	POST_SET_ALPHA_TO_ONE,
	POST_RGB_TO_LUMI
};

enum DEPTH_TEST
{
	DEPTH_EQUAL,
	DEPTH_LESS,
	DEPTH_LEQUAL,
	DEPTH_GREATER,
	DEPTH_GEQUAL,
	DEPTH_ALWAYS
};

enum CULL_MODE
{
	CULL_NONE,
	CULL_FRONT,
	CULL_BACK
};

enum CULL_WINDING
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
	INIT_ERROR_MIDI,
	INIT_ERROR_FREETYPE
};

// forward declations

class Color;
class Dictionary;
class Font;
class Framework;
class Gamepad;
class Keyboard;
class Midi;
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

// globals

extern Framework framework;
extern Mouse mouse;
extern Keyboard keyboard;
extern Gamepad gamepad[MAX_GAMEPAD];
extern Midi midi;

// event handlers

typedef void (*ActionHandler)(const std::string & action, const Dictionary & args);
typedef void (*FillCachesCallback)(float filePercentage);
typedef void (*FillCachesUnknownResourceCallback)(const char * filename);
typedef void (*RealTimeEditCallback)(const std::string & filename);
typedef void (*InitErrorHandler)(INIT_ERROR error);

//

typedef int32_t GxImmediateIndex;
typedef uint32_t GxShaderId;
typedef uint32_t GxShaderBufferId;

#if USE_LEGACY_OPENGL

// these must match the OpenGL definition for things to work when mapping GX calls to legacy OpenGL

enum GX_PRIMITIVE_TYPE
{
	GX_INVALID_PRIM = -1,
	GX_POINTS = GL_POINTS,
	GX_LINES = GL_LINES,
	GX_LINE_LOOP = GL_LINE_LOOP,
	GX_LINE_STRIP = GL_LINE_STRIP,
	GX_TRIANGLES = GL_TRIANGLES,
	GX_TRIANGLE_FAN = GL_TRIANGLE_FAN,
	GX_TRIANGLE_STRIP = GL_TRIANGLE_STRIP,
	GX_QUADS = GL_QUADS
};

enum GX_MATRIX
{
	GX_MODELVIEW = GL_MODELVIEW,
	GX_PROJECTION = GL_PROJECTION
};

enum GX_SAMPLE_FILTER
{
	GX_SAMPLE_NEAREST = GL_NEAREST,
	GX_SAMPLE_LINEAR = GL_LINEAR
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
	GX_SAMPLE_LINEAR
};

#endif

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
	
	Window & getMainWindow();
	Window & getCurrentWindow();
	void setFullscreen(bool fullscreen);
	
	void getCurrentViewportSize(int & sx, int & sy) const;

	void beginDraw(int r, int g, int b, int a, float depth = 1.f);
	void endDraw();
	
	void beginScreenshot(int r, int g, int b, int a, int scale);
	void endScreenshot(const char * name, int index = -1, bool omitAlpha = true);
	void screenshot(const char * name, int index = -1, bool omitAlpha = true);

	void registerShaderSource(const char * name, const char * text);
	void unregisterShaderSource(const char * name);
	bool tryGetShaderSource(const char * name, const char *& text) const;
	
	bool fileHasChanged(const char * filename) const;

	void blinkTaskbarIcon(int count);

	bool quitRequested;
	float time;
	float timeStep;
	
	bool waitForEvents;
	bool fullscreen;
	bool useClosestDisplayMode;
	bool exclusiveFullscreen;
	int msaaLevel;
	bool basicOpenGL;
	bool enableDepthBuffer;
	bool enableDrawTiming;
	bool enableProfiling;
	bool allowHighDpi;
	int minification;
	bool enableMidi;
	int midiDeviceIndex;
	bool reloadCachesOnActivate;
	bool cacheResourceData;
	bool enableRealTimeEditing;
	bool filedrop;
	int numSoundSources;
	int windowX;
	int windowY;
	bool windowBorder;
	std::string windowTitle;
	std::string windowIcon;
	int windowSx;
	int windowSy;
	bool windowIsActive;
	ActionHandler actionHandler;
	FillCachesCallback fillCachesCallback;
	FillCachesUnknownResourceCallback fillCachesUnknownResourceCallback;
	RealTimeEditCallback realTimeEditCallback;
	InitErrorHandler initErrorHandler;
	
	std::vector<SDL_Event> events;
	std::vector<std::string> changedFiles;
	
private:
	uint32_t m_lastTick;
	
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

//

class Window
{
private:
	Window(SDL_Window * window);
	
public:
	friend class Framework;
	
	Window(const char * title, const int sx, const int sy, const bool resizable = false);
	~Window();
	
	void setPosition(const int x, const int y);
	void setPositionCentered();
	void setSize(const int sx, const int sy);
	void setFullscreen(const bool fullscreen);
	
	void show();
	void hide();
	
	bool isHidden() const;
	bool hasFocus() const;
	
	void raise();
	
	void getPosition(int & x, int & y) const;
	int getWidth() const;
	int getHeight() const;
	
	bool getQuitRequested() const;
	
	SDL_Window * getWindow() const;
	class WindowData * getWindowData() const;
	
private:
	// book keeping
	Window * m_prev;
	Window * m_next;
	
	// SDL window
	SDL_Window * m_window;
	
	// keyboard and mouse data
	class WindowData * m_windowData;
};

void pushWindow(Window & window);
void popWindow();

//

enum SURFACE_FORMAT
{
	SURFACE_RGBA8,
	SURFACE_RGBA16F,
	SURFACE_RGBA32F,
	SURFACE_R8,
	SURFACE_R16F,
	SURFACE_R32F,
	SURFACE_RG16F,
	SURFACE_RG32F
};

enum DEPTH_FORMAT
{
	DEPTH_FLOAT16,
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
	int m_size[2];
	int m_backingScale; // backing scale puts a multiplier on the physical size (in pixels) of the surface. it's like MSAA, but fully super-sampled. it's used t orender to retina screens, where the 'resolve' operation just copies pixels 1:1, where a resolve onto a non-retina screen would downsample the surface instead
	
	int m_bufferId;
	SURFACE_FORMAT m_format;
	bool m_colorIsDoubleBuffered;
	bool m_depthIsDoubleBuffered;
	uint32_t m_buffer[2];
	GxTextureId m_colorTexture[2];
	GxTextureId m_depthTexture[2];
	
	void construct();
	void destruct();
	
public:
	Surface();
	explicit Surface(int sx, int sy, bool highPrecision, bool withDepthBuffer = false, bool doubleBuffered = true);
	explicit Surface(int sx, int sy, bool withDepthBuffer, bool doubleBuffered, SURFACE_FORMAT format);
	~Surface();
	
	void swapBuffers();

	bool init(const SurfaceProperties & properties);
	bool init(int sx, int sy, SURFACE_FORMAT format, bool withDepthBuffer, bool doubleBuffered);
	void setSwizzle(int r, int g, int b, int a);
	
	uint32_t getFramebuffer() const; // todo : make internally accessible only
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
};

void blitBackBufferToSurface(Surface * surface);

//

class ShaderBase
{
public:
	virtual ~ShaderBase() { }
	
	virtual bool isValid() const = 0;
	virtual GxShaderId getProgram() const = 0; // todo : make internally accessible only and add functionality on a per use-case basis
	virtual SHADER_TYPE getType() const = 0;
	virtual int getVersion() const = 0;
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const = 0;
};

//

class Shader : public ShaderBase
{
	class ShaderCacheElem * m_shader;

public:
	Shader();
	Shader(const char * filename);
	Shader(const char * name, const char * filenameVs, const char * filenamePs);
	virtual ~Shader();
	
	void load(const char * name, const char * filenameVs, const char * filenamePs);
	virtual bool isValid() const override;
	virtual GxShaderId getProgram() const override; // todo : make internally accessible only and add functionality on a per use-case basis
	virtual SHADER_TYPE getType() const override { return SHADER_VSPS; }
	virtual int getVersion() const override;
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const override;
	
	GxImmediateIndex getImmediate(const char * name);
	
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
	void setTextureUnit(const char * name, int unit); // bind <name> to GL_TEXTURE0 + unit
	void setTextureUnit(GxImmediateIndex index, int unit); // bind <name> to GL_TEXTURE0 + unit
	void setTexture(const char * name, int unit, GxTextureId texture);
	void setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureUniform(GxImmediateIndex index, int unit, GxTextureId texture);
	void setTextureArray(const char * name, int unit, GxTextureId texture);
	void setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureCube(const char * name, int unit, GxTextureId texture);
	void setBuffer(const char * name, const ShaderBuffer & buffer);
	void setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer);
	void setBufferRw(const char * name, const ShaderBufferRw & buffer);
	void setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer);

	const ShaderCacheElem & getCacheElem() const { return *m_shader; }
	void reload();
};

//

class ComputeShader : public ShaderBase
{
public:
	class ComputeShaderCacheElem * m_shader;

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
	virtual GxShaderId getProgram() const override; // todo : make internally accessible only and add functionality on a per use-case basis
	virtual SHADER_TYPE getType() const override { return SHADER_CS; }
	virtual int getVersion() const override;
	virtual bool getErrorMessages(std::vector<std::string> & errorMessages) const override;

	int getGroupSx() const;
	int getGroupSy() const;
	int getGroupSz() const;
	int toThreadSx(const int sx) const;
	int toThreadSy(const int sy) const;
	int toThreadSz(const int sz) const;

	GxImmediateIndex getImmediate(const char * name);

	void setImmediate(const char * name, float x);	
	void setImmediate(const char * name, float x, float y);
	void setImmediate(const char * name, float x, float y, float z);
	void setImmediate(const char * name, float x, float y, float z, float w);
	void setImmediate(GxImmediateIndex index, float x, float y);
	void setImmediate(GxImmediateIndex index, float x, float y, float z, float w);
	void setImmediateMatrix4x4(const char * name, const float * matrix);
	void setImmediateMatrix4x4(GxImmediateIndex index, const float * matrix);
	void setTexture(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureArray(const char * name, int unit, GxTextureId texture, bool filtered, bool clamp = true);
	void setTextureRw(const char * name, int unit, GxTextureId texture, uint32_t format, bool filtered, bool clamp = true); // todo : add enum for graphics api independent buffer formats
	void setBuffer(const char * name, const ShaderBuffer & buffer);
	void setBuffer(GxImmediateIndex index, const ShaderBuffer & buffer);
	void setBufferRw(const char * name, const ShaderBufferRw & buffer);
	void setBufferRw(GxImmediateIndex index, const ShaderBufferRw & buffer);

	void dispatch(const int dispatchSx, const int dispatchSy, const int dispatchSz);

	const ComputeShaderCacheElem & getCacheElem() const { return *m_shader; }
	void reload();
};

//

class ShaderBuffer
{
	GxShaderBufferId m_buffer;

public:
	ShaderBuffer();
	~ShaderBuffer();

	GxShaderBufferId getBuffer() const; // todo : make internally accessible only

	void setData(const void * bytes, int numBytes);
};

//

class ShaderBufferRw
{
	GxShaderBufferId m_buffer;

public:
	ShaderBufferRw();
	~ShaderBufferRw();

	GxShaderBufferId getBuffer() const; // todo : make internally accessible only

	void setDataRaw(const void * bytes, int numBytes);

	template <typename T>
	void setData(const T * elements, int numElements)
	{
		setDataRaw(elements, sizeof(T) * numElements);
	}
};

class Color
{
public:
	Color();
	explicit Color(int r, int g, int b, int a = 255);
	explicit Color(float r, float g, float b, float a = 1.f);
	
	static Color fromHex(const char * str);
	static Color fromHSL(float hue, float sat, float lum);
	void toHSL(float & hue, float & sat, float & lum) const;
	Color interp(const Color & other, float t) const;
	Color hueShift(float shift) const;

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
	
	void calculateAABB(Vec3 & min, Vec3 & max, const bool applyAnimation) const;
	
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

class Path2d
{
	enum ELEM_TYPE
	{
		ELEM_LINE,
		ELEM_CURVE,
		ELEM_ARC
	};

	struct Vertex
	{
		float x;
		float y;

		float dot(const Vertex & v) const
		{
			return x * v.x + y * v.y;
		}

		float len() const
		{
			return sqrtf(x * x + y * y);
		}

		Vertex operator-(const Vertex & v) const
		{
			Vertex r;
			r.x = x - v.x;
			r.y = y - v.y;
			return r;
		}

		Vertex operator+(const Vertex & v) const
		{
			Vertex r;
			r.x = x + v.x;
			r.y = y + v.y;
			return r;
		}

		Vertex operator*(const float v) const
		{
			Vertex r;
			r.x = x * v;
			r.y = y * v;
			return r;
		}

		Vertex operator/(const float v) const
		{
			Vertex r;
			r.x = x / v;
			r.y = y / v;
			return r;
		}
	};

	struct PathElem
	{
		ELEM_TYPE type;

		Vertex v1;
		Vertex v2;
		Vertex v3;
		Vertex v4;

		void lineHeading(float & x, float & y) const;
		void curveHeading(float & x, float & y, const float t) const;

		void curveEval(float & x, float & y, const float t) const;
		void curveSubdiv(const float t1, const float t2, float *& xy, float *& hxy, int & numPoints) const;
		void curveSubdiv(const Vertex & v1, const Vertex & v2, const Vertex & v3, float *& xy, float *& hxy, int & numPoints) const;
	};

	std::vector<PathElem> elems;

	float x;
	float y;
	bool hasMove;

	PathElem & allocElem()
	{
		elems.resize(elems.size() + 1);

		return elems.back();
	}

public:
	Path2d()
		: x(0.f)
		, y(0.f)
		, hasMove(false)
	{
	}

	void moveTo(const float x, const float y);
	void lineTo(const float x, const float y);
	void line(const float dx, const float dy);
	void curveTo(const float x, const float y, const float tx1, const float ty1, const float tx2, const float ty2);
	void curveToAbs(const float x, const float y, const float cx1, const float cy1, const float cx2, const float cy2);
	void curve(const float dx, const float dy, const float tx1, const float ty1, const float tx2, const float ty2);
	void arc(const float angle, const float radius);
	void close();

	void generatePoints(float * xy, float * hxy, const int maxPoints, const float curveFlatness, int & numPoints) const;
};

//

class Mouse
{
public:
	int x, y;
	int dx, dy;
	
	int scrollY;
	
	Mouse()
	{
		x = y = 0;
		dx = dy = 0;
		scrollY = 0;
	}
	
	bool isDown(BUTTON button) const;
	bool wentDown(BUTTON button) const;
	bool wentUp(BUTTON button) const;
	void showCursor(bool enabled);
	void setRelative(bool isRelative);
	
	bool isIdle() const; // return true when there is no mouse movement and there are no buttons being pressed
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

class Midi
{
	friend class Framework;

public:
	Midi();

	bool isConnected;
	bool isDown(int key) const;
	bool wentDown(int key) const;
	bool wentUp(int key) const;
	float getValue(int key, float _default) const;
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

void projectScreen2d();
void projectPerspective3d(const float fov, const float nearZ, const float farZ);
void viewLookat3d(const float originX, const float originY, const float originZ, const float targetX, const float targetY, const float targetZ, const float upX, const float upY, const float upZ);
Vec4 transformToWorld(const Vec4 & v);
Vec2 transformToScreen(const Vec3 & v, float & w);

void pushSurface(Surface * surface);
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

void pushLineSmooth(bool enabled);
void popLineSmooth();

void pushWireframe(bool enabled);
void popWireframe();

void setDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled = true);
void pushDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled = true);
void popDepthTest();

void pushDepthWrite(bool enabled);
void popDepthWrite();

void pushCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding);
void popCullMode();

void setColor(const Color & color);
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
void drawPath(const Path2d & path);

void drawLine3d(int axis = 0);
void drawRect3d(int axis1 = 0, int axis2 = 1);
void drawGrid3d(int resolution1, int resolution2, int axis1 = 0, int axis2 = 1);
void drawGrid3dLine(int resolution1, int resolution2, int axis1 = 0, int axis2 = 1, bool optimized = false);
void lineCube(Vec3Arg position, Vec3Arg size);
void fillCube(Vec3Arg position, Vec3Arg size);
void fillCylinder(Vec3Arg position, const float radius, const float height, const int resolution, const float angleOffset = 0.f);
void fillHexagon(Vec3Arg position, const float radius, const float height, const float angleOffset = 0.f);
void beginCubeBatch();
void endCubeBatch();

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromRGB8(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromR8(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromRGBF32(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromR16(const void * source, int sx, int sy, bool filter, bool clamp);
GxTextureId createTextureFromR32F(const void * source, int sx, int sy, bool filter, bool clamp);

void freeTexture(GxTextureId & textureId);

void debugDrawText(float x, float y, int size, float alignX, float alignY, const char * format, ...);

// OpenGL legacy mode drawing

#if !ENABLE_OPENGL

SDL_Surface * getWindowSurface();

static inline void gxMatrixMode(GX_MATRIX mode) { }
static inline void gxPopMatrix() { }
static inline void gxPushMatrix() { }
static inline void gxLoadIdentity() { }
static inline void gxLoadMatrixf(const float * m) { }
static inline void gxGetMatrixf(GX_MATRIX mode, float * m) { }
static inline void gxMultMatrixf(const float * m) { }
static inline void gxTranslatef(float x, float y, float z) { }
static inline void gxRotatef(float angle, float x, float y, float z) { }
static inline void gxScalef(float x, float y, float z) { }
static inline void gxValidateMatrices() { }

static inline void gxInitialize() { }
static inline void gxShutdown() { }
static inline void gxBegin(GX_PRIMITIVE_TYPE primitiveType) { }
static inline void gxEnd() { }
static inline void gxColor4f(float r, float g, float b, float a) { }
static inline void gxColor4fv(const float * rgba) { }
static inline void gxColor3ub(int r, int g, int b) { }
static inline void gxColor4ub(int r, int g, int b, int a) { }
static inline void gxTexCoord2f(float u, float v) { }
static inline void gxNormal3f(float x, float y, float z) { }
static inline void gxNormal3fv(const float  * v) { }
static inline void gxVertex2f(float x, float y) { }
static inline void gxVertex3f(float x, float y, float z) { }
static inline void gxVertex4f(float x, float y, float z, float w) { }
static inline void gxSetTexture(GxTextureId texture) { }

#elif !USE_LEGACY_OPENGL

void gxMatrixMode(GX_MATRIX mode);
GX_MATRIX gxGetMatrixMode();
void gxPopMatrix();
void gxPushMatrix();
void gxLoadIdentity();
void gxLoadMatrixf(const float * m);
void gxGetMatrixf(GX_MATRIX mode, float * m);
void gxSetMatrixf(GX_MATRIX mode, float * m);
void gxMultMatrixf(const float * m);
void gxTranslatef(float x, float y, float z);
void gxRotatef(float angle, float x, float y, float z);
void gxScalef(float x, float y, float z);
void gxValidateMatrices();

void gxInitialize();
void gxShutdown();
void gxBegin(GX_PRIMITIVE_TYPE primitiveType);
void gxEnd();
void gxEmitVertices(int primitiveType, int numVertices);
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

#else

#define gxMatrixMode glMatrixMode
GX_MATRIX gxGetMatrixMode();
#define gxPopMatrix glPopMatrix
#define gxPushMatrix glPushMatrix
#define gxLoadIdentity glLoadIdentity
#define gxLoadMatrixf glLoadMatrixf
void gxGetMatrixf(GX_MATRIX mode, float * m);
#define gxMultMatrixf glMultMatrixf
#define gxTranslatef glTranslatef
#define gxRotatef glRotatef
#define gxScalef glScalef
static inline void gxValidateMatrices() { }

void gxInitialize();
static inline void gxShutdown() { }
#define gxBegin glBegin
void gxEnd();
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


#endif

#if FRAMEWORK_ENABLE_GL_ERROR_LOG
	void checkErrorGL_internal(const char * function, int line);
	#define checkErrorGL() checkErrorGL_internal(__FUNCTION__, __LINE__)
#else
	#define checkErrorGL() do { } while (false)
#endif

// utility

void changeDirectory(const char * path);
std::string getDirectory();
std::vector<std::string> listFiles(const char * path, bool recurse);
void showErrorMessage(const char * caption, const char * format, ...);

// builtin shaders

void makeGaussianKernel(int kernelSize, ShaderBuffer & kernel, float sigma = 1.632f);

void setShader_GaussianBlurH(const GxTextureId source, const int kernelSize, const float radius);
void setShader_GaussianBlurV(const GxTextureId source, const int kernelSize, const float radius);
void setShader_ThresholdLumi(const GxTextureId source, const float lumi, const Color & failColor, const Color & passColor, const float opacity);
void setShader_ThresholdLumiFail(const GxTextureId source, const float lumi, const Color & failColor, const float opacity);
void setShader_ThresholdLumiPass(const GxTextureId source, const float lumi, const Color & passColor, const float opacity);
void setShader_ThresholdValue(const GxTextureId source, const Color & value, const Color & failColor, const Color & passColor, const float opacity);
// todo : implement these shaders .. ! and make source code shared/includable
void setShader_GrayscaleLumi(const GxTextureId source, const float opacity);
void setShader_GrayscaleWeights(const GxTextureId source, const Vec3 & weights, const float opacity);
void setShader_Colorize(const GxTextureId source, const float hue, const float opacity);
void setShader_HueShift(const GxTextureId source, const float hue, const float opacity);
void setShader_Composite(const GxTextureId source1, const GxTextureId source2);
void setShader_CompositePremultiplied(const GxTextureId source1, const GxTextureId source2);
void setShader_Premultiply(const GxTextureId source);
void setShader_ColorMultiply(const GxTextureId source, const Color & color, const float opacity);
void setShader_ColorTemperature(const GxTextureId source, const float temperature, const float opacity);

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

void hqSetTexture(const Mat4x4 & matrix, const GxTextureId texture);
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

void hqDrawPath(const Path2d & path, float stroke);

// math

template <typename T>
static T clamp(T v, T vmin, T vmax)
{
	return v < vmin ? vmin : v > vmax ? vmax : v;
}

template <typename T>
static T saturate(T v)
{
	return clamp<T>(v, (T)0, (T)1);
}

template <typename T>
static T lerp(T v1, T v2, float t)
{
	return v1 * (1.f - t) + v2 * t;
}

template <typename T>
static T lerp(T v1, T v2, double t)
{
	return v1 * (1.f - t) + v2 * t;
}

template <typename T>
static T sine(T min, T max, float t)
{
	t = t * float(M_PI) / 180.f;
	return static_cast<T>(min + (max - min) * (sinf(t) + 1.f) / 2.f);
}

template <typename T>
static T cosine(T min, T max, float t)
{
	t = t * float(M_PI) / 180.f;
	return static_cast<T>(min + (max - min) * (cosf(t) + 1.f) / 2.f);
}

template <typename T>
static T getAngle(T dx, T dy)
{
	return static_cast<T>(atan2f(dy, dx) / M_PI * 180.f + 90.f);
}

template <typename T>
static T random(T min, T max)
{
	return static_cast<T>(min + (max - min) * ((rand() % 1000) / 999.f));
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
