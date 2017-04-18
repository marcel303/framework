#include "colorwheel.h"
#include "framework.h"
#include "nfd.h"
#include "particle.h"
#include "particle_editor.h"
#include "particle_framework.h"
#include "textfield.h"
#include "tinyxml2.h"
#include "ui.h"

#include "StringEx.h" // _s functions

/*

+ color curve key highlight on hover
+ color curve key color on inactive
+ make UiElem from color picker
+ implement color picker fromColor
+ add alpha selection color picker
+ fix unable to select alpha when mouse is outside bar
+ fix save and 'save as' functionality

+ fix color curve key select = move
+ move color picker and emitter info to the right side of the screen
+ fix 'loop' toggle behavior
+ add 'restart simulation' button
+ add support for multiple particle emitters
- add support for subemitters
+ fix subemitter UI reusing elems etc
- fix elems not refreshing when not drawn at refresh frame. use version number on elems?

+ add copy & paste buttons pi, pei

*/

using namespace tinyxml2;

// ui design
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
static const int kMenuSpacing = 15;
static const Color kBackgroundFocusColor(0.f, 0.f, 1.f, .5f);
static const Color kBackgroundColor(0.f, 0.f, 0.f, .7f);

// ui draw state
static bool g_doActions = false;
static bool g_doDraw = false;
static int g_drawX = 0;
static int g_drawY = 0;
static bool g_forceUiRefreshRequested = false;
static bool g_forceUiRefresh = false;

// library
static const int kMaxParticleInfos = 6;
static ParticleEmitterInfo g_peiList[kMaxParticleInfos];
static ParticleInfo g_piList[kMaxParticleInfos];

// current editing
static int g_activeEditingIndex = 0;
static ParticleEmitterInfo & g_pei() { return g_peiList[g_activeEditingIndex]; }
static ParticleInfo & g_pi() { return g_piList[g_activeEditingIndex]; }

// preview
static ParticlePool g_pool[kMaxParticleInfos];
static ParticleEmitter g_pe[kMaxParticleInfos];
static ParticleCallbacks g_callbacks;
static int randomInt(void * userData, int min, int max) { return min + (rand() % (max - min + 1)); }
static float randomFloat(void * userData, float min, float max) { return min + (rand() % 4096) / 4095.f * (max - min); }
static bool getEmitterByName(void * userData, const char * name, const ParticleEmitterInfo *& pei, const ParticleInfo *& pi, ParticlePool *& pool, ParticleEmitter *& pe)
{
	for (int i = 0; i < kMaxParticleInfos; ++i)
	{
		if (!strcmp(name, g_peiList[i].name))
		{
			pei = &g_peiList[i];
			pi = &g_piList[i];
			pool = &g_pool[i];
			pe = &g_pe[i];
			return true;
		}
	}
	return false;
}
static bool checkCollision(void * userData, float x1, float y1, float x2, float y2, float & t, float & nx, float & ny)
{
	const float px = 0.f;
	const float py = 100.f;
	const float pnx = 0.f;
	const float pny = -1.f;
	const float pd = px * pnx + py * pny;
	const float d1 = x1 * pnx + y1 * pny - pd;
	const float d2 = x2 * pnx + y2 * pny - pd;
	if (d1 >= 0.f && d2 < 0.f)
	{
		t = -d1 / (d2 - d1);
		nx = pnx;
		ny = pny;
		return true;
	}
	else
	{
		return false;
	}
}

// copy & paste
static ParticleEmitterInfo g_copyPei;
static bool g_copyPeiIsValid = false;
static ParticleInfo g_copyPi;
static bool g_copyPiIsValid = false;

//

inline float lerp(const float v1, const float v2, const float t)
{
	return v1 * (1.f - t) + v2 * t;
}

inline float saturate(const float v)
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

struct UiElem
{
	static const int kMaxVars = 10;
	
	bool hasFocus;
	bool isActive;
	bool clicked;
	
	struct Var
	{
		union
		{
			bool boolValue;
			int intValue;
			float floatValue;
			void * pointerValue;
		};
	};
	
	Var vars[kMaxVars];
	int varMask;
	
	EditorTextField textField; // ugh
	
	UiElem()
		: hasFocus(false)
		, isActive(false)
		, clicked(false)
		, vars()
		, varMask(0)
	{
	}
	
