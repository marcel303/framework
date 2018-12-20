#include "framework.h"
#include "OptionMenu.h"
#include "Options.h"
#include "StatTimerMenu.h"
#include "StatTimers.h"

OPTION_DECLARE(bool, s_checkbox, true);
OPTION_DECLARE(int, s_integer, 10);
OPTION_DECLARE(float, s_float, 3.14f);
OPTION_DECLARE(float, s_suboption1, 1.f);
OPTION_DECLARE(float, s_suboption2, 1.f);

OPTION_DEFINE(bool, s_checkbox, "Checkbox Option");
OPTION_DEFINE(int, s_integer, "Integer Option");
OPTION_DEFINE(float, s_float, "Float Option");
OPTION_DEFINE(float, s_suboption1, "Suboptions/Option 1");
OPTION_DEFINE(float, s_suboption2, "Suboptions/Option 2");

static float s_flash = 0.f;
COMMAND_OPTION(s_command, "Command Option", []{ logDebug("Command!"); s_flash = 1.f; });

TIMER_DEFINE(s_timer1, StatTimer::PerFrame, "Timer 1");
TIMER_DEFINE(s_timer2, StatTimer::PerSecond, "Timer 2");
TIMER_DEFINE(s_drawTime, StatTimer::PerFrame, "Draw");

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (!framework.init(800, 600))
		return -1;

	OptionMenu optionMenu;
	
	StatTimerMenu statMenu;
	
	Shader shader("shader");
	
	for (;;)
	{
		TIMER_INC(s_timer2);
		
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		const float dt = framework.timeStep;
		
		TIMER_ADD_SECONDS(s_timer1, dt);
		
		s_flash = fmaxf(s_flash - dt, 0.f);
		
		//
		
		g_statTimerManager.Update();
		
		optionMenu.Update();
		
		statMenu.Update();
	
		//
		
		MultiLevelMenuBase * menu = &optionMenu;
	
		if (keyboard.isDown(SDLK_UP) || gamepad[0].isDown(DPAD_UP))
			menu->HandleAction(OptionMenu::Action_NavigateUp, dt);
		if (keyboard.isDown(SDLK_DOWN) || gamepad[0].isDown(DPAD_DOWN))
			menu->HandleAction(OptionMenu::Action_NavigateDown, dt);
		if (keyboard.wentDown(SDLK_RETURN) || gamepad[0].wentDown(GAMEPAD_A))
			menu->HandleAction(OptionMenu::Action_NavigateSelect);
		if (keyboard.wentDown(SDLK_BACKSPACE) || gamepad[0].wentDown(GAMEPAD_B))
		{
			if (menu->HasNavParent())
				menu->HandleAction(OptionMenu::Action_NavigateBack);
		}
		if (keyboard.wentDown(SDLK_LEFT, true) || gamepad[0].wentDown(DPAD_LEFT))
			menu->HandleAction(OptionMenu::Action_ValueDecrement, dt);
		if (keyboard.wentDown(SDLK_RIGHT, true) || gamepad[0].wentDown(DPAD_RIGHT))
			menu->HandleAction(OptionMenu::Action_ValueIncrement, dt);

		framework.beginDraw(80, 40, 20, 0);
		{
			TIMER_SCOPE(s_drawTime);
			
			setFont("calibri.ttf");
			
			shader.setImmediate("time", framework.time);
			shader.setImmediate("flash", s_flash);
			setShader(shader);
			drawRect(0, 0, 800, 600);
			clearShader();
			
			optionMenu.Draw(40, 170, 300, 500);
			
			statMenu.Draw(410, 200, 340, 500);
		}
		framework.endDraw();
	}
	
	framework.shutdown();

	return 0;
}
