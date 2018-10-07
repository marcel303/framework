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
	ImGui calls should be made between processBegin and processEnd calls. The first parameter to processBegin is the time since the last frame for which the ImGui context got processed. The second and third parameters are the display size ImGui should take into consideration when (re)sizing and moving elements. inputIsCaptured is a boolean which is usually set to false, but may be true in case there's ui on top of ImGui which has already captured input. When ImGui captures the input, this boolean will be set to true. In this case, ui and code running 'below' ImGui shouldn't handle mouse and keyboard input, as ImGui has already claimed exclusive access to it.
	*/

	bool inputIsCaptured = false;

	context.processBegin(framework.timeStep, 640, 480, inputIsCaptured);
	{
		ImGui::Begin("My Window");
		{
			ImGui::Button("Click Me!");
		}
		ImGui::End();
	}
	context.processEnd();

	framework.beginDraw(100, 110, 120, 0);
	{
		context.draw();
	}
	framework.endDraw();
}

context.shut();
```

## License ##

The following license information appears on top of each source file.

```
Copyright (C) 2017 Marcel Smit
marcel303@gmail.com
https://www.facebook.com/marcel.smit981

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
```
