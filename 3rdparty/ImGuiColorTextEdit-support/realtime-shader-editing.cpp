#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include "imgui-framework.h"
#include <fstream>

#include "data/Shader.vs"
#include "data/Shader.ps"

#define GFX_SX 800
#define GFX_SY 800

int main(int argc, char * argv[])
{
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		FrameworkImGuiContext framework_context;
		
		framework_context.init();
		
		TextEditor textEditor;

		textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
		
		shaderSource("shader.vs", s_shaderVs);
		shaderSource("shader.ps", s_shaderPs);
		
		textEditor.SetText(s_shaderPs);
		
		SDL_StartTextInput();
		
		std::string shaderPs = s_shaderPs;
		
		Window window("Preview", 600, 600, true);
		
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
						ImGuiWindowFlags_HorizontalScrollbar);
				{
					Shader shader("shader");
					std::vector<std::string> errorMessages;
					TextEditor::ErrorMarkers errorMarkers;
					if (shader.getErrorMessages(errorMessages))
					{
						for (size_t i = 0; i < errorMessages.size(); ++i)
							errorMarkers[i + 1] = errorMessages[i];
					}
					textEditor.SetErrorMarkers(errorMarkers);

					textEditor.Render("TextEditor");
				}
				ImGui::End();
			
			// update mouse cursor
			
				framework_context.updateMouseCursor();
			}
			framework_context.processEnd();
			
			const std::string newShaderPs = textEditor.GetText();
			
			if (newShaderPs != shaderPs)
			{
				shaderPs = newShaderPs;
				
				shaderSource("shader.ps", shaderPs.c_str());
			}
			
			framework.beginDraw(80, 90, 100, 0);
			{
				framework_context.draw();
			}
			framework.endDraw();
			
			pushWindow(window);
			{
				framework.beginDraw(0, 0, 0, 0);
				{
					Shader shader("shader");
					shader.setImmediate("time", framework.time);
					shader.setImmediate("mouse", mouse.x, mouse.y);
					shader.setImmediate("mouse_down", mouse.isDown(BUTTON_LEFT));
					
					if (shader.isValid())
					{
						setShader(shader);
						setColor(colorWhite);
						drawRect(0, 0, window.getWidth(), window.getHeight());
						clearShader();
					}
				}
				framework.endDraw();
			}
			popWindow();
		}
		
		SDL_StopTextInput();
		
		framework_context.shut();

		framework.shutdown();
	}

	return 0;
}
