#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include <fstream>

static void ShowTextEditor();

struct FrameworkImGuiContext
{
	ImGuiIO * io = nullptr;
	
	SDL_Cursor * mouse_cursors[ImGuiMouseCursor_COUNT] = { };
	
	char * clipboard_text = nullptr;
	
	GLuint font_texture_id = 0;
	
	~FrameworkImGuiContext()
	{
		fassert(font_texture_id == 0);
	}
	
	void init(ImGuiIO * _io)
	{
		io = _io;
		
		io->BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io->BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
		
		// setup keyboard mappings
		
		io->KeyMap[ImGuiKey_Tab] = SDL_SCANCODE_TAB;
		io->KeyMap[ImGuiKey_LeftArrow] = SDL_SCANCODE_LEFT;
		io->KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
		io->KeyMap[ImGuiKey_UpArrow] = SDL_SCANCODE_UP;
		io->KeyMap[ImGuiKey_DownArrow] = SDL_SCANCODE_DOWN;
		io->KeyMap[ImGuiKey_PageUp] = SDL_SCANCODE_PAGEUP;
		io->KeyMap[ImGuiKey_PageDown] = SDL_SCANCODE_PAGEDOWN;
		io->KeyMap[ImGuiKey_Home] = SDL_SCANCODE_HOME;
		io->KeyMap[ImGuiKey_End] = SDL_SCANCODE_END;
		io->KeyMap[ImGuiKey_Insert] = SDL_SCANCODE_INSERT;
		io->KeyMap[ImGuiKey_Delete] = SDL_SCANCODE_DELETE;
		io->KeyMap[ImGuiKey_Backspace] = SDL_SCANCODE_BACKSPACE;
		io->KeyMap[ImGuiKey_Space] = SDL_SCANCODE_SPACE;
		io->KeyMap[ImGuiKey_Enter] = SDL_SCANCODE_RETURN;
		io->KeyMap[ImGuiKey_Escape] = SDL_SCANCODE_ESCAPE;
		io->KeyMap[ImGuiKey_A] = SDL_SCANCODE_A;
		io->KeyMap[ImGuiKey_C] = SDL_SCANCODE_C;
		io->KeyMap[ImGuiKey_V] = SDL_SCANCODE_V;
		io->KeyMap[ImGuiKey_X] = SDL_SCANCODE_X;
		io->KeyMap[ImGuiKey_Y] = SDL_SCANCODE_Y;
		io->KeyMap[ImGuiKey_Z] = SDL_SCANCODE_Z;
		
		// set clipboard handling functions
		
		io->SetClipboardTextFn = setClipboardText;
		io->GetClipboardTextFn = getClipboardText;
		io->ClipboardUserData = this;
		
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
		io->Fonts->GetTexDataAsRGBA32(&pixels, &sx, &sy);
		
		font_texture_id = createTextureFromRGBA8(pixels, sx, sy, false, true);
		io->Fonts->TexID = (void*)(uintptr_t)font_texture_id;
	}
	
