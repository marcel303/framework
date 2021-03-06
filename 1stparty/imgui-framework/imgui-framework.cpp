/*
	Copyright (C) 2020 Marcel Smit
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

#include "framework.h"
#include "imgui.h"
#include "imgui-framework.h"
#include <math.h>

#if FRAMEWORK_USE_SDL
	#include <SDL2/SDL_clipboard.h>
#endif

FrameworkImGuiContext::~FrameworkImGuiContext()
{
	fassert(font_texture.id == 0);
}

void FrameworkImGuiContext::init(const bool enableIniFiles)
{
	imgui_context = ImGui::CreateContext();
	
	pushImGuiContext();
	
	auto & io = ImGui::GetIO();
	
	if (enableIniFiles == false)
	{
		io.IniFilename = nullptr;
	}
	
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

#if FRAMEWORK_USE_SDL
	// setup keyboard mappings
	
	io.KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
	io.KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
	io.KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
	io.KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
	io.KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
	io.KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
	io.KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
	io.KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
	io.KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
	io.KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
	io.KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
	io.KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
	io.KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
	io.KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
	io.KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
	io.KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
	io.KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
#endif

#if FRAMEWORK_USE_SDL
	// set clipboard handling functions
	
	io.SetClipboardTextFn = setClipboardText;
	io.GetClipboardTextFn = getClipboardText;
	io.ClipboardUserData = this;
#endif

#if FRAMEWORK_USE_SDL
	// create mouse cursors
	
	mouse_cursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	mouse_cursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	mouse_cursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	mouse_cursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	mouse_cursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	mouse_cursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	mouse_cursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
#endif

	setFrameworkStyleColors();
	
	popImGuiContext();
	
	// update the font texture
	
	updateFontTexture();
}

void FrameworkImGuiContext::shut()
{
#if FRAMEWORK_USE_SDL
	if (clipboard_text != nullptr)
    {
        SDL_free((void*)clipboard_text);
        clipboard_text = nullptr;
	}
#endif
	
	if (font_texture.id != 0)
	{
		font_texture.free();
	}

#if FRAMEWORK_USE_SDL
	for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i)
	{
		if (mouse_cursors[i] != nullptr)
		{
			SDL_FreeCursor(mouse_cursors[i]);
			mouse_cursors[i] = nullptr;
		}
	}
#endif
	
	if (imgui_context != nullptr)
	{
		ImGui::DestroyContext(imgui_context);
		imgui_context = nullptr;
	}
}

void FrameworkImGuiContext::processBegin(const float dt, const int displaySx, const int displaySy, bool & inputIsCaptured)
{
	pushImGuiContext();
	
	auto & io = ImGui::GetIO();
	
	io.DeltaTime = fmaxf(1e-6f, dt);
	io.DisplaySize.x = displaySx;
	io.DisplaySize.y = displaySy;

	io.MousePos.x = mouse.x;
	io.MousePos.y = mouse.y;
	
	if (inputIsCaptured)
	{
		io.MouseDown[0] = false;
		io.MouseDown[1] = false;
		
		io.MouseWheel = 0.f;
		
		io.KeyCtrl = false;
		io.KeyShift = false;
		io.KeyAlt = false;
		io.KeySuper = false;
		
		memset(io.KeysDown, 0, sizeof(io.KeysDown));
		
	#if DO_KINETIC_SCROLL
		num_touches = 0;
		kinetic_scroll.SetZero();
	#endif
	}
	else
	{
		io.MouseDown[0] = mouse.isDown(BUTTON_LEFT);
		io.MouseDown[1] = mouse.isDown(BUTTON_RIGHT);
		
	#if DO_KINETIC_SCROLL
	#if FRAMEWORK_USE_SDL
		const bool hasTouchDevice = SDL_GetNumTouchDevices() > 0;
	#else
		const bool hasTouchDevice = false;
	#endif
		
		if (num_touches != 2)
		{
			kinetic_scroll *= powf(hasTouchDevice ? .1f : .01f, dt);
			
			if (fabsf(kinetic_scroll[0]) < .01f)
				kinetic_scroll[0] = 0.f;
			if (fabsf(kinetic_scroll[1]) < .01f)
				kinetic_scroll[1] = 0.f;
		}
		
	#if FRAMEWORK_USE_SDL
		if (hasTouchDevice)
		{
			Vec2 new_kinetic_scroll;
			
			for (auto & e : framework.events)
			{
				if (framework.windowIsActive == false)
				{
					kinetic_scroll.SetZero();
					kinetic_scroll_smoothed[0] = 0.0;
					kinetic_scroll_smoothed[1] = 0.0;
					
					num_touches = 0;
				}
				else if (e.type == SDL_FINGERDOWN)
				{
					kinetic_scroll.SetZero();
					kinetic_scroll_smoothed[0] = 0.0;
					kinetic_scroll_smoothed[1] = 0.0;
					
					num_touches++;
				}
				else if (e.type == SDL_FINGERUP)
				{
					num_touches--;
					
					if (num_touches < 0)
						num_touches = 0;
					
					if (num_touches == 1)
					{
						if (fabs(kinetic_scroll_smoothed[0]) < 1.2f)
							kinetic_scroll_smoothed[0] = 0.0;
						if (fabs(kinetic_scroll_smoothed[1]) < 1.2f)
							kinetic_scroll_smoothed[1] = 0.0;
						
						kinetic_scroll = Vec2(kinetic_scroll_smoothed[0], kinetic_scroll_smoothed[1]);
						
						kinetic_scroll_smoothed[0] = 0.0;
						kinetic_scroll_smoothed[1] = 0.0;
					}
				}
				else if (e.type == SDL_FINGERMOTION && num_touches == 2 && dt > 0.f/* && e.tfinger.firstMove == false*/)
				{
					new_kinetic_scroll += Vec2(e.tfinger.dx * 100.f, e.tfinger.dy * 10.f) / dt;
				}
			}
			
			const double retain = pow(0.6, dt * 100.0);
			const double attain = 1.0 - retain;
			
			kinetic_scroll_smoothed[0] = kinetic_scroll_smoothed[0] * retain + new_kinetic_scroll[0] * attain;
			kinetic_scroll_smoothed[1] = kinetic_scroll_smoothed[1] * retain + new_kinetic_scroll[1] * attain;
			
			if (num_touches == 2)
				kinetic_scroll = new_kinetic_scroll;
		}
		else
	#endif
		{
			kinetic_scroll += Vec2(0.f, mouse.scrollY * -10.f);
		}
		
		io.MouseWheelH = kinetic_scroll[0] * dt;
		io.MouseWheel  = kinetic_scroll[1] * dt;
	#else
		io.MouseWheelH = 0;
		io.MouseWheel  = mouse.scrollY;
	#endif
	
		for (auto & pointer : vrPointer)
		{
			if (pointer.isPrimary)
			{
				const float speed = 10.f;
				
				io.MouseWheelH -= pointer.getAnalog(VrAnalog_X) * speed * dt;
				io.MouseWheel  += pointer.getAnalog(VrAnalog_Y) * speed * dt;
			}
		}

		io.KeyCtrl = keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL);
		io.KeyShift = keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT);
		io.KeyAlt = keyboard.isDown(SDLK_LALT) || keyboard.isDown(SDLK_RALT);
		io.KeySuper = keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI);

	#if FRAMEWORK_USE_SDL
		// handle keyboard and text input

		for (auto & e : keyboard.events)
		{
			if (e.type == SDL_KEYDOWN)
				io.KeysDown[e.key.keysym.scancode] = true;
			else if (e.type == SDL_KEYUP)
				io.KeysDown[e.key.keysym.scancode] = false;
			else if (e.type == SDL_TEXTINPUT)
				io.AddInputCharactersUTF8(e.text.text);
		}
	#endif
		
		inputIsCaptured |= io.WantCaptureKeyboard;
		inputIsCaptured |= io.WantCaptureMouse;
	}
	
	ImGui::NewFrame();
}

