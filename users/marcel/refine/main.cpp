#include "framework.h"
#include "imgui-framework.h"
#include "Path.h"
#include "StringEx.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include "fileBrowser.h"

#include "fileEditor.h"
#include "fileEditor_audioGraph.h"
#include "fileEditor_audioStream_vorbis.h"
#include "fileEditor_font.h"
#include "fileEditor_jgmod.h"
#include "fileEditor_jsfx.h"
#include "fileEditor_model.h"
#include "fileEditor_particleSystem.h"
#include "fileEditor_shader.h"
#include "fileEditor_sprite.h"
#include "fileEditor_spriter.h"
#include "fileEditor_text.h"
#include "fileEditor_vfxGraph.h"
#include "fileEditor_video.h"

#include <algorithm>
#include <functional>

#undef min
#undef max

#define CHIBI_INTEGRATION 1

#if CHIBI_INTEGRATION
	#include "chibi.h"
	#include "nfd.h"

	#ifdef _MSC_VER
		#ifndef PATH_MAX
			#include <Windows.h>
			#define PATH_MAX _MAX_PATH
		#endif
	#endif
#endif

static const int VIEW_SX = 1200;
static const int VIEW_SY = 800;

static std::string s_dataFolder;

//

struct EditorWindow
{
	Window window;

	FileEditor * editor = nullptr;
	
	Surface * surface = nullptr;
	
	bool wantsToDock = false;

	EditorWindow(FileEditor * in_editor)
		: window("View", 800, 600, true)
		, editor(in_editor)
	{
		process(0.f);
	}

	~EditorWindow()
	{
		delete editor;
		editor = nullptr;
	}

