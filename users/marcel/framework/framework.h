#pragma once

#include <GL/glew.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include <algorithm>
#include <assert.h>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "Mat4x4.h"
#include "Vec3.h"

//#define fassert assert
#define fassert(x) do { } while (false)

// configuration

#if defined(DEBUG) || 1
	#define ENABLE_LOGGING 1
#else
	#define ENABLE_LOGGING 0 // do not alter
#endif

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
	FILTER_LINEAR
};

enum TRANSFORM
{
	TRANSFORM_SCREEN, // pixel based coordinate syste
	TRANSFORM_2D,     // use transform set through setTransform2d
	TRANSFORM_3D      // use transform set through setTransform3d
};

// forward declations

class Color;
class Dictionary;
class Font;
class Framework;
class Gamepad;
class Gradient;
class Keyboard;
class Model;
class Mouse;
class Music;
class Shader;
class Sound;
class Sprite;
class Stage;
class StageObject;
class Surface;
class Ui;

// globals

extern Framework framework;
extern Dictionary settings;
extern Mouse mouse;
extern Keyboard keyboard;
extern Gamepad gamepad[MAX_GAMEPAD];
extern Stage stage;
extern Ui ui;

// event handlers

typedef void (*ActionHandler)(const std::string & action, const Dictionary & args);
	
//

class Framework
{
public:
	friend class Sprite;
	friend class Model;
	
	Framework();
	~Framework();
	
	void setActionHandler(ActionHandler actionHandler);
	
	bool init(int argc, char * argv[], int sx, int sy);
	bool shutdown();
	void process();
	void processAction(const std::string & action, const Dictionary & args);
	void processActions(const std::string & actions, const Dictionary & args);
	void reloadCaches();
	void fillCachesWithPath(const char * path);
	
	void beginDraw(int r, int g, int b, int a);
	void endDraw();
	
	float time;
	float timeStep;
	
	bool fullscreen;
	int minification;
	bool reloadCachesOnActivate;
	int numSoundSources;
	int windowX;
	int windowY;
	std::string windowTitle;
	ActionHandler actionHandler;
	
private:
	typedef std::set<Sprite*> SpriteSet;
	typedef std::set<Model*> ModelSet;
	
	SpriteSet m_sprites;
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

//

class Shader
{
	class ShaderCacheElem * m_shader;

public:
	Shader();
	Shader(const char * filename);
	
	void load(const char * filename);
	GLuint getProgram() const;
	
	GLint getImmediate(const char * name);
	GLint getAttribute(const char * name);
	
	void setImmediate(const char * name, float x);	
	void setImmediate(const char * name, float x, float y);
	void setImmediate(const char * name, float x, float y, float z);
	void setImmediate(const char * name, float x, float y, float z, float w);
	void setImmediateMatrix4x4(const char * name, const float * matrix);
	void setTextureUnit(const char * name, int unit); // bind <name> to GL_TEXTURE0 + unit
	void setTexture(const char * name, int unit, GLuint texture);
};

//

class Color
{
public:
	Color();
	explicit Color(int r, int g, int b, int a = 255);
	explicit Color(float r, float g, float b, float a = 1.f);
	
	Color interp(const Color & other, float t) const;
	
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
	
	std::string getString(const char * name, const char * _default) const;	
	int getInt(const char * name, int _default) const;
	bool getBool(const char * name, bool _default) const;
	float getFloat(const char * name, float _default) const;
	
	std::string & operator[](const char * name);
};

//

class Sprite
{
public:
	friend class Framework;
	
	Sprite(const char * filename, float pivotX = 0.f, float pivotY = 0.f, const char * spritesheet = 0);
	~Sprite();
	
	void draw();
	void drawEx(float x, float y, float angle = 0.f, float scale = 1.f, bool pixelpos = true, TEXTURE_FILTER filter = FILTER_POINT);
	
	// animation
	void startAnim(const char * anim, int frame = 0);
	void stopAnim();
	void pauseAnim() { animIsPaused = true; }
	void resumeAnim() { animIsPaused = false; }
	const std::string & getAnim() const;
	void setAnimFrame(int frame);
	int getAnimFrame() const;
	
	// drawing
	float pivotX;
	float pivotY;
	float x;
	float y;
	float angle;
	float scale;
	bool flipX;
	bool flipY;
	bool pixelpos;
	TEXTURE_FILTER filter;
	
	int getWidth() const;
	int getHeight() const;
	
	// animation
	float animSpeed;
	bool animIsActive;
	bool animIsPaused;
	
private:
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

	void updateAnimationSegment();
	void updateAnimation(float timeStep);
	void processAnimationFrameChange(int frame1, int frame2);
	void processAnimationTriggersForFrame(int frame, int event);
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
	
//private:
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
	
	void play();
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
	
	Mouse()
	{
		x = y = 0;
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
	bool wentDown(SDLKey key) const;
	bool wentUp(SDLKey key) const;
};

class Gamepad
{
	friend class Framework;

	float m_analog[2][ANALOG_MAX];

public:
	Gamepad();
	
	bool isConnected;
	bool isDown[GAMEPAD_MAX];
	bool wentDown[GAMEPAD_MAX];
	bool wentUp[GAMEPAD_MAX];
	float getAnalog(int stick, ANALOG analog, float scale = 1.f) const;
};

//

class StageObject
{	
public:
	StageObject();
	~StageObject();
	
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
	
	std::string m_over;
	std::string m_down;
	
	std::string getImage(Dictionary & d);
	bool getArea(Dictionary & d, int & x, int & y, int & sx, int & sy);
	std::string findMouseOver();
	
public:
	Ui();
	Ui(const char * filename);
	
	void load(const char * filename);
	void process();
	void draw();
	
	Dictionary & operator[](const char * name);
};

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
void setFont(Font & font);
void setShader(Shader & shader);
void clearShader();

void drawLine(float x1, float y1, float x2, float y2);
void drawRect(float x1, float y1, float x2, float y2);
void drawRectLine(float x1, float y1, float x2, float y2);
void drawRectGradient(float x1, float y1, float x2, float y2);
void drawText(float x, float y, int size, float alignX, float alignY, const char * format, ...);

void debugDrawText(float x, float y, int size, float alignX, float alignY, const char * format, ...);

// utility

void changeDirectory(const char * path);

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
	t = t * M_PI / 180.f;
	return static_cast<T>(min + (max - min) * (std::sin(t) + 1.f) / 2.f);
}

template <typename T>
static T cosine(T min, T max, float t)
{
	t = t * M_PI / 180.f;
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

#if ENABLE_LOGGING
	void logDebug(const char * format, ...);
	void log(const char * format, ...);
	void logWarning(const char * format, ...);
	void logError(const char * format, ...);
#else
	#define logDebug(...) do { } while (false)
	#define log(...) do { } while (false)
	#define logWarning(...) do { } while (false)
	#define logError(...) do { } while (false)
#endif

// constants

extern Color colorBlack;
extern Color colorWhite;
extern Color colorRed;
extern Color colorGreen;
extern Color colorBlue;
