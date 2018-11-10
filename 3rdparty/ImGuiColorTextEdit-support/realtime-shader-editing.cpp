#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include "imgui-framework.h"
#include "Path.h"
#include "TextIO.h"
#include <fstream>

#include "data/Shader.vs"
#include "data/Shader.ps"

#define GFX_SX 800
#define GFX_SY 800

static bool showFileBrowser(std::string & shaderNamePs, const std::vector<std::string> & files, std::string & filename);
static void showUniforms(Shader & shader);
static void showStatistics(Shader & shader);
static void showErrors(Shader & shader);

enum Tab
{
	kTab_Uniforms,
	kTab_Errors,
	kTab_Statistics,
	kTab_FileBrowser
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (framework.init(GFX_SX, GFX_SY))
	{
		FrameworkImGuiContext framework_context;
		
		framework_context.init();
		
		TextEditor textEditor;

		textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
		
		shaderSource("demoShader.vs", s_shaderVs);
		shaderSource("demoShader.ps", s_shaderPs);
		
		textEditor.SetText(s_shaderPs);
		
		SDL_StartTextInput();
		
		std::string shaderPs = s_shaderPs;
		
		Window window("Preview", 600, 600, true);
		
		Shader shader("demoShader");
		std::string shaderNamePs = "demoShader.ps";
		
		Tab tab = kTab_Uniforms;
		
		auto files = listFiles("testShaders", false);
		auto removed = std::remove_if(files.begin(), files.end(), [](const std::string & s) { return Path::GetExtension(s, true) != "ps"; });
		files.erase(removed, files.end());
		std::sort(files.begin(), files.end());
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
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
							ImGui::MenuItem("Load..");
							ImGui::MenuItem("Save as..");
							ImGui::EndMenu();
						}
						
						if (ImGui::BeginMenu("Edit"))
						{
							if (ImGui::MenuItem("Undo"))
								textEditor.Undo();
							if (ImGui::MenuItem("Redo"))
								textEditor.Redo();
							
							ImGui::Separator();
							if (ImGui::MenuItem("Copy"))
								textEditor.Copy();
							if (ImGui::MenuItem("Cut"))
								textEditor.Cut();
							if (ImGui::MenuItem("Paste"))
								textEditor.Paste();
							
							ImGui::Separator();
							if (ImGui::MenuItem("Select all"))
								textEditor.SelectAll();
							
							ImGui::EndMenu();
						}
						
						ImGui::EndMenuBar();
					}
					
					std::vector<std::string> errorMessages;
					TextEditor::ErrorMarkers errorMarkers;
					if (shader.getErrorMessages(errorMessages))
					{
						// make sure the buffers passed to sscanf are large enough to store the result
						
						size_t maxSize = 0;
						
						for (size_t i = 0; i < errorMessages.size(); ++i)
							if (errorMessages[i].size() > maxSize)
								maxSize = errorMessages[i].size();
						
						char * type = new char[maxSize + 1];
						char * message = new char[maxSize + 1];
						
						for (size_t i = 0; i < errorMessages.size(); ++i)
						{
							int lineNumber;
							int fileId;
							
							const int n = sscanf(errorMessages[i].c_str(), "%s %d:%d: %[^]", type, &fileId, &lineNumber, message);
							
							if (n == 4 && fileId == 0 && strstr(type, "ERROR") != nullptr)
							{
								errorMarkers[lineNumber + 1] = message;
							}
						}
						
						delete [] type;
						delete [] message;
					}
					textEditor.SetErrorMarkers(errorMarkers);

					textEditor.Render("TextEditor", ImVec2(0, GFX_SY - 170 - 30));
					
				#if 1
					if (ImGui::RadioButton("File browser", tab == kTab_FileBrowser))
						tab = kTab_FileBrowser;
					ImGui::SameLine();
					if (ImGui::RadioButton("Uniforms", tab == kTab_Uniforms))
						tab = kTab_Uniforms;
					ImGui::SameLine();
					if (ImGui::RadioButton("Statistics", tab == kTab_Statistics))
						tab = kTab_Statistics;
					ImGui::SameLine();
					if (ImGui::RadioButton("Errors", tab == kTab_Errors))
						tab = kTab_Errors;
				#endif
					
					ImGui::BeginChild("Tabs", ImVec2(0, 170));
					{
						if (tab == kTab_FileBrowser)
						{
							std::string filename;
							
							if (showFileBrowser(shaderNamePs, files, filename))
							{
								std::vector<std::string> lines;
								TextIO::LineEndings lineEndings;
								
								if (TextIO::load(filename.c_str(), lines, lineEndings))
								{
									textEditor.SetTextLines(lines);
								
									// we need to create a virtual copy of the shader from disk. otherwise,
									// when the automatic shader reloading kicks in, it will reload the file
									// from disk, instead of the cached shader source we give it here
									// we just simply append '-copy' to the original file name to make it distinct
									shaderNamePs = shaderNamePs + "-copy";
									
									// register the shader's source code with framework
									const std::string & source = textEditor.GetText();
									shaderSource(shaderNamePs.c_str(), source.c_str());
									shaderPs = source;
									
									// and set the shader
									shader = Shader(shaderNamePs.c_str(), "testShader.vs", shaderNamePs.c_str());
								}
							}
						}
						else if (tab == kTab_Uniforms)
							showUniforms(shader);
						else if (tab == kTab_Statistics)
							showStatistics(shader);
						else if (tab == kTab_Errors)
							showErrors(shader);
						else
							Assert(false);
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
				
				shaderSource(shaderNamePs.c_str(), shaderPs.c_str());
			}
			
			framework.beginDraw(80, 90, 100, 0);
			{
				framework_context.draw();
			}
			framework.endDraw();
			
			pushWindow(window);
			{
				if (keyboard.wentDown(SDLK_ESCAPE))
					framework.quitRequested = true;
				
				framework.beginDraw(0, 0, 0, 0);
				{
					setShader(shader);
					shader.setImmediate("time", framework.time);
					shader.setImmediate("mouse", mouse.x / float(window.getWidth()), mouse.y / float(window.getHeight()));
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

static void showUniforms(Shader & shader)
{
	const GLuint program = shader.getProgram();
	
	GLint numUniforms = 0;

	if (shader.isValid())
	{
		glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
		checkErrorGL();
	}

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

static bool showFileBrowser(std::string & shaderNamePs, const std::vector<std::string> & files, std::string & filename)
{
	const int numItems = int(files.size());
	const char ** items = (const char **)alloca(sizeof(char*) * numItems);
	for (int i = 0; i < numItems; ++i)
		items[i] = files[i].c_str();
	
	int currentItem = -1;
	
	if (ImGui::ListBox("File", &currentItem, items, numItems))
	{
		filename = items[currentItem];
		
		shaderNamePs = filename;
		
		return true;
	}
	else
	{
		return false;
	}
}

static void showStatistics(Shader & shader)
{
}

static void showErrors(Shader & shader)
{
	std::vector<std::string> errorMessages;
	TextEditor::ErrorMarkers errorMarkers;
	if (shader.getErrorMessages(errorMessages))
	{
		const int numItems = int(errorMessages.size());
		const char ** items = (const char**)alloca(sizeof(char*) * numItems);
		for (size_t i = 0; i < numItems; ++i)
			items[i] = errorMessages[i].c_str();
		int currentItem = -1;
		ImGui::ListBox("Errors", &currentItem, items, numItems);
	}
}
