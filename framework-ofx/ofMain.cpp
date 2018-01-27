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

#include "framework.h"
#include "ofMain.h"
#include "Parse.h"
#include "StringEx.h"

struct Style
{
	ofColor color;
	float opacity;
	float lineWidth;
	
	Style()
		: color()
		, opacity(1.f)
		, lineWidth(1.f)
	{
	}
};

//

const int kMaxStyleStackSize = 32;

static Style sStyleStack[kMaxStyleStackSize];
static int sStyleStackDepth = 0;

//

static int colorValueToInt(const float v)
{
	return int(v);
}

ofColor ofColor::white(255, 255, 255, 255);
ofColor ofColor::black(0, 0, 0, 255);

ofColor ofColor::fromHex(const char * hex)
{
	const Color color = Color::fromHex(hex);
	
	return ofColor(
		colorValueToInt(color.r * 255.f),
		colorValueToInt(color.g * 255.f),
		colorValueToInt(color.b * 255.f),
		colorValueToInt(color.a * 255.f));
}

ofColor ofColor::fromHex(uint32_t hex)
{
	const int r = (hex >> 16) & 0xff;
	const int g = (hex >> 8 ) & 0xff;
	const int b = (hex >> 0 ) & 0xff;
	
	return ofColor(r, g, b);
}

int ofColor::getHex() const
{
	const int rh = (r << 16);
	const int gh = (g << 8 );
	const int bh = (b << 0 );
	
	return rh | gh | bh;
}

//

ofImage::ofImage()
	: sprite(nullptr)
{
}

ofImage::~ofImage()
{
	delete sprite;
	sprite = nullptr;
}

void ofImage::load(const std::string & filename)
{
	delete sprite;
	sprite = nullptr;
	
	sprite = new Sprite(filename.c_str());
}

void ofImage::draw(const float x, const float y, const float sx, const float sy) const
{
	const float scaleX = sx / sprite->getWidth();
	const float scaleY = sy / sprite->getHeight();
	
	sprite->drawEx(x, y, 0.f, scaleX, scaleY);
}

void ofImage::draw(const ofRectangle & rect) const
{
	draw(rect.x, rect.y, rect.width, rect.height);
}

//

void ofVbo::setColorData(const ofFloatColor * _colors, const int numColors, const int usage)
{
	colors.resize(numColors);
	for (int i = 0; i < numColors; ++i)
		colors[i] = _colors[i];
}

void ofVbo::setVertexData(const ofVec2f * _vertices, const int numVertices, const int usage)
{
	vertices.resize(numVertices);
	for (int i = 0; i < numVertices; ++i)
		vertices[i] = _vertices[i];
}

void ofVbo::draw(const int primitiveType, const int vertexOffset, const int numVertices)
{
	gxBegin(primitiveType);
	{
		const int numVerticesToDraw = std::min(int(vertices.size()) - vertexOffset, numVertices);
		
		for (int i = vertexOffset, j = 0; j < numVerticesToDraw; ++i, ++j)
		{
			if (i < colors.size())
				gxColor4f(colors[i].r, colors[i].g, colors[i].b, colors[i].a);
			
			gxVertex2f(vertices[i].x, vertices[i].y);
		}
	}
	gxEnd();
}

//

ofFbo::ofFbo()
	: surface(nullptr)
{
}

ofFbo::~ofFbo()
{
	delete surface;
	surface = nullptr;
}

void ofFbo::allocate(const int sx, const int sy)
{
	delete surface;
	surface = nullptr;
	
	surface = new Surface(sx, sy, false);
}

void ofFbo::begin()
{
	pushSurface(surface);
}

void ofFbo::end()
{
	popSurface();
}

void ofFbo::draw(const float x, const float y)
{
	// todo : draw surface contents onto screen ?
}

//

int ofGetVersionMajor()
{
	return 9;
}

int ofGetVersionMinor()
{
	return 0;
}

int ofGetVersionPatch()
{
	return 0;
}

int ofGetWidth()
{
	return framework.windowSx;
}

int ofGetHeight()
{
	return framework.windowSy;
}

int ofGetFrameRate()
{
	if (framework.timeStep == 0.f)
		return 0;
	else
		return int(1.f / framework.timeStep);
}

float ofGetElapsedTimef()
{
	return framework.time;
}

int ofGetMouseX()
{
	return mouse.x;
}

int ofGetMouseY()
{
	return mouse.y;
}

bool ofGetMousePressed()
{
	return mouse.isDown(BUTTON_LEFT);
}

void ofSetColor(const ofColor & color, const float opacity)
{
	sStyleStack[sStyleStackDepth].color = color;
	sStyleStack[sStyleStackDepth].opacity = opacity;
	
	setColor(color.r, color.g, color.b, colorValueToInt(color.a * opacity));
}

void ofSetLineWidth(const float width)
{
	sStyleStack[sStyleStackDepth].lineWidth = width;
	
	glLineWidth(width);
	checkErrorGL();
}

void ofPushStyle()
{
	sStyleStack[sStyleStackDepth + 1] = sStyleStack[sStyleStackDepth];
	sStyleStackDepth++;
}

void ofPopStyle()
{
	sStyleStackDepth--;
	
	ofSetColor(sStyleStack[sStyleStackDepth].color, sStyleStack[sStyleStackDepth].opacity);
	ofSetLineWidth(sStyleStack[sStyleStackDepth].lineWidth);
}

void ofClear(const uint8_t r, const uint8_t g, const uint8_t b, const uint8_t a)
{
	glClearColor(r, g, b, a);
	glClear(GL_COLOR_BUFFER_BIT);
}

void ofDrawRectangle(const float x, const float y, const float sx, const float sy)
{
	drawRect(x, y, x + sx, y + sy);
}

void ofDrawRectangle(const ofRectangle & rect)
{
	ofDrawRectangle(rect.x, rect.y, rect.width, rect.height);
}

void ofDrawLine(const float x1, const float y1, const float x2, const float y2)
{
	drawLine(x1, y1, x2, y2);
}

void ofDrawLine(const ofPoint & p1, const ofPoint & p2)
{
	ofDrawLine(p1.x, p1.y, p2.x, p2.y);
}

void ofDrawCircle(const ofPoint & p, const float radius)
{
	// todo : filled ?
	
	drawCircle(p.x, p.y, radius, 100);
}

void ofFill()
{
	// todo : ??
}

int ofToLower(const int c)
{
	return toupper(c);
}

int ofToUpper(const int c)
{
	return tolower(c);
}

std::string ofToLower(const std::string & s)
{
	// todo
	
	return s;
}

std::string ofToUpper(const std::string & s)
{
	// todo
	
	return s;
}

std::string ofToString(const double v)
{
	return String::ToString(v);
}

std::string ofToString(const double v, const int numDecimals)
{
	// todo
	
	return String::ToString(v);
}

float ofToFloat(const std::string & s)
{
	return Parse::Float(s);
}

int ofHexToInt(const std::string & s)
{
	return std::stoul(s, 0, 16);
}

float ofRandom(const float min, const float max)
{
	return random(min, max);
}

//

ofCoreEvents sEvents;

ofCoreEvents & ofEvents()
{
	return sEvents;
}
