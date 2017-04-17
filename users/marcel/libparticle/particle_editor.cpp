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
	bool hasFocus;
	bool isActive;

	UiElem()
		: hasFocus(false)
		, isActive(false)
	{
	}

	void tick(int x1, int y1, int x2, int y2);
};

static UiElem * g_activeElem = 0;

void UiElem::tick(int x1, int y1, int x2, int y2)
{
	hasFocus = mouse.x >= x1 && mouse.x <= x2 && mouse.y >= y1 && mouse.y <= y2;
	if (hasFocus && mouse.wentDown(BUTTON_LEFT))
		g_activeElem = this;
	isActive = (g_activeElem == this);
}

//

static ColorWheel g_colorWheel;
static ParticleColor * g_activeColor = 0;

//

static bool doButton(const char * name, float xOffset, float xScale, bool lineBreak, UiElem & elem)
{
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
static void doTextBox(T & value, const char * name, float xOffset, float xScale, bool lineBreak, UiElem & elem, EditorTextField & textField, bool & textFieldIsInit, float dt)
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

		const bool wasActive = elem.isActive;

		elem.tick(x1, y1, x2, y2);

		if (elem.isActive != wasActive)
		{
			if (elem.isActive)
			{
				textField.open(32, false, false);

				char temp[32];
				valueToString(value, temp, sizeof(temp));
				textField.setText(temp);
				
				textField.setTextIsSelected(true);
			}
			else if (textField.isActive())
				textField.close();
		}

		if (textField.isActive())
		{
			textField.tick(dt);

			stringToValue(textField.getText(), value);
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

static bool doCheckBox(bool & value, const char * name, bool isCollapsable, UiElem & elem)
{
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
static void doEnum(E & value, const char * name, const std::vector<EnumValue> & enumValues, UiElem & elem)
{
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

void doParticleCurve(ParticleCurve & curve, const char * name, UiElem & elem)
{
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

void doParticleColor(ParticleColor & color, const char * name, UiElem & elem)
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

static float screenToCurve(int x1, int x2, int x, float offset)
{
	return saturate((x - x1) / float(x2 - x1) + offset);
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

void doParticleColorCurve(ParticleColorCurve & curve, const char * name, UiElem & elem, ParticleColorCurve::Key *& selectedKey, bool & isDragging, float & dragOffset)
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
		elem.tick(x1, y1, x2, y2);

		if (!elem.isActive)
			selectedKey = 0;
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

				if (selectedKey)
				{
					isDragging = true;
					dragOffset = key->t - t;
				}
			}
			
			if (!mouse.isDown(BUTTON_LEFT))
			{
				if (mouse.wentDown(BUTTON_RIGHT) || keyboard.wentDown(SDLK_DELETE) || keyboard.wentDown(SDLK_BACKSPACE))
				{
					selectedKey = 0;

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
			if (elem.isActive && selectedKey && mouse.isDown(BUTTON_LEFT))
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
			g_activeColor = selectedKey ? &selectedKey->color : 0;
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

#define DO_TEXTBOX(sym, value, name) \
	static UiElem sym ## elem; \
	static EditorTextField sym ## textField; \
	static bool sym ## textFieldIsInit = false; \
	if (g_doActions && g_forceUiRefresh) \
		sym ## textFieldIsInit = false; \
	doTextBox(value, name, 0.f, 1.f, true, sym ## elem, sym ## textField, sym ## textFieldIsInit, dt);

#define DO_TEXTBOX2(sym, value, xOffset, xScale, lineBreak, name) \
	static UiElem sym ## elem; \
	static EditorTextField sym ## textField; \
	static bool sym ## textFieldIsInit = false; \
	if (g_doActions && g_forceUiRefresh) \
		sym ## textFieldIsInit = false; \
	doTextBox(value, name, xOffset, xScale, lineBreak, sym ## elem, sym ## textField, sym ## textFieldIsInit, dt);

#define DO_CHECKBOX(sym, value, name, isCollapsable) \
	static UiElem sym ## elem; \
	doCheckBox(value, name, isCollapsable, sym ## elem);

#define DO_ENUM(sym, value, enumValues, name) \
	static UiElem sym ## elem; \
	doEnum(value, name, enumValues, sym ## elem);

#define DO_PARTICLECURVE(sym, value, name) \
	static UiElem sym ## elem; \
	doParticleCurve(value, name, sym ## elem);

#define DO_PARTICLECOLORCURVE(sym, value, name) \
	static UiElem sym ## elem; \
	static ParticleColorCurve::Key * sym ## selectedKey = 0; \
	static bool sym ## isDragging = false; \
	static float sym ## dragOffset = 0.f; \
	doParticleColorCurve(value, name, sym ## elem, sym ## selectedKey, sym ## isDragging, sym ## dragOffset);

static void refreshUi()
{
	g_activeElem = 0;
	g_activeColor = 0;
	g_forceUiRefreshRequested = true;
}

static void doMenu_LoadSave(float dt)
{
	static std::string activeFilename;
	static UiElem loadElem;
	if (doButton("Load", 0.f, 1.f, true, loadElem))
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

				activeFilename = path;

				g_activeEditingIndex = 0;
				refreshUi();
			}
		}
	}

	bool save = false;
	std::string saveFilename;

	static UiElem saveElem;
	if (doButton("Save", 0.f, 1.f, true, saveElem))
	{
		save = true;
		saveFilename = activeFilename;
	}

	static UiElem saveAsElem;
	if (doButton("Save as..", 0.f, 1.f, true, saveAsElem))
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

			activeFilename = saveFilename;
		}
	}

	static UiElem restartSimulationElem;
	if (doButton("Restart simulation", 0.f, 1.f, true, restartSimulationElem))
	{
		for (int i = 0; i < kMaxParticleInfos; ++i)
			g_pe[i].restart(g_pool[i]);
	}
}

static void doMenu_EmitterSelect(float dt)
{
	static UiElem uiElems[kMaxParticleInfos];
	for (int i = 0; i < 6; ++i)
	{
		char name[32];
		sprintf_s(name, sizeof(name), "System %d", i + 1);
		if (doButton(name, 0.f, 1.f, true, uiElems[i]))
		{
			g_activeEditingIndex = i;
			refreshUi();
		}
	}
}

static void doMenu_Pi(float dt)
{
	static UiElem copyPiElem;
	if (doButton("Copy", 0.f, .5f, !g_copyPiIsValid, copyPiElem))
	{
		g_copyPi = g_pi();
		g_copyPiIsValid = true;
	}

	static UiElem pastePiElem;
	if (g_copyPiIsValid && doButton("Paste", .5f, .5f, true, pastePiElem))
	{
		g_pi() = g_copyPi;
		refreshUi();
	}

	DO_TEXTBOX(rate, g_pi().rate, "Rate (Particles/Sec)");

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
	DO_ENUM(shape, g_pi().shape, shapeValues, "Shape");

	DO_CHECKBOX(randomDirection, g_pi().randomDirection, "Random Direction", false);
	DO_TEXTBOX(circleRadius, g_pi().circleRadius, "Circle Radius");
	DO_TEXTBOX(boxSizeX, g_pi().boxSizeX, "Box Width");
	DO_TEXTBOX(boxSizeY, g_pi().boxSizeY, "Box Height");
	DO_CHECKBOX(emitFromShell, g_pi().emitFromShell, "Emit From Shell", false);

	static UiElem velocityOverLifetimeElem;
	if (doCheckBox(g_pi().velocityOverLifetime, "Velocity Over Lifetime", true, velocityOverLifetimeElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX2(velocityOverLifetimeValueX, g_pi().velocityOverLifetimeValueX, 0.f, .5f, false, "X");
		DO_TEXTBOX2(velocityOverLifetimeValueY, g_pi().velocityOverLifetimeValueY, .5f, .5f, true, "Y");
	}

	static UiElem velocityOverLifetimeLimitElem;
	if (doCheckBox(g_pi().velocityOverLifetimeLimit, "Velocity Over Lifetime Limit", true, velocityOverLifetimeLimitElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(velocityOverLifetimeLimitCurve, g_pi().velocityOverLifetimeLimitCurve, "Curve");
		DO_TEXTBOX(velocityOverLifetimeLimitDampen, g_pi().velocityOverLifetimeLimitDampen, "Dampen/Sec");
	}

	static UiElem forceOverLifetimeElem;
	if (doCheckBox(g_pi().forceOverLifetime, "Force Over Lifetime", true, forceOverLifetimeElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX2(forceOverLifetimeValueX, g_pi().forceOverLifetimeValueX, 0.f, .5f, false, "X");
		DO_TEXTBOX2(forceOverLifetimeValueY, g_pi().forceOverLifetimeValueY, .5f, .5f, true, "Y");
	}

	static UiElem colorOverLifetimeElem;
	if (doCheckBox(g_pi().colorOverLifetime, "Color Over Lifetime", true, colorOverLifetimeElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECOLORCURVE(colorOverLifetimeCurve, g_pi().colorOverLifetimeCurve, "Curve");
	}

	static UiElem colorBySpeedElem;
	if (doCheckBox(g_pi().colorBySpeed, "Color By Speed", true, colorBySpeedElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECOLORCURVE(colorBySpeedCurve, g_pi().colorBySpeedCurve, "Curve");
		DO_TEXTBOX2(colorBySpeedRangeMin, g_pi().colorBySpeedRangeMin, 0.f, .5f, false, "Range");
		DO_TEXTBOX2(colorBySpeedRangeMax, g_pi().colorBySpeedRangeMax, .5f, .5f, true, "");
	}

	static UiElem sizeOverLifetimeElem;
	if (doCheckBox(g_pi().sizeOverLifetime, "Size Over Lifetime", true, sizeOverLifetimeElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(sizeOverLifetimeCurve, g_pi().sizeOverLifetimeCurve, "Curve");
	}

	static UiElem sizeBySpeedElem;
	if (doCheckBox(g_pi().sizeBySpeed, "Size By Speed", true, sizeBySpeedElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(sizeBySpeedCurve, g_pi().sizeBySpeedCurve, "Curve");
		DO_TEXTBOX2(sizeBySpeedRangeMin, g_pi().sizeBySpeedRangeMin, 0.f, .5f, false, "Range");
		DO_TEXTBOX2(sizeBySpeedRangeMax, g_pi().sizeBySpeedRangeMax, .5f, .5f, true, "");
	}

	static UiElem rotationOverLifetimeElem;
	if (doCheckBox(g_pi().rotationOverLifetime, "Rotation Over Lifetime", true, rotationOverLifetimeElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX(rotationOverLifetimeValue, g_pi().rotationOverLifetimeValue, "Degrees/Sec");
	}

	static UiElem rotationBySpeedElem;
	if (doCheckBox(g_pi().rotationBySpeed, "Rotation By Speed", true, rotationBySpeedElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_PARTICLECURVE(rotationBySpeedCurve, g_pi().rotationBySpeedCurve, "Curve");
		DO_TEXTBOX2(rotationBySpeedRangeMin, g_pi().rotationBySpeedRangeMin, 0.f, .5f, false, "Range");
		DO_TEXTBOX2(rotationBySpeedRangeMax, g_pi().rotationBySpeedRangeMax, .5f, .5f, true, "");
	}

	static UiElem collisionElem;
	if (doCheckBox(g_pi().collision, "Collision", true, collisionElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		DO_TEXTBOX(bounciness, g_pi().bounciness, "Bounciness");
		DO_TEXTBOX(lifetimeLoss, g_pi().lifetimeLoss, "Lifetime Loss On Collision");
		DO_TEXTBOX(minKillSpeed, g_pi().minKillSpeed, "Kill Speed");
		DO_TEXTBOX(collisionRadius, g_pi().collisionRadius, "Collision Radius");
	}

	static UiElem subEmittersElem;
	if (doCheckBox(g_pi().enableSubEmitters, "Sub Emitters", true, subEmittersElem))
	{
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		static bool onEvent[ParticleInfo::kSubEmitterEvent_COUNT] = { };
		const char * onEventName[ParticleInfo::kSubEmitterEvent_COUNT] = { "onBirth", "onCollision", "onDeath" };
		static UiElem onEventElem[ParticleInfo::kSubEmitterEvent_COUNT];
		struct SubEmitterField
		{
			SubEmitterField()
				: textFieldIsInit(false)
			{
			}

			UiElem elem;
			EditorTextField textField;
			bool textFieldIsInit;
		};
		static SubEmitterField fields[ParticleInfo::kSubEmitterEvent_COUNT][3];
		for (int i = 0; i < ParticleInfo::kSubEmitterEvent_COUNT; ++i) // fixme : cannot use loops
		{
			if (doCheckBox(g_pi().subEmitters[i].enabled, onEventName[i], true, onEventElem[i]))
			{
				ScopedValueAdjust<int> xAdjust(g_drawX, +10);
				doTextBox(g_pi().subEmitters[i].chance, "Spawn Chance", 0.f, 1.f, true, fields[i][0].elem, fields[i][0].textField, fields[i][0].textFieldIsInit, dt);
				doTextBox(g_pi().subEmitters[i].count, "Spawn Count", 0.f, 1.f, true, fields[i][1].elem, fields[i][1].textField, fields[i][1].textFieldIsInit, dt);

				std::string emitterName = g_pi().subEmitters[i].emitterName;
				doTextBox(emitterName, "Emitter Name", 0.f, 1.f, true, fields[i][2].elem, fields[i][2].textField, fields[i][2].textFieldIsInit, dt);
				strcpy_s(g_pi().subEmitters[i].emitterName, sizeof(g_pi().subEmitters[i].emitterName), emitterName.c_str());
			}
		}
		//SubEmitter onBirth;
		//SubEmitter onCollision;
		//SubEmitter onDeath;
	}

	std::vector<EnumValue> sortModeValues;
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_OldestFirst, "Oldest First"));
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_YoungestFirst, "Youngest First"));
	DO_ENUM(sortMode, g_pi().sortMode, sortModeValues, "Sort Mode");

	std::vector<EnumValue> blendModeValues;
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_AlphaBlended, "Use Alpha"));
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_Additive, "Additive"));
	DO_ENUM(blendMode, g_pi().blendMode, blendModeValues, "Blend Mode");
}

static void doMenu_Pei(float dt)
{
	static UiElem copyPeiElem;
	if (doButton("Copy", 0.f, .5f, !g_copyPeiIsValid, copyPeiElem))
	{
		g_copyPei = g_pei();
		g_copyPeiIsValid = true;
	}

	static UiElem pastePeiElem;
	if (g_copyPeiIsValid && doButton("Paste", .5f, .5f, true, pastePeiElem))
	{
		g_pei() = g_copyPei;
		refreshUi();
	}

	std::string name;
	name = g_pei().name;
	DO_TEXTBOX(name, name, "Name");
	strcpy_s(g_pei().name, sizeof(g_pei().name), name.c_str());
	DO_TEXTBOX(duration, g_pei().duration, "Duration");
	DO_CHECKBOX(loop, g_pei().loop, "Loop", false);
	DO_CHECKBOX(prewarm, g_pei().prewarm, "Prewarm", false);
	DO_TEXTBOX(startDelay, g_pei().startDelay, "Start Delay");
	DO_TEXTBOX(startLifetime, g_pei().startLifetime, "Start Lifetime");
	DO_TEXTBOX(startSpeed, g_pei().startSpeed, "Start Speed");
	DO_TEXTBOX(startSize, g_pei().startSize, "Start Size");
	DO_TEXTBOX(startRotation, g_pei().startRotation, "Start Rotation"); // todo : min and max values for random start rotation?
	static UiElem startColorElem;
	doParticleColor(g_pei().startColor, "Start Color", startColorElem);
	DO_TEXTBOX(gravityMultiplier, g_pei().gravityMultiplier, "Gravity Multiplier");
	DO_CHECKBOX(inheritVelocity, g_pei().inheritVelocity, "Inherit Velocity", false);
	DO_CHECKBOX(worldSpace, g_pei().worldSpace, "World Space", false);
	DO_TEXTBOX(maxParticles, g_pei().maxParticles, "Max Particles");
	std::string materialName = g_pei().materialName;
	DO_TEXTBOX(materialName, materialName, "Material");
	strcpy_s(g_pei().materialName, sizeof(g_pei().materialName), materialName.c_str());
}

static void doMenu_ColorWheel(float dt)
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

static void doMenu(bool doActions, bool doDraw, int sx, int sy, float dt)
{
	g_doActions = doActions;
	g_doDraw = doDraw;

	if (g_doActions && g_forceUiRefreshRequested)
	{
		g_forceUiRefreshRequested = false;
		g_forceUiRefresh = true;
	}

	// left side menu

	setFont("calibri.ttf");
	g_drawX = 10;
	g_drawY = 0;

	doMenu_Pi(dt);

	// right side menu

	setFont("calibri.ttf");
	g_drawX = sx - kMenuWidth - 10;
	g_drawY = 0;

	doMenu_LoadSave(dt);
	g_drawY += kMenuSpacing;

	doMenu_EmitterSelect(dt);
	g_drawY += kMenuSpacing;

	doMenu_Pei(dt);
	g_drawY += kMenuSpacing;

	doMenu_ColorWheel(dt);
	g_drawY += kMenuSpacing;

	if (g_doActions)
		g_forceUiRefresh = false;
}

void particleEditorTick(bool menuActive, float sx, float sy, float dt)
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
		doMenu(true, false, sx, sy, dt);

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

void particleEditorDraw(bool menuActive, float sx, float sy)
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
		doMenu(false, true, sx, sy, 0.f);
}