	void tick(const int x1, const int y1, const int x2, const int y2);
	
	void resetVars()
	{
		varMask = 0;
	}
	
	bool & getBool(const int index, const bool defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].boolValue = defaultValue;
			varMask |= (1 << index);
		}
		return vars[index].boolValue;
	}
	
	int & getInt(const int index, const int defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].intValue = defaultValue;
			varMask |= (1 << index);
		}
		return vars[index].intValue;
	}
	
	float & getFloat(const int index, const float defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].floatValue = defaultValue;
			varMask |= (1 << index);
		}
		return vars[index].floatValue;
	}
	
	template <typename T>
	T & getPointer(const int index, const T defaultValue)
	{
		fassert(index >= 0 && index < kMaxVars);
		if ((varMask & (1 << index)) == 0)
		{
			vars[index].pointerValue = defaultValue;
			varMask |= (1 << index);
		}
		return (T&)vars[index].pointerValue;
	}
};

struct UiMenu
{
	std::map<std::string, UiElem> elems;
	
	UiElem & getElem(const char * name)
	{
		return elems[name];
	}
};

//

static const int kMaxUiElemStoreDepth = 4;
static std::map<std::string, UiMenu> g_menus;
static std::string g_menuStack[kMaxUiElemStoreDepth];
static int g_menuStackSize = -1;
static UiMenu * g_menu = nullptr;

static void pushMenu(const char * name)
{
	std::string childName;
	
	if (g_menuStackSize == -1)
	{
		childName = name;
	}
	else
	{
		const std::string & parentName = g_menuStack[g_menuStackSize];
		
		childName = parentName + "." + name;
	}
	
	g_menuStackSize++;
	
	g_menuStack[g_menuStackSize] = childName;
	
	g_menu = &g_menus[g_menuStack[g_menuStackSize]];
}

static void popMenu()
{
	g_menuStackSize--;
	
	if (g_menuStackSize == -1)
	{
		g_menu = nullptr;
	}
	else
	{
		g_menu = &g_menus[g_menuStack[g_menuStackSize]];
	}
}

//

static UiElem * g_activeElem = 0;

void UiElem::tick(const int x1, const int y1, const int x2, const int y2)
{
	clicked = false;
	
	hasFocus = mouse.x >= x1 && mouse.x <= x2 && mouse.y >= y1 && mouse.y <= y2;
	if (hasFocus && mouse.wentDown(BUTTON_LEFT))
	{
		g_activeElem = this;
		clicked = true;
	}
	isActive = (g_activeElem == this);
}

//

static ColorWheel g_colorWheel;
static ParticleColor * g_activeColor = 0;

//

static bool doButton(const char * name, const float xOffset, const float xScale, const bool lineBreak)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;

	const int x1 = g_drawX + kMenuWidth * xOffset;
	const int x2 = g_drawX + kMenuWidth * xOffset + kMenuWidth * xScale;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCheckBoxHeight;

	if (lineBreak)
		g_drawY += kCheckBoxHeight;

	bool result = false;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus && mouse.wentUp(BUTTON_LEFT))
		{
			result = true;
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);
	}

	return result;
}

static bool valueToString(const std::string & src, char * dst, const int dstSize)
{
	sprintf_s(dst, dstSize, "%s", src.c_str());
	return true;
}

static bool valueToString(int src, char * dst, const int dstSize)
{
	sprintf_s(dst, dstSize, "%d", src);
	return true;
}

static bool valueToString(float src, char * dst, const int dstSize)
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

static void increment(const char * text, int direction, int & value)
{
	value += direction;
}

static void increment(const char * text, int direction, float & value)
{
	value += direction;
}

static void increment(const char * text, int direction, std::string & value)
{
}

