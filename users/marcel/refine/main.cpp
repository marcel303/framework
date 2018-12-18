#include "framework.h"
#include "imgui-framework.h"
#include "imgui/TextEditor.h"
#include "Path.h"
#include "StringEx.h"
#include "TextIO.h"
#include "ui.h"
#include <algorithm>
#include <functional>

static const int VIEW_SX = 900;
static const int VIEW_SY = 600;
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

		const char * rootFolder = "/Users/thecat/framework/vfxGraph-examples/data";
		//const char * rootFolder = "/Users/thecat/framework/";
		
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
		// todo : draw foldable menu items for each file
		
		ImGui::SetNextWindowSize(ImVec2(300, 0));
		if (ImGui::Begin("File Browser", nullptr, ImGuiWindowFlags_NoScrollbar |
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

struct FileEditor
{
	virtual ~FileEditor()
	{
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) = 0;
};

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "vfxGraphManager.h"

struct FileEditor_VfxGraph : FileEditor
{
	static const int defaultSx = 600;
	static const int defaultSy = 600;
	
	VfxGraphManager_RTE vfxGraphMgr;
	VfxGraphInstance * instance = nullptr;
	
	FileEditor_VfxGraph(const char * path)
		: vfxGraphMgr(defaultSx, defaultSy)
	{
		vfxGraphMgr.init();
		
		instance = vfxGraphMgr.createInstance(path, defaultSx, defaultSy);
		vfxGraphMgr.selectInstance(instance);
	}
	
	virtual ~FileEditor_VfxGraph() override
	{
		vfxGraphMgr.free(instance);
		Assert(instance == nullptr);
		
		vfxGraphMgr.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		// update real-time editing
		
		inputIsCaptured |= vfxGraphMgr.tickEditor(dt, inputIsCaptured);
		
		// resize instance and tick & draw vfx graph
		
		instance->sx = sx;
		instance->sy = sy;
	
		vfxGraphMgr.tick(dt);
		
		vfxGraphMgr.traverseDraw();
		
		// draw vfx graph
		
		pushBlend(BLEND_OPAQUE);
		gxSetTexture(instance->texture);
		setColor(colorWhite);
		drawRect(0, 0, instance->sx, instance->sy);
		gxSetTexture(0);
		popBlend();
		
		// update visualizers and draw editor
		
		vfxGraphMgr.tickVisualizers(dt);
		
		vfxGraphMgr.drawEditor();
	}
};

struct FileEditor_Text : FileEditor
{
	std::string path;
	TextEditor textEditor;
	TextIO::LineEndings lineEndings;
	bool isValid = false;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Text(const char * in_path)
	{
		path = in_path;
		
		std::vector<std::string> lines;
		isValid = TextIO::load(path.c_str(), lines, lineEndings);
		
		textEditor.SetTextLines(lines);
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Text() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			auto filename = Path::GetFileName(path);
			
			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(sx, sy), ImGuiCond_Always);
			if (ImGui::Begin("editor", nullptr,
				ImGuiWindowFlags_NoTitleBar*1 |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove))
			{
				textEditor.Render(filename.c_str(), ImVec2(sx - 20, sy - 20), false);
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		guiContext.draw();
	}
};

struct FileEditor_Sprite : FileEditor
{
	Sprite sprite;
	
	FileEditor_Sprite(const char * path)
		: sprite(path)
	{
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		setColor(colorWhite);
		sprite.draw();
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
	
	auto dataFolder = getDirectory();
	framework.fillCachesWithPath(".", true);
	
// todo : create ImGui context

	FrameworkImGuiContext guiContext;
	guiContext.init(true);

	FileBrowser fileBrowser;
	fileBrowser.init();

	FileEditor * editor = nullptr;
	Surface * editorSurface = new Surface(VIEW_SX - 300, VIEW_SY, false);
	
	fileBrowser.onFileSelected = [&](const std::string & path)
	{
		delete editor;
		editor = nullptr;
		
		//
		
	// fixme : ugly hack to construct an absolute path here
	
		auto directory = "/" + Path::GetDirectory(path);
		changeDirectory(directory.c_str());
		
		auto filename = Path::GetFileName(path);
		auto extension = Path::GetExtension(filename, true);
		
		if (extension == "ps" ||
			extension == "vs" ||
			extension == "txt" ||
			extension == "ini" ||
			extension == "cpp" ||
			extension == "h")
		{
			editor = new FileEditor_Text(filename.c_str());
		}
		else if (extension == "bmp" || extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "tga")
		{
			editor = new FileEditor_Sprite(filename.c_str());
		}
		else if (extension == "xml")
		{
			editor = new FileEditor_VfxGraph(filename.c_str());
		}
	};
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
		// todo : process ui

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
							editorSurface->clear(100, 100, 100, 255);
							
							bool inputIsCaptured = ImGui::IsItemHovered() == false;
							
							editor->tick(
								size.x,
								size.y,
								dt,
								inputIsCaptured);
						}
						popSurface();
						
						editorSurface->setAlphaf(1.f);
						
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
				}
				ImGui::End();
			}
		}
		guiContext.processEnd();

		framework.beginDraw(0, 0, 0, 0);
		{
			// todo : draw ui

			guiContext.draw();
		}
		framework.endDraw();
	}
	
	changeDirectory(dataFolder.c_str());
	
	guiContext.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
