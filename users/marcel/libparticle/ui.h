#pragma once

struct ParticleColor;
struct ParticleColorCurve;
struct ParticleCurve;
struct UiElem;
struct UiMenu;

extern ParticleColor * g_activeColor;
extern UiElem * g_activeElem;

void initUi();
void shutUi();

void drawUiRectCheckered(float x1, float y1, float x2, float y2, float scale);
void drawUiCircle(const float x, const float y, const float radius, const float r, const float g, const float b, const float a);

extern void hlsToRGB(float hue, float lum, float sat, float & r, float & g, float & b);
extern void rgbToHSL(float r, float g, float b, float & hue, float & lum, float & sat);

//

void pushMenu(const char * name, const int width = 0);
void popMenu();
void resetMenuState();

//

bool doButton(const char * name, const float xOffset, const float xScale, const bool lineBreak);

void doTextBox(int & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt);
void doTextBox(float & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt);
void doTextBox(std::string & value, const char * name, const float xOffset, const float xScale, const bool lineBreak, const float dt);

void doTextBox(int & value, const char * name, const float dt);
void doTextBox(float & value, const char * name, const float dt);
void doTextBox(std::string & value, const char * name, const float dt);

bool doCheckBox(bool & value, const char * name, const bool isCollapsable);

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

void doParticleCurve(ParticleCurve & curve, const char * name);
void doParticleColor(ParticleColor & color, const char * name);
void doParticleColorCurve(ParticleColorCurve & curve, const char * name);

void doColorWheel(ParticleColor & color, const char * name, const float dt);
