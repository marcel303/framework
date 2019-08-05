/*
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
*/

#pragma once

#include "framework.h"
#include "imgui.h"

#define DO_KINETIC_SCROLL 1
#define DO_TOUCH_SCROLL 1

struct FrameworkImGuiContext
{
	ImGuiContext * imgui_context = nullptr;
	
	SDL_Cursor * mouse_cursors[ImGuiMouseCursor_COUNT] = { };
	
	const char * clipboard_text = nullptr;
	
	GxTexture font_texture;
	
	ImGuiContext * previous_context = nullptr;
	
#if DO_KINETIC_SCROLL
#if DO_TOUCH_SCROLL
	int num_touches = 0;
#endif
	Vec2 kinetic_scroll;
	double kinetic_scroll_smoothed[2] = { };
#endif

	~FrameworkImGuiContext();
	
	void init(const bool enableIniFiles = false);
	void shut();
	
	void processBegin(const float dt, const int displaySx, const int displaySy, bool & inputIsCaptured);
	void processEnd();
	
	void draw();
	
	void pushImGuiContext();
	void popImGuiContext();
	void updateMouseCursor();
	void updateFontTexture();
	
	void setFrameworkStyleColors();
	
	static const char * getClipboardText(void * user_data);
	static void setClipboardText(void * user_data, const char * text);
	static void render(const ImDrawData * draw_data);
};

namespace ImGui
{
	void Image(GxTextureId user_texture_id, const ImVec2& size, const ImVec2& uv0 = ImVec2(0,0), const ImVec2& uv1 = ImVec2(1,1), const ImVec4& tint_col = ImVec4(1,1,1,1), const ImVec4& border_col = ImVec4(0,0,0,0));
}
