#include "framework.h"
#include "particle.h"
#include "particle_editor.h"
#include "particle_framework.h"

#define UI_TEXTCHAT_CARET_PADDING_X 0
#define UI_TEXTCHAT_CARET_PADDING_Y 5 // todo : look these up
#define UI_TEXTCHAT_CARET_SX 4

static const int kMenuWidth = 300;
static const int kFontSize = 12;
static const int kTextBoxHeight = 20;
static const int kTextBoxTextOffset = 150;
static const int kCheckBoxHeight = 20;
static const int kEnumHeight = 20;
static const int kEnumSelectOffset = 150;
static const int kCurveHeight = 20;
static const int kColorHeight = 20;
static const int kColorCurveHeight = 20;
static const int kMenuSpacing = 30;
static const Color kBackgroundFocusColor(1.f, 1.f, 1.f, .5f);
static const Color kBackgroundColor(0.f, 0.f, 0.f, .5f);

static bool g_doActions = false;
static bool g_doDraw = false;
static int g_drawX = 0;
static int g_drawY = 0;

#define COLORWHEEL_X(sx) (kMenuWidth + kMenuSpacing + ColorWheel::size)
#define COLORWHEEL_Y(sy) (kMenuSpacing + ColorWheel::size)

static ParticleEmitterInfo g_pei;
static ParticleInfo g_pi;
static ParticlePool g_pool;
static ParticleEmitter g_pe;

static float lerp(const float v1, const float v2, const float t)
{
	return v1 * (1.f - t) + v2 * t;
}

static float saturate(const float v)
{
	return v < 0.f ? 0.f : v > 1.f ? 1.f : v;
}

template<typename T>
struct ScopedValueAdjust
{
	T m_oldValue;
	T & m_value;
	ScopedValueAdjust(T & value, T amount) : m_oldValue(value), m_value(value) { m_value += amount; }
	~ScopedValueAdjust() { m_value = m_oldValue; }
};

//

static bool isAllowed(int c)
{
	if (c >= 32 && c <= 126)
		return true;
	return false;
}

class TextField
{
public:
	TextField();

	bool tick(float dt);
	void draw(int ax, int ay, int asy);

	void open(int maxLength, bool canCancel, bool clearText);
	void close();
	bool isActive() const;

	void setText(const char * text);
	const char * getText() const;

private:
	void complete();
	void addChar(char c);
	void removeChar();

	static const int kMaxBufferSize = 256;

	bool m_isActive;
	bool m_canCancel;

	int m_maxBufferSize;

	char m_buffer[kMaxBufferSize + 1];
	int m_bufferSize;

	int m_caretPosition;
	float m_caretTimer;
};

TextField::TextField()
	: m_isActive(false)
	, m_canCancel(false)
	, m_maxBufferSize(0)
	, m_bufferSize(0)
	, m_caretPosition(0)
	, m_caretTimer(0.f)
{
	m_buffer[m_bufferSize] = 0;
}

bool TextField::tick(float dt)
{
	bool result = false;

	// SDL key codes are basically just ASCII codes, so that's easy!

	if (m_isActive)
	{
		fassert(m_caretPosition <= m_bufferSize);

		m_caretTimer += dt;

		for (int i = 0; i < 256; ++i)
		{
			SDLKey key = (SDLKey)i;

			if (keyboard.wentDown(key, true) && isAllowed(i))
			{
				int c = i;

				if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
				{
					c = toupper(c);
				}

				addChar(c);
			}
		}

		if (keyboard.wentDown(SDLK_HOME))
			m_caretPosition = 0;
		if (keyboard.wentDown(SDLK_END))
			m_caretPosition = m_bufferSize;

		if (keyboard.wentDown(SDLK_LEFT, true) && m_caretPosition > 0)
			m_caretPosition--;
		if (keyboard.wentDown(SDLK_RIGHT, true) && m_caretPosition < m_bufferSize)
			m_caretPosition++;

		if (keyboard.wentDown(SDLK_DELETE, true) && m_caretPosition < m_bufferSize)
		{
			m_caretPosition++;
			removeChar();
		}

		if (keyboard.wentDown(SDLK_BACKSPACE, true) && m_caretPosition > 0)
			removeChar();

		if (keyboard.wentDown(SDLK_RETURN))
		{
			if (m_bufferSize > 0)
			{
				result = true;

				complete();
			}
			else if (m_canCancel)
			{
				close();
			}
		}
		else if (keyboard.wentDown(SDLK_ESCAPE) && m_canCancel)
		{
			close();
		}
	}

	return result;
}

void TextField::draw(int ax, int ay, int asy)
{
	//if (m_isActive)
	{
		const char * text = getText();

		// draw text

		if (m_isActive)
			setColor(127, 227, 255);
		else
			setColor(127, 127, 127);
		setFont("calibri.ttf");
		drawText(ax, ay, kFontSize, +1.f, +1.f, "%s", text);

		// draw caret

		if (m_isActive && std::fmodf(m_caretTimer, .5f) < .25f)
		{
			char textToCaretPosition[kMaxBufferSize + 1];
			strcpy_s(textToCaretPosition, sizeof(textToCaretPosition), text);

			textToCaretPosition[m_caretPosition] = 0;

			float sx, sy;
			measureText(kFontSize, sx, sy, "%s", textToCaretPosition);

			const int x = ax + sx + UI_TEXTCHAT_CARET_PADDING_X;

			setColor(63, 127, 255);
			drawRect(x, ay + UI_TEXTCHAT_CARET_PADDING_Y, x + UI_TEXTCHAT_CARET_SX, ay + asy - UI_TEXTCHAT_CARET_PADDING_Y * 2);
		}

		setColor(colorWhite);
	}
}

