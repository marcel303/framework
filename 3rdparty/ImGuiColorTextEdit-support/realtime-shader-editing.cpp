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
	if (framework.init(GFX_SX, GFX_SY))
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
		
		Shader shader("shader");
		
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
					std::vector<std::string> errorMessages;
					TextEditor::ErrorMarkers errorMarkers;
					if (shader.getErrorMessages(errorMessages))
					{
						for (size_t i = 0; i < errorMessages.size(); ++i)
						{
							char type[128];
							int lineNumber;
							int fileId;
							char message[4096];
							
							// fixme : this is very unsafe. there is no gaurantee we won't cause a buffer overflow in either type or message buffers!
							
							const int n = sscanf(errorMessages[i].c_str(), "%s %d:%d: %s", type, &fileId, &lineNumber, message);
							
							if (n == 4 && fileId == 0 && strstr(type, "ERROR") != nullptr)
							{
								errorMarkers[lineNumber + 1] = message;
							}
						}
					}
					textEditor.SetErrorMarkers(errorMarkers);

					textEditor.Render("TextEditor", ImVec2(0, GFX_SY - 130));
					
					ImGui::BeginChild("ShaderConstants", ImVec2(0, 100));
					{
						const GLuint program = shader.getProgram();
						
						GLint numUniforms = 0;
						glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
						checkErrorGL();
						
						for (auto i = 0; i < numUniforms; ++i)
						{
							GLint size = 0;
							GLenum type = GL_INVALID_ENUM;
							const GLsizei kMaxNameSize = 64;
							GLchar name[kMaxNameSize];
							GLsizei nameSize = 0;
							
							glGetActiveUniform(program, (GLuint)i, kMaxNameSize, &nameSize, &size, &type, name);
							checkErrorGL();
							
							if (nameSize <= 0)
								continue;
							
							const GLint location = glGetUniformLocation(program, name);
							
							if (location == 0)
								continue;
							
							ImGui::PushID(i);
							
							if (type == GL_FLOAT)
							{
								float value[1] = { 0.f };
								glGetUniformfv(program, location, value);
								checkErrorGL();
								
								if (ImGui::InputFloat(name, value))
									shader.setImmediate(name, value[0]);
							}
							else if (type == GL_FLOAT_VEC2)
							{
								float value[2] = { 0.f };
								glGetUniformfv(program, location, value);
								checkErrorGL();
								
								if (ImGui::InputFloat2(name, value))
									shader.setImmediate(name, value[0], value[1]);
							}
							else if (type == GL_FLOAT_VEC3)
							{
								float value[3] = { 0.f };
								glGetUniformfv(program, location, value);
								checkErrorGL();
								
								if (ImGui::InputFloat3(name, value))
									shader.setImmediate(name, value[0], value[1], value[2]);
							}
							else if (type == GL_FLOAT_VEC4)
							{
								float value[4] = { 0.f };
								glGetUniformfv(program, location, value);
								checkErrorGL();
								
								if (ImGui::InputFloat4(name, value))
									shader.setImmediate(name, value[0], value[1], value[2], value[3]);
							}
							else if (type == GL_SAMPLER_2D)
							{
								GLint value = 0;
								glGetUniformiv(program, location, &value);
								checkErrorGL();
								
								glActiveTexture(GL_TEXTURE0 + value);
								checkErrorGL();
								
								GLint texture = 0;
								glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
								checkErrorGL();
								
								if (ImGui::InputInt(name, &texture))
									shader.setTexture(name, value, texture);
								
								glActiveTexture(GL_TEXTURE0);
								checkErrorGL();
							}
							
							clearShader();
							
							ImGui::PopID();
						}
					}
					ImGui::EndChild();
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
					setShader(shader);
					shader.setImmediate("time", framework.time);
					shader.setImmediate("mouse", mouse.x, mouse.y);
					shader.setImmediate("mouse_down", mouse.isDown(BUTTON_LEFT));
					
					if (shader.isValid())
					{
						setColor(colorWhite);
						drawRect(0, 0, window.getWidth(), window.getHeight());
					}
					
					clearShader();
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
