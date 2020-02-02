#include "fileEditor_shader.h"
#include "framework.h"
#include "imgui/TextEditor.h"
#include "imgui-framework.h"
#include "nfd.h"
#include "Path.h"
#include "StringEx.h"
#include "TextIO.h"
#include <SDL2/SDL.h> // SDL_ShowSimpleMessageBox, SDL_StartTextInput

#define ENABLE_FILEBROWSER 0

// during real-time edits of the shader, the program is recompiled and the currently set uniforms are lost.
// to avoid this loss, uniforms are saved and restored before and after the program is recompiled
struct SavedUniform
{
	GX_IMMEDIATE_TYPE type = (GX_IMMEDIATE_TYPE)-1;
	std::string name;
	float floatValue[4] = { };
};

#if ENABLE_FILEBROWSER
static bool showFileBrowser(std::string & shaderNamePs, const std::vector<std::string> & files, std::string & filename);
#endif
static bool loadIntoTextEditor(const char * filename, TextIO::LineEndings & lineEndings, TextEditor & textEditor);

static void showUniforms(Shader & shader);

static void saveUniforms(Shader & shader, std::vector<SavedUniform> & result);
static void loadUniforms(Shader & shader, const std::vector<SavedUniform> & uniforms, const bool allowMissingUniforms);

static void showStatistics(Shader & shader);
static void showErrors(Shader & shader);

struct RealtimeShaderEditor
{
	enum Tab
	{
		kTab_Uniforms,
		kTab_Errors,
		kTab_Statistics,
		kTab_FileBrowser
	};

	FrameworkImGuiContext framework_context;
	
	TextEditor textEditor;
	
	std::string filename;
	std::string sourceName;
	std::string text;
	
	Shader shader;
	
	Tab tab;
	
#if ENABLE_FILEBROWSER
	std::vector<std::string> files;
#endif
	
	float idleTime = 0.f;
	float alphaAnim = 0.f;

	std::vector<SavedUniform> savedUniforms;
	
	std::string path;
	bool pathIsValid = false;

	TextIO::LineEndings lineEndings = TextIO::kLineEndings_Unix;
	
	void init(const char * shaderName, const char * filename)
	{
		framework_context.init();

		textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());
	
		this->filename = filename;
		
		char * text = nullptr;
		size_t textSize;
		if (TextIO::loadFileContents(filename, text, textSize))
		{
			textEditor.SetText(text);
			this->text = text;
		}
		
		delete [] text;
		text = nullptr;
		
		SDL_StartTextInput();
	
		std::string vsFilename;
		std::string psFilename;
		
		if (Path::GetExtension(filename, true) == "vs")
		{
			vsFilename = String::FormatC("%s-shaderEditor.vs", shaderName);
			psFilename = String::FormatC("%s.ps", shaderName);
			sourceName = vsFilename;
		}
		else
		{
			vsFilename = String::FormatC("%s.vs", shaderName);
			psFilename = String::FormatC("%s-shaderEditor.ps", shaderName);
			sourceName = psFilename;
		}
		
		shaderSource(sourceName.c_str(), this->text.c_str());
		
		std::string shaderNameCopy = sourceName + "-copy";
		
		shader = Shader(shaderNameCopy.c_str(), vsFilename.c_str(), psFilename.c_str());
	
		tab = kTab_Uniforms;
	