	void process(const float dt)
	{
		const bool isResize =
			surface == nullptr ||
			surface->getWidth() != window.getWidth() ||
			surface->getHeight() != window.getHeight();
		
		const bool hasFocus = window.hasFocus();
		
		const bool wantsTick = editor->wantsTick(hasFocus, false);
		
		if (wantsTick == false && isResize == false)
			return;
		
		if (isResize)
		{
			delete surface;
			surface = nullptr;
			
			surface = new Surface(window.getWidth(), window.getHeight(), false, true);
		}
		
		pushWindow(window);
		{
			pushSurface(surface);
			{
				bool inputIsCaptured = false;
				
				editor->tickBegin(surface);
				{
					editor->tick(
						surface->getWidth(),
						surface->getHeight(),
						dt,
						hasFocus,
						inputIsCaptured);
				}
				editor->tickEnd();
			}
			popSurface();
			
			framework.beginDraw(0, 0, 0, 0);
			{
				surface->blit(BLEND_OPAQUE);
			}
			framework.endDraw();
		}
		popWindow();
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	framework.windowIsResizable = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	initUi();
	
	// remember the initial path and load all of the resources in there now, as we'll be changing the current working directory later
	
	s_dataFolder = getDirectory();
	framework.fillCachesWithPath(".", true);
	
	// create ImGui context and set a custom font

	FrameworkImGuiContext guiContext;
	guiContext.init();
	guiContext.pushImGuiContext();
	ImGuiIO& io = ImGui::GetIO();
	io.FontDefault = io.Fonts->AddFontFromFileTTF("calibri.ttf", 16);
	guiContext.popImGuiContext();
	guiContext.updateFontTexture();

	// file browser
	FileBrowser fileBrowser;
	fileBrowser.init(CHIBI_RESOURCE_PATH "/../../../..");

	// file editor
	FileEditor * editor = nullptr;
	Surface * editorSurface = nullptr;
	
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

	bool mitigateHitch = false;
	
	auto openEditor = [&](const std::string & path)
	{
		delete editor;
		editor = nullptr;
		
		//
		
	// fixme : ugly hack to construct an absolute path here
	
		auto directory = Path::GetDirectory(path);
	#ifndef WIN32
		directory = "/" + directory;
	#endif
		changeDirectory(directory.c_str());
		
		auto filename = Path::GetFileName(path);
		auto extension = Path::GetExtension(filename, true);
		
		if (extension == "frag" || // Fragment shader
			extension == "txt" ||
			extension == "plist" || // OSX property list
			extension == "ini" ||
			extension == "c" || // c
			extension == "cpp" || // c++
			extension == "h" || // Header file
			extension == "m" || // objective-c
			extension == "mm" || // objective-c++
			extension == "py" || // Python
			extension == "jsfx-inc" || // JsusFx include file
			extension == "inc" || // Include file
			extension == "md" || // Markdown
			extension == "bat" || // Batch file
			extension == "sh" || // Shell script
			extension == "java" ||
			extension == "js" || // Javascript
			extension == "pde" || // Processing sketch
			extension == "ino") // Arduino sketch)
		{
		// todo : think of a nicer way to open files with ambiguous filename extensions
		// todo : investigate possibility of removing ambiguous filename extensions
			if (extension == "txt" && (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT)))
				editor = new FileEditor_Model(filename.c_str());
			else
				editor = new FileEditor_Text(filename.c_str());
		}
		else if (extension == "ps" || extension == "vs")
		{
			editor = new FileEditor_Shader(filename.c_str());
		}
		else if (extension == "bmp" || extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "psd" || extension == "tga" || extension == "gif")
		{
			editor = new FileEditor_Sprite(filename.c_str());
		}
		else if (extension == "ogg")
		{
			editor = new FileEditor_AudioStream_Vorbis(filename.c_str());
		}
		else if (extension == "fbx")
		{
			editor = new FileEditor_Model(filename.c_str());
		}
		else if (extension == "scml")
		{
			editor = new FileEditor_Spriter(filename.c_str());
		}
		else if (extension == "mpg" || extension == "mpeg" || extension == "mp4" || extension == "avi" || extension == "mov")
		{
			editor = new FileEditor_Video(filename.c_str());
		}
		else if (extension == "s3m" || extension == "xm" || extension == "mod" || extension == "it" || extension == "umx")
		{
			editor = new FileEditor_Jgmod(filename.c_str());
		}
		else if (extension == "pfx")
		{
			editor = new FileEditor_ParticleSystem(filename.c_str());
		}
		else if (extension == "jsfx")
		{
			editor = new FileEditor_JsusFx(filename.c_str());
		}
		else if (extension == "ttf" || extension == "otf")
		{
			editor = new FileEditor_Font(filename.c_str());
		}
		else if (extension == "xml")
		{
			// determine content type
			
			char * text = nullptr;
			TextIO::LineEndings lineEndings;
			
			char type = 0;
			
			TextIO::loadWithCallback(filename.c_str(), text, lineEndings, [](void * userData, const char * begin, const char * end)
			{
				if (strstr(begin, "typeName=\"voice\"") != nullptr || strstr(begin, "typeName=\"voice.4d\"") != nullptr)
				{
					char * type = (char*)userData;
					
					*type = 'a';
				}
				else if (strstr(begin, "typeName=\"draw.display\"") != nullptr)
				{
					char * type = (char*)userData;
					
					*type = 'v';
				}
			}, &type);
			
			delete [] text;
			text = nullptr;
			
			if (type == 'a')
			{
				editor = new FileEditor_AudioGraph(filename.c_str());
			}
			else
			{
				editor = new FileEditor_VfxGraph(filename.c_str());
			}
		}
		
		if (editor != nullptr)
		{
			editor->path = path;
		}
		
		mitigateHitch = true;
	};
	
	fileBrowser.onFileSelected = openEditor;
	
	std::vector<EditorWindow*> editorWindows;

