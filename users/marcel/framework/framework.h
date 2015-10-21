#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "Mat4x4.h"
#include "Vec3.h"

#if defined(DEBUG)
	#define fassert assert
#else
	#define fassert(x) do { } while (false)
#endif

// configuration

#if defined(DEBUG)
	#define ENABLE_LOGGING_DBG 1
	#define ENABLE_LOGGING 1
	#define ENABLE_PROFILING 1
#else
	#define ENABLE_LOGGING_DBG 0 // do not alter
	#define ENABLE_LOGGING 0 // do not alter
	#define ENABLE_PROFILING 0 // do not alter
#endif

#define USE_LEGACY_OPENGL 1
#define ENABLE_MIDI_INPUT 0
#define ENABLE_UTF8_SUPPORT 0

static const int MAX_GAMEPAD = 4;

// enumerations

enum BLEND_MODE // setBlend
{
	BLEND_OPAQUE,
	BLEND_ALPHA,
	BLEND_ADD,
	BLEND_SUBTRACT,
	BLEND_INVERT,
	BLEND_MUL
};

enum COLOR_MODE // setColorMode
{
	COLOR_MUL,
	COLOR_ADD,
	COLOR_SUB,
	COLOR_IGNORE
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

enum RESOURCE_CACHE
{
	CACHE_FONT = 1 << 0,
	CACHE_SHADER = 1 << 1,
	CACHE_SOUND = 1 << 2,
	CACHE_SPRITE = 1 << 3,
	CACHE_SPRITER = 1 << 4,
	CACHE_TEXTURE = 1 << 5
};

// forward declations

class Color;
class Dictionary;
class Font;
class Framework;
class Gamepad;
class Gradient;
class Keyboard;
class Midi;
class Model;
class Mouse;
class Music;
class Shader;
class Sound;
class Sprite;
class Spriter;
struct SpriterState;
class Stage;
class StageObject;
class Surface;
class Ui;

namespace spriter
{
	struct Drawable;
	class Scene;
}

// globals

extern Framework framework;
extern Dictionary settings;
extern Mouse mouse;
extern Keyboard keyboard;
extern Gamepad gamepad[MAX_GAMEPAD];
extern Midi midi;
extern Stage stage;
extern Ui ui;

// event handlers

typedef void (*ActionHandler)(const std::string & action, const Dictionary & args);
typedef void (*FillCachesCallback)(float filePercentage);

//

class Framework
{
public:
	friend class Sprite;
	friend class Model;
	
	Framework();
	~Framework();
	
	void setActionHandler(ActionHandler actionHandler);
	
	bool init(int argc, const char * argv[], int sx, int sy);
	bool shutdown();
	void process();
	void processAction(const std::string & action, const Dictionary & args);
	void processActions(const std::string & actions, const Dictionary & args);
	void reloadCaches();
	void fillCachesWithPath(const char * path, bool recurse);
	
	void setFullscreen(bool fullscreen);

	void beginDraw(int r, int g, int b, int a);
	void endDraw();

	void blinkTaskbarIcon(int count);
	
	bool quitRequested;
	float time;
	float timeStep;
	
	bool fullscreen;
	bool useClosestDisplayMode;
	int minification;
	bool reloadCachesOnActivate;
	bool filedrop;
	int numSoundSources;
	int windowX;
	int windowY;
	std::string windowTitle;
	std::string windowIcon;
	bool windowIsActive;
	ActionHandler actionHandler;
	FillCachesCallback fillCachesCallback;
	
private:
	typedef std::set<Model*> ModelSet;
	
	Sprite * m_sprites;
	ModelSet m_models;
	
	void registerSprite(Sprite * sprite);
	void unregisterSprite(Sprite * sprite);
	
	void registerModel(Model * model);
	void unregisterModel(Model * model);
};

//

class Surface
{
	int m_size[2];
	int m_bufferId;
	GLuint m_buffer[2];
	GLuint m_texture[2];
	
	void construct();
	void destruct();
	
public:
	Surface();
	Surface(int sx, int sy);
	~Surface();
	
	void swapBuffers();

	bool init(int sx, int sy);
	GLuint getFramebuffer() const;
	GLuint getTexture() const;
	int getWidth() const;
	int getHeight() const;
	