template <typename T>
static void doTextBox(T & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt)
{
	UiElem & elem = g_menu->getElem(name);
	
	enum Vars
	{
		kVar_TextfieldIsInit
	};
	
	bool & textFieldIsInit = elem.getBool(kVar_TextfieldIsInit, false);
	EditorTextField & textField = elem.textField;
	
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
		
		elem.tick(x1, y1, x2, y2);

		if (elem.clicked)
		{
			textField.open(32, false, false);

			char temp[32];
			valueToString(value, temp, sizeof(temp));
			textField.setText(temp);
			
			textField.setTextIsSelected(true);
		}
		
		if (!elem.isActive)
		{
			if (textField.isActive())
				textField.close();
		}

		if (textField.isActive())
		{
			textField.tick(dt);
			
			stringToValue(textField.getText(), value);
			
			int incrementDirection = 0;
			
			if (keyboard.wentDown(SDLK_DOWN, true))
				incrementDirection--;
			if (keyboard.wentDown(SDLK_UP, true))
				incrementDirection++;
			
			if (incrementDirection != 0)
			{
				increment(textField.getText(), incrementDirection, value);
				
				char temp[32];
				valueToString(value, temp, sizeof(temp));
				textField.setText(temp);
			}
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);

		textField.draw(x1 + kPadding + kTextBoxTextOffset * xScale, y1, y2 - y1, kFontSize);
	}
}

template <typename T>
static void doTextBox(T & value, const char * name, const float dt)
{
	doTextBox(value, name, 0.f, 1.f, true, dt);
}

static bool doCheckBox(bool & value, const char * name, const bool isCollapsable)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCheckBoxHeight;

	g_drawY += kCheckBoxHeight;

	elem.tick(x1, y1, x2, y2);

	const int cx1 = x1 + kPadding;
	const int cx2 = x1 + kPadding + kCheckButtonSize;
	const int cy1 = y1 + kPadding;
	const int cy2 = y2 - kPadding;

	if (g_doActions)
	{
		if (elem.isActive && elem.hasFocus && mouse.wentUp(BUTTON_LEFT))
		{
			value = !value;
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
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
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);

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
static void doEnum(E & value, const char * name, const std::vector<EnumValue> & enumValues)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kEnumHeight;

	g_drawY += kEnumHeight;

	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus)
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
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);

		int index = -1;
		for (size_t i = 0; i < enumValues.size(); ++i)
			if (enumValues[i].value == value)
				index = i;
		if (index != -1)
			drawText(x1 + kPadding + kEnumSelectOffset, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", enumValues[index].name.c_str());
	}
}

void doParticleCurve(ParticleCurve & curve, const char * name)
{
	UiElem & elem = g_menu->getElem(name);
	
	const int kPadding = 5;
	const int kCheckButtonSize = kCheckBoxHeight - kPadding * 2;

	const int x1 = g_drawX;
	const int x2 = g_drawX + kMenuWidth;
	const int y1 = g_drawY;
	const int y2 = g_drawY + kCurveHeight;

	g_drawY += kCurveHeight;
    
	if (g_doActions)
	{
		elem.tick(x1, y1, x2, y2);

		// todo : curve type selection
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);
	}
}