	for (;;)
	{
		framework.process();
		
		if (mitigateHitch)
		{
			// some operations, like opening a new editor, may cause a hitch. to avoid huge time steps
			// from causing jerky animation, we set the time step to zero here
			
			mitigateHitch = false;
			framework.timeStep = 0.f;
		}
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		const int windowSx = framework.getMainWindow().getWidth();
		const int windowSy = framework.getMainWindow().getHeight();
		
		if (editorSurface == nullptr ||
			editorSurface->getWidth() != windowSx ||
			editorSurface->getHeight() != windowSy)
		{
			delete editorSurface;
			editorSurface = nullptr;
			
			//
			
			editorSurface = new Surface(
				windowSx - 300,
				windowSy - 30,
				false,
				true);
		}
		
		const float dt = framework.timeStep;
		
		// update editor windows
		
		for (auto i = editorWindows.begin(); i != editorWindows.end(); )
		{
			auto editorWindow = *i;

			editorWindow->process(dt);

			if (editorWindow->window.getQuitRequested())
			{
				delete editorWindow;
				editorWindow = nullptr;

				i = editorWindows.erase(i);
			}
			else if (editorWindow->wantsToDock && editor == nullptr)
			{
				editor = editorWindow->editor;
				editorWindow->editor = nullptr;
				
				//
				
				delete editorWindow;
				editorWindow = nullptr;
				
				i = editorWindows.erase(i);
			}
			else
				++i;
		}

		// process ui

		bool inputIsCaptured = false;

		guiContext.processBegin(dt, windowSx, windowSy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(300, 0));
			if (ImGui::Begin("File Browser", nullptr,
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_MenuBar |
				ImGuiWindowFlags_NoScrollbar*0 |
				ImGuiWindowFlags_NoScrollWithMouse*0))
			{
				ImGui::BeginMenuBar();
				{
					if (ImGui::BeginMenu("File"))
					{
						if (ImGui::MenuItem("Refresh"))
							fileBrowser.clearFiles();
						if (ImGui::MenuItem("Select root"))
						{
							nfdchar_t * path = 0;
							nfdresult_t result = NFD_OpenDialog("", "", &path);

							if (result == NFD_OKAY)
							{
								const std::string dir = Path::GetDirectory(path);
								
								fileBrowser.init(dir.c_str());
							}
							
							if (path != nullptr)
							{
								free(path);
								path = nullptr;
							}
						}
						
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
							ImGui::Text("%d/%d selected",
								(int)chibi.selected_targets.size(),
								(int)chibi.apps.size());
							ImGui::SameLine();
							if (chibi.selected_targets.empty())
								ImGui::Text("(Generate all)");
							else if (ImGui::Button("Clear selection"))
								chibi.selected_targets.clear();
							
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
				}
				ImGui::EndMenuBar();
			
				fileBrowser.tick();
			}
			ImGui::End();
			
			if (editor != nullptr)
			{
				ImVec2 pos(300, 0);
				ImVec2 size(editorSurface->getWidth(), editorSurface->getHeight());
				
				ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(size, ImGuiCond_Always);
				
				pos += ImVec2(0, 30);
				
				if (ImGui::Begin("Editor", nullptr,
					ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_MenuBar |
					ImGuiWindowFlags_NoScrollbar |
					ImGuiWindowFlags_NoScrollWithMouse))
				{
					auto window = ImGui::GetCurrentWindow();
					
					const ImGuiID id = window->GetID("editorContent");
					const ImRect bb(pos, pos + size);
					
					ImGui::ItemSize(bb);
					if (ImGui::ItemAdd(bb, id))
					{
						auto & io = ImGui::GetIO();
						
						if (ImGui::IsItemHovered() && io.MouseClicked[0])
						{
							//ImGui::SetActiveID(id, window);
                    		ImGui::SetFocusID(id, window);
                			ImGui::FocusWindow(window);
						}
						
						auto oldMouse = mouse;
						mouse.x -= pos.x;
						mouse.y -= pos.y;
						
						pushSurface(editorSurface);
						{
							bool inputIsCaptured = ImGui::IsItemHovered() == false;
							
							editor->tickBegin(editorSurface);
							{
								editor->tick(
									size.x,
									size.y,
									dt,
									true,
									inputIsCaptured);
							}
							editor->tickEnd();
						}
						popSurface();
						
						editorSurface->setAlphaf(1.f); // fix for ImGui drawing the surface translucently. ideally we'd draw it opaque, but the only way to do that through ImGui is to force the alpha to one, here
						
						mouse = oldMouse;
						
						//
						
						window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0,0,0,0)), 0.0f);
						window->DrawList->AddImage(
							(ImTextureID)(uintptr_t)editorSurface->getTexture(),
							bb.Min, bb.Max,
							ImVec2(0.f, 0.f),
							ImVec2(1.f, 1.f),
							ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 1.f)));
					}
					
					//
					
					ImGui::SetCursorPos(ImVec2(0, 0));
					
					if (ImGui::BeginMenuBar())
					{
						editor->doButtonBar();
						
						if (ImGui::MenuItem("Pop Out!"))
						{
							EditorWindow * window = new EditorWindow(editor);

							editorWindows.push_back(window);

							editor = nullptr;
						}
						
						if (ImGui::MenuItem("Reopen"))
						{
							auto path_copy = editor->path;
							
							openEditor(path_copy);
						}
						
						if (ImGui::MenuItem("Close"))
						{
							delete editor;
							editor = nullptr;
						}
						
						ImGui::EndMenuBar();
					}
				}
				ImGui::End();
			}
		}
		guiContext.processEnd();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// draw ui

			guiContext.draw();
		}
		framework.endDraw();
	}
	
	changeDirectory(s_dataFolder.c_str());
	
	fileBrowser.shut();
	
	guiContext.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