	void clear(int r = 0, int g = 0, int b = 0, int a = 0);
	void clearf(float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.f);
	void clearAlpha();
	void setAlpha(int a);
	void setAlphaf(float a);
	void mulf(float r, float g, float b, float a = 1.f);
	
	void postprocess(Shader & shader);
	
	void invert();
	void invertColor();
	void invertAlpha();
	void blitTo(Surface * surface);
};

void blitBackBufferToSurface(Surface * surface);

//

class Shader
{
	class ShaderCacheElem * m_shader;

public:
	Shader();
	Shader(const char * filename);
	~Shader();
	
	void load(const char * filename);
	bool isValid() const { return m_shader != 0; }
	GLuint getProgram() const;
	
	GLint getImmediate(const char * name);
	GLint getAttribute(const char * name);
	
	void setImmediate(const char * name, float x);	
	void setImmediate(const char * name, float x, float y);
	void setImmediate(const char * name, float x, float y, float z);
	void setImmediate(const char * name, float x, float y, float z, float w);
	void setImmediate(GLint index, float x, float y, float z, float w);
	void setImmediateMatrix4x4(const char * name, const float * matrix);
	void setImmediateMatrix4x4(GLint index, const float * matrix);
	void setTextureUnit(const char * name, int unit); // bind <name> to GL_TEXTURE0 + unit
	void setTextureUnit(GLint index, int unit); // bind <name> to GL_TEXTURE0 + unit
	void setTexture(const char * name, int unit, GLuint texture, bool filtered);

	const ShaderCacheElem & getCacheElem() const { return *m_shader; }
};

//

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
	
	float r, g, b, a;
};

//

class Gradient
{
public:
	float x1;
	float y1;
	Color color1;
	float x2;
	float y2;
	Color color2;
	
	Gradient();
	Gradient(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2);
	
	void set(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2);
	Color eval(float x, float y) const;
};

//

class Dictionary
{
	typedef std::map<std::string, std::string> Map;
	Map m_map;
	
public:
	bool load(const char * filename);
	bool parse(const std::string & line, bool clear = true); // line = key1:value1 key2:value2 key3:value3 ..
	
	bool contains(const char * name) const;
	
	void setString(const char * name, const char * value);
	void setInt(const char * name, int value);
	void setBool(const char * name, bool value);
	void setFloat(const char * name, float value);
	void setPtr(const char * name, void * value);
	
	std::string getString(const char * name, const char * _default) const;
	int getInt(const char * name, int _default) const;
	bool getBool(const char * name, bool _default) const;
	float getFloat(const char * name, float _default) const;
	void * getPtr(const char * name, void * _default) const;
	template <typename T> T * getPtrType(const char * name, T * _default) const { return (T*)getPtr(name, _default); }
	
	std::string & operator[](const char * name);
};

//

class Sprite
{
public:
	friend class Framework;
	
	Sprite(const char * filename, float pivotX = 0.f, float pivotY = 0.f, const char * spritesheet = 0, bool autoUpdate = false, bool hasSpriteSheet = true);
	~Sprite();
	
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
	GLuint getTexture() const;
	
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
	DrawBoundingBox        = 0x0100
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
	
	bool animIsActive;
	bool animIsPaused;
	float animTime;
	int animLoop;
	int animLoopCount;
	float animSpeed;
	Vec3 animRootMotion;
	
	Model(const char * filename);
	Model(class ModelCacheElem & cacheElem);
	~Model();
	
	void startAnim(const char * name, int loop = 1);
	void stopAnim();
	void pauseAnim() { animIsPaused = true; }
	void resumeAnim() { animIsPaused = false; }
	std::vector<std::string> getAnimList() const;
	
	void draw(int drawFlags = DrawMesh);
	void drawEx(Vec3 position, Vec3 axis, float angle = 0.f, float scale = 1.f, int drawFlags = DrawMesh);
	void drawEx(const Mat4x4 & matrix, int drawFlags = DrawMesh);
	
private:
	void ctor();

	// drawing
	class ModelCacheElem * m_model;
	
	// animation
	std::string m_animSegmentName;
	void * m_currentAnim;
	void * m_animSegment;
	bool m_isAnimStarted;