	#if ENABLE_FILEBROWSER
		files = listFiles("testShaders", false);
		auto removed = std::remove_if(files.begin(), files.end(), [](const std::string & s) { return Path::GetExtension(s, true) != "ps"; });
		files.erase(removed, files.end());
		std::sort(files.begin(), files.end());
	#endif
	}
	
	void shut()
	{
		SDL_StopTextInput();
	
		framework_context.shut();
	}
	
	void tick(const float dt, bool & inputIsCaptured, const int sx, const int sy, const bool showEditor)
	{
		Assert(pathIsValid == !path.empty());
		
		if (keyboard.isIdle() && mouse.isIdle())
			idleTime += framework.timeStep;
		else
			idleTime = 0.f;
		
		if (idleTime < 4.f && showEditor)
			alphaAnim -= framework.timeStep / .5f;
		else
			alphaAnim += framework.timeStep / (showEditor ? 2.f : .5f);
		
		alphaAnim = clamp(alphaAnim, 0.f, 1.f);
		
		framework_context.processBegin(framework.timeStep, sx, sy, inputIsCaptured);
		{
			const float alpha = lerp(.9f, .0f, alphaAnim);
			ImGui::GetStyle().Alpha = alpha;
			
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(sx, sy));
			
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

				textEditor.Render("TextEditor", ImVec2(0, sy - 170 - 30));
				
			#if ENABLE_FILEBROWSER
				if (ImGui::RadioButton("File browser", tab == kTab_FileBrowser))
					tab = kTab_FileBrowser;
				ImGui::SameLine();
			#endif
				if (ImGui::RadioButton("Uniforms", tab == kTab_Uniforms))
					tab = kTab_Uniforms;
				ImGui::SameLine();
				if (ImGui::RadioButton("Statistics", tab == kTab_Statistics))
					tab = kTab_Statistics;
				ImGui::SameLine();
				if (ImGui::RadioButton("Errors", tab == kTab_Errors))
					tab = kTab_Errors;
				
				ImGui::BeginChild("Tabs", ImVec2(0, 170));
				{
				#if ENABLE_FILEBROWSER
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
					else
				#endif
					if (tab == kTab_Uniforms)
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
		
		const std::string newText = textEditor.GetText();
		
		if (newText != text)
		{
			text = newText;
			
			saveUniforms(shader, savedUniforms);
			{
				shaderSource(sourceName.c_str(), text.c_str());
			}
			loadUniforms(shader, savedUniforms, true);
		}
	}
	
	void draw(const int sx, const int sy)
	{
		auto setShaderConstants = [&](const int viewSx, const int viewSy)
		{
			shader.setImmediate("time", framework.time);
			shader.setImmediate("mouse", mouse.x / float(viewSx), mouse.y / float(viewSy));
			shader.setImmediate("mouse_down", mouse.isDown(BUTTON_LEFT));
			shader.setTexture("s", 0, framework_context.font_texture.id);
		};
		
		if (shader.isValid())
		{
			setShader(shader);
			setShaderConstants(sx, sy);
			setColor(colorWhite);
			gxNormal3f(0, 0, 1);
			drawRect(0, 0, sx, sy);
			clearShader();
		}
	
		framework_context.draw();
	}
};

//

FileEditor_Shader::FileEditor_Shader(const char * path)
{
	const std::string shaderName = Path::StripExtension(path);
	
	shaderEditor = new RealtimeShaderEditor();
	shaderEditor->init(shaderName.c_str(), path);
}

FileEditor_Shader::~FileEditor_Shader()
{
	shaderEditor->shut();

	delete shaderEditor;
	shaderEditor = nullptr;
}

void FileEditor_Shader::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	if (inputIsCaptured == false && keyboard.isDown(SDLK_LSHIFT) && keyboard.wentDown(SDLK_TAB))
	{
		inputIsCaptured = true;
		showEditor = !showEditor;
	}

	if (showEditor == false)
		inputIsCaptured = true;

	shaderEditor->tick(dt, inputIsCaptured, sx, sy, showEditor);
	
	clearSurface(0, 0, 0, 255);

	shaderEditor->draw(sx, sy);
}

//

