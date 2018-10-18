#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include "imgui-framework.h"
#include "nfd.h"
#include <fstream>

#include "Benchmark.h"

#define GFX_SX 800
#define GFX_SY 800

static bool loadIntoTextEditor(const char * filename, TextIO::LineEndings & lineEndings, TextEditor & textEditor)
{
	std::vector<std::string> lines;

	textEditor.SetText("");
	
	if (TextIO::load(filename, lines, lineEndings) == false)
	{
		return false;
	}
	else
	{
		Benchmark bm("loadIntoTextEditor");
		
		textEditor.SetTextLines(lines);
		
		return true;
	}
}

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		FrameworkImGuiContext framework_context;
		
		framework_context.init();
		
		TextEditor textEditor;

		textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
		
		TextIO::LineEndings lineEndings;
		
		if (loadIntoTextEditor("CONTRIBUTING", lineEndings, textEditor) == false)
			logError("failed to load");
		else
		{
			if (TextIO::save("test.txt", textEditor.GetTextLines(), lineEndings) == false)
				logError("failed to save");
		}
		
		SDL_StartTextInput();
		
		while (!framework.quitRequested)
		{
			framework.process();

			bool inputIsCaptured = false;
			
			framework_context.processBegin(framework.timeStep, GFX_SX, GFX_SY, inputIsCaptured);
			{
				ImGui::SetNextWindowPos(ImVec2(0, 0));
				ImGui::SetNextWindowSize(ImVec2(GFX_SX, GFX_SY));
				
				ImGui::Begin("Text Editor Demo", nullptr,
						ImGuiWindowFlags_NoTitleBar |
						ImGuiWindowFlags_NoResize |
						ImGuiWindowFlags_HorizontalScrollbar |
						ImGuiWindowFlags_MenuBar);
				{
					if (ImGui::BeginMenuBar())
					{
						if (ImGui::BeginMenu("File"))
						{
							if (ImGui::MenuItem("Save.."))
							{
								nfdchar_t * filename = nullptr;
								
								if (NFD_SaveDialog(nullptr, nullptr, &filename) == NFD_OKAY)
								{
									std::vector<std::string> lines = textEditor.GetTextLines();
									
									if (save(filename, lines, lineEndings) == false)
									{
										SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save error", "Failed to save to file", nullptr);
									}
								}
								
								if (filename != nullptr)
								{
									free(filename);
									filename = nullptr;
								}
							}

							if (ImGui::MenuItem("Load.."))
							{
								nfdchar_t * filename = nullptr;
								
								if (NFD_OpenDialog(nullptr, nullptr, &filename) == NFD_OKAY)
								{
									loadIntoTextEditor(filename, lineEndings, textEditor);
								}
								
								if (filename != nullptr)
								{
									free(filename);
									filename = nullptr;
								}
							}
							
							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu("Line Endings"))
						{
							if (ImGui::MenuItem("Unix", nullptr, lineEndings == TextIO::kLineEndings_Unix))
								lineEndings = TextIO::kLineEndings_Unix;
							if (ImGui::MenuItem("Windows", nullptr, lineEndings == TextIO::kLineEndings_Windows))
								lineEndings = TextIO::kLineEndings_Windows;
							
							ImGui::EndMenu();
						}

						ImGui::EndMenuBar();
					}

					textEditor.Render("TextEditor");
				}
				ImGui::End();
			
			// update mouse cursor
			
				framework_context.updateMouseCursor();
			}
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