void TextField::open(int maxLength, bool canCancel, bool clearText)
{
	fassert(!m_isActive);

	m_isActive = true;
	m_canCancel = canCancel;

	m_maxBufferSize = maxLength < kMaxBufferSize ? maxLength : kMaxBufferSize;
	m_buffer[m_maxBufferSize] = 0;

	if (clearText)
	{
		m_bufferSize = 0;
		m_buffer[m_bufferSize] = 0;
	}

	m_caretPosition = 0;
	m_caretTimer = 0.f;
}

void TextField::close()
{
	fassert(m_isActive);

	m_isActive = false;
	m_canCancel = false;

	m_maxBufferSize = 0;
	m_bufferSize = 0;

	m_caretPosition = 0;
	m_caretTimer = 0.f;
}

bool TextField::isActive() const
{
	return m_isActive;
}

void TextField::setText(const char * text)
{
	strcpy_s(m_buffer, m_maxBufferSize, text);
	m_bufferSize = strlen(m_buffer);
}

const char * TextField::getText() const
{
	return m_buffer;
}

void TextField::complete()
{
	close();
}

void TextField::addChar(char c)
{
	fassert(m_isActive);

	if (m_bufferSize < kMaxBufferSize && m_bufferSize < m_maxBufferSize)
	{
		if (m_caretPosition < m_bufferSize)
			memcpy(&m_buffer[m_caretPosition + 1], &m_buffer[m_caretPosition], m_bufferSize - m_caretPosition);

		m_buffer[m_caretPosition] = c;

		m_bufferSize++;
		m_buffer[m_bufferSize] = 0;
		m_caretPosition++;
	}
}

void TextField::removeChar()
{
	fassert(m_isActive);

	if (m_caretPosition > 0)
	{
		memcpy(&m_buffer[m_caretPosition - 1], &m_buffer[m_caretPosition], m_bufferSize - 1);

		m_bufferSize--;
		m_buffer[m_bufferSize] = 0;
		m_caretPosition--;
	}
}

//

class ColorWheel
{
	static void getTriangleAngles(float a[3]);
	static void getTriangleCoords(float xy[6]);

public:
	static const int size = 100;
	static const int wheelThickness = 20;

	float hue;
	float baryWhite;
	float baryBlack;

	enum State
	{
		kState_Idle,
		kState_SelectHue,
		kState_SelectSatValue
	};

	State state;

	ColorWheel();

	void tick(const float mouseX, const float mouseY, const bool mouseDown, const float dt);
	void draw();

	bool intersectWheel(const float x, const float y, float & hue) const;
	bool intersectTriangle(const float x, const float y, float & saturation, float & value) const;

	void fromColor(ParticleColor & color);
	void toColor(const float hue, const float saturation, const float value, ParticleColor & color) const;
	void toColor(ParticleColor & color) const;
};

void ColorWheel::getTriangleAngles(float a[3])
{
	a[0] = 2.f * float(M_PI) / 3.f * 0.f;
	a[1] = 2.f * float(M_PI) / 3.f * 1.f;
	a[2] = 2.f * float(M_PI) / 3.f * 2.f;
}

void ColorWheel::getTriangleCoords(float xy[6])
{
	const float r = size - wheelThickness;

	float a[3];
	getTriangleAngles(a);

	for (int i = 0; i < 3; ++i)
	{
		xy[i * 2 + 0] = cosf(a[i]) * r;
		xy[i * 2 + 1] = sinf(a[i]) * r;
	}
}

ColorWheel::ColorWheel()
	: hue(0.f)
	, baryWhite(0.f)
	, baryBlack(0.f)
	, state(kState_Idle)
{
}

void ColorWheel::tick(const float mouseX, const float mouseY, const bool mouseDown, const float dt)
{
	switch (state)
	{
	case kState_Idle:
		{
			float h, bw, bb;

			if (mouseDown && intersectWheel(mouseX, mouseY, h))
			{
				state = kState_SelectHue;

				hue = h;
			}
			else if (mouseDown && intersectTriangle(mouseX, mouseY, bw, bb))
			{
				state = kState_SelectSatValue;

				baryWhite = bw;
				baryBlack = bb;
			}
		}
		break;

	case kState_SelectHue:
		{
			if (mouseDown)
				intersectWheel(mouseX, mouseY, hue);
			else
				state = kState_Idle;
		}
		break;

	case kState_SelectSatValue:
		{
			if (mouseDown)
				intersectTriangle(mouseX, mouseY, baryWhite, baryBlack);
			else
				state = kState_Idle;
		}
		break;
	}
}