	void shut()
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
	}
	
	void process(const float dt, const int displaySx, const int displaySy)
	{
		io->DeltaTime = dt;
		io->DisplaySize.x = displaySx;
		io->DisplaySize.y = displaySy;
	
		io->MousePos.x = mouse.x;
		io->MousePos.y = mouse.y;
		io->MouseDown[0] = mouse.isDown(BUTTON_LEFT);
		io->MouseDown[1] = mouse.isDown(BUTTON_RIGHT);
	// todo : add kinectic scrolling
		io->MouseWheel = mouse.scrollY * -.1f;
	
		io->KeyCtrl = keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL);
		io->KeyShift = keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT);
		io->KeyAlt = keyboard.isDown(SDLK_LALT) || keyboard.isDown(SDLK_RALT);
		io->KeySuper = keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI);
	}
	
	void update_mouse_cursor()
	{
		if ((io->ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0)
		{
			const ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
			
			if (io->MouseDrawCursor || imgui_cursor == ImGuiMouseCursor_None)
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
	
	static const char * getClipboardText(void * user_data);
	static void setClipboardText(void * user_data, const char * text);
	static void render(const ImDrawData * draw_data);
};

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

int main(int argc, char * argv[])
{
	if (framework.init(0, nullptr, 800, 600))
	{
		ImGuiContext * imgui_context = ImGui::CreateContext();
		
		ImGui::SetCurrentContext(imgui_context);
     	ImGuiIO & io = ImGui::GetIO();
		
		//io.ImeSetInputScreenPosFn
		
		FrameworkImGuiContext framework_context;
		
		framework_context.init(&io);
		
		bool showDemoWindow = true;
		
		SDL_StartTextInput();
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			framework_context.process(framework.timeStep, 800, 600);
			
			for (auto & e : framework.events)
			{
				if (e.type == SDL_KEYDOWN)
					io.KeysDown[e.key.keysym.scancode] = true;
				else if (e.type == SDL_KEYUP)
					io.KeysDown[e.key.keysym.scancode] = false;
				else if (e.type == SDL_TEXTINPUT)
					io.AddInputCharactersUTF8(e.text.text);
			}
			
			//KeyCtrl, KeyShift, KeySuper, KeyDown
			//AddInputCharacter(..char..) -> IME support ?
			
			ImGui::NewFrame();
			
			ShowTextEditor();
			
			ImGui::ShowDemoWindow(&showDemoWindow);
			
		// todo : check WantCaptureMouse, WantCaptureKeyboard
		// todo : WantTextInput -> enable IME
		
		// update mouse position
		
			if (io.WantSetMousePos)
			{
				//mouse.setPosition(io.MousePos.x, io.MousePos.y);
				
				SDL_WarpMouseInWindow(nullptr, (int)io.MousePos.x, (int)io.MousePos.y);
			}
			
		// update mouse cursor
		
			framework_context.update_mouse_cursor();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				ImGui::Render();
				
				framework_context.render(ImGui::GetDrawData());
				
				logDebug("ImGui metrics: #vertices: %d, #indices: %d, #render_windows: %d, #active_windows: %d, #active_allocations: %d",
					io.MetricsRenderVertices,
					io.MetricsRenderIndices,
					io.MetricsRenderWindows,
					io.MetricsActiveWindows,
					io.MetricsActiveAllocations);
			}
			framework.endDraw();
		}
		
		SDL_StopTextInput();
		
		framework_context.shut();
		
		ImGui::DestroyContext(imgui_context);
		imgui_context = nullptr;

		framework.shutdown();
	}

	return 0;
}

