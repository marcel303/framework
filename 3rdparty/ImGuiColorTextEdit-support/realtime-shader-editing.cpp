#include <GL/glew.h> // glGetUniformfv etc. todo : remove and add inspection functionality to Shader object ?
#include "framework.h"
#include "imgui.h"
#include "imgui/TextEditor.h"
#include "imgui-framework.h"
#include "nfd.h"
#include "Path.h"
#include "TextIO.h"
#include <fstream>

#include "data/Shader.vs"
#include "data/Shader.ps"

#define GFX_SX 800
#define GFX_SY 800

#define ENABLE_PREVIEW_WINDOW 0

// during real-time edits of the shader, the program is recompiled and the currently set uniforms are lost.
// to avoid this loss, uniforms are saved and restored before and after the program is recompiled
struct SavedUniform
{
	static const GLsizei kMaxNameSize = 64;
	
	GLenum type = GL_INVALID_ENUM;
	GLchar name[kMaxNameSize];
	GLfloat floatValue[4] = { };
	GLint intValue = 0;
};

static bool showFileBrowser(std::string & shaderNamePs, const std::vector<std::string> & files, std::string & filename);
static bool loadIntoTextEditor(const char * filename, TextIO::LineEndings & lineEndings, TextEditor & textEditor);
static void showUniforms(Shader & shader);
static void saveUniforms(Shader & shader, std::vector<SavedUniform> & result);
static void loadUniforms(Shader & shader, const std::vector<SavedUniform> & uniforms, const bool allowMissingUniforms);
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
		
	#if ENABLE_PREVIEW_WINDOW
		Window window("Preview", 600, 600, true);
	#endif
		
		Shader shader("demoShader");
		std::string shaderNamePs = "demoShader.ps";
		
		Tab tab = kTab_Uniforms;
		
		auto files = listFiles("testShaders", false);
		auto removed = std::remove_if(files.begin(), files.end(), [](const std::string & s) { return Path::GetExtension(s, true) != "ps"; });
		files.erase(removed, files.end());
		std::sort(files.begin(), files.end());
		
		float idleTime = 0.f;
		float alphaAnim = 0.f;
		bool showEditor = true;
		
		std::vector<SavedUniform> savedUniforms;
		
		std::string path;
		bool pathIsValid = false;
		
		TextIO::LineEndings lineEndings = TextIO::kLineEndings_Unix;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			Assert(pathIsValid == !path.empty());
			
			bool inputIsCaptured = false;
			
			if (keyboard.isDown(SDLK_LSHIFT) && keyboard.wentDown(SDLK_TAB))
			{
				inputIsCaptured = true;
				showEditor = !showEditor;
			}
			
			if (showEditor == false)
				inputIsCaptured = true;
			
			if (keyboard.isIdle() && mouse.isIdle())
				idleTime += framework.timeStep;
			else
				idleTime = 0.f;
			
			if (idleTime < 4.f && showEditor)
				alphaAnim -= framework.timeStep / .5f;
			else
				alphaAnim += framework.timeStep / (showEditor ? 2.f : .5f);
			
			alphaAnim = clamp(alphaAnim, 0.f, 1.f);
			
			framework_context.processBegin(framework.timeStep, GFX_SX, GFX_SY, inputIsCaptured);
			{
				const float alpha = lerp(.9f, .0f, alphaAnim);
				ImGui::GetStyle().Alpha = alpha;
				
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
							if (ImGui::MenuItem("Load.."))
							{
								nfdchar_t * filename = nullptr;
								
								if (NFD_OpenDialog(nullptr, nullptr, &filename) == NFD_OKAY)
								{
									if (loadIntoTextEditor(filename, lineEndings, textEditor))
									{
										path = filename;
										pathIsValid = true;
									}
									else
									{
										path.clear();
										pathIsValid = false;
									}
								}
								
								if (filename != nullptr)
								{
									free(filename);
									filename = nullptr;
								}
							}
							
							if (ImGui::MenuItem("Save", nullptr, false, pathIsValid))
							{
								if (pathIsValid)
								{
									auto lines = textEditor.GetTextLines();
									
									if (TextIO::save(path.c_str(), lines, lineEndings) == false)
									{
										SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save error", "Failed to save to file", nullptr);
									}
								}
							}
							
							if (ImGui::MenuItem("Save as.."))
							{
								nfdchar_t * filename = nullptr;
								
								if (NFD_SaveDialog(nullptr, nullptr, &filename) == NFD_OKAY)
								{
									auto lines = textEditor.GetTextLines();
									
									if (TextIO::save(filename, lines, lineEndings) == false)
									{
										SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save error", "Failed to save to file", nullptr);
										
										path.clear();
										pathIsValid = false;
									}
									else
									{
										path = filename;
										pathIsValid = true;
									}
								}
								
								if (filename != nullptr)
								{
									free(filename);
									filename = nullptr;
								}
							}
							
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
							
							const int n = sscanf(errorMessages[i].c_str(), "%s %d:%d: %[^\n]", type, &fileId, &lineNumber, message);
							
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
								
								if (TextIO::load(filename.c_str(), lines, lineEndings) == false)
								{
									path.clear();
									pathIsValid = false;
								}
								else
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
									
									path = filename;
									pathIsValid = true;
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
				
				if (shader.isValid())
					saveUniforms(shader, savedUniforms);
				
				shaderSource(shaderNamePs.c_str(), shaderPs.c_str());
				
				loadUniforms(shader, savedUniforms, true);
			}
			
			auto setShaderConstants = [&](const int viewSx, const int viewSy)
			{
				shader.setImmediate("time", framework.time);
				shader.setImmediate("mouse", mouse.x / float(viewSx), mouse.y / float(viewSy));
				shader.setImmediate("mouse_down", mouse.isDown(BUTTON_LEFT));
			};
			
			framework.beginDraw(0, 0, 0, 0);
			{
				if (shader.isValid())
				{
					setShader(shader);
					setShaderConstants(GFX_SX, GFX_SY);
					drawRect(0, 0, GFX_SX, GFX_SY);
					clearShader();
				}
				
				framework_context.draw();
			}
			framework.endDraw();
			
		#if ENABLE_PREVIEW_WINDOW
			pushWindow(window);
			{
				if (keyboard.wentDown(SDLK_ESCAPE))
					framework.quitRequested = true;
				
				framework.beginDraw(0, 0, 0, 0);
				{
					if (shader.isValid())
					{
						setShader(shader);
						setShaderConstants(window.getWidth(), window.getHeight());
						drawRect(0, 0, window.getWidth(), window.getHeight());
						clearShader();
					}
				}
				framework.endDraw();
			}
			popWindow();
		#endif
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
		{
			logDebug("unable to fetch name (possibly too long). skipping uniform %d", (int)i);
			continue;
		}
		
		const GLint location = glGetUniformLocation(program, name);
		
		if (location == -1)
		{
			logDebug("failed to fetch location for uniform %d", (int)i);
			continue;
		}
		
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
		/*
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
			
			ImGui::Image((ImTextureID)(uintptr_t)texture, ImVec2(30, 30));
		}
		*/
		
		//clearShader();
		
		ImGui::PopID();
	}
}

