#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include <fstream>

static void ShowTextEditor();

static void FrameworkRenderFunction(ImDrawData* drawData)
{
	glEnable(GL_SCISSOR_TEST);
	
	for (int n = 0; n < drawData->CmdListsCount; n++)
	{
		const ImDrawList * cmd_list = drawData->CmdLists[n];
		const ImDrawVert * vertices = cmd_list->VtxBuffer.Data;
		const ImDrawIdx * indices = cmd_list->IdxBuffer.Data;
		
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
		{
			const ImDrawCmd * pcmd = &cmd_list->CmdBuffer[cmd_i];
			
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				const ImVec2 & pos = drawData->DisplayPos;
				const ImVec4 clip_rect = ImVec4(
					pcmd->ClipRect.x - pos.x,
					pcmd->ClipRect.y - pos.y,
					pcmd->ClipRect.z - pos.x,
					pcmd->ClipRect.w - pos.y);
				
                if (clip_rect.x >= 800 || clip_rect.y > 600 || clip_rect.z < 0 || clip_rect.w < 0)
                	continue;
				
				glScissor((int)clip_rect.x, (int)(600 - clip_rect.w), (int)(clip_rect.z - clip_rect.x), (int)(clip_rect.w - clip_rect.y));
				
				const GLuint textureId = (GLuint)(uintptr_t)pcmd->TextureId;
				gxSetTexture(textureId);
				
				gxBegin(GL_TRIANGLES);
				{
					for (int e = 0; e < pcmd->ElemCount; ++e)
					{
						const int index = indices[e];
						
						const ImDrawVert & vertex = vertices[index];
						
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
				
				gxSetTexture(0);
				
				#if 1
				Assert(pcmd->ClipRect.x <= pcmd->ClipRect.z);
				Assert(pcmd->ClipRect.y <= pcmd->ClipRect.w);
				setColor(colorWhite);
				drawRectLine(
					pcmd->ClipRect.x - pos.x,
					pcmd->ClipRect.y - pos.y,
					pcmd->ClipRect.z - pos.x,
					pcmd->ClipRect.w - pos.y);
				#endif
			}
			
			indices += pcmd->ElemCount;
		}
	}
	
	glDisable(GL_SCISSOR_TEST);
}

int main(int argc, char * argv[])
{
	if (framework.init(0, nullptr, 800, 600))
	{
		ImGui::CreateContext();
     	ImGuiIO& io = ImGui::GetIO();
		
		int width, height;
		unsigned char* pixels = NULL;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
		
		const GLuint textureId = createTextureFromRGBA8(pixels, width, height, false, true);
		io.Fonts->TexID = (void*)(uintptr_t)textureId;
		
		bool showDemoWindow = true;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			io.DeltaTime = framework.timeStep;
			io.DisplaySize.x = 800;
			io.DisplaySize.y = 600;
			io.MousePos.x = mouse.x;
			io.MousePos.y = mouse.y;
			io.MouseDown[0] = mouse.isDown(BUTTON_LEFT);
			io.MouseDown[1] = mouse.isDown(BUTTON_RIGHT);
			io.MouseWheel = mouse.scrollY * -.1f;
			
			ImGui::NewFrame();
			
			ShowTextEditor();
			
			ImGui::ShowDemoWindow(&showDemoWindow);
			
			framework.beginDraw(0, 0, 0, 0);
			{
				ImGui::Render();
				
				FrameworkRenderFunction(ImGui::GetDrawData());
			}
			framework.endDraw();
		}
		
		ImGui::DestroyContext();

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
