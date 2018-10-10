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

#include "framework.h"
#include "imgui.h"
#include "imgui-framework.h"

FrameworkImGuiContext::~FrameworkImGuiContext()
{
	fassert(font_texture_id == 0);
}

void FrameworkImGuiContext::init()
{
	imgui_context = ImGui::CreateContext();
	
	pushImGuiContext();
	
	auto & io = ImGui::GetIO();
	
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	
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
	
	// set clipboard handling functions
	
	io.SetClipboardTextFn = setClipboardText;
	io.GetClipboardTextFn = getClipboardText;
	io.ClipboardUserData = this;
	
	// create mouse cursors
	
	mouse_cursors[ImGuiMouseCursor_Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
	mouse_cursors[ImGuiMouseCursor_TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
	mouse_cursors[ImGuiMouseCursor_ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
	mouse_cursors[ImGuiMouseCursor_ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
	mouse_cursors[ImGuiMouseCursor_ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
	mouse_cursors[ImGuiMouseCursor_ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
	mouse_cursors[ImGuiMouseCursor_ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
	mouse_cursors[ImGuiMouseCursor_Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
	
	// create font texture
	
	int sx, sy;
	uint8_t * pixels = nullptr;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &sx, &sy);
	
	font_texture_id = createTextureFromRGBA8(pixels, sx, sy, false, true);
	io.Fonts->TexID = (void*)(uintptr_t)font_texture_id;
	
	popImGuiContext();
}

void FrameworkImGuiContext::shut()
{
	if (font_texture_id != 0)
	{
		glDeleteTextures(1, &font_texture_id);
		font_texture_id = 0;
	}
	
	for (int i = 0; i < ImGuiMouseCursor_COUNT; ++i)
	{
		if (mouse_cursors[i] != nullptr)
		{
			SDL_FreeCursor(mouse_cursors[i]);
			mouse_cursors[i] = nullptr;
		}
	}
	
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
	
	io.DeltaTime = dt;
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
	#if DO_TOUCH_SCROLL
		num_touches = 0;
	#endif
		kinetic_scroll.SetZero();
	#endif
	}
	else
	{
		io.MouseDown[0] = mouse.isDown(BUTTON_LEFT);
		io.MouseDown[1] = mouse.isDown(BUTTON_RIGHT);
		
	#if DO_KINETIC_SCROLL
	#if DO_TOUCH_SCROLL
		Vec2 new_kinetic_scroll;
		for (auto & e : framework.events)
		{
			if (e.type == SDL_FINGERDOWN)
			{
				kinetic_scroll.SetZero();
				
				num_touches++;
			}
			else if (e.type == SDL_FINGERUP)
			{
				num_touches--;
				
				if (num_touches < 0)
					num_touches = 0;
				
				if (num_touches < 2 && fabsf(kinetic_scroll.CalcSize()) < .02f)
					kinetic_scroll.SetZero();
			}
			else if (e.type == SDL_FINGERMOTION && num_touches == 2)
			{
				new_kinetic_scroll += Vec2(e.tfinger.dx * 100.f, e.tfinger.dy * 10.f);
			}
		}
		
		if (num_touches == 2)
			kinetic_scroll = new_kinetic_scroll;
		else
			kinetic_scroll *= powf(.1f, dt);
		
		io.MouseWheelH = kinetic_scroll[0];
		io.MouseWheel = kinetic_scroll[1];
	#else
	// todo : add kinectic scrolling
		if (mouse.scrollY == 0)
			kinetic_scroll *= powf(.1f, dt);
		else
			kinetic_scroll = Vec2(0.f, mouse.scrollY * -.1f);
		io.MouseWheel = kinetic_scroll[1];
	#endif
	#else
		io.MouseWheel = mouse.scrollY * -.1f;
	#endif

		io.KeyCtrl = keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL);
		io.KeyShift = keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT);
		io.KeyAlt = keyboard.isDown(SDLK_LALT) || keyboard.isDown(SDLK_RALT);
		io.KeySuper = keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI);
		
		// handle keyboard and text input
		
		for (auto & e : framework.events)
		{
			if (e.type == SDL_KEYDOWN)
				io.KeysDown[e.key.keysym.scancode] = true;
			else if (e.type == SDL_KEYUP)
				io.KeysDown[e.key.keysym.scancode] = false;
			else if (e.type == SDL_TEXTINPUT)
				io.AddInputCharactersUTF8(e.text.text);
		}
		
		inputIsCaptured |= io.WantCaptureKeyboard;
		inputIsCaptured |= io.WantCaptureMouse;
	}
	
	ImGui::NewFrame();
}

void FrameworkImGuiContext::processEnd()
{
	popImGuiContext();
}

void FrameworkImGuiContext::draw()
{
	pushImGuiContext();
	{
		ImGui::Render();
		
		render(ImGui::GetDrawData());
	}
	popImGuiContext();
}

void FrameworkImGuiContext::pushImGuiContext()
{
	fassert(previous_context == nullptr);
	previous_context = ImGui::GetCurrentContext();
	
	ImGui::SetCurrentContext(imgui_context);
}

void FrameworkImGuiContext::popImGuiContext()
{
	ImGui::SetCurrentContext(previous_context);
	previous_context = nullptr;
}

void FrameworkImGuiContext::updateMouseCursor()
{
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
}

const char * FrameworkImGuiContext::getClipboardText(void * user_data)
{
	FrameworkImGuiContext * context = (FrameworkImGuiContext*)user_data;
	
    if (context->clipboard_text != nullptr)
    {
        SDL_free(context->clipboard_text);
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
        SDL_free(context->clipboard_text);
        context->clipboard_text = nullptr;
	}
	
	//
	
    SDL_SetClipboardText(text);
}

void FrameworkImGuiContext::render(const ImDrawData * draw_data)
{
	glEnable(GL_SCISSOR_TEST);
	
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
				
				glScissor(
					(int)clip_rect.x,
					(int)(draw_data->DisplaySize.y - clip_rect.w),
					(int)(clip_rect.z - clip_rect.x),
					(int)(clip_rect.w - clip_rect.y));
				
				const GLuint textureId = (GLuint)(uintptr_t)cmd->TextureId;
				
				gxSetTexture(textureId);
				{
					gxBegin(GL_TRIANGLES);
					{
						for (int e = 0; e < cmd->ElemCount; ++e)
						{
							const int index = index_list[e];
							
							const ImDrawVert & vertex = vertex_list[index];
							
							gxColor4ub(
								(vertex.col >> IM_COL32_R_SHIFT) & 0xff,
								(vertex.col >> IM_COL32_G_SHIFT) & 0xff,
								(vertex.col >> IM_COL32_B_SHIFT) & 0xff,
								(vertex.col >> IM_COL32_A_SHIFT) & 0xff);
							
							gxTexCoord2f(vertex.uv.x, vertex.uv.y);
							gxVertex2f(vertex.pos.x, vertex.pos.y);
						}
					}
					gxEnd();
				}
				gxSetTexture(0);
				
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
	
	glDisable(GL_SCISSOR_TEST);
}