void doParticleColor(ParticleColor & color, const char * name)
{
	UiElem & elem = g_menu->getElem(name);
	
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
		const bool wasActive = elem.isActive;

		elem.tick(x1, y1, x2, y2);

		if (elem.isActive && elem.hasFocus && mouse.wentDown(BUTTON_LEFT))
		{
			g_activeColor = &color;

			if (!wasActive)
				g_colorWheel.fromColor(g_activeColor->rgba[0], g_activeColor->rgba[1], g_activeColor->rgba[2], g_activeColor->rgba[3]);
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawRectCheckered(cx1, cy1, cx2, cy2, 4.f);
        setColorf(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
        drawRect(cx1, cy1, cx2, cy2);

		setColor(colorWhite);
		drawText(x1 + kPadding + kCheckButtonSize + kPadding, (y1+y2)/2, kFontSize, +1.f, 0.f, "%s", name);
	}
}

static float screenToCurve(const int x1, const int x2, const int x, const float offset)
{
	return saturate((x - x1) / float(x2 - x1) + offset);
}

static float curveToScreen(int x1, int x2, float t)
{
	return x1 + (x2 - x1) * t;
}

static ParticleColorCurve::Key * findNearestKey(ParticleColorCurve & curve, const float t, const float maxDeviation)
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

void doParticleColorCurve(ParticleColorCurve & curve, const char * name)
{
	UiElem & elem = g_menu->getElem(name);
	
	enum Vars
	{
		kVar_SelectedKey,
		kVar_IsDragging,
		kVar_DragOffset
	};
	
	ParticleColorCurve::Key *& selectedKey = elem.getPointer<ParticleColorCurve::Key*>(kVar_SelectedKey, nullptr);
	bool & isDragging = elem.getBool(kVar_IsDragging, false);
	float & dragOffset = elem.getFloat(kVar_DragOffset, 0.f);
	
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
		elem.tick(x1, y1, x2, y2);

		if (!elem.isActive)
			selectedKey = nullptr;
		else if (elem.hasFocus)
		{
			if (mouse.wentDown(BUTTON_LEFT))
			{
				// select or insert key

				const float t = screenToCurve(x1, x2, mouse.x, 0.f);
				auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
				
				if (key)
				{
					g_colorWheel.fromColor(
						key->color.rgba[0],
						key->color.rgba[1],
						key->color.rgba[2],
						key->color.rgba[3]);
				}
				else
				{
					// insert a new key

					ParticleColor color;
					curve.sample(t, color);

					if (curve.allocKey(key))
					{
						key->color = color;
						key->t = t;

						g_colorWheel.fromColor(
							key->color.rgba[0],
							key->color.rgba[1],
							key->color.rgba[2],
							key->color.rgba[3]);

						key = curve.sortKeys(key);
					}
				}

				selectedKey = key;

				if (selectedKey != nullptr)
				{
					isDragging = true;
					dragOffset = key->t - t;
				}
			}
			
			if (!mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.wentDown(BUTTON_RIGHT) || keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
				{
					selectedKey = nullptr;

					// erase key

					const float t = screenToCurve(x1, x2, mouse.x, 0.f);
					auto * key = findNearestKey(curve, t, kMaxSelectionDeviation);

					if (key)
					{
						curve.freeKey(key);
					}
				}
			}
		}

		if (isDragging)
		{
			if (elem.isActive && selectedKey != nullptr && mouse.isDown(BUTTON_LEFT))
			{
				// move selected key around

				const float t = screenToCurve(x1, x2, mouse.x, dragOffset);

				selectedKey->t = t;
				selectedKey = curve.sortKeys(selectedKey);

				fassert(selectedKey->t == t);
			}
			else
			{
				isDragging = false;
			}
		}

		if (elem.isActive)
		{
			g_activeColor = selectedKey != nullptr ? &selectedKey->color : 0;
		}
	}

	if (g_doDraw)
	{
		if (elem.hasFocus)
			setColor(kBackgroundFocusColor);
		else
			setColor(kBackgroundColor);
		drawRect(x1, y1, x2, y2);
		setColor(colorBlue);
		drawRectLine(x1, y1, x2, y2);

		setColor(colorWhite);
		drawRectCheckered(cx1, cy1, cx2, cy2, 4.f);
		for (int x = cx1; x < cx2; ++x)
		{
			const float t = (x - cx1 + .5f) / float(cx2 - cx1 - .5f);
			ParticleColor c;
			curve.sample(t, c);
			setColorf(c.rgba[0], c.rgba[1], c.rgba[2], c.rgba[3]);
			drawRect(x, cy1, x + 1.f, cy2);
		}
		
		{
			const float t = screenToCurve(x1, x2, mouse.x, 0.f);
			auto key = findNearestKey(curve, t, kMaxSelectionDeviation);
			for (int i = 0; i < curve.numKeys; ++i)
			{
				const float c =
					  !elem.hasFocus ? .5f
					: (key == &curve.keys[i]) ? 1.f
					: (selectedKey == &curve.keys[i]) ? .8f
					: .5f;
				const float x = curveToScreen(x1, x2, curve.keys[i].t);
				const float y = (y1 + y2) / 2.f;
				drawUiCircle(x, y, 5.5f, c, c, c, 1.f);
			}
		}

		//setColor(colorWhite);
		//drawText(x1 + kPadding + kCheckButtonSize + kPadding, y1, kFontSize, +1.f, +1.f, "%s", name);
	}
}

static void refreshUi()
{
	g_activeElem = 0;
	g_activeColor = 0;
	g_forceUiRefreshRequested = true;
}

struct Menu_LoadSave
{
	std::string activeFilename;
	
	Menu_LoadSave()
		: activeFilename()
	{
	}
};