static void ShowTextEditor()
{
	///////////////////////////////////////////////////////////////////////
	// TEXT EDITOR SAMPLE
	static TextEditor editor;
	
	static const char* fileToEdit = "/Users/thecat/testrepos/ImGuiColorTextEdit/TextEditor.cpp";
	
	static bool init = false;
	
	if (init == false)
	{
		init = true;
		
		auto lang = TextEditor::LanguageDefinition::CPlusPlus();

		// set your own known preprocessor symbols...
		static const char* ppnames[] = { "NULL", "PM_REMOVE",
			"ZeroMemory", "DXGI_SWAP_EFFECT_DISCARD", "D3D_FEATURE_LEVEL", "D3D_DRIVER_TYPE_HARDWARE", "WINAPI","D3D11_SDK_VERSION", "assert" };
		// ... and their corresponding values
		static const char* ppvalues[] = {
			"#define NULL ((void*)0)",
			"#define PM_REMOVE (0x0001)",
			"Microsoft's own memory zapper function\n(which is a macro actually)\nvoid ZeroMemory(\n\t[in] PVOID  Destination,\n\t[in] SIZE_T Length\n); ",
			"enum DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_DISCARD = 0",
			"enum D3D_FEATURE_LEVEL",
			"enum D3D_DRIVER_TYPE::D3D_DRIVER_TYPE_HARDWARE  = ( D3D_DRIVER_TYPE_UNKNOWN + 1 )",
			"#define WINAPI __stdcall",
			"#define D3D11_SDK_VERSION (7)",
			" #define assert(expression) (void)(                                                  \n"
			"    (!!(expression)) ||                                                              \n"
			"    (_wassert(_CRT_WIDE(#expression), _CRT_WIDE(__FILE__), (unsigned)(__LINE__)), 0) \n"
			" )"
			};

		for (int i = 0; i < sizeof(ppnames) / sizeof(ppnames[0]); ++i)
		{
			TextEditor::Identifier id;
			id.mDeclaration = ppvalues[i];
			lang.mPreprocIdentifiers.insert(std::make_pair(std::string(ppnames[i]), id));
		}

		// set your own identifiers
		static const char* identifiers[] = {
			"HWND", "HRESULT", "LPRESULT","D3D11_RENDER_TARGET_VIEW_DESC", "DXGI_SWAP_CHAIN_DESC","MSG","LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
			"ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
			"ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
			"IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "TextEditor" };
		static const char* idecls[] =
		{
			"typedef HWND_* HWND", "typedef long HRESULT", "typedef long* LPRESULT", "struct D3D11_RENDER_TARGET_VIEW_DESC", "struct DXGI_SWAP_CHAIN_DESC",
			"typedef tagMSG MSG\n * Message structure","typedef LONG_PTR LRESULT","WPARAM", "LPARAM","UINT","LPVOID",
			"ID3D11Device", "ID3D11DeviceContext", "ID3D11Buffer", "ID3D11Buffer", "ID3D10Blob", "ID3D11VertexShader", "ID3D11InputLayout", "ID3D11Buffer",
			"ID3D10Blob", "ID3D11PixelShader", "ID3D11SamplerState", "ID3D11ShaderResourceView", "ID3D11RasterizerState", "ID3D11BlendState", "ID3D11DepthStencilState",
			"IDXGISwapChain", "ID3D11RenderTargetView", "ID3D11Texture2D", "class TextEditor" };
		for (int i = 0; i < sizeof(identifiers) / sizeof(identifiers[0]); ++i)
		{
			TextEditor::Identifier id;
			id.mDeclaration = std::string(idecls[i]);
			lang.mIdentifiers.insert(std::make_pair(std::string(identifiers[i]), id));
		}
		editor.SetLanguageDefinition(lang);
		//editor.SetPalette(TextEditor::GetLightPalette());

		// error markers
		TextEditor::ErrorMarkers markers;
		markers.insert(std::make_pair<int, std::string>(6, "Example error here:\nInclude file not found: \"TextEditor.h\""));
		markers.insert(std::make_pair<int, std::string>(41, "Another example error"));
		editor.SetErrorMarkers(markers);

		// "breakpoint" markers
		//TextEditor::Breakpoints bpts;
		//bpts.insert(24);
		//bpts.insert(47);
		//editor.SetBreakpoints(bpts);

		{
			std::ifstream t(fileToEdit);
			if (t.good())
			{
				std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
				
				for (int i = 0; i < 6; ++i)
					str = str + str;
				
				editor.SetText(str);
			}
		}
	}
	
	//
	
	auto cpos = editor.GetCursorPosition();
	ImGui::Begin("Text Editor Demo", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
	ImGui::SetWindowSize(ImVec2(640, 480), ImGuiCond_FirstUseEver);
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save"))
			{
				auto textToSave = editor.GetText();
				/// save text....
			}
			if (ImGui::MenuItem("Quit", "Alt-F4"))
			{
				ImGui::EndMenu();
				ImGui::EndMenuBar();
				ImGui::End();
				return;
			}
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit"))
		{
			bool ro = editor.IsReadOnly();
			if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
				editor.SetReadOnly(ro);
			ImGui::Separator();

			if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && editor.CanUndo()))
				editor.Undo();
			if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && editor.CanRedo()))
				editor.Redo();

			ImGui::Separator();

			if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
				editor.Copy();
			if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && editor.HasSelection()))
				editor.Cut();
			if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && editor.HasSelection()))
				editor.Delete();
			if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
				editor.Paste();
			
			ImGui::Separator();
			if (ImGui::MenuItem("Fill"))
				editor.SetText("Hello\nWorld!");
			if (ImGui::MenuItem("Clear"))
				editor.SetText("");

			ImGui::Separator();

			if (ImGui::MenuItem("Select all", nullptr, nullptr))
				editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Dark palette"))
				editor.SetPalette(TextEditor::GetDarkPalette());
			if (ImGui::MenuItem("Light palette"))
				editor.SetPalette(TextEditor::GetLightPalette());
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
		editor.IsOverwrite() ? "Ovr" : "Ins",
		editor.CanUndo() ? "*" : " ",
		editor.GetLanguageDefinition().mName.c_str(), fileToEdit);

	editor.Render("TextEditor");
	ImGui::End();
}
