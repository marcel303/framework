#pragma once

#include "fileEditor.h"
#include "nfd.h"

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
		textEditor.SetTextLines(lines);
		
		return true;
	}
}

#define CHIBI_INTEGRATION 1

#if CHIBI_INTEGRATION
	#include "chibi.h"
	#include "nfd.h"
#endif

struct FileEditor_Text : FileEditor
{
	std::string path;
	TextEditor textEditor;
	TextIO::LineEndings lineEndings;
	bool isValid = false;
	
	FrameworkImGuiContext guiContext;
	
#if CHIBI_INTEGRATION
	struct Chibi
	{
		bool has_listed = false;
		bool has_list = false;
		
		std::vector<std::string> libraries;
		std::vector<std::string> apps;
		
		char filter[PATH_MAX] = { };
		
		std::set<std::string> selected_targets;
	};
	
	Chibi chibi;
#endif
	
	FileEditor_Text(const char * in_path)
	{
		path = in_path;
		
		textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());

		isValid = loadIntoTextEditor(path.c_str(), lineEndings, textEditor);
		
		guiContext.init();
		
		// set a custom background color on the text editor
		const float lightness = .08f;
		TextEditor::Palette palette = textEditor.GetPalette();
		palette[(int)TextEditor::PaletteIndex::Background] = ImGui::ColorConvertFloat4ToU32(ImVec4(lightness, lightness, lightness, 1.f));
		textEditor.SetPalette(palette);
		