static void doMenu_LoadSave(Menu_LoadSave & menu, const float dt)
{
	if (doButton("Load", 0.f, 1.f, true))
	{
		nfdchar_t * path = 0;
		nfdresult_t result = NFD_OpenDialog("pfx", "", &path);

		if (result == NFD_OKAY)
		{
			XMLDocument d;

			if (d.LoadFile(path) == XML_NO_ERROR)
			{
				for (int i = 0; i < kMaxParticleInfos; ++i)
				{
					g_peiList[i] = ParticleEmitterInfo();
					g_piList[i] = ParticleInfo();
					g_pe[i].clearParticles(g_pool[i]);
					fassert(g_pool[i].head == 0);
					fassert(g_pool[i].tail == 0);
					g_pe[i] = ParticleEmitter();
				}

				int peiIdx = 0;
				for (XMLElement * emitterElem = d.FirstChildElement("emitter"); emitterElem; emitterElem = emitterElem->NextSiblingElement("emitter"))
				{
					g_peiList[peiIdx++].load(emitterElem);
				}

				int piIdx = 0;
				for (XMLElement * particleElem = d.FirstChildElement("particle"); particleElem; particleElem = particleElem->NextSiblingElement("particle"))
				{
					g_piList[piIdx++].load(particleElem);
				}

				menu.activeFilename = path;

				g_activeEditingIndex = 0;
				refreshUi();
			}
		}
	}

	bool save = false;
	std::string saveFilename;
	
	if (doButton("Save", 0.f, 1.f, true))
	{
		save = true;
		saveFilename = menu.activeFilename;
	}
	
	if (doButton("Save as..", 0.f, 1.f, true))
	{
		save = true;
	}

	if (save)
	{
		if (saveFilename.empty())
		{
			nfdchar_t * path = 0;
			nfdresult_t result = NFD_SaveDialog("pfx", "", &path);

			if (result == NFD_OKAY)
			{
				saveFilename = path;
			}
		}

		if (!saveFilename.empty())
		{
			XMLPrinter p;

			for (int i = 0; i < kMaxParticleInfos; ++i)
			{
				p.OpenElement("emitter");
				{
					g_peiList[i].save(&p);
				}
				p.CloseElement();

				p.OpenElement("particle");
				{
					g_piList[i].save(&p);
				}
				p.CloseElement();
			}

			XMLDocument d;
			d.Parse(p.CStr());
			d.SaveFile(saveFilename.c_str());

			menu.activeFilename = saveFilename;
		}
	}
	
	if (doButton("Restart simulation", 0.f, 1.f, true))
	{
		for (int i = 0; i < kMaxParticleInfos; ++i)
			g_pe[i].restart(g_pool[i]);
	}
}

static void doMenu_EmitterSelect(const float dt)
{
	for (int i = 0; i < 6; ++i)
	{
		char name[32];
		sprintf_s(name, sizeof(name), "System %d", i + 1);
		if (doButton(name, 0.f, 1.f, true))
		{
			g_activeEditingIndex = i;
			refreshUi();
		}
	}
}