	void updateAnimationSegment();
	void updateAnimation(float timeStep);
};

//

#pragma pack(push)
#pragma pack(1)

struct SpriterState
{
	SpriterState();

	int16_t x;
	int16_t y;
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
	
	void play(int volume = -1, int speed = -1);
	void stop();
	void setVolume(int volume);
	void setSpeed(int speed);
	
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
	
private:
	class FontCacheElem * m_font;
};

//

class Mouse
{
public:
	int x, y;
	int dx, dy;
	
	Mouse()
	{
		x = y = 0;
		dx = dy = 0;
	}
	
	bool isDown(BUTTON button) const;
	bool wentDown(BUTTON button) const;
	bool wentUp(BUTTON button) const;
	void showCursor(bool enabled);
};

class Keyboard
{
public:
	bool isDown(SDLKey key) const;
	bool wentDown(SDLKey key, bool allowRepeat = false) const;
	bool wentUp(SDLKey key) const;
	bool keyRepeat(SDLKey key) const;

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
	float m_lastVibrationStrength;

public:
	Gamepad();
	
	bool isConnected;
	bool isDown(GAMEPAD button) const;
	bool wentDown(GAMEPAD button) const;
	bool wentUp(GAMEPAD button) const;
	float getAnalog(int stick, ANALOG analog, float scale = 1.f) const;

	void vibrate(float duration, float strength);
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
	float getValue(int key) const;
};

//

class StageObject
{
public:
	StageObject();
	virtual ~StageObject();
	
	virtual void process(float timeStep);
	virtual void draw();
	
	bool isDead;
	Sprite * sprite;
};

class StageObject_SpriteAnim : public StageObject
{
public:
	StageObject_SpriteAnim(const char * name, const char * anim, const char * sheet = 0);
	
	virtual void process(float timeStep);
};

class Stage
{
	typedef std::map<int, StageObject*> ObjectList;
	
	ObjectList m_objects;
	
	int m_objectId;
	
public:
	Stage();
	
	void process(float timeStep);
	void draw();
	
	int addObject(StageObject * object);
	void removeObject(int objectId);
};

//

class Ui
{
	class UiCacheElem * m_ui;
	bool m_ownsUi;
	
	std::string m_over;
	std::string m_down;
	
	std::string getImage(Dictionary & d);
	bool getArea(Dictionary & d, int & x, int & y, int & sx, int & sy);
	std::string findMouseOver();
	
public:
	Ui();
	Ui(const char * filename);
	~Ui();
	
	void load(const char * filename);
	void process();
	void draw();
	
	void remove(const char * name);
	void clear();

	Dictionary & operator[](const char * name);
};

void clearCaches(int caches);

// drawing

void setTransform(TRANSFORM transform);
void applyTransform();
void setTransform2d(const Mat4x4 & transform);
void setTransform3d(const Mat4x4 & transform);
Vec2 transformToScreen(const Vec3 & v);

void pushSurface(Surface * surface);
void popSurface();
void setDrawRect(int x, int y, int sx, int sy);
void clearDrawRect();

void setBlend(BLEND_MODE blendMode);
void setColorMode(COLOR_MODE colorMode);
void setColor(const Color & color);
void setColor(int r, int g, int b, int a = 255, int rgbMul = 255);
void setColorf(float r, float g, float b, float a = 1.f, float rgbMul = 1.f);
void setGradientf(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2);
void setGradientf(float x1, float y1, float r1, float g1, float b1, float a1, float x2, float y2, float r2, float g2, float b2, float a2);
void setFont(const Font & font);
void setFont(const char * font);
void setShader(const Shader & shader);
void clearShader();

void drawLine(float x1, float y1, float x2, float y2);
void drawRect(float x1, float y1, float x2, float y2);
void drawRectLine(float x1, float y1, float x2, float y2);
void drawRectGradient(float x1, float y1, float x2, float y2);
void measureText(int size, float & sx, float & sy, const char * format, ...);
void drawText(float x, float y, int size, float alignX, float alignY, const char * format, ...);
void drawTextArea(float x, float y, float sx, int size, const char * format, ...);
void drawTextArea(float x, float y, float sx, float sy, int size, float alignX, float alignY, const char * format, ...);

