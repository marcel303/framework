#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include "imgui-framework.h"
#include <fstream>

#define GFX_SX 800
#define GFX_SY 800

static bool ShowTextEditor();

int main(int argc, char * argv[])
{
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		//io.ImeSetInputScreenPosFn
		
		FrameworkImGuiContext framework_context;
		
		framework_context.init();
		
		bool showDemoWindow = false;
		
		bool showTextEditor = true;
		
		SDL_StartTextInput();
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			framework_context.processBegin(framework.timeStep, GFX_SX, GFX_SY);
			
			auto & io = ImGui::GetIO();
			
			if (ImGui::Begin("Statistics", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::Text("vertices: %d, indices: %d",
					io.MetricsRenderVertices,
					io.MetricsRenderIndices);
				ImGui::Text("render_windows: %d, active_windows: %d",
					io.MetricsRenderWindows,
					io.MetricsActiveWindows);
				ImGui::Text("active_allocations: %d",
					io.MetricsActiveAllocations);
				
				if (ImGui::Button("Toggle Text Editor Window"))
					showTextEditor = !showTextEditor;
			}
			ImGui::End();
			
			if (showTextEditor)
			{
				if (ShowTextEditor() == false)
					framework.quitRequested = true;
			}
			
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
		
			framework_context.updateMouseCursor();
			
			framework_context.processEnd();
			
			framework.beginDraw(80, 90, 100, 0);
			{
				framework_context.draw();
			}
			framework.endDraw();
		}
		
		SDL_StopTextInput();
		
		framework_context.shut();

		framework.shutdown();
	}

	return 0;
}

static bool ShowTextEditor()
{
	bool result = true;
	
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
	#if 0
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Save"))
			{
				auto textToSave = editor.GetText();
				/// save text....
			}
			if (ImGui::MenuItem("Quit", "Alt-F4"))
			{
				result = false;
			}
			ImGui::EndMenu();
		}
	#endif
	
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

#if 0
	ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, editor.GetTotalLines(),
		editor.IsOverwrite() ? "Ovr" : "Ins",
		editor.CanUndo() ? "*" : " ",
		editor.GetLanguageDefinition().mName.c_str(), fileToEdit);
#endif

	editor.Render("TextEditor");
	ImGui::End();
	
	return result;
}
