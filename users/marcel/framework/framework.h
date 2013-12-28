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
	ANALOG_Y
};

enum GAMEPAD
{
	DPAD_LEFT,
	DPAD_RIGHT,
	DPAD_UP,
	DPAD_DOWN,
	GAMEPAD_L1,
	GAMEPAD_L2,
	GAMEPAD_R1,
	GAMEPAD_R2,
	GAMEPAD_MENU1,
	GAMEPAD_MENU2
};

//

class Dictionary;
class Sprite;

//

class Framework
{
public:
	friend class Sprite;
	
	typedef void (*ActionHandler)(const std::string & action, const Dictionary & args);
	
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
	bool contains(const char * name) const
	{
		return m_map.count(name) != 0;
	}
	
	void setString(const char * name, const char * value)
	{
		m_map[name] = value;
	}
	
	std::string getString(const char * name, const char * _default) const
	{
		Map::const_iterator i = m_map.find(name);
		if (i != m_map.end())
			return i->second;
		else
			return _default;
	}
	
	int getInt(const char * name, int _default) const
	{
		Map::const_iterator i = m_map.find(name);
		if (i != m_map.end())
			return atoi(i->second.c_str());
		else
			return _default;
	}
};

class Sprite
{
public:
	friend class Framework;

	Sprite(const char * filename, float pivotX = 0.f, float pivotY = 0.f);
	~Sprite();
	
	void draw();
	void drawEx(float x, float y, float angle = 0.f, float scale = 1.f, BLEND_MODE blendMode = BLEND_ALPHA);
	
	void setPosition(float x, float y);
	void setAngle(float angle);
	void setScale(float scale);
	void setBlend(BLEND_MODE blendMode);
	void setFlip(bool flipX, bool flipY = false);
	
	// animation
	void startAnim(const char * anim, int frame = 0);
	void stopAnim();
	void setAnimFrame(int frame);
	int getAnimFrame() const;
	void setAnimSpeed(float speed);
	
private:
	// drawing
	class TextureCacheElem * m_texture;
	float m_pivotX;
	float m_pivotY;
	float m_positionX;
	float m_positionY;
	float m_angle;
	float m_scaleX;
	float m_scaleY;
	BLEND_MODE m_blendMode;
	bool m_flipX;
	bool m_flipY;
	
	// animation
	class AnimCacheElem * m_anim;
	int m_animVersion;
	std::string m_animSegmentName;
	void * m_animSegment;
	bool m_isAnimActive;
	float m_animFrame;
	float m_animSpeed;

	void updateAnimationSegment();
	void updateAnimation(float dt);
};

class Sound
{
public:
	Sound(const char * filename);
	
	void play(int volume = 100, int speed = 100);
	void stop();
	void setVolume(int volume);
	void setSpeed(int speed);
	
	static void stopAll();
	
private:
	class SoundCacheElem * m_sound;
	int m_playId;
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
	float getX();
	float getY();
	bool isDown(BUTTON button);
};

class Keyboard
{
public:
	bool isDown(SDLKey key);
};

class Gamepad
{
public:
	bool isConnected();
	bool isDown(GAMEPAD button);
	float getAnalog(int stick, ANALOG analog);
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