void FrameworkImGuiContext::processEnd()
{
	popImGuiContext();
}

void FrameworkImGuiContext::draw() const
{
	pushImGuiContext();
	{
		ImGui::Render();
		
		render(ImGui::GetDrawData());
	}
	popImGuiContext();
}

void FrameworkImGuiContext::skipDraw() const
{
	pushImGuiContext();
	{
		ImGui::Render();

		ImGui::GetDrawData();
	}
	popImGuiContext();
}

void FrameworkImGuiContext::pushImGuiContext() const
{
	fassert(previous_context == nullptr);
	previous_context = ImGui::GetCurrentContext();
	
	ImGui::SetCurrentContext(imgui_context);
}

void FrameworkImGuiContext::popImGuiContext() const
{
	ImGui::SetCurrentContext(previous_context);
	previous_context = nullptr;
}

void FrameworkImGuiContext::updateMouseCursor()
{
#if FRAMEWORK_USE_SDL
	auto & io = ImGui::GetIO();
	
	if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0)
	{
		const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
		
		if (io.MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
		{
			// hide OS mouse cursor if imgui is drawing it or if it wants no cursor
			
			SDL_ShowCursor(SDL_FALSE);
		}
		else
		{
			// show OS mouse cursor
			
			SDL_SetCursor(
				mouse_cursors[imgui_cursor] != nullptr
				? mouse_cursors[imgui_cursor]
				: mouse_cursors[ImGuiMouseCursor_Arrow]);
			
			SDL_ShowCursor(SDL_TRUE);
		}
	}
#endif
}

