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

/*

OpenFrameworks -> framework interop classes

*/

#include <iostream>
#include <cmath>
#include <memory>
#include <sstream>
#include <stdint.h>
#include <string>
#include <vector>

#include <GL/glew.h> // OpenFrameworks exposes OpenGL
#include "framework.h"

using namespace std;

static const float PI = float(M_PI);

#define OF_KEY_RETURN SDLK_RETURN
#define OF_KEY_TAB SDLK_TAB
#define OF_KEY_BACKSPACE SDLK_BACKSPACE
#define OF_KEY_DEL SDLK_DELETE
#define OF_KEY_LEFT SDLK_LEFT
#define OF_KEY_RIGHT SDLK_RIGHT
#define OF_KEY_UP SDLK_UP
#define OF_KEY_DOWN SDLK_DOWN

#define OF_EVENT_ORDER_BEFORE_APP 0
#define OF_EVENT_ORDER_AFTER_APP 1

struct ofBuffer
{
	unsigned char * bytes;
	int numBytes;
	
	ofBuffer()
		: bytes(nullptr)
		, numBytes(0)
	{
	}
	
	ofBuffer(unsigned char * _bytes, int _numBytes)
		: bytes(_bytes)
		, numBytes(_numBytes)
	{
	}
	
	unsigned char * getBinaryBuffer()
	{
		return bytes;
	}
	
	int size() const
	{
		return numBytes;
	}
};

struct ofColor
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	
	static ofColor black;
	static ofColor white;
	
	static ofColor fromHex(const char * hex);
	static ofColor fromHex(uint32_t hex);
	
	ofColor()
		: r(255)
		, g(255)
		, b(255)
		, a(255)
	{
	}
	
	ofColor(const uint8_t _r, const uint8_t _g, const uint8_t _b, const uint8_t _a = 0xff)
		: r(_r)
		, g(_g)
		, b(_b)
		, a(_a)
	{
	}
	
	int getHex() const;
};

struct ofFloatColor
{
	float r;
	float g;
	float b;
	float a;
	
	ofFloatColor()
		: r(1.f)
		, g(1.f)
		, b(1.f)
		, a(1.f)
	{
	}
	
	ofFloatColor(const ofColor & c)
		: r(c.r / 255.f)
		, g(c.g / 255.f)
		, b(c.b / 255.f)
		, a(c.a / 255.f)
	{
	}
};

struct ofVec2f
{
	float x;
	float y;
	
	ofVec2f()
		: x(0.f)
		, y(0.f)
	{
	}
	
	ofVec2f(const float _x, const float _y)
		: x(_x)
		, y(_y)
	{
	}
};

struct ofPoint
{
	float x;
	float y;
	
	ofPoint()
		: x(0.f)
		, y(0.f)
	{
	}
	
	ofPoint(const float _x, const float _y)
		: x(_x)
		, y(_y)
	{
	}
};

inline ofPoint operator-(const ofPoint & p1, const ofPoint & p2)
{
	return ofPoint(p1.x - p2.x, p1.y - p2.y);
}

struct ofRectangle
{
	float x;
	float y;
	float width;
	float height;
	
	ofRectangle()
		: x(0.f)
		, y(0.f)
		, width(0.f)
		, height(0.f)
	{
	}
	
	ofRectangle(const float _x, const float _y, const float _width, const float _height)
		: x(_x)
		, y(_y)
		, width(_width)
		, height(_height)
	{
	}
	
	float getLeft() const { return x; }
	float getRight() const { return x + width; }
	float getTop() const { return y; }
	float getBottom() const { return y + height; }
	
	bool inside(const float x, const float y) const { return x > getLeft() && x < getRight() && y > getTop() && y < getBottom(); }
	bool inside(const ofPoint & pt) const { return inside(pt.x, pt.y); }
};

struct ofImage
{
	class Sprite * sprite;
	
	ofImage();
	~ofImage();
	
	void load(const std::string & filename);
	
	void draw(const float x, const float y, const float sx, const float sy) const;
	void draw(const ofRectangle & rect) const;
};

template <typename T>
struct ofParameter
{
	std::string name;
	T value;
	T min;
	T max;
	
	ofParameter()
		: name()
		, value()
		, min()
		, max()
	{
	}
	
	ofParameter(const T & _value)
		: name()
		, value(_value)
		, min()
		, max()
	{
	}
	
	ofParameter(const std::string & _name, const T & _value, const T & _min, const T & _max)
		: name(_name)
		, value(_value)
		, min(_min)
		, max(_max)
	{
	}
	
	T get() const
	{
		return value;
	}
	
	void set(const T & _value)
	{
		value = _value;
	}
	
	const std::string getName() const
	{
		return name;
	}
	
	T getMin() const { return min; }
	T getMax() const { return max; }
	
	//
	
	template <typename EventArgment, typename ListenerClass>
	void addListener(ListenerClass  * listenerClass, void (ListenerClass::*listenerMethod)(EventArgment&))
	{
	}
};

struct ofKeyEventArgs
{
	int key;
};

struct ofMouseEventArgs
{
	int x;
	int y;
	int scrollY;
};