static void saveUniforms(Shader & shader, std::vector<SavedUniform> & result)
{
	result.clear();
	
	Assert(shader.isValid());
	if (!shader.isValid())
		return;
	
	//
	
	const GLuint program = shader.getProgram();
	
	GLint numUniforms = 0;

	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
	checkErrorGL();

	for (auto i = 0; i < numUniforms; ++i)
	{
		SavedUniform savedUniform;
		
		GLint size = 0;
		GLsizei nameSize = 0;
		
		glGetActiveUniform(
			program, (GLuint)i,
			savedUniform.kMaxNameSize, &nameSize,
			&size, &savedUniform.type,
			savedUniform.name);
		checkErrorGL();
		
		if (nameSize <= 0)
		{
			logDebug("unable to fetch name (possibly too long). skipping uniform %d", (int)i);
			continue;
		}
		
		const GLint location = glGetUniformLocation(program, savedUniform.name);
		
		if (location == -1)
			continue;
		
		if (savedUniform.type == GL_FLOAT || savedUniform.type == GL_FLOAT_VEC2 || savedUniform.type == GL_FLOAT_VEC3 || savedUniform.type == GL_FLOAT_VEC4)
		{
			glGetUniformfv(program, location, savedUniform.floatValue);
			checkErrorGL();
			result.push_back(savedUniform);
		}
		else if (savedUniform.type == GL_SAMPLER_2D)
		{
			glGetUniformiv(program, location, &savedUniform.intValue);
			checkErrorGL();
			result.push_back(savedUniform);
		}
		else
		{
			logDebug("unknown uniform type for %s", savedUniform.name);
		}
	}
}

static void loadUniforms(Shader & shader, const std::vector<SavedUniform> & uniforms, const bool allowMissingUniforms)
{
	const GLuint program = shader.getProgram();
	
	for (auto & uniform : uniforms)
	{
		if (uniform.type == GL_FLOAT)
			shader.setImmediate(uniform.name, uniform.floatValue[0]);
		else if (uniform.type == GL_FLOAT_VEC2)
			shader.setImmediate(uniform.name, uniform.floatValue[0], uniform.floatValue[1]);
		else if (uniform.type == GL_FLOAT_VEC3)
			shader.setImmediate(uniform.name, uniform.floatValue[0], uniform.floatValue[1], uniform.floatValue[2]);
		else if (uniform.type == GL_FLOAT_VEC4)
			shader.setImmediate(uniform.name, uniform.floatValue[0], uniform.floatValue[1], uniform.floatValue[2], uniform.floatValue[3]);
		//else if (uniform.type == GL_SAMPLER_2D)
		//	shader.setTexture(uniform.name, textureUnit, uniform.intValue);
		
		const GLint location = glGetUniformLocation(program, uniform.name);
		
		if (location == -1)
			continue;
		
	#if 0
		// todo : check if new uniform shares same or similar type, and set directly instead of using above method ?
		
		if (uniform.type == GL_FLOAT || uniform.type == GL_FLOAT_VEC2 || uniform.type == GL_FLOAT_VEC3 || uniform.type == GL_FLOAT_VEC4)
		{
			glGetUniformfv(program, location, uniform.floatValue);
			checkErrorGL();
		}
		else if (uniform.type == GL_SAMPLER_2D)
		{
			glGetUniformiv(program, location, &uniform.intValue);
			checkErrorGL();
		}
	#endif
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