void FrameworkImGuiContext::updateFontTexture()
{
	pushImGuiContext();
	
	auto & io = ImGui::GetIO();
	
	if (font_texture.id != 0)
	{
		font_texture.free();
	}
	
	// create font texture
	
	int sx, sy;
	uint8_t * pixels = nullptr;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &sx, &sy);
	
	const int numPixels = sx * sy;
	
	for (int i = 0; i < numPixels; ++i)
	{
		pixels[i * 4 + 0] = (pixels[i * 4 + 0] * (pixels[i * 4 + 3] + 1)) >> 8;
		pixels[i * 4 + 1] = (pixels[i * 4 + 1] * (pixels[i * 4 + 3] + 1)) >> 8;
		pixels[i * 4 + 2] = (pixels[i * 4 + 2] * (pixels[i * 4 + 3] + 1)) >> 8;
	}
	
	font_texture.allocate(sx, sy, GX_RGBA8_UNORM, false, true);
	font_texture.upload(pixels, 1, 0);
	io.Fonts->TexID = (void*)(uintptr_t)font_texture.id;
	
	popImGuiContext();
}

bool FrameworkImGuiContext::isIdle() const
{
	if (!mouse.isIdle() || !keyboard.isIdle())
		return false;
	
	if (kinetic_scroll != Vec2(0.f))
		return false;
	
	return true;
}

