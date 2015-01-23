#pragma once

#include <map>
#include <vector>
#include "Options.h"

OPTION_EXTERN(bool, g_devMode);
OPTION_EXTERN(bool, g_monkeyMode);

class OptionMenu;
class StatTimerMenu;

class App
{
	OptionMenu * m_optionMenu;
	bool m_optionMenuIsOpen;

	StatTimerMenu * m_statTimerMenu;
	bool m_statTimerMenuIsOpen;

public:
	App();
	~App();

	bool init();
	void shutdown();

	bool tick();
	void draw();
};

extern App * g_app;
extern int g_updateTicks;

extern Surface * g_colorMap;
extern Surface * g_lightMap;
extern Surface * g_finalMap;

void applyLightMap(Surface & colormap, Surface & lightmap, Surface & dest);