	#if 0
		// set a custom font on the text editor
		// todo : find a good mono spaced font for the editor
		guiContext.pushImGuiContext();
		ImGuiIO& io = ImGui::GetIO();
		io.FontDefault = io.Fonts->AddFontFromFileTTF((s_dataFolder + "/SFMono-Medium.otf").c_str(), 16);
		guiContext.popImGuiContext();
		guiContext.updateFontTexture();
	#endif
	}
	
	virtual ~FileEditor_Text() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			auto filename = Path::GetFileName(path);
			
			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(sx, sy), ImGuiCond_Always);
			if (ImGui::Begin("editor", nullptr,
				ImGuiWindowFlags_MenuBar |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove))
			{
				if (ImGui::BeginMenuBar())
				{
					if (ImGui::BeginMenu("File"))
					{
						if (ImGui::MenuItem("Load.."))
						{
							nfdchar_t * filename = nullptr;
							
							if (NFD_OpenDialog(nullptr, nullptr, &filename) == NFD_OKAY)
							{
								// attempt to load file into editor
								
								isValid = loadIntoTextEditor(filename, lineEndings, textEditor);
								
								// remember path
								
								if (isValid)
								{
									path = filename;
								}
								else
								{
									path.clear();
								}
							}
							
							if (filename != nullptr)
							{
								free(filename);
								filename = nullptr;
							}
						}
						
						if (ImGui::MenuItem("Save", nullptr, false, isValid) && isValid)
						{
							const std::vector<std::string> lines = textEditor.GetTextLines();
							
							if (TextIO::save(path.c_str(), lines, lineEndings) == false)
							{
								SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save error", "Failed to save to file", nullptr);
							}
						}
						
						if (ImGui::MenuItem("Save as..", nullptr, false, isValid) && isValid)
						{
							nfdchar_t * filename = nullptr;
							
							nfdchar_t * defaultPath = nullptr;
							
							if (isValid && !path.empty())
							{
								defaultPath = new char[path.length() + 1];
							
								strcpy(defaultPath, path.c_str());
							}
							
							if (NFD_SaveDialog(nullptr, defaultPath, &filename) == NFD_OKAY)
							{
								std::vector<std::string> lines = textEditor.GetTextLines();
								
								if (TextIO::save(filename, lines, lineEndings) == false)
								{
									SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save error", "Failed to save to file", nullptr);
								}
								else
								{
									path = filename;
								}
							}
							
							if (filename != nullptr)
							{
								free(filename);
								filename = nullptr;
							}
							
							if (defaultPath != nullptr)
							{
								free(defaultPath);
								defaultPath = nullptr;
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
					
				#if CHIBI_INTEGRATION
					if (ImGui::BeginMenu("Chibi"))
					{
						if (!chibi.has_listed)
						{
							chibi.has_listed = true;
							
							char build_root[PATH_MAX];
							if (find_chibi_build_root(getDirectory().c_str(), build_root, sizeof(build_root)))
							{
								chibi.has_list = list_chibi_targets(build_root, chibi.libraries, chibi.apps);
							}
						}
						
						if (chibi.has_list)
						{
						// todo : add option to generate Xcode project (OSX) or Visual Studio 2015, 2017 solution file (Windows)
						
							if (ImGui::Button("Generate CMakeLists.txt"))
							{
								nfdchar_t * filename = nullptr;
								
								if (NFD_SaveDialog(nullptr, nullptr, &filename) == NFD_OKAY)
								{
									framework.process(); // hack fix for making the debugger response after the dialog
									
									std::string dst_path = Path::GetDirectory(filename);
									
									const int num_targets = chibi.selected_targets.size();
									const char ** targets = (const char**)alloca(num_targets * sizeof(char*));
									int index = 0;
									for (auto & target : chibi.selected_targets)
										targets[index++] = target.c_str();
									
									if (chibi_generate(nullptr, ".", dst_path.c_str(), targets, num_targets) == false)
									{
										showErrorMessage("Failed", "Failed to generate CMakeLists.txt");
									}
								}
								
								if (filename != nullptr)
								{
									free(filename);
									filename = nullptr;
								}
							}
							
							ImGui::InputText("Filter", chibi.filter, sizeof(chibi.filter));
							
							for (auto & app : chibi.apps)
							{
								if (chibi.filter[0] == 0 || strcasestr(app.c_str(), chibi.filter) != nullptr)
								{
									bool selected = chibi.selected_targets.count(app) != 0;
									if (ImGui::Selectable(app.c_str(), &selected, ImGuiSelectableFlags_DontClosePopups))
									{
										if (selected)
											chibi.selected_targets.insert(app);
										else
											chibi.selected_targets.erase(app);
									}
								}
							}
							for (auto & library : chibi.libraries)
							{
								if (chibi.filter[0] == 0 || strcasestr(library.c_str(), chibi.filter) != nullptr)
								{
									bool selected = chibi.selected_targets.count(library) != 0;
									if (ImGui::Selectable(library.c_str(), &selected, ImGuiSelectableFlags_DontClosePopups))
									{
										if (selected)
											chibi.selected_targets.insert(library);
										else
											chibi.selected_targets.erase(library);
									}
								}
							}
						}
						
						ImGui::EndMenu();
					}
				#endif

					ImGui::EndMenuBar();
				}
				else
				{
					// window focus is needed to make mouse/touchpad scrolling work for the editor
					// however, we only give it focus when the menu bar isn't active. otherwise
					// menu bar menus don't open..
					ImGui::SetNextWindowFocus();
				}
				
				auto cpos = textEditor.GetCursorPosition();
				ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, textEditor.GetTotalLines(),
					lineEndings == TextIO::kLineEndings_Unix ? "Unix" :
					lineEndings == TextIO::kLineEndings_Windows ? "Win " :
					"",
					textEditor.IsOverwrite() ? "Ovr" : "Ins",
					textEditor.CanUndo() ? "*" : " ",
					textEditor.GetLanguageDefinition().mName.c_str(), path.c_str());
				
				textEditor.Render(filename.c_str(), ImVec2(sx - 20, sy - 70), false);
			}
			ImGui::End();
			
			guiContext.updateMouseCursor();
		}
		guiContext.processEnd();
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		guiContext.draw();
	}
};