static void showUniforms(Shader & shader)
{
	if (shader.isValid() == false)
		return;
	
	auto uniformInfos = shader.getImmediateInfos();

	for (auto & uniformInfo : uniformInfos)
	{
		const GX_IMMEDIATE_TYPE type = uniformInfo.type;
		const char * name = uniformInfo.name.c_str();
		const GxImmediateIndex index = uniformInfo.index;
		
		ImGui::PushID(index);
		
		if (type == GX_IMMEDIATE_FLOAT)
		{
			float value[1] = { 0.f };
			shader.getImmediateValuef(index, value);
			
			if (ImGui::InputFloat(name, value))
			{
				setShader(shader);
				shader.setImmediate(name, value[0]);
				clearShader();
			}
		}
		else if (type == GX_IMMEDIATE_VEC2)
		{
			float value[2] = { 0.f };
			shader.getImmediateValuef(index, value);
			
			if (ImGui::InputFloat2(name, value))
			{
				setShader(shader);
				shader.setImmediate(name, value[0], value[1]);
				clearShader();
			}
		}
		else if (type == GX_IMMEDIATE_VEC3)
		{
			float value[3] = { 0.f };
			shader.getImmediateValuef(index, value);
			
			if (ImGui::InputFloat3(name, value))
			{
				setShader(shader);
				shader.setImmediate(name, value[0], value[1], value[2]);
				clearShader();
			}
		}
		else if (uniformInfo.type == GX_IMMEDIATE_VEC4)
		{
			float value[4] = { 0.f };
			shader.getImmediateValuef(index, value);
			
			if (ImGui::InputFloat4(name, value))
			{
				setShader(shader);
				shader.setImmediate(name, value[0], value[1], value[2], value[3]);
				clearShader();
			}
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
		
		ImGui::PopID();
	}
}

static void saveUniforms(Shader & shader, std::vector<SavedUniform> & result)
{
	Assert(shader.isValid());
	if (!shader.isValid())
		return;
	
	result.clear();
	
	//
	
	auto uniformInfos = shader.getImmediateInfos();
	
	for (auto & uniformInfo : uniformInfos)
	{
		SavedUniform savedUniform;
		savedUniform.type = uniformInfo.type;
		savedUniform.name = uniformInfo.name;
		
		if (uniformInfo.type == GX_IMMEDIATE_FLOAT ||
			uniformInfo.type == GX_IMMEDIATE_VEC2 ||
			uniformInfo.type == GX_IMMEDIATE_VEC3 ||
			uniformInfo.type == GX_IMMEDIATE_VEC4)
		{
			shader.getImmediateValuef(uniformInfo.index, savedUniform.floatValue);
			result.push_back(savedUniform);
		}
	// todo : add texture support
		/*
		else if (savedUniform.type == GL_SAMPLER_2D)
		{
			glGetUniformiv(program, location, &savedUniform.intValue);
			checkErrorGL();
			result.push_back(savedUniform);
		}
		*/
		else
		{
			logDebug("unknown uniform type for %s", savedUniform.name.c_str());
		}
	}
}

static void loadUniforms(Shader & shader, const std::vector<SavedUniform> & uniforms, const bool allowMissingUniforms)
{
	for (auto & uniform : uniforms)
	{
		setShader(shader);
		
		if (uniform.type == GX_IMMEDIATE_FLOAT)
			shader.setImmediate(uniform.name.c_str(), uniform.floatValue[0]);
		else if (uniform.type == GX_IMMEDIATE_VEC2)
			shader.setImmediate(uniform.name.c_str(), uniform.floatValue[0], uniform.floatValue[1]);
		else if (uniform.type == GX_IMMEDIATE_VEC3)
			shader.setImmediate(uniform.name.c_str(), uniform.floatValue[0], uniform.floatValue[1], uniform.floatValue[2]);
		else if (uniform.type == GX_IMMEDIATE_VEC4)
			shader.setImmediate(uniform.name.c_str(), uniform.floatValue[0], uniform.floatValue[1], uniform.floatValue[2], uniform.floatValue[3]);
		//else if (uniform.type == GL_SAMPLER_2D)
		//	shader.setTexture(uniform.name, textureUnit, uniform.intValue);
		clearShader();
	}
}

#if ENABLE_FILEBROWSER
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
#endif

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
		for (auto & errorMessage : errorMessages)
			ImGui::TextWrapped("%s", errorMessage.c_str());
	}
}
