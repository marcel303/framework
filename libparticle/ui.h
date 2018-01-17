#pragma once

#include <map>
#include <string>

class Color;
class ColorWheel;
class EditorTextField;
struct ParticleColor;
struct ParticleColorCurve;
struct ParticleCurve;
struct UiElem;
struct UiMenu;
struct UiMenuStates;
struct UiState;

extern UiMenu * g_menu;

enum UiTextboxResult
{
	kUiTextboxResult_NoEvent,
	kUiTextboxResult_EditingComplete,
	kUiTextboxResult_EditingCompleteCanceled,
	kUiTextboxResult_EditingCompleteCleared
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
	
	EditorTextField * textField; // ugh
	
	UiElem();
	~UiElem();
	
	void tick(const int x1, const int y1, const int x2, const int y2);
	
	void deactivate();
	
	void reset();
	void resetVars();
	
	bool & getBool(const int index, const bool defaultValue);
	int & getInt(const int index, const int defaultValue);
	float & getFloat(const int index, const float defaultValue);
	void *& getPointerImpl(const int index, const void * defaultValue);
	
	template <typename T>
	T & getPointer(const int index, const T defaultValue)
	{
		return (T&)getPointerImpl(index, defaultValue);
	}
	
	EditorTextField & getTextField();
};

struct UiMenu
{
	int sx;
	
	std::map<std::string, UiElem> elems;
	
	UiElem & getElem(const char * name)
	{
		return elems[name];
	}
	
	void reset()
	{
		elems.clear();
	}
};

struct UiState
{
	int x;
	int y;
	int sx;
	
	std::string font;
	float textBoxTextOffset;
	
	float opacity;
	
	UiElem * activeElem;
	ParticleColor * activeColor;
	ColorWheel * colorWheel;
	UiMenuStates * menuStates;
	
	UiState();
	~UiState();
	
	void reset();
};

extern UiState * g_uiState;

extern bool g_doActions;
extern bool g_doDraw;
extern int g_drawX;
extern int g_drawY;

void initUi();
void shutUi();

void drawUiRectCheckered(float x1, float y1, float x2, float y2, float scale);
void drawUiCircle(const float x, const float y, const float radius, const float r, const float g, const float b, const float a);
void drawUiShadedTriangle(float x1, float y1, float x2, float y2, float x3, float y3, const Color & c1, const Color & c2, const Color & c3);

extern void hlsToRGB(float hue, float lum, float sat, float & r, float & g, float & b);
extern void rgbToHSL(float r, float g, float b, float & hue, float & lum, float & sat);
extern void srgbToLinear(float r, float g, float b, float & out_r, float & out_g, float & out_b);
extern void linearToSrgb(float r, float g, float b, float & out_r, float & out_g, float & out_b);

//

void makeActive(UiState * state, const bool doActions, const bool doDraw);

void pushMenu(const char * name, const int width = 0);
void popMenu();
void resetMenu();

//

bool doButton(const char * name, const float xOffset, const float xScale, const bool lineBreak);
bool doButton(const char * name);

UiTextboxResult doTextBox(int & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt);
UiTextboxResult doTextBox(float & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt);
UiTextboxResult doTextBox(double & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt);
UiTextboxResult doTextBox(std::string & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt);

UiTextboxResult doTextBox(int & value, const char * name, const float dt);
UiTextboxResult doTextBox(float & value, const char * name, const float dt);
UiTextboxResult doTextBox(double & value, const char * name, const float dt);
UiTextboxResult doTextBox(std::string & value, const char * name, const float dt);

bool doCheckBox(bool & value, const char * name, const bool isCollapsable);

bool doDrawer(bool & value, const char * name);

void doLabel(const char * text, const float xAlign);
void doBreak();

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

void doEnumImpl(int & value, const char * name, const std::vector<EnumValue> & enumValues);

template <typename E>
void doEnum(E & value, const char * name, const std::vector<EnumValue> & enumValues)
{
	int valueInt = int(value);
	doEnumImpl(valueInt, name, enumValues);
	value = E(valueInt);
}

void doDropdownImpl(int & value, const char * name, const std::vector<EnumValue> & enumValues);

template <typename E>
void doDropdown(E & value, const char * name, const std::vector<EnumValue> & enumValues)
{
	int valueInt = int(value);
	doDropdownImpl(valueInt, name, enumValues);
	value = E(valueInt);
}

void doParticleCurve(ParticleCurve & curve, const char * name);
void doParticleColor(ParticleColor & color, const char * name);
void doParticleColorCurve(ParticleColorCurve & curve, const char * name);

void doColorWheel(float & r, float & g, float & b, float & a, const char * name, const float dt);
void doColorWheel(ParticleColor & color, const char * name, const float dt);