void ColorWheel::draw()
{
	const int numSteps = 100;
	const float r1 = size;
	const float r2 = size - wheelThickness;

	// color wheel

	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numSteps; ++i)
		{
			ParticleColor c;
			toColor(i / float(numSteps) * 2.f * M_PI, 0.f, 0.f, c);
			gxColor4f(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);

			const float a1 = (i + 0) / float(numSteps) * 2.f * M_PI;
			const float a2 = (i + 1) / float(numSteps) * 2.f * M_PI;
			const float x1 = cosf(a1);
			const float y1 = sinf(a1);
			const float x2 = cosf(a2);
			const float y2 = sinf(a2);

			gxVertex2f(x1 * r1, y1 * r1);
			gxVertex2f(x2 * r1, y2 * r1);
			gxVertex2f(x2 * r2, y2 * r2);
			gxVertex2f(x1 * r2, y1 * r2);
		}
	}
	gxEnd();

	// selection circle on the color wheel

	{
		setColor(colorWhite);
		const float x = cosf(hue) * (size - wheelThickness / 2.f);
		const float y = sinf(hue) * (size - wheelThickness / 2.f);
		Sprite("circle.png", 7.5f, 7.5f).drawEx(x, y);
	}

	// lightness triangle

	{
		gxBegin(GL_TRIANGLES);
		{
			float xy[6];
			getTriangleCoords(xy);

			ParticleColor c;
			toColor(hue, 0.f, 0.f, c);
			gxColor4f(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
			gxVertex2f(xy[0], xy[1]);

			gxColor4f(0.f, 0.f, 0.f, 1.f);
			gxVertex2f(xy[2], xy[3]);

			gxColor4f(1.f, 1.f, 1.f, 1.f);
			gxVertex2f(xy[4], xy[5]);
		}
		gxEnd();
	}

	{
		float xy[6];
		getTriangleCoords(xy);

		setColor(colorWhite);
		const float x = xy[0] + (xy[2] - xy[0]) * baryBlack + (xy[4] - xy[0]) * baryWhite;
		const float y = xy[1] + (xy[3] - xy[1]) * baryBlack + (xy[5] - xy[1]) * baryWhite;
		Sprite("circle.png", 7.5f, 7.5f).drawEx(x, y);
	}

#if 1 // fixme : work around for weird (driver?) issue where next draw call retains the color of the previous one
	gxBegin(GL_TRIANGLES);
	gxColor4f(0.f, 0.f, 0.f, 0.f);
	gxEnd();
#endif

#if 0
	{
		ParticleColor c;
		toColor(c);
		setColorf(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
		drawRect(0.f, 0.f, 100.f, 20.f);
		setFont("calibri.ttf");
		drawText(0.f, 0.f, 18, +1.f, +1.f, "%f, %f, %f", c.rgba[0], c.rgba[1], c.rgba[2]);
	}
#endif
}

bool ColorWheel::intersectWheel(const float x, const float y, float & hue) const
{
	const float dSq = x * x + y * y;
	if (dSq != 0.f)
		hue = atan2f(y, x);
	if (dSq > size * size)
		return false;
	if (dSq < (size - wheelThickness) * (size - wheelThickness))
		return false;
	return true;
}

static const float computeEdgeValue(const float x1, const float y1, const float x2, const float y2, const float x, const float y)
{
	const float dx = x2 - x1;
	const float dy = y2 - y1;
	const float s = sqrtf(dx * dx + dy * dy);
	if (s == 0.f)
		return 0.f;
	else
	{
		const float nx = dx / s;
		const float ny = dy / s;
		const float d1 = x1 * nx + y1 * ny;
		const float d2 = x  * nx + y  * ny;
		const float t = (d2 - d1) / s;
		return t;
	}
}

static void computeBarycentric(const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const float x, const float y, float & b1, float & b2, float & b3)
{
	b1 = ((y2 - y3) * (x - x3) + (x3 - x2) * (y - y3)) / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
	b2 = ((y3 - y1) * (x - x3) + (x1 - x3) * (y - y3)) / ((y2 - y3) * (x1 - x3) + (x3 - x2) * (y1 - y3));
	b3 = 1.f - b1 - b2;
}

bool ColorWheel::intersectTriangle(const float x, const float y, float & baryWhite, float & baryBlack) const
{
	float xy[6];
	getTriangleCoords(xy);

	float temp;
	computeBarycentric(
		xy[0], xy[1],
		xy[2], xy[3],
		xy[4], xy[5],
		x, y, temp, baryBlack, baryWhite);

	const bool inside =
		baryWhite >= 0.f && baryWhite <= 1.f &&
		baryBlack >= 0.f && baryBlack <= 1.f;

	baryWhite = std::max(0.f, baryWhite);
	baryBlack = std::max(0.f, baryBlack);

	const float bs = baryWhite + baryBlack;
	if (bs > 1.f)
	{
		baryWhite /= bs;
		baryBlack /= bs;
	}

#if 0
	setFont("calibri.ttf");
	setColor(colorWhite);
	drawText(xy[2], xy[3], 16, +1.f, +1.f, "bb: %f", baryWhite);
	drawText(xy[4], xy[5], 16, +1.f, +1.f, "bw: %f", baryBlack);
#endif

	return inside;
}

void ColorWheel::fromColor(ParticleColor & color)
{
	// todo
}

void ColorWheel::toColor(const float hue, const float baryWhite, const float baryBlack, ParticleColor & color) const
{
	const float baryColor = 1.f - baryWhite - baryBlack;

	float r = (cosf(hue + 2.f * M_PI / 3.f * 0.f) + 1.f) / 2.f;
	float g = (cosf(hue + 2.f * M_PI / 3.f * 1.f) + 1.f) / 2.f;
	float b = (cosf(hue + 2.f * M_PI / 3.f * 2.f) + 1.f) / 2.f;

	r = r * baryColor + 1.f * baryWhite + 0.f * baryBlack;
	g = g * baryColor + 1.f * baryWhite + 0.f * baryBlack;
	b = b * baryColor + 1.f * baryWhite + 0.f * baryBlack;

	color.set(r, g, b, 1.f);
}

void ColorWheel::toColor(ParticleColor & color) const
{
	toColor(hue, baryWhite, baryBlack, color);
}

//

static ColorWheel g_colorWheel;
static ParticleColor * g_activeColor = 0;

//

static bool doButton(const char * name, bool & hasFocus)
{
	const int kPadding = 5;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCheckBoxHeight;

	g_drawY += kCheckBoxHeight;

	bool result = false;

	if (g_doActions)
	{
		if (mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			hasFocus = true;
		}
		else
		{
			hasFocus = false;
		}

		if (mouse.wentUp(BUTTON_LEFT) && hasFocus)
		{
			result = true;
		}
	}

	if (g_doDraw)
	{
		if (hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);
	}

	return result;
}

static bool valueToString(const std::string & src, char * dst, int dstSize)
{
	sprintf_s(dst, dstSize, "%s", src.c_str());
	return true;
}

static bool valueToString(int src, char * dst, int dstSize)
{
	sprintf_s(dst, dstSize, "%d", src);
	return true;
}

static bool valueToString(float src, char * dst, int dstSize)
{
	sprintf_s(dst, dstSize, "%g", src);
	return true;
}

static bool stringToValue(const char * src, std::string & dst)
{
	dst = src;
	return true;
}

static bool stringToValue(const char * src, int & dst)
{
	dst = atoi(src);
	return true;
}

static bool stringToValue(const char * src, float & dst)
{
	dst = (float)atof(src);
	return true;
}

template <typename T>
static void doTextBox(T & value, const char * name, float xOffset, float xScale, bool lineBreak, bool & hasFocus, TextField & textField, bool & textFieldIsInit)
{
	const int kPadding = 5;

	const int x1 = g_drawX + kMenuWidth * xOffset;
	const int x2 = g_drawX + kMenuWidth * xOffset + kMenuWidth * xScale;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kTextBoxHeight;

	if (lineBreak)
		g_drawY += kTextBoxHeight;

	if (g_doActions)
	{
		if (!textFieldIsInit)
		{
			textFieldIsInit = true;

			textField.open(32, false, false);

			char temp[32];
			valueToString(value, temp, sizeof(temp));
			textField.setText(temp);

			textField.close();
		}

		const bool hadFocus = hasFocus;

		if (mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			hasFocus = true;
		}
		else
		{
			hasFocus = false;
		}

		if (hasFocus != hadFocus)
		{
			if (hasFocus)
			{
				textField.open(32, false, false);

				char temp[32];
				valueToString(value, temp, sizeof(temp));
				textField.setText(temp);
			}
			else if (textField.isActive())
				textField.close();
		}

		if (textField.isActive())
		{
			textField.tick(1.f / 60.f); // fixme : timeStep

			stringToValue(textField.getText(), value);
		}
	}

	if (g_doDraw)
	{
		if (hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);

		textField.draw(x1 + kPadding + kTextBoxTextOffset * xScale, y1, y2 - y1);
	}
}

static bool doCheckBox(bool & value, const char * name, bool isCollapsable, bool & hasFocus)
{
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCheckBoxHeight;

	g_drawY += kCheckBoxHeight;

	const int cx1 = x1 + kPadding;
	const int cx2 = x1 + kPadding + kCheckButtonSize;
	const int cy1 = y1 + kPadding;
	const int cy2 = y2 - kPadding;

	if (g_doActions)
	{
		if (mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			hasFocus = true;
		}
		else
		{
			hasFocus = false;
		}

		if (mouse.wentUp(BUTTON_LEFT) &&
			mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			value = !value;
		}
	}

	if (g_doDraw)
	{
		if (hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorGreen);
		if (value)
			drawRect(cx1, cy1, cx2, cy2);
		setColor(colorWhite);
		drawRectLine(cx1, cy1, cx2, cy2);

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);

		if (isCollapsable)
		{
			setColor(colorWhite);
			drawLine(
				x2 - kPadding - kCheckButtonSize,
				(y1 + y2) / 2,
				x2 - kPadding,
				(y1 + y2) / 2);

			if (!value)
			{
				drawLine(
					x2 - kPadding - kCheckButtonSize / 2,
					y1 + kPadding,
					x2 - kPadding - kCheckButtonSize / 2,
					y2 - kPadding);
			}
		}
	}

	return value;
}