void FrameworkImGuiContext::setFrameworkStyleColors()
{
	ImVec4 * colors = ImGui::GetStyle().Colors;
	
	colors[ImGuiCol_Text]                   = ImVec4(0.60f, 0.59f, 0.54f, 1.00f);
	colors[ImGuiCol_TextDisabled]           = ImVec4(0.40f, 0.40f, 0.35f, 1.00f);
	colors[ImGuiCol_WindowBg]               = ImVec4(0.01f, 0.01f, 0.04f, 1.00f);
	colors[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
	colors[ImGuiCol_Border]                 = ImVec4(0.39f, 0.39f, 0.39f, 0.50f);
	colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
	colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.19f, 0.19f, 0.57f, 0.52f);
	colors[ImGuiCol_FrameBgActive]          = ImVec4(0.23f, 0.21f, 0.54f, 0.69f);
	colors[ImGuiCol_TitleBg]                = ImVec4(0.09f, 0.11f, 0.17f, 0.83f);
	colors[ImGuiCol_TitleBgActive]          = ImVec4(0.08f, 0.09f, 0.13f, 0.87f);
	colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.40f, 0.40f, 0.80f, 0.20f);
	colors[ImGuiCol_MenuBarBg]              = ImVec4(0.09f, 0.09f, 0.18f, 0.80f);
	colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.13f, 0.13f, 0.13f, 0.60f);
	colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.79f, 0.79f, 1.00f, 0.30f);
	colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.40f, 0.40f, 0.80f, 0.40f);
	colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
	colors[ImGuiCol_CheckMark]              = ImVec4(0.68f, 0.66f, 0.53f, 0.50f);
	colors[ImGuiCol_SliderGrab]             = ImVec4(1.00f, 1.00f, 1.00f, 0.30f);
	colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.41f, 0.39f, 0.80f, 0.60f);
	colors[ImGuiCol_Button]                 = ImVec4(0.32f, 0.34f, 0.44f, 0.62f);
	colors[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.71f, 0.79f);
	colors[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.80f, 1.00f);
	colors[ImGuiCol_Header]                 = ImVec4(0.40f, 0.40f, 0.90f, 0.45f);
	colors[ImGuiCol_HeaderHovered]          = ImVec4(0.19f, 0.19f, 0.51f, 0.68f);
	colors[ImGuiCol_HeaderActive]           = ImVec4(0.22f, 0.22f, 0.69f, 0.80f);
	colors[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
	colors[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
	colors[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
	colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
	colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
	colors[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
	colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight]           = ImVec4(0.45f, 0.45f, 0.90f, 0.80f);
	colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
}

#if FRAMEWORK_USE_SDL

const char * FrameworkImGuiContext::getClipboardText(void * user_data)
{
	FrameworkImGuiContext * context = (FrameworkImGuiContext*)user_data;

    if (context->clipboard_text != nullptr)
    {
        SDL_free((void*)context->clipboard_text);
        context->clipboard_text = nullptr;
	}
	
	//

    context->clipboard_text = SDL_GetClipboardText();
	
    return context->clipboard_text;
}

void FrameworkImGuiContext::setClipboardText(void * user_data, const char * text)
{
	FrameworkImGuiContext * context = (FrameworkImGuiContext*)user_data;
	
	if (context->clipboard_text != nullptr)
    {
        SDL_free((void*)context->clipboard_text);
        context->clipboard_text = nullptr;
	}
	
	//
	
    SDL_SetClipboardText(text);
}

#endif

void FrameworkImGuiContext::render(const ImDrawData * draw_data)
{
	for (int i = 0; i < draw_data->CmdListsCount; ++i)
	{
		const ImDrawList * cmd_list = draw_data->CmdLists[i];
		const ImDrawVert * vertex_list = cmd_list->VtxBuffer.Data;
		const ImDrawIdx * index_list = cmd_list->IdxBuffer.Data;
		
		for (int c = 0; c < cmd_list->CmdBuffer.Size; ++c)
		{
			const ImDrawCmd * cmd = &cmd_list->CmdBuffer[c];
			
			if (cmd->UserCallback != nullptr)
			{
				cmd->UserCallback(cmd_list, cmd);
			}
			else
			{
				const ImVec2 & display_position = draw_data->DisplayPos;
				
				const ImVec4 clip_rect = ImVec4(
					cmd->ClipRect.x - display_position.x,
					cmd->ClipRect.y - display_position.y,
					cmd->ClipRect.z - display_position.x,
					cmd->ClipRect.w - display_position.y);
				
                if (clip_rect.x >= draw_data->DisplaySize.x ||
                	clip_rect.y >= draw_data->DisplaySize.y ||
                	clip_rect.z < 0 ||
                	clip_rect.w < 0)
                	continue;
				
				setDrawRect(
					(int)clip_rect.x,
					(int)clip_rect.y,
					(int)(clip_rect.z - clip_rect.x),
					(int)(clip_rect.w - clip_rect.y));
				
				const GxTextureId textureId = (GxTextureId)(uintptr_t)cmd->TextureId;
				
				pushBlend(BLEND_PREMULTIPLIED_ALPHA);
				gxSetTexture(textureId);
				{
					gxBegin(GX_TRIANGLES);
					{
						for (int e = 0; e < cmd->ElemCount; ++e)
						{
							const int index = index_list[e];
							
							const ImDrawVert & vertex = vertex_list[index];
							
							const int r = (vertex.col >> IM_COL32_R_SHIFT) & 0xff;
							const int g = (vertex.col >> IM_COL32_G_SHIFT) & 0xff;
							const int b = (vertex.col >> IM_COL32_B_SHIFT) & 0xff;
							const int a = (vertex.col >> IM_COL32_A_SHIFT) & 0xff;
							
							gxColor4ub(
								(r * (a + 1)) >> 8,
								(g * (a + 1)) >> 8,
								(b * (a + 1)) >> 8,
								a);
							
							gxTexCoord2f(vertex.uv.x, vertex.uv.y);
							gxVertex2f(vertex.pos.x, vertex.pos.y);
						}
					}
					gxEnd();
				}
				gxSetTexture(0);
				popBlend();
				
			#if 0
				Assert(cmd->ClipRect.x <= cmd->ClipRect.z);
				Assert(cmd->ClipRect.y <= cmd->ClipRect.w);
				setColor(colorWhite);
				drawRectLine(
					cmd->ClipRect.x - display_position.x,
					cmd->ClipRect.y - display_position.y,
					cmd->ClipRect.z - display_position.x,
					cmd->ClipRect.w - display_position.y);
			#endif
			}
			
			index_list += cmd->ElemCount;
		}
	}
	
	clearDrawRect();
}

namespace ImGui
{
	void Image(
		GxTextureId user_texture_id,
		const ImVec2& size,
		const ImVec2& uv0, const ImVec2& uv1,
		const ImVec4& tint_col,
		const ImVec4& border_col)
	{
		ImGui::Image((ImTextureID)(uintptr_t)user_texture_id, size, uv0, uv1, tint_col, border_col);
	}
}