struct ofResizeEventArgs
{
};

struct ofEventArgs
{
};

struct ofVbo
{
	std::vector<ofFloatColor> colors;
	std::vector<ofVec2f> vertices;
	
	void setColorData(const ofFloatColor * colors, const int numColors, const int usage);
	void setVertexData(const ofVec2f * vertices, const int numVertices, const int usage);
	
	void draw(const int primitiveType, const int vertexOffset, const int numVertices);
};

struct ofFbo
{
	class Surface * surface;
	
	ofFbo();
	~ofFbo();
	
	void allocate(const int sx, const int sy);
	
	void begin();
	void end();
	
	void draw(const float x, const float y);
};

int ofGetVersionMajor();
int ofGetVersionMinor();
int ofGetVersionPatch();

int ofGetWidth();
int ofGetHeight();
int ofGetFrameRate();
float ofGetElapsedTimef();

int ofGetMouseX();
int ofGetMouseY();
bool ofGetMousePressed();

void ofSetColor(const ofColor & color, const float opacity = 1.f);
void ofSetLineWidth(const float width);
void ofPushStyle();
void ofPopStyle();

void ofClear(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a);
void ofDrawRectangle(const float x, const float y, const float sx, const float sy);
void ofDrawRectangle(const ofRectangle & rect);
void ofDrawLine(const float x1, const float y1, const float x2, const float y2);
void ofDrawLine(const ofPoint & p1, const ofPoint & p2);
void ofDrawCircle(const ofPoint & p, const float radius);
void ofFill();

int ofToLower(const int c);
int ofToUpper(const int c);

std::string ofToLower(const std::string & s);
std::string ofToUpper(const std::string & s);

std::string ofToString(const double v);
std::string ofToString(const double v, const int numDecimals);
float ofToFloat(const std::string & s);

int ofHexToInt(const std::string & s);

float ofRandom(const float min, const float max);

//

#include <functional>

template <typename T>
struct ofEvent
{
	struct Listener
	{
		const void * instance;
		const void * method;
		
		std::function<void(T&)> function;
		
		bool operator==(const Listener & other) const
		{
			return
				instance == other.instance &&
				method == other.method;
		}
	};
	
	std::vector<Listener> listeners;
	
	template <typename C, typename M>
	void add(C c, M m, int prio)
	{
		Listener listener;
		listener.instance = c;
		listener.method = &m;
		listener.function = std::bind(m, c, std::placeholders::_1);
		
		listeners.push_back(listener);
	}
	
	template <typename C, typename M>
	void remove(C c, M m, int prio)
	{
		Listener listener;
		listener.instance = c;
		listener.method = &m;
		listener.function = std::bind(m, c, std::placeholders::_1);
		
		for (auto i = listeners.begin(); i != listeners.end();)
		{
			//if (*i == listener)
			if (i->function.target_type() == listener.function.target_type() && i->instance == listener.instance)
				i = listeners.erase(i);
			else
				++i;
		}
	}
	
	void notify(T & args)
	{
		for (auto & listener : listeners)
		{
			listener.function(args);
		}
	}
};

struct ofCoreEvents
{
	ofEvent<ofResizeEventArgs> windowResized;
	ofEvent<ofKeyEventArgs> keyPressed;
	ofEvent<ofMouseEventArgs> mouseScrolled;
	ofEvent<ofEventArgs> update;
	ofEvent<ofEventArgs> draw;
};

ofCoreEvents & ofEvents();

template <typename Event, typename EventArgment, typename ListenerClass>
void ofAddListener(Event & event, ListenerClass  * listenerClass, void (ListenerClass::*listenerMethod)(EventArgment&), int prio = OF_EVENT_ORDER_AFTER_APP)
{
    event.remove(listenerClass, listenerMethod, prio);
    event.add(listenerClass, listenerMethod, prio);
}

template <typename Event, typename EventArgment, typename ListenerClass>
void ofRemoveListener(Event & event, ListenerClass  * listenerClass, void (ListenerClass::*listenerMethod)(EventArgment&), int prio = OF_EVENT_ORDER_AFTER_APP)
{
    event.remove(listenerClass, listenerMethod, prio);
}

#if 0

void ofxTick(const float dt)
{
	// todo : move event triggering to ofMain
	
	for (auto & e : keyboard.events)
	{
		if (e.type == SDL_KEYDOWN)
		{
			ofKeyEventArgs args;
			
			int c = e.key.keysym.sym;
			
			int caps = 0;
			
			if (e.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT))
				caps = +1;
			if (e.key.keysym.mod & (KMOD_CAPS))
				caps = caps ? -1 : +1;
			
			if (caps == -1)
				c = tolower(c);
			if (caps == +1)
				c = toupper(c);
			
			args.key = c;
			
			ofEvents().keyPressed.notify(args);
		}
	}
	
	{
		ofMouseEventArgs args;
		ofEvents().mouseScrolled.notify(args);
	}
	
	ofEventArgs e;
	
	ofEvents().update.notify(e);
}

void ofxDraw()
{
	// todo : move event triggering to ofMain
	
	ofEventArgs e;
	
	ofEvents().draw.notify(e);
}

#endif