struct EnumValue
{
	EnumValue()
	{
	}

	EnumValue(int aValue, std::string aName)
		: value(aValue)
		, name(aName)
	{
	}

	int value;
	std::string name;
};

template <typename E>
static void doEnum(E & value, const char * name, const std::vector<EnumValue> & enumValues, bool & hasFocus)
{
	const int kPadding = 5;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kEnumHeight;

	g_drawY += kEnumHeight;

	if (g_doActions)
	{
		if (mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			hasFocus = true;
		}
		else
		{
			hasFocus = false;
		}

		if (hasFocus)
		{
			// cycle prev/next

			int direction = 0;

			if (mouse.wentDown(BUTTON_LEFT))
				direction = +1;
			if (mouse.wentDown(BUTTON_RIGHT))
				direction = -1;

			if (direction != 0)
			{
				int oldIndex = -1;
				for (size_t i = 0; i < enumValues.size(); ++i)
					if (enumValues[i].value == value)
						oldIndex = i;
				if (oldIndex != -1)
					value = (E)enumValues[(oldIndex + enumValues.size() + direction) % enumValues.size()].value;
			}
		}
	}

	if (g_doDraw)
	{
		if (hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);

		int index = -1;
		for (size_t i = 0; i < enumValues.size(); ++i)
			if (enumValues[i].value == value)
				index = i;
		if (index != -1)
			drawText(x1 + kPadding + kEnumSelectOffset, y1, kFontSize, +1.f, +1.f, "%s", enumValues[index].name.c_str());
	}
}