static void doMenu_Pi(const float dt)
{
	if (doButton("Copy", 0.f, .5f, !g_copyPiIsValid))
	{
		g_copyPi = g_pi();
		g_copyPiIsValid = true;
	}
	
	if (g_copyPiIsValid && doButton("Paste", .5f, .5f, true))
	{
		g_pi() = g_copyPi;
		refreshUi();
	}

	doTextBox(g_pi().rate, "Rate (Particles/Sec)", dt);

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
	doEnum(g_pi().shape, "Shape", shapeValues);
	
	doCheckBox(g_pi().randomDirection, "Random Direction", false);
	doTextBox(g_pi().circleRadius, "Circle Radius", dt);
	doTextBox(g_pi().boxSizeX, "Box Width", dt);
	doTextBox(g_pi().boxSizeY, "Box Height", dt);
	doCheckBox(g_pi().emitFromShell, "Emit From Shell", false);

	if (doCheckBox(g_pi().velocityOverLifetime, "Velocity Over Lifetime", true))
	{
		pushMenu("Velocity Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().velocityOverLifetimeValueX, "X", 0.f, .5f, false, dt);
		doTextBox(g_pi().velocityOverLifetimeValueY, "Y", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().velocityOverLifetimeLimit, "Velocity Over Lifetime Limit", true))
	{
		pushMenu("Velocity Over Lifetime Limit");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().velocityOverLifetimeLimitCurve, "Curve");
		doTextBox(g_pi().velocityOverLifetimeLimitDampen, "Dampen/Sec", dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().forceOverLifetime, "Force Over Lifetime", true))
	{
		pushMenu("Force Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().forceOverLifetimeValueX, "X", 0.f, .5f, false, dt);
		doTextBox(g_pi().forceOverLifetimeValueY, "Y", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().colorOverLifetime, "Color Over Lifetime", true))
	{
		pushMenu("Color Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleColorCurve(g_pi().colorOverLifetimeCurve, "Curve");
		popMenu();
	}
	
	if (doCheckBox(g_pi().colorBySpeed, "Color By Speed", true))
	{
		pushMenu("Color By Speed");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleColorCurve(g_pi().colorBySpeedCurve, "Curve");
		doTextBox(g_pi().colorBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
		doTextBox(g_pi().colorBySpeedRangeMax, "", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().sizeOverLifetime, "Size Over Lifetime", true))
	{
		pushMenu("Size Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().sizeOverLifetimeCurve, "Curve");
		popMenu();
	}
	
	if (doCheckBox(g_pi().sizeBySpeed, "Size By Speed", true))
	{
		pushMenu("Size By Speed");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().sizeBySpeedCurve, "Curve");
		doTextBox(g_pi().sizeBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
		doTextBox(g_pi().sizeBySpeedRangeMax, "", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().rotationOverLifetime, "Rotation Over Lifetime", true))
	{
		pushMenu("Rotation Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().rotationOverLifetimeValue, "Degrees/Sec", dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().rotationBySpeed, "Rotation By Speed", true))
	{
		pushMenu("Rotation By Speed");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().rotationBySpeedCurve, "Curve");
		doTextBox(g_pi().rotationBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
		doTextBox(g_pi().rotationBySpeedRangeMax, "", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().collision, "Collision", true))
	{
		pushMenu("Collision");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().bounciness, "Bounciness", dt);
		doTextBox(g_pi().lifetimeLoss, "Lifetime Loss On Collision", dt);
		doTextBox(g_pi().minKillSpeed, "Kill Speed", dt);
		doTextBox(g_pi().collisionRadius, "Collision Radius", dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().enableSubEmitters, "Sub Emitters", true))
	{
		pushMenu("Sub Emitters");
		{
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			
			const char * eventNames[ParticleInfo::kSubEmitterEvent_COUNT] =
			{
				"onBirth",
				"onCollision",
				"onDeath"
			};
			
			for (int i = 0; i < ParticleInfo::kSubEmitterEvent_COUNT; ++i) // fixme : cannot use loops
			{
				if (doCheckBox(g_pi().subEmitters[i].enabled, eventNames[i], true))
				{
					pushMenu(eventNames[i]);
					{
						ScopedValueAdjust<int> xAdjust(g_drawX, +10);
						doTextBox(g_pi().subEmitters[i].chance, "Spawn Chance", dt);
						doTextBox(g_pi().subEmitters[i].count, "Spawn Count", dt);

						std::string emitterName = g_pi().subEmitters[i].emitterName;
						doTextBox(emitterName, "Emitter Name", dt);
						strcpy_s(g_pi().subEmitters[i].emitterName, sizeof(g_pi().subEmitters[i].emitterName), emitterName.c_str());
					}
					popMenu();
				}
			}
			//SubEmitter onBirth;
			//SubEmitter onCollision;
			//SubEmitter onDeath;
		}
		popMenu();
	}

	std::vector<EnumValue> sortModeValues;
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_OldestFirst, "Oldest First"));
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_YoungestFirst, "Youngest First"));
	doEnum(g_pi().sortMode, "Sort Mode", sortModeValues);

	std::vector<EnumValue> blendModeValues;
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_AlphaBlended, "Use Alpha"));
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_Additive, "Additive"));
	doEnum(g_pi().blendMode, "Blend Mode", blendModeValues);
}

