#include "framework.h"
#include "imgui-framework.h"
#include "imgui/TextEditor.h"
#include "Path.h"
#include "StringEx.h"
#include "TextIO.h"
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
		
		if (ImGui::Begin("File Browser"))
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
	
	virtual void tick(const float dt, bool & inputIsCaptured) = 0;
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
	
	Surface surface;
	
	FileEditor_VfxGraph(const char * path)
		: vfxGraphMgr(defaultSx, defaultSy)
		, surface(defaultSx, defaultSy, false)
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
	
	bool doGraphEdit(const char * label, const float dt)
	{
		const ImVec2 size_arg(instance->sx, instance->sy);
		
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;
		
		const ImGuiID id = window->GetID(label);
		
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		ImVec2 pos = window->DC.CursorPos;
	
		ImVec2 size = ImGui::CalcItemSize(size_arg, style.FramePadding.x * 2.0f, style.FramePadding.y * 2.0f);
	
		const ImRect bb(pos, pos + size);
		ImGui::ItemSize(bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(bb, id))
			return false;
		
		bool inputIsCaptured = false;
		
		auto oldMouse = mouse;
		auto mousePos = ImGui::GetMousePos();
		auto cursorPos = pos;//ImGui::GetCursorScreenPos();
		mouse.x = mousePos.x - cursorPos.x;
		mouse.y = mousePos.y - cursorPos.y;
		inputIsCaptured |= vfxGraphMgr.tickEditor(dt, ImGui::IsWindowFocused() == false);
		mouse = oldMouse;
		
		pushSurface(&surface);
		{
			surface.clear(100, 100, 100, 255);
			
			vfxGraphMgr.tick(dt);
			
			vfxGraphMgr.traverseDraw();
			
			vfxGraphMgr.tickVisualizers(dt);
			
			vfxGraphMgr.drawEditor();
		}
		popSurface();
		
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(ImVec4(0,0,0,0)), 0.0f);
        window->DrawList->AddImage((ImTextureID)(uintptr_t)surface.getTexture(), bb.Min, bb.Max, ImVec2(0.f, 0.f), ImVec2(1.f, 1.f), ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 1.f)));
		
		return true;
	}
	
	virtual void tick(const float dt, bool & inputIsCaptured) override
	{
		doGraphEdit("graphEdit", dt);
		
		//ImGui::Image((ImTextureID)(uintptr_t)surface.getTexture(), ImVec2(defaultSx, defaultSy));
		
		//ImGui::Image((ImTextureID)(uintptr_t)instance->texture, ImVec2(defaultSx, defaultSy));
	}
};

struct FileEditor_Text : FileEditor
{
	std::string path;
	TextEditor textEditor;
	TextIO::LineEndings lineEndings;
	bool isValid = false;
	
	FileEditor_Text(const char * in_path)
	{
		path = in_path;
		
		std::vector<std::string> lines;
		isValid = TextIO::load(path.c_str(), lines, lineEndings);
		
		textEditor.SetTextLines(lines);
	}
	
	virtual void tick(const float dt, bool & inputIsCaptured) override
	{
		auto filename = Path::GetFileName(path);
		
		textEditor.Render(filename.c_str());
	}
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	auto dataFolder = getDirectory();
	framework.fillCachesWithPath(".", true);
	
// todo : create ImGui context

	FrameworkImGuiContext guiContext;
	guiContext.init(true);

	FileBrowser fileBrowser;
	fileBrowser.init();

	FileEditor * editor = nullptr;
	
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
		
		if (extension == "ps" || extension == "vs" || extension == "txt" || extension == "ini")
		{
			editor = new FileEditor_Text(filename.c_str());
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
				if (ImGui::Begin("editor", nullptr,
					/*ImGuiWindowFlags_NoTitleBar |*/
					/*ImGuiWindowFlags_NoMove |*/
					ImGuiWindowFlags_NoScrollbar |
					ImGuiWindowFlags_NoScrollWithMouse))
				{
					editor->tick(dt, inputIsCaptured);
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