void doParticleCurve(ParticleCurve & curve, const char * name, bool & hasFocus)
{
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCurveHeight;

	g_drawY += kCurveHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		if (mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			hasFocus = true;
		}
		else
		{
			hasFocus = false;
		}

		// todo : curve type selection
	}

	if (g_doDraw)
	{
		if (hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);
	}
}

void doParticleColor(ParticleColor & color, const char * name, bool & hasFocus)
{
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kColorHeight;

	g_drawY += kColorHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		if (mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			hasFocus = true;
		}
		else
		{
			hasFocus = false;
		}

		// todo : add, remove, move keys and select color
		if (hasFocus && mouse.wentDown(BUTTON_LEFT))
		{
			g_activeColor = &color;
		}
	}

	if (g_doDraw)
	{
		if (hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorBlack); // todo : checkers pattern
		drawRect(cx1, cy1, cx2, cy2);
		for (int x = cx1; x < cx2; ++x)
		{
			const float t = (x - cx1 + .5f) / float(cx2 - cx1 - .5f);
			setColorf(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
			drawRect(x, cy1, x + 1.f, cy2);
		}

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);
	}
}

static float screenToCurve(int x1, int x2, int x)
{
	return saturate((x - x1) / float(x2 - x1));
}

static float curveToScreen(int x1, int x2, float t)
{
	return x1 + (x2 - x1) * t;
}

static ParticleColorCurve::Key * findNearestKey(ParticleColorCurve & curve, float t, float maxDeviation)
{
	ParticleColorCurve::Key * nearestKey = 0;
	float nearestDistance = 0.f;

	for (int i = 0; i < curve.numKeys; ++i)
	{
		const float dt = curve.keys[i].t - t;
		const float distance = std::sqrtf(dt * dt);

		if (distance < maxDeviation && (distance < nearestDistance || nearestKey == 0))
		{
			nearestKey = &curve.keys[i];
			nearestDistance = distance;
		}
	}

	return nearestKey;
}

void doParticleColorCurve(ParticleColorCurve & curve, const char * name, bool & hasFocus, ParticleColorCurve::Key *& selectedKey)
{
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;
	const float kMaxSelectionDeviation = 5 / float(kMenuWidth);

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kColorCurveHeight;

	g_drawY += kColorCurveHeight;

	const int cx1 = x1;
	const int cy1 = y1;
	const int cx2 = x2;
	const int cy2 = y2;

	if (g_doActions)
	{
		if (mouse.x >= x1 &&
			mouse.y >= y1 &&
			mouse.x <= x2 &&
			mouse.y <= y2)
		{
			hasFocus = true;
		}
		else
		{
			hasFocus = false;
		}

		if (hasFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				// select or insert key

				const float t = screenToCurve(x1, x2, mouse.x);
				auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
				
				if (key)
				{
					g_colorWheel.fromColor(key->color);
				}
				else
				{
					// insert a new key

					if (curve.allocKey(key))
					{
						g_colorWheel.toColor(key->color);
						key->t = t;

						key = curve.sortKeys(key);
					}
				}

				selectedKey = key;
			}

			if (mouse.wentDown(BUTTON_RIGHT) && !mouse.isDown(BUTTON_LEFT))
			{
				selectedKey = 0;

				// erase key

				const float t = screenToCurve(x1, x2, mouse.x);
				auto * key = findNearestKey(curve, t, kMaxSelectionDeviation);

				if (key)
				{
					curve.freeKey(key);
				}
			}

			if (selectedKey && mouse.isDown(BUTTON_LEFT))
			{
				// move selected key around

				const float t = screenToCurve(x1, x2, mouse.x);

				selectedKey->t = t;
				selectedKey = curve.sortKeys(selectedKey);

				fassert(selectedKey->t == t);
			}

			g_activeColor = selectedKey ? &selectedKey->color : 0;
		}
	}

	if (g_doDraw)
	{
		if (hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorBlack); // todo : checkers pattern
		drawRect(cx1, cy1, cx2, cy2);
		for (int x = cx1; x < cx2; ++x)
		{
			const float t = (x - cx1 + .5f) / float(cx2 - cx1 - .5f);
			ParticleColor c;
			curve.sample(t, c);
			setColorf(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
			drawRect(x, cy1, x + 1.f, cy2);
		}

		for (int i = 0; i < curve.numKeys; ++i)
		{
			if (selectedKey == &curve.keys[i])
				setColor(colorYellow);
			else
				setColor(colorWhite);
			const float x = curveToScreen(x1, x2, curve.keys[i].t);
			const float y = (y1 + y2) / 2.f;
			Sprite("circle.png", 7.5f, 7.5f).drawEx(x, y);
		}

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);
	}
}

