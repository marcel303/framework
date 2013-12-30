#pragma once

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

#include <map>
#include <set>
#include <string>
#include <vector>

static const int MAX_GAMEPAD = 4;

enum BLEND_MODE
{
	BLEND_OPAQUE,
	BLEND_ALPHA,
	BLEND_ADD,
	BLEND_SUBTRACT
};

enum BUTTON
{
	BUTTON_LEFT,
	BUTTON_RIGHT
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

//

class Dictionary;
class Sprite;

//

typedef void (*ActionHandler)(const std::string & action, const Dictionary & args);
	
//

class Framework
{
public:
	friend class Sprite;
	
	Framework();
	~Framework();
	
	void setMinification(int scale);
	void setNumSoundSources(int num);
	void setActionHandler(ActionHandler actionHandler);
	
	bool init(int argc, char * argv[], int sx, int sy);
	bool shutdown();
	void process();
	void processAction(const std::string & action, const Dictionary & args);
	void reloadCaches();
	
	void beginDraw(int r, int g, int b, int a);
	void endDraw();
	
	float timeStep;
	
private:
	typedef std::set<Sprite*> SpriteSet;
	
	int m_minification;
	int m_numSoundSources;
	ActionHandler m_actionHandler;
	
	SpriteSet m_sprites;
	
	void registerSprite(Sprite * sprite);
	void unregisterSprite(Sprite * sprite);
};

class Color
{
public:
	Color();
	explicit Color(int r, int g, int b, int a = 255);
	explicit Color(float r, float g, float b, float a = 1.f);
	
	float r, g, b, a;
};

class Dictionary
{
	typedef std::map<std::string, std::string> Map;
	Map m_map;
	
public:
	bool contains(const char * name) const;
	void setString(const char * name, const char * value);	
	void setInt(const char * name, int value);	
	std::string getString(const char * name, const char * _default) const;	
	int getInt(const char * name, int _default) const;
};

class Sprite
{
public:
	friend class Framework;
	
	Sprite(const char * filename, float pivotX = 0.f, float pivotY = 0.f, const char * spritesheet = 0);
	~Sprite();
	
	void draw();
	void drawEx(float x, float y, float angle = 0.f, float scale = 1.f, BLEND_MODE blendMode = BLEND_ALPHA);
	
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
	BLEND_MODE blend;
	bool flipX;
	bool flipY;
	
	// animation
	float animSpeed;
	bool animIsPaused;
	
private:
	// drawing
	class TextureCacheElem * m_texture;
	
	// animation
	class AnimCacheElem * m_anim;
	int m_animVersion;
	std::string m_animSegmentName;
	void * m_animSegment;
	bool m_isAnimActive;
	float m_animFramef;
	int m_animFrame;

	void updateAnimationSegment();
	void updateAnimation(float timeStep);
	void processAnimationFrameChange(int frame1, int frame2);
	void processAnimationTriggersForFrame(int frame, int event);
};

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

class Music
{
public:
	Music(const char * filename);
	
	void play();
	void stop();
	void setVolume(float volume);
	
private:
	// ???
};

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

class Mouse
{
public:
	int x;
	int y;
	
	Mouse()
	{
		x = y = 0;
	}
	
	bool isDown(BUTTON button);
};

class Keyboard
{
public:
	bool isDown(SDLKey key);
};

class Gamepad
{
	float m_analog[2][ANALOG_MAX];

public:
	Gamepad();
	
	bool isConnected;
	bool isDown[GAMEPAD_MAX];
	float getAnalog(int stick, ANALOG analog, float scale = 1.f) const;
};

//

void setBlend(BLEND_MODE blendMode);
void setColor(const Color & color);
void setColor(int r, int g, int b, int a = 255);
void setColorf(float r, float g, float b, float a = 1.f);
void setFont(Font & font);

void drawLine(float x1, float y1, float x2, float y2);
void drawRect(float x1, float y1, float x2, float y2);
void drawText(float x, float y, int size, int alignX, int alignY, const char * format, ...);

void log(const char * format, ...);
void logWarning(const char * format, ...);
void logError(const char * format, ...);

//

extern Color colorBlack;
extern Color colorWhite;
extern Color colorRed;
extern Color colorGreen;
extern Color colorBlue;

//

extern Framework framework;
extern Mouse mouse;
extern Keyboard keyboard;
extern Gamepad gamepad[MAX_GAMEPAD];

