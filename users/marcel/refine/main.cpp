#include "framework.h"
#include "imgui-framework.h"
#include "Path.h"
#include "StringEx.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include "fileEditor.h"
#include "fileEditor_audioGraph.h"
#include "fileEditor_audioStream_vorbis.h"
#include "fileEditor_font.h"
#include "fileEditor_jgmod.h"
#include "fileEditor_jsfx.h"
#include "fileEditor_model.h"
#include "fileEditor_particleSystem.h"
#include "fileEditor_sprite.h"
#include "fileEditor_spriter.h"
#include "fileEditor_text.h"
#include "fileEditor_vfxGraph.h"
#include "fileEditor_video.h"

#include <algorithm>
#include <functional>

#undef min
#undef max

static const int VIEW_SX = 1200;
static const int VIEW_SY = 800;

static std::string s_dataFolder;

struct FileElem
{
	bool isFolded = true;

	std::string name;
	std::string path;
	bool isFile = false;

	std::vector<FileElem> childElems;

	void fold(const bool recursive)
	{
		isFolded = true;

		if (recursive)
			for (auto & childElem : childElems)
				childElem.fold(recursive);
	}

	void unfold(const bool recursive)
	{
		isFolded = false;

		if (recursive)
			for (auto & childElem : childElems)
				childElem.unfold(recursive);
	}
};

struct FileBrowser
{
	FileElem rootElem;
	
	std::function<void (const std::string & filename)> onFileSelected;

	void init()
	{
		scanFiles();
	}

	void clearFiles()
	{
		rootElem = FileElem();
	}

	void scanFiles()
	{
		// clear the old file tree
		
		clearFiles();

		// list files

		//const char * rootFolder = "/Users/thecat/framework/vfxGraph-examples/data";
		const char * rootFolder = "/Users/thecat/framework/";
		//const char * rootFolder = "/Users/thecat/";
		//const char * rootFolder = "d:/repos";
		
		auto files = listFiles(rootFolder, true);
		
		std::sort(files.begin(), files.end(), [](const std::string & s1, const std::string & s2) { return strcasecmp(s1.c_str(), s2.c_str()) < 0; });
		
		// build a tree structure of the files
		
		for (size_t i = 0; i < files.size(); ++i)
		{
			const std::string & path = files[i];
			
			std::string name = files[i];
			
			if (String::StartsWith(name, rootFolder))
				name = String::SubString(name, strlen(rootFolder) + 1);
			
			FileElem * parent = &rootElem;
			
			size_t elemBegin = 0;
			
			for (size_t j = 0; j < name.size(); ++j)
			{
				if (name[j] == '/')
				{
					size_t elemEnd = j;
					
					const std::string elemName = name.substr(elemBegin, elemEnd - elemBegin);
					
					bool found = false;
					
					for (auto & childElem : parent->childElems)
					{
						if (childElem.name == elemName)
						{
							parent = &childElem;
							found = true;
						}
					}
					
					if (found == false)
					{
						FileElem childElem;
						childElem.name = elemName;
						childElem.isFile = false;
						
						parent->childElems.push_back(childElem);
						
						parent = &parent->childElems.back();
					}
					
					//
					
					elemBegin = elemEnd + 1;
				}
			}
			
			Assert(elemBegin < name.size());
			if (elemBegin < name.size())
			{
				const std::string elemName = name.substr(elemBegin, name.size() - elemBegin);
				
				FileElem childElem;
				childElem.name = elemName;
				childElem.path = path;
				childElem.isFile = true;
				
				parent->childElems.push_back(childElem);
			}
		}
	}

	void tickRecurse(FileElem & elem)
	{
		ImGui::PushID(elem.name.c_str());
		
		// draw foldable menu items for each file
		
		for (auto & childElem : elem.childElems)
		{
			if (childElem.isFile == false)
			{
				if (ImGui::CollapsingHeader(childElem.name.c_str()))
				{
					ImGui::Indent();
					tickRecurse(childElem);
					ImGui::Unindent();
				}
			}
		}
		
		for (auto & childElem : elem.childElems)
			if (childElem.isFile == true)
				if (ImGui::Button(childElem.name.c_str()))
					onFileSelected(childElem.path);
		
		ImGui::PopID();
	}
	
	void tick()
	{
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(300, 0));
		if (ImGui::Begin("File Browser", nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar*0 |
			ImGuiWindowFlags_NoScrollWithMouse*0))
		{
			ImGui::PushID("root");
			{
				tickRecurse(rootElem);
			}
			ImGui::PopID();
		}
		ImGui::End();
	}
};

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
	fileBrowser.init();

	// file editor
	FileEditor * editor = nullptr;
	Surface * editorSurface = new Surface(VIEW_SX - 300, VIEW_SY, false, true);
	
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
		
		if (extension == "ps" || // Pixel shader
			extension == "vs" || // Vertex shader
			extension == "frag" || // Fragment shader
			extension == "txt" ||
			extension == "plist" || // OSX property list
			extension == "ini" ||
			extension == "c" || // c
			extension == "cpp" || // c++
			extension == "h" || // Header file
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
			if (extension == "txt" && (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT)))
				editor = new FileEditor_Model(filename.c_str());
			else
				editor = new FileEditor_Text(filename.c_str());
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

		guiContext.processBegin(dt, VIEW_SX, VIEW_SY, inputIsCaptured);
		{
			fileBrowser.tick();
			
			if (editor != nullptr)
			{
				ImVec2 pos(300, 0);
				ImVec2 size(editorSurface->getWidth(), editorSurface->getHeight());
				
				ImGui::SetNextWindowPos(pos, ImGuiCond_Always);
				ImGui::SetNextWindowSize(size, ImGuiCond_Always);
				
				if (ImGui::Begin("editor", nullptr,
					ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoMove |
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
					
					editor->doButtonBar();
					
					if (ImGui::GetCursorPos().y != 0)
						ImGui::SameLine();
					
					if (ImGui::Button("Pop Out!"))
					{
						EditorWindow * window = new EditorWindow(editor);

						editorWindows.push_back(window);

						editor = nullptr;
					}
					
					ImGui::SameLine();
					if (ImGui::Button("Reopen"))
					{
						auto path_copy = editor->path;
						
						openEditor(path_copy);
					}
					
					ImGui::SameLine();
					if (ImGui::Button("Close"))
					{
						delete editor;
						editor = nullptr;
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
	
	guiContext.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
