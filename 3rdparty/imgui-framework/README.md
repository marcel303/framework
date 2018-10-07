# README #

ImGui bindings for Framework, the free and open creative coding library.
https://github.com/marcel303/framework

## Usage ##

```cpp
#include "imgui-framework.h"

// ...

FrameworkImGuiContext context;

context.init();

while (!framework.quitRequested)
{
	/*
	ImGui calls should be made between processBegin and processEnd calls. The first parameter to processBegin is the time since the last frame for which the ImGui context got processed. The second and third parameters are the display size ImGui should take into consideration when (re)sizing and moving elements.
	*/

	context.processBegin(framework.timeStep, 640, 480);
	{
		ImGui::Begin("My Window");
		{
			ImGui::Button("Click Me!");
		}
		ImGui::End();
	}
	context.processEnd();

	framework.beginDraw(100, 110, 120);
	{
		context.draw();
	}
	framework.endDraw();
}

context.shut();
```

## License ##

See the license information on top of each source file.
