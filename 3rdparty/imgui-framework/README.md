# README #

ImGui bindings for Framework.

## Usage ##

#include "imgui-framework.h"

FrameworkImGuiContext context;

context.init();

while (!framework.quitRequested)
{
	context.processBegin(framework.timeStep);
	{
		ImGui::Demo();
	}
	context.processEnd();

	framework.beginDraw(100, 110, 120);
	{
		context.draw();
	}
	framework.endDraw();
}

context.shut();

## License ##