#define DO_TEXTBOX(sym, value, name) \
	static bool sym ## hasFocus = false; \
	static TextField sym ## textField; \
	static bool sym ## textFieldIsInit = false; \
	doTextBox(value, name, 0.f, 1.f, true, sym ## hasFocus, sym ## textField, sym ## textFieldIsInit);

#define DO_TEXTBOX2(sym, value, xOffset, xScale, lineBreak, name) \
	static bool sym ## hasFocus = false; \
	static TextField sym ## textField; \
	static bool sym ## textFieldIsInit = false; \
	doTextBox(value, name, xOffset, xScale, lineBreak, sym ## hasFocus, sym ## textField, sym ## textFieldIsInit);

#define DO_CHECKBOX(sym, value, name, isCollapsable) \
	static bool sym ## hasFocus = false; \
	doCheckBox(value, name, isCollapsable, sym ## hasFocus);

#define DO_ENUM(sym, value, enumValues, name) \
	static bool sym ## hasFocus = false; \
	doEnum(value, name, enumValues, sym ## hasFocus);

#define DO_PARTICLECURVE(sym, value, name) \
	static bool sym ## hasFocus = false; \
	doParticleCurve(value, name, sym ## hasFocus);

#define DO_PARTICLECOLORCURVE(sym, value, name) \
	static bool sym ## hasFocus = false; \
	static ParticleColorCurve::Key * sym ## selectedKey = 0; \
	doParticleColorCurve(value, name, sym ## hasFocus, sym ## selectedKey);

static void doMenu(bool doActions, bool doDraw)
{
	g_doActions = doActions;
	g_doDraw = doDraw;

	setFont("calibri.ttf");

	g_drawX = 10;
	g_drawY = 0;

	DO_TEXTBOX(rate, g_pi.rate, "Rate (Particles/Sec)");

	/*
	struct Burst
	{
		Burst()
			: time(0.f)
			, numParticles(10)
		{
		}

		float time;
		int numParticles;
	} bursts[kMaxBursts];
	int numBursts; // Allows extra particles to be emitted at specified times (only available when the Rate is in Time mode).
	*/

	std::vector<EnumValue> shapeValues;
	shapeValues.push_back(EnumValue('e', "Edge"));
	shapeValues.push_back(EnumValue('b', "Box"));
	shapeValues.push_back(EnumValue('c', "Circle"));
	DO_ENUM(shape, g_pi.shape, shapeValues, "Shape");

	DO_CHECKBOX(randomDirection, g_pi.randomDirection, "Random Direction", false);
	DO_TEXTBOX(circleRadius, g_pi.circleRadius, "Circle Radius");
	DO_TEXTBOX(boxSizeX, g_pi.boxSizeX, "Box Width");
	DO_TEXTBOX(boxSizeY, g_pi.boxSizeY, "Box Height");
	DO_CHECKBOX(emitFromShell, g_pi.emitFromShell, "Emit From Shell", false);

	static bool velocityOverLifetimeHasFocus = false;
	if (doCheckBox(g_pi.velocityOverLifetime, "Velocity Over Lifetime", true, velocityOverLifetimeHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX2(velocityOverLifetimeValueX, g_pi.velocityOverLifetimeValueX, 0.f, .5f, false, "X");
		DO_TEXTBOX2(velocityOverLifetimeValueY, g_pi.velocityOverLifetimeValueY, .5f, .5f, true, "Y");
	}

	static bool velocityOverLifetimeLimitHasFocus = false;
	if (doCheckBox(g_pi.velocityOverLifetimeLimit, "Velocity Over Lifetime Limit", true, velocityOverLifetimeLimitHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(velocityOverLifetimeLimitCurve, g_pi.velocityOverLifetimeLimitCurve, "Curve");
		DO_TEXTBOX(velocityOverLifetimeLimitDampen, g_pi.velocityOverLifetimeLimitDampen, "Dampen/Sec");
	}

	static bool forceOverLifetimeHasFocus = false;
	if (doCheckBox(g_pi.forceOverLifetime, "Force Over Lifetime", true, forceOverLifetimeHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX2(forceOverLifetimeValueX, g_pi.forceOverLifetimeValueX, 0.f, .5f, false, "X");
		DO_TEXTBOX2(forceOverLifetimeValueY, g_pi.forceOverLifetimeValueY, .5f, .5f, true, "Y");
	}

	static bool colorOverLifetimeHasFocus = false;
	if (doCheckBox(g_pi.colorOverLifetime, "Color Over Lifetime", true, colorOverLifetimeHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECOLORCURVE(colorOverLifetimeCurve, g_pi.colorOverLifetimeCurve, "Curve");
	}

	static bool colorBySpeedHasFocus = false;
	if (doCheckBox(g_pi.colorBySpeed, "Color By Speed", true, colorBySpeedHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECOLORCURVE(colorBySpeedCurve, g_pi.colorBySpeedCurve, "Curve");
		DO_TEXTBOX2(colorBySpeedRangeMin, g_pi.colorBySpeedRangeMin, 0.f, .5f, false, "Range");
		DO_TEXTBOX2(colorBySpeedRangeMax, g_pi.colorBySpeedRangeMax, .5f, .5f, true, "");
	}

	static bool sizeOverLifetimeHasFocus = false;
	if (doCheckBox(g_pi.sizeOverLifetime, "Size Over Lifetime", true, sizeOverLifetimeHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(sizeOverLifetimeCurve, g_pi.sizeOverLifetimeCurve, "Curve");
	}

	static bool sizeBySpeedHasFocus = false;
	if (doCheckBox(g_pi.sizeBySpeed, "Size By Speed", true, sizeBySpeedHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(sizeBySpeedCurve, g_pi.sizeBySpeedCurve, "Curve");
		DO_TEXTBOX2(sizeBySpeedRangeMin, g_pi.sizeBySpeedRangeMin, 0.f, .5f, false, "Range");
		DO_TEXTBOX2(sizeBySpeedRangeMax, g_pi.sizeBySpeedRangeMax, .5f, .5f, true, "");
	}

	static bool rotationOverLifetimeHasFocus = false;
	if (doCheckBox(g_pi.rotationOverLifetime, "Rotation Over Lifetime", true, rotationOverLifetimeHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX(rotationOverLifetimeValue, g_pi.rotationOverLifetimeValue, "Degrees/Sec");
	}

	static bool rotationBySpeedHasFocus = false;
	if (doCheckBox(g_pi.rotationBySpeed, "Rotation By Speed", true, rotationBySpeedHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(rotationBySpeedCurve, g_pi.rotationBySpeedCurve, "Curve");
		DO_TEXTBOX2(rotationBySpeedRangeMin, g_pi.rotationBySpeedRangeMin, 0.f, .5f, false, "Range");
		DO_TEXTBOX2(rotationBySpeedRangeMax, g_pi.rotationBySpeedRangeMax, .5f, .5f, true, "");
	}

	static bool collisionHasFocus = false;
	if (doCheckBox(g_pi.collision, "Collision", true, collisionHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX(bounciness, g_pi.bounciness, "Bounciness");
		DO_TEXTBOX(lifetimeLoss, g_pi.lifetimeLoss, "Lifetime Loss On Collision");
		DO_TEXTBOX(minKillSpeed, g_pi.minKillSpeed, "Kill Speed");
		DO_TEXTBOX(collisionRadius, g_pi.collisionRadius, "Collision Radius");
	}

	static bool subEmittersHasFocus = false;
	if (doCheckBox(g_pi.subEmitters, "Sub Emitters", true, subEmittersHasFocus))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		//SubEmitter onBirth;
		//SubEmitter onCollision;
		//SubEmitter onDeath;
	}

	std::vector<EnumValue> sortModeValues;
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_OldestFirst, "Oldest First"));
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_YoungestFirst, "Youngest First"));
	DO_ENUM(sortMode, g_pi.sortMode, sortModeValues, "Sort Mode");

	std::vector<EnumValue> blendModeValues;
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_AlphaBlended, "Use Alpha"));
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_Additive, "Additive"));
	DO_ENUM(blendMode, g_pi.blendMode, blendModeValues, "Blend Mode");

	g_drawY += kMenuSpacing;

	DO_TEXTBOX(duration, g_pei.duration, "Duration");
	DO_CHECKBOX(loop, g_pei.loop, "Loop", false);
	DO_CHECKBOX(prewarm, g_pei.prewarm, "Prewarm", false);
	DO_TEXTBOX(startDelay, g_pei.startDelay, "Start Delay");
	DO_TEXTBOX(startLifetime, g_pei.startLifetime, "Start Lifetime");
	DO_TEXTBOX(startSpeed, g_pei.startSpeed, "Start Speed");
	DO_TEXTBOX(startSize, g_pei.startSize, "Start Size");
	DO_TEXTBOX(startRotation, g_pei.startRotation, "Start Rotation"); // todo : min and max values for random start rotation?
	static bool startColorHasFocus = false;
	doParticleColor(g_pei.startColor, "Start Color", startColorHasFocus);
	DO_TEXTBOX(gravityMultiplier, g_pei.gravityMultiplier, "Gravity Multiplier");
	DO_CHECKBOX(inheritVelocity, g_pei.inheritVelocity, "Inherit Velocity", false);
	DO_CHECKBOX(worldSpace, g_pei.worldSpace, "World Space", false);
	DO_TEXTBOX(maxParticles, g_pei.maxParticles, "Max Particles");
	// todo : char materialName[32];

	// right side menu

	setFont("calibri.ttf");

	g_drawX = 10 + kMenuWidth + 10;
	g_drawY = 0;

	static bool loadHasFocus = false;
	if (doButton("Load", loadHasFocus))
	{
	}

	static bool saveHasFocus = false;
	if (doButton("Save", saveHasFocus))
	{
	}

	static bool saveAsHasFocus = false;
	if (doButton("Save as..", saveAsHasFocus))
	{
	}
}

void particleEditorTick(bool menuActive, float dt)
{
	if (menuActive)
		doMenu(true, false);

	const float gravityX = 0.f;
	const float gravityY = 100.f;
	for (Particle * p = g_pool.head; p; )
		if (!tickParticle(g_pei, g_pi, dt, gravityX, gravityY, *p))
			p = g_pool.freeParticle(p);
		else
			p = p->next;
	tickParticleEmitter(g_pei, g_pi, g_pool, dt, gravityX, gravityY, g_pe);
}

void particleEditorDraw(bool menuActive, float sx, float sy)
{
	if (menuActive)
		doMenu(false, true);

	gxPushMatrix();
	gxTranslatef(sx/2.f, sy/2.f, 0.f);

#if 1
	setColor(0, 255, 0, 31);
	switch (g_pi.shape)
	{
	case ParticleInfo::kShapeBox:
		drawRectLine(
			-g_pi.boxSizeX,
			-g_pi.boxSizeY,
			+g_pi.boxSizeX,
			+g_pi.boxSizeY);
		break;
	case ParticleInfo::kShapeCircle:
		drawRectLine(
			-g_pi.circleRadius,
			-g_pi.circleRadius,
			+g_pi.circleRadius,
			+g_pi.circleRadius);
		break;
	case ParticleInfo::kShapeEdge:
		drawLine(
			-g_pi.boxSizeX,
			0.f,
			+g_pi.boxSizeX,
			0.f);
		break;
	}

	strcpy_s(g_pei.materialName, sizeof(g_pei.materialName), "texture.png");

	gxSetTexture(Sprite(g_pei.materialName).getTexture());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (g_pi.blendMode == ParticleInfo::kBlendMode_AlphaBlended)
		setBlend(BLEND_ALPHA);
	else if (g_pi.blendMode == ParticleInfo::kBlendMode_Additive)
		setBlend(BLEND_ADD);
	else
		fassert(false);

	for (Particle * p = (g_pi.sortMode == ParticleInfo::kSortMode_OldestFirst) ? g_pool.head : g_pool.tail;
		         p; p = (g_pi.sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
	{
		const float particleLife = 1.f - p->life;
		const float particleSpeed = std::sqrtf(p->speed[0] * p->speed[0] + p->speed[1] * p->speed[1]);

		ParticleColor color;
		computeParticleColor(g_pei, g_pi, particleLife, particleSpeed, color);
		const float size = computeParticleSize(g_pei, g_pi, particleLife, particleSpeed);
		gxPushMatrix();
		gxTranslatef(
			p->position[0],
			p->position[1],
			0.f);
		gxRotatef(p->rotation, 0.f, 0.f, 1.f);
		setColorf(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
		drawRect(
			- size / 2.f,
			- size / 2.f,
			+ size / 2.f,
			+ size / 2.f);
		gxPopMatrix();
	}
	gxSetTexture(0);

	setBlend(BLEND_ALPHA);
#endif

	gxPopMatrix();

	if (menuActive && g_activeColor)
	{
	#if 1
		gxPushMatrix();
		const float wheelX = COLORWHEEL_X(sx);
		const float wheelY = COLORWHEEL_Y(sy);
		gxTranslatef(wheelX, wheelY, 0.f);

		g_colorWheel.tick(mouse.x - wheelX, mouse.y - wheelY, mouse.isDown(BUTTON_LEFT), 1.f / 60.f); // fixme : mouseDown and dt
		g_colorWheel.toColor(*g_activeColor);

		g_colorWheel.draw();

	#if 1 // fixme : work around for weird (driver?) issue where next draw call retains the color of the previous one
		gxBegin(GL_TRIANGLES);
		gxColor4f(0.f, 0.f, 0.f, 0.f);
		gxEnd();
	#endif

		gxBegin(GL_QUADS);
		{
			ParticleColor c;
			g_colorWheel.toColor(c);
			gxColor4f(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
			gxVertex2f(-g_colorWheel.size, g_colorWheel.size);
			gxVertex2f(+g_colorWheel.size, g_colorWheel.size);
			gxVertex2f(+g_colorWheel.size, g_colorWheel.size + 10.f);
			gxVertex2f(-g_colorWheel.size, g_colorWheel.size + 10.f);
		}
		gxEnd();
		gxPopMatrix();
	#endif
	}
}