static void doMenu_Pei(const float dt)
{
	if (doButton("Copy", 0.f, .5f, !g_copyPeiIsValid))
	{
		g_copyPei = g_pei();
		g_copyPeiIsValid = true;
	}
	
	if (g_copyPeiIsValid && doButton("Paste", .5f, .5f, true))
	{
		g_pei() = g_copyPei;
		refreshUi();
	}
	
	std::string name;
	name = g_pei().name;
	doTextBox(name, "Name", dt);
	strcpy_s(g_pei().name, sizeof(g_pei().name), name.c_str());
	
	doTextBox(g_pei().duration, "Duration", dt);
	doCheckBox(g_pei().loop, "Loop", false);
	doCheckBox(g_pei().prewarm, "Prewarm", false);
	doTextBox(g_pei().startDelay, "Start Delay", dt);
	doTextBox(g_pei().startLifetime, "Start Lifetime", dt);
	doTextBox(g_pei().startSpeed, "Start Speed", dt);
	doTextBox(g_pei().startSize, "Start Size", dt);
	doTextBox(g_pei().startRotation, "Start Rotation", dt); // todo : min and max values for random start rotation?
	doParticleColor(g_pei().startColor, "Start Color");
	doTextBox(g_pei().gravityMultiplier, "Gravity Multiplier", dt);
	doCheckBox(g_pei().inheritVelocity, "Inherit Velocity", false);
	doCheckBox(g_pei().worldSpace, "World Space", false);
	doTextBox(g_pei().maxParticles, "Max Particles", dt);
	
	std::string materialName = g_pei().materialName;
	doTextBox(materialName, "Material", dt);
	strcpy_s(g_pei().materialName, sizeof(g_pei().materialName), materialName.c_str());
}

static void doMenu_ColorWheel(const float dt)
{
	static UiElem colorPickerElem;
	if (g_activeColor)
	{
		const float wheelX = g_drawX + (kMenuWidth - g_colorWheel.getSx()) / 2;
		const float wheelY = g_drawY;

		if (g_doActions)
		{
			colorPickerElem.tick(wheelX, wheelY, wheelX + g_colorWheel.getSx(), wheelY + g_colorWheel.getSy());
			if (colorPickerElem.isActive)
				g_colorWheel.tick(mouse.x - wheelX, mouse.y - wheelY, mouse.wentDown(BUTTON_LEFT), mouse.isDown(BUTTON_LEFT), dt); // fixme : mouseDown and dt
			g_colorWheel.toColor(
				g_activeColor->rgba[0],
				g_activeColor->rgba[1],
				g_activeColor->rgba[2],
				g_activeColor->rgba[3]);
		}

		if (g_doDraw)
		{
			gxPushMatrix();
			{
				gxTranslatef(wheelX, wheelY, 0.f);
				g_colorWheel.draw();
			}
			gxPopMatrix();
		}

		g_drawY += g_colorWheel.getSy();
	}
}

struct Menu
{
	Menu_LoadSave loadSave;
	
	Menu()
		: loadSave()
	{
	}
};

static Menu s_menu;

static void doMenu(Menu & menu, const bool doActions, const bool doDraw, const int sx, const int sy, const float dt)
{
	g_doActions = doActions;
	g_doDraw = doDraw;

	if (g_doActions && g_forceUiRefreshRequested)
	{
		g_forceUiRefreshRequested = false;
		g_forceUiRefresh = true;
		
		g_activeElem = nullptr;
		
		g_menus.clear();
	}

	// left side menu

	setFont("calibri.ttf");
	g_drawX = 10;
	g_drawY = 0;
	
	pushMenu("pi");
	doMenu_Pi(dt);
	popMenu();

	// right side menu

	setFont("calibri.ttf");
	g_drawX = sx - kMenuWidth - 10;
	g_drawY = 0;
	
	pushMenu("loadSave");
	doMenu_LoadSave(menu.loadSave, dt);
	g_drawY += kMenuSpacing;
	popMenu();

	pushMenu("emitterSelect");
	doMenu_EmitterSelect(dt);
	g_drawY += kMenuSpacing;
	popMenu();
	
	pushMenu("pei");
	doMenu_Pei(dt);
	g_drawY += kMenuSpacing;
	popMenu();

	pushMenu("colorWheel");
	doMenu_ColorWheel(dt);
	g_drawY += kMenuSpacing;
	popMenu();

	if (g_doActions)
		g_forceUiRefresh = false;
}

