#include "framework.h"
#include "OptionMenu.h"
#include "Options.h"

OPTION_DECLARE(bool, s_checkbox, true);
OPTION_DECLARE(int, s_integer, 10);
OPTION_DECLARE(float, s_float, 3.14f);

OPTION_DEFINE(bool, s_checkbox, "Checkbox Option");
OPTION_DEFINE(int, s_integer, "Integer Option");
OPTION_DEFINE(float, s_float, "Float Option");

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (!framework.init(800, 600))
		return -1;

	OptionMenu optionMenu;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		const float dt = framework.timeStep;
	
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

		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			optionMenu.Update();
			
			optionMenu.Draw(10, 10, 300, 500);
		}
		framework.endDraw();
	}
	
	framework.shutdown();

	return 0;
}
