#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include "imgui-framework.h"
#include "nfd.h"
#include <fstream>

#include "Benchmark.h"

#define GFX_SX 800
#define GFX_SY 800

enum LineEndings
{
	kLineEndings_Unix,
	kLineEndings_Windows
};

static bool load(const char * filename, std::vector<std::string> & lines, LineEndings & lineEndings)
{
	bool result = false;
	
	FILE * file = nullptr;
	ssize_t size = 0;
	char * text = nullptr;
	
	bool found_cr = false;
	bool found_lf = false;

	std::string line;
	
	//

	file = fopen(filename, "rb");

	if (file == nullptr)
		goto cleanup;

	if (fseek(file, 0, SEEK_END) != 0)
		goto cleanup;

	size = ftell(file);
	
	if (size < 0)
		goto cleanup;

	if (fseek(file, 0, SEEK_SET) != 0)
		goto cleanup;

	text = new char[size];
	
	if (fread(text, 1, size, file) != size)
		goto cleanup;

	for (int i = 0; i < size; )
	{
		bool is_linebreak = false;

		if (text[i] == '\r')
		{
			is_linebreak = true;

			found_cr = true;

			i++;

			if (i < size && text[i] == '\n')
			{
				found_lf = true;

				i++;
			}
		}
		else if (text[i] == '\n')
		{
			is_linebreak = true;

			found_lf = true;
			
			i++;
		}
		else
		{
			line.push_back(text[i]);

			i++;
		}

		if (is_linebreak)
		{
			lines.emplace_back(std::move(line));
		}
	}
	
	if (line.empty() == false)
	{
		lines.emplace_back(std::move(line));

		line.clear();
	}
	
	//
	
	lineEndings =
		found_cr && found_lf
		? kLineEndings_Windows
		: kLineEndings_Unix;

	result = true;

cleanup:
	delete [] text;
	text = nullptr;

	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}

	return result;
}

static bool save(const char * filename, const std::vector<std::string> & lines, const LineEndings lineEndings)
{
	bool result = false;
	
	FILE * file = nullptr;
	
	file = fopen(filename, "wb");
	
	if (file == nullptr)
		goto cleanup;
	
	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string & line = lines[i];
		
		if (fwrite(line.c_str(), 1, line.size(), file) != line.size())
			goto cleanup;
		
		if (i + 1 < lines.size())
		{
			if (lineEndings == kLineEndings_Unix)
			{
				if (fwrite("\n", 1, 1, file) != 1)
					goto cleanup;
			}
			else if (lineEndings == kLineEndings_Windows)
			{
				if (fwrite("\r\n", 1, 2, file) != 2)
					goto cleanup;
			}
		}
	}
	
	result = true;
	
cleanup:
	if (file != nullptr)
	{
		fclose(file);
		file = nullptr;
	}
	
	return result;
}

static bool loadIntoTextEditor(const char * filename, LineEndings & lineEndings, TextEditor & textEditor)
{
	std::vector<std::string> lines;

	textEditor.SetText("");
	
	if (load(filename, lines, lineEndings) == false)
	{
		return false;
	}
	else
	{
		Benchmark bm("loadIntoTextEditor");
		
		textEditor.SetText(lines);
		
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
		
		LineEndings lineEndings;
		
		if (loadIntoTextEditor("ImGuiColorTextEdit/TextEditor.cpp", lineEndings, textEditor) == false)
			logError("failed to load");
		else
		{
			if (save("test.txt", textEditor.GetLines(), lineEndings) == false)
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
							if (ImGui::MenuItem("Save"))
							{
								std::vector<std::string> lines = textEditor.GetLines();
								
								if (save("test.txt", lines, lineEndings) == false)
									logError("failed to save");
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
							if (ImGui::MenuItem("Unix", nullptr, lineEndings == kLineEndings_Unix))
								lineEndings = kLineEndings_Unix;
							if (ImGui::MenuItem("Windows", nullptr, lineEndings == kLineEndings_Windows))
								lineEndings = kLineEndings_Windows;
							
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