void particleEditorTick(const bool menuActive, const float sx, const float sy, const float dt)
{
	static bool firstTick = true;
	if (firstTick)
	{
		firstTick = false;

		g_callbacks.randomInt = randomInt;
		g_callbacks.randomFloat = randomFloat;
		g_callbacks.getEmitterByName = getEmitterByName;
		g_callbacks.checkCollision = checkCollision;

		g_piList[0].rate = 1.f;
		for (int i = 0; i < kMaxParticleInfos; ++i)
			strcpy_s(g_peiList[i].materialName, sizeof(g_peiList[i].materialName), "texture.png");
	}
	
	if (menuActive)
		doMenu(s_menu, true, false, sx, sy, dt);

	for (int i = 0; i < kMaxParticleInfos; ++i)
	{
		const float gravityX = 0.f;
		const float gravityY = 100.f;
		for (Particle * p = g_pool[i].head; p; )
			if (!tickParticle(g_callbacks, g_peiList[i], g_piList[i], dt, gravityX, gravityY, *p))
				p = g_pool[i].freeParticle(p);
			else
				p = p->next;
		tickParticleEmitter(g_callbacks, g_peiList[i], g_piList[i], g_pool[i], dt, gravityX, gravityY, g_pe[i]);
	}
}

void particleEditorDraw(const bool menuActive, const float sx, const float sy)
{
	gxPushMatrix();
	gxTranslatef(sx/2.f, sy/2.f, 0.f);

#if 1
	setColor(0, 255, 0, 31);
	switch (g_pi().shape)
	{
	case ParticleInfo::kShapeBox:
		drawRectLine(
			-g_pi().boxSizeX,
			-g_pi().boxSizeY,
			+g_pi().boxSizeX,
			+g_pi().boxSizeY);
		break;
	case ParticleInfo::kShapeCircle:
		drawRectLine(
			-g_pi().circleRadius,
			-g_pi().circleRadius,
			+g_pi().circleRadius,
			+g_pi().circleRadius);
		break;
	case ParticleInfo::kShapeEdge:
		drawLine(
			-g_pi().boxSizeX,
			0.f,
			+g_pi().boxSizeX,
			0.f);
		break;
	}

	for (int i = 0; i < kMaxParticleInfos; ++i)
	{
		gxSetTexture(Sprite(g_peiList[i].materialName).getTexture());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (g_piList[i].blendMode == ParticleInfo::kBlendMode_AlphaBlended)
			setBlend(BLEND_ALPHA);
		else if (g_piList[i].blendMode == ParticleInfo::kBlendMode_Additive)
			setBlend(BLEND_ADD);
		else
			fassert(false);

		//if (rand() % 2)
		if (true)
		{
			gxBegin(GL_QUADS);
			{
				for (Particle * p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? g_pool[i].head : g_pool[i].tail;
							 p; p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
				{
					const float particleLife = 1.f - p->life;
					//const float particleSpeed = std::sqrtf(p->speed[0] * p->speed[0] + p->speed[1] * p->speed[1]);
					const float particleSpeed = p->speedScalar;

					ParticleColor color(true);
					computeParticleColor(g_peiList[i], g_piList[i], particleLife, particleSpeed, color);
					const float size_div_2 = computeParticleSize(g_peiList[i], g_piList[i], particleLife, particleSpeed) / 2.f;

					const float s = std::sinf(-p->rotation * float(M_PI) / 180.f);
					const float c = std::cosf(-p->rotation * float(M_PI) / 180.f);

					gxColor4fv(color.rgba);
					gxTexCoord2f(0.f, 1.f); gxVertex2f(p->position[0] + (- c - s) * size_div_2, p->position[1] + (+ s - c) * size_div_2);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(p->position[0] + (+ c - s) * size_div_2, p->position[1] + (- s - c) * size_div_2);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(p->position[0] + (+ c + s) * size_div_2, p->position[1] + (- s + c) * size_div_2);
					gxTexCoord2f(0.f, 0.f); gxVertex2f(p->position[0] + (- c + s) * size_div_2, p->position[1] + (+ s + c) * size_div_2);
				}
			}
			gxEnd();
		}
		else
		{
			for (Particle * p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? g_pool[i].head : g_pool[i].tail;
						 p; p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
			{
				const float particleLife = 1.f - p->life;
				const float particleSpeed = std::sqrtf(p->speed[0] * p->speed[0] + p->speed[1] * p->speed[1]);

				ParticleColor color;
				computeParticleColor(g_peiList[i], g_piList[i], particleLife, particleSpeed, color);
				const float size = computeParticleSize(g_peiList[i], g_piList[i], particleLife, particleSpeed);
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
		}
		gxSetTexture(0);
	}

	setBlend(BLEND_ALPHA);
#endif

	gxPopMatrix();

	if (menuActive)
		doMenu(s_menu, false, true, sx, sy, 0.f);
}
