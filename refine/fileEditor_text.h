#pragma once

#include "fileEditor.h"
#include "framework.h"
#include "imgui/TextEditor.h"
#include "nfd.h"
#include "TextIO.h"
#include <SDL2/SDL.h> // SDL_ShowSimpleMessageBox

// todo : add option to select font

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

struct FileEditor_Text : FileEditor
{
	std::string path;
	TextEditor textEditor;
	TextIO::LineEndings lineEndings;
	bool isValid = false;
	bool textIsDirty = false;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Text(const char * in_path)
	{
		path = in_path;
		
		textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());

		isValid = loadIntoTextEditor(path.c_str(), lineEndings, textEditor);
		
		guiContext.init();
		
		// set a custom background color on the text editor
		const float lightness = .02f;
		TextEditor::Palette palette = textEditor.GetPalette();
		palette[(int)TextEditor::PaletteIndex::Background] = ImGui::ColorConvertFloat4ToU32(ImVec4(lightness, lightness, lightness, 1.f));
		palette[(int)TextEditor::PaletteIndex::Selection] = ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, .5f, 1.f, .5f));
		palette[(int)TextEditor::PaletteIndex::Identifier] = ImGui::ColorConvertFloat4ToU32(ImVec4(1.f, 0.5f, 0.5f, 1.f));
		palette[(int)TextEditor::PaletteIndex::Comment] = ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 1.f, 0.f, 1.f));
		palette[(int)TextEditor::PaletteIndex::MultiLineComment] = ImGui::ColorConvertFloat4ToU32(ImVec4(0.f, 1.f, 0.f, 1.f));
		textEditor.SetPalette(palette);
		textEditor.SetTabSize(8);
		
	#if 1
		// set a custom font on the text editor
		// todo : find a good mono spaced font for the editor
		// todo : update to latest version of the text editor, and evaluate support for non mono spaces fonts
		guiContext.pushImGuiContext();
		ImGuiIO& io = ImGui::GetIO();
		io.FontDefault = io.Fonts->AddFontFromFileTTF(framework.resolveResourcePath("calibri.ttf"), 18);
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
		
	#if defined(MACOS)
		const bool commandKeyDown = keyboard.isDown(SDLK_LGUI) || keyboard.isDown(SDLK_RGUI);
	#else
		const bool commandKeyDown = keyboard.isDown(SDLK_LCTRL) || keyboard.isDown(SDLK_RCTRL);
	#endif
		
		if ((commandKeyDown && keyboard.wentDown(SDLK_s)) && isValid)
		{
			const std::vector<std::string> lines = textEditor.GetTextLines();
			
			if (TextIO::save(path.c_str(), lines, lineEndings) == false)
			{
				SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save error", "Failed to save to file", nullptr);
			}
			else
			{
				textIsDirty = false;
			}
		}
		
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
								
								textIsDirty = false;
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
							else
							{
								textIsDirty = false;
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
									
									textIsDirty = false;
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
					textIsDirty ? "dirty" : "-----",
					textEditor.GetLanguageDefinition().mName.c_str(), path.c_str());
				
				textEditor.Render(filename.c_str(), ImVec2(sx - 20, sy - 70), false);
				
				textIsDirty |= textEditor.IsTextChanged();
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