void debugDrawText(float x, float y, int size, float alignX, float alignY, const char * format, ...);

// OpenGL legacy mode drawing

#if !USE_LEGACY_OPENGL

void gxMatrixMode(GLenum mode);
void gxPopMatrix();
void gxPushMatrix();
void gxLoadIdentity();
void gxLoadMatrixf(const float * m);
void gxGetMatrixf(GLenum mode, float * m);
void gxTranslatef(float x, float y, float z);
void gxRotatef(float angle, float x, float y, float z);
void gxScalef(float x, float y, float z);
void gxValidateMatrices();

void gxInitialize();
void gxShutdown();
void gxBegin(int primitiveType);
void gxEnd();
void gxColor4f(float r, float g, float b, float a);
void gxColor4fv(const float * rgba);
void gxColor3ub(int r, int g, int b);
void gxTexCoord2f(float u, float v);
void gxNormal3f(float x, float y, float z);
void gxVertex2f(float x, float y);
void gxVertex3f(float x, float y, float z);
void gxSetTexture(GLuint texture);

#else

#define gxMatrixMode glMatrixMode
#define gxPopMatrix glPopMatrix
#define gxPushMatrix glPushMatrix
#define gxLoadIdentity glLoadIdentity
#define gxLoadMatrixf glLoadMatrixf
void gxGetMatrixf(GLenum mode, float * m);
#define gxTranslatef glTranslatef
#define gxRotatef glRotatef
#define gxScalef glScalef
static inline void gxValidateMatrices() { }

static inline void gxInitialize() { }
static inline void gxShutdown() { }
void gxBegin(int primitiveType);
#define gxEnd glEnd
#define gxColor4f glColor4f
#define gxColor4fv glColor4fv
#define gxColor3ub glColor3ub
#define gxTexCoord2f glTexCoord2f
#define gxNormal3f glNormal3f
#define gxVertex2f glVertex2f
#define gxVertex3f glVertex3f
void gxSetTexture(GLuint texture);

#endif

// utility

void changeDirectory(const char * path);
std::vector<std::string> listFiles(const char * path, bool recurse);

// math

template <typename T>
static T clamp(T v, T vmin, T vmax)
{
	return std::min(std::max(v, vmin), vmax);
}

template <typename T>
static T saturate(T v)
{
	return clamp<T>(v, (T)0, (T)1);
}

template <typename T>
static T sine(T min, T max, float t)
{
	t = t * float(M_PI) / 180.f;
	return static_cast<T>(min + (max - min) * (std::sin(t) + 1.f) / 2.f);
}

template <typename T>
static T cosine(T min, T max, float t)
{
	t = t * float(M_PI) / 180.f;
	return static_cast<T>(min + (max - min) * (std::cos(t) + 1.f) / 2.f);
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
	void log(const char * format, ...);
	void logWarning(const char * format, ...);
	void logError(const char * format, ...);
#else
	#define log(...) do { } while (false)
	#define logWarning(...) do { } while (false)
	#define logError(...) do { } while (false)
#endif

// profiling

#if ENABLE_PROFILING
	#include "remotery.h"
	#define cpuTimingBlock(name) rmt_ScopedCPUSample(name)
	#define cpuTimingBegin(name) rmt_BeginCPUSample(name)
	#define cpuTimingEnd() rmt_EndCPUSample()
	#define gpuTimingBlock(name) rmt_ScopedOpenGLSample(name)
	#define gpuTimingBegin(name) rmt_BeginOpenGLSample(name)
	#define gpuTimingEnd() rmt_EndOpenGLSample()
#else
	#define cpuTimingBlock(name) do { } while (false)
	#define cpuTimingBegin(name) do { } while (false)
	#define cpuTimingEnd() do { } while (false)
	#define gpuTimingBlock(name) do { } while (false)
	#define gpuTimingBegin(name) do { } while (false)
	#define gpuTimingEnd() do { } while (false)
#endif

// constants

extern Color colorBlack;
extern Color colorWhite;
extern Color colorRed;
extern Color colorGreen;
extern Color colorBlue;
extern Color colorYellow;
