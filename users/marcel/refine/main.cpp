#include "framework.h"
#include "imgui-framework.h"
#include "imgui/TextEditor.h"
#include "Path.h"
#include "StringEx.h"
#include "TextIO.h"
#include "ui.h"
#include <algorithm>
#include <functional>

#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include "paobject.h"
#include "vfxGraphManager.h"
#include "vfxUi.h"

#include "nfd.h"

#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"

#include "video.h"

#include "framework-allegro2.h"
#include "jgmod.h"
#include "jgvis.h"

#include "particle_editor.h"

#include "gfx-framework.h"
#include "jsusfx_file.h"
#include "jsusfx-framework.h"

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
		//const char * rootFolder = "/Users/thecat/framework/";
		//const char * rootFolder = "/Users/thecat/";
		const char * rootFolder = "d:/repos";
		
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

struct FileEditor
{
	Surface * surface = nullptr;
	
	virtual ~FileEditor()
	{
	}
	
	void clearSurface(const int r, const int g, const int b, const int a)
	{
		surface->clear(r, g, b, a);
		
		surface->clearDepth(1.f);
	}
	
	void tickBegin(Surface * in_surface)
	{
		surface = in_surface;
	}
	
	void tickEnd()
	{
		surface = nullptr;
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) = 0;
};

extern SDL_mutex * g_vfxAudioMutex;
extern AudioVoiceManager * g_vfxAudioVoiceMgr;
extern AudioGraphManager * g_vfxAudioGraphMgr;

struct FileEditor_VfxGraph : FileEditor
{
	static const int defaultSx = 600;
	static const int defaultSy = 600;
	
	VfxGraphManager_RTE vfxGraphMgr;
	VfxGraphInstance * instance = nullptr;
	
	AudioMutex audioMutex;
	AudioVoiceManagerBasic audioVoiceMgr;
	AudioGraphManager_Basic audioGraphMgr;
	
	UiState uiState;
	
	FileEditor_VfxGraph(const char * path)
		: vfxGraphMgr(defaultSx, defaultSy)
		, audioGraphMgr(true)
	{
		// init vfx graph
		vfxGraphMgr.init();
		
		// init audio graph
		audioMutex.init();
		audioVoiceMgr.init(audioMutex.mutex, 64, 64);
		audioGraphMgr.init(audioMutex.mutex, &audioVoiceMgr);
		
		g_vfxAudioMutex = audioMutex.mutex;
		g_vfxAudioVoiceMgr = &audioVoiceMgr;
		g_vfxAudioGraphMgr = &audioGraphMgr;
		
		// create instance
		instance = vfxGraphMgr.createInstance(path, defaultSx, defaultSy);
		vfxGraphMgr.selectInstance(instance);
	}
	
	virtual ~FileEditor_VfxGraph() override
	{
		// free instance
		vfxGraphMgr.free(instance);
		Assert(instance == nullptr);
		
		// shut audio graph
		g_vfxAudioMutex = nullptr; // todo : needs a nicer solution .. make it part of globals .. side data for additional node support .. ?
		g_vfxAudioVoiceMgr = nullptr;
		g_vfxAudioGraphMgr = nullptr;
		
		audioGraphMgr.shut();
		audioVoiceMgr.shut();
		audioMutex.shut();
		
		// shut vfx graph
		vfxGraphMgr.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		auto doMenus = [&](const bool doActions, const bool doDraw)
		{
			uiState.x = 10;
			uiState.y = VIEW_SY - 100;
			uiState.sx = 200;
			
			makeActive(&uiState, doActions, doDraw);
			pushMenu("mem editor");
			doVfxMemEditor(*vfxGraphMgr.selectedFile->activeInstance->vfxGraph, dt);
			popMenu();
		};
		
		// update memory editing
		
		doMenus(true, false);
		
		inputIsCaptured |= uiState.activeElem != nullptr;
		
		// update real-time editing
		
		inputIsCaptured |= vfxGraphMgr.tickEditor(sx, sy, dt, inputIsCaptured);
		
		// update vfx graph and draw ?
		
		const GraphEdit * graphEdit = vfxGraphMgr.selectedFile->graphEdit;
		
		if (hasFocus == false && graphEdit->animationIsDone)
			return;
		
		// resize instance and tick & draw vfx graph
		
		instance->sx = sx;
		instance->sy = sy;
	
		vfxGraphMgr.tick(dt);
		
		vfxGraphMgr.traverseDraw();
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		// draw vfx graph
		
		pushBlend(BLEND_OPAQUE);
		gxSetTexture(instance->texture);
		setColor(colorWhite);
		drawRect(0, 0, instance->sx, instance->sy);
		gxSetTexture(0);
		popBlend();
		
		// update visualizers and draw editor
		
		vfxGraphMgr.tickVisualizers(dt);
		
		vfxGraphMgr.drawEditor(sx, sy);
		
		doMenus(false, true);
	}
};

struct FileEditor_AudioGraph : FileEditor
{
	static const int defaultSx = 600;
	static const int defaultSy = 600;
	
	AudioMutex audioMutex;
	AudioVoiceManagerBasic audioVoiceMgr;
	AudioGraphManager_RTE audioGraphMgr;
	
	AudioUpdateHandler audioUpdateHandler;
	PortAudioObject paObject;
	
	AudioGraphInstance * instance = nullptr;
	
	FileEditor_AudioGraph(const char * path)
		: audioGraphMgr(defaultSx, defaultSy)
	{
		// init audio graph
		audioMutex.init();
		audioVoiceMgr.init(audioMutex.mutex, 64, 64);
		audioVoiceMgr.outputStereo = true;
		audioGraphMgr.init(audioMutex.mutex, &audioVoiceMgr);
		
		// init audio output
		audioUpdateHandler.init(audioMutex.mutex, nullptr, 0);
		audioUpdateHandler.voiceMgr = &audioVoiceMgr;
		audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
		
		paObject.init(44100, 2, 0, 256, &audioUpdateHandler);
		
		// create instance
		instance = audioGraphMgr.createInstance(path);
		audioGraphMgr.selectInstance(instance);
	}
	
	virtual ~FileEditor_AudioGraph() override
	{
		// free instance
		audioGraphMgr.free(instance, false);
		Assert(instance == nullptr);
		
		// shut audio output
		paObject.shut();
		audioUpdateHandler.shut();
		
		// shut audio graph
		audioGraphMgr.shut();
		audioVoiceMgr.shut();
		audioMutex.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		// update real-time editing
		
		inputIsCaptured |= audioGraphMgr.tickEditor(sx, sy, dt, inputIsCaptured);
		
		// tick audio graph
		
		audioGraphMgr.tickMain();
		
		// draw ?
		
		if (hasFocus == false && audioGraphMgr.selectedFile->graphEdit->animationIsDone)
			return;
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		// update visualizers and draw editor
		
		audioGraphMgr.drawEditor(sx, sy);
	}
};

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
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Text(const char * in_path)
	{
		path = in_path;
		
		textEditor.SetLanguageDefinition(TextEditor::LanguageDefinition::CPlusPlus());

		isValid = loadIntoTextEditor(path.c_str(), lineEndings, textEditor);
		
		guiContext.init();
	#if 0
		// todo : find a good mono spaced font for the editor
		guiContext.pushImGuiContext();
		ImGuiIO& io = ImGui::GetIO();
		io.FontDefault = io.Fonts->AddFontFromFileTTF((s_dataFolder + "/unispace.ttf").c_str(), 16)
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
				
				auto cpos = textEditor.GetCursorPosition();
				ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, textEditor.GetTotalLines(),
					lineEndings == TextIO::kLineEndings_Unix ? "Unix" :
					lineEndings == TextIO::kLineEndings_Windows ? "Win " :
					"",
					textEditor.IsOverwrite() ? "Ovr" : "Ins",
					textEditor.CanUndo() ? "*" : " ",
					textEditor.GetLanguageDefinition().mName.c_str(), path.c_str());
				
				textEditor.Render(filename.c_str(), ImVec2(sx - 20, sy - 40), false);
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		guiContext.draw();
	}
};

struct FileEditor_Sprite : FileEditor
{
	Sprite sprite;
	std::string path;
	char sheetFilename[64];
	
	FrameworkImGuiContext guiContext;
	
	bool firstFrame = true;
	
	FileEditor_Sprite(const char * in_path)
		: sprite(in_path)
		, path(in_path)
	{
		sheetFilename[0] = 0;
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Sprite() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		sprite.update(dt);
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		if (firstFrame)
		{
			firstFrame = false;
			const float scaleX = sx / float(sprite.getWidth());
			const float scaleY = sy / float(sprite.getHeight());
			const float scale = fminf(1.f, fminf(scaleX, scaleY));
			
			sprite.scale = scale;
		}
		
		setColor(colorWhite);
		sprite.x = (sx - sprite.getWidth() * sprite.scale) / 2;
		sprite.y = (sy - sprite.getHeight() * sprite.scale) / 2;
		sprite.draw();
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(8, 8));
			if (ImGui::Begin("Sprite", nullptr,
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (ImGui::InputText("Sheet", sheetFilename, sizeof(sheetFilename)))
				{
					if (sheetFilename[0] == 0)
						sprite = Sprite(path.c_str());
					else
						sprite = Sprite(path.c_str(), 0.f, 0.f, sheetFilename);
				}
				
				ImGui::SliderFloat("angle", &sprite.angle, -360.f, +360.f);
				ImGui::SliderFloat("scale", &sprite.scale, 0.f, 100.f, "%.2f", 2);
				ImGui::Checkbox("flip X", &sprite.flipX);
				ImGui::Checkbox("flip Y", &sprite.flipY);
				ImGui::InputFloat("pivot X", &sprite.pivotX);
				ImGui::InputFloat("pivot Y", &sprite.pivotY);
				
				auto animList = sprite.getAnimList();
				
				if (!animList.empty())
				{
					const char ** animItems = (const char**)alloca(animList.size() * sizeof(char*));
					auto & animName = sprite.getAnim();
					int animIndex = -1;
					for (int i = 0; i < (int)animList.size(); ++i)
					{
						if (animList[i] == animName)
							animIndex = i;
						animItems[i]= animList[i].c_str();
					}
					if (ImGui::Combo("Animation", &animIndex, animItems, animList.size()))
						sprite.startAnim(animItems[animIndex]);
				}
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		guiContext.draw();
	}
};

struct FileEditor_AudioStream_Vorbis : FileEditor
{
	AudioOutput_PortAudio audioOutput;
	AudioStream_Vorbis * audioStream = nullptr;
	
	FileEditor_AudioStream_Vorbis(const char * path)
	{
		audioStream = new AudioStream_Vorbis();
		audioStream->Open(path, true);
		
		audioOutput.Initialize(2, audioStream->SampleRate_get(), 256);
		audioOutput.Play(audioStream);
	}
	
	virtual ~FileEditor_AudioStream_Vorbis()
	{
		audioOutput.Shutdown();
		
		delete audioStream;
		audioStream = nullptr;
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		audioOutput.Update();
		
		if (hasFocus == false)
			return;
		
		clearSurface(0, 0, 0, 0);
		
		if (audioStream->Duration_get() > 0)
		{
			gxPushMatrix();
			gxTranslatef(0, sy/2, 0);
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			setColor(100, 100, 200);
			hqFillRoundedRect(4, -10, int64_t(sx - 4) * audioStream->Position_get() / audioStream->Duration_get(), +10, 10);
			hqEnd();
			
			hqBegin(HQ_STROKED_ROUNDED_RECTS);
			setColor(140, 140, 200);
			hqStrokeRoundedRect(4, -10, sx - 4, +10, 10, 2.f);
			hqEnd();
			
			gxPopMatrix();
		}
	}
};

struct FileEditor_Model : FileEditor
{
	Model model;
	Vec3 min;
	Vec3 max;
	float rotationX = 0.f;
	float rotationY = 0.f;
	
	bool showColorNormals = true;
	bool showColorTexCoords = false;
	bool showNormals = false;
	float normalsScale = 1.f;
	bool showBones = false;
	bool showBindPose = false;
	bool showUnskinned = false;
	bool showHardskinned = false;
	bool showColorBlendIndices = false;
	bool showColorBlendWeights = false;
	bool showBoundingBox = false;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Model(const char * path)
		: model(path)
	{
		model.calculateAABB(min, max, true);
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Model() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		// tick
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_Always);
			if (ImGui::Begin("Model", nullptr,
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_AlwaysAutoResize))
			{
				auto animList = model.getAnimList();
				
				const char ** animItems = (const char**)alloca(animList.size() * sizeof(char*));
				const char * animName = model.getAnimName();
				int animIndex = -1;
				for (int i = 0; i < (int)animList.size(); ++i)
				{
					if (animList[i] == animName)
						animIndex = i;
					animItems[i]= animList[i].c_str();
				}
				if (ImGui::Combo("Animation", &animIndex, animItems, animList.size()))
					model.startAnim(animItems[animIndex]);
				
				if (ImGui::Button("Stop animation"))
					model.stopAnim();
				if (ImGui::Button("Restart animation"))
					model.startAnim(model.getAnimName());
				
				ImGui::SliderFloat("Animation speed", &model.animSpeed, 0.f, 10.f, "%.2f", 2.f);
				
				ImGui::Checkbox("Show colored normals", &showColorNormals);
				ImGui::Checkbox("Show colored texture UVs", &showColorTexCoords);
				ImGui::Checkbox("Show normals", &showNormals);
				ImGui::SliderFloat("Normals scale", &normalsScale, 0.f, 100.f, "%.2f", 2.f);
				ImGui::Checkbox("Show bones", &showBones);
				ImGui::Checkbox("Show bind pose", &showBindPose);
				ImGui::Checkbox("Unskinned", &showUnskinned);
				ImGui::Checkbox("Hard skinned", &showHardskinned);
				ImGui::Checkbox("Show colored blend indices", &showColorBlendIndices);
				ImGui::Checkbox("Show colored blend weights", &showColorBlendWeights);
				ImGui::Checkbox("Show bounding box", &showBoundingBox);
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		if (inputIsCaptured == false && mouse.isDown(BUTTON_LEFT))
		{
			inputIsCaptured = true;

			rotationX += mouse.dy;
			rotationY -= mouse.dx;
		}
		
		//
		
		model.animRootMotionEnabled = false;
		
		model.tick(dt);
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		projectPerspective3d(60.f, .01f, 100.f);
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			
			float maxAxis = 0.f;
			
			for (int i = 0; i < 3; ++i)
				maxAxis = fmaxf(maxAxis, fmaxf(fabsf(min[i]), fabsf(max[i])));
			
			if (maxAxis > 0.f)
			{
				const int drawFlags =
					DrawMesh
					| DrawColorNormals * showColorNormals
					| DrawNormals * showNormals
					| DrawColorTexCoords * showColorTexCoords
					| DrawBones * showBones
					| DrawPoseMatrices * showBindPose
					| DrawUnSkinned * showUnskinned
					| DrawHardSkinned * showHardskinned
					| DrawColorBlendIndices * showColorBlendIndices
					| DrawColorBlendWeights * showColorBlendWeights
					| DrawBoundingBox * showBoundingBox;
				
				model.drawNormalsScale = normalsScale;
				
				gxPushMatrix();
				gxTranslatef(0, 0, 2.f);
				gxScalef(1.f / maxAxis, 1.f / maxAxis, 1.f / maxAxis);
				gxTranslatef(0.f, 0.f, 1.f);
				gxRotatef(rotationY, 0.f, 1.f, 0.f);
				gxRotatef(rotationX, 1.f, 0.f, 0.f);
				model.draw(drawFlags);
				gxPopMatrix();
			}
			
			glDisable(GL_DEPTH_TEST);
		}
		projectScreen2d();
		
		guiContext.draw();
	}
};

struct FileEditor_Spriter : FileEditor
{
	Spriter spriter;
	SpriterState spriterState;
	
	bool firstFrame = true;
	
	FrameworkImGuiContext guiContext;
	
	FileEditor_Spriter(const char * path)
		: spriter(path)
	{
		spriterState.startAnim(spriter, "Idle");
		
		guiContext.init();
	}
	
	virtual ~FileEditor_Spriter() override
	{
		guiContext.shut();
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		if (firstFrame)
		{
			firstFrame = false;
			
			spriterState.x = sx/2.f;
			spriterState.y = sy/2.f;
		}
		
		spriterState.updateAnim(spriter, dt);
		
		//
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(8, 8));
			ImGui::SetNextWindowSize(ImVec2(240, 0));
			if (ImGui::Begin("Spriter"))
			{
				const int animCount = spriter.getAnimCount();
				
				if (animCount > 0)
				{
					const char ** animItems = (const char**)alloca(animCount * sizeof(char*));
					int animIndex = spriterState.animIndex;
					for (int i = 0; i < animCount; ++i)
						animItems[i]= spriter.getAnimName(i);
					if (ImGui::Combo("Animation", &animIndex, animItems, animCount))
						spriterState.startAnim(spriter, animIndex);
					if (ImGui::Button("Restart animation"))
						spriterState.startAnim(spriter, spriterState.animIndex);
				}
			}
			ImGui::End();
		}
		guiContext.processEnd();
		
		if (inputIsCaptured == false && mouse.isDown(BUTTON_LEFT))
		{
			inputIsCaptured = true;
			
			spriterState.x = mouse.x;
			spriterState.y = mouse.y;
		}
		
		// draw
		
		clearSurface(0, 0, 0, 0);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		setColor(colorWhite);
		spriter.draw(spriterState);
		
		guiContext.draw();
	}
};

struct FileEditor_Video : FileEditor
{
	MediaPlayer mp;
	AudioOutput_PortAudio audioOutput;
	
	bool hasAudioInfo = false;
	bool audioIsStarted = false;
	
	FileEditor_Video(const char * path)
	{
		MediaPlayer::OpenParams openParams;
		openParams.enableAudioStream = true;
		openParams.enableVideoStream = true;
		openParams.filename = path;
		openParams.outputMode = MP::kOutputMode_RGBA;
		openParams.audioOutputMode = MP::kAudioOutputMode_Stereo;
		
		mp.openAsync(openParams);
	}
	
	virtual ~FileEditor_Video() override
	{
		audioOutput.Shutdown();
		
		mp.close(true);
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		// attempt to get information about the video so we can start the audio

		int channelCount;
		int sampleRate;
		
		if (hasAudioInfo == false && mp.getAudioProperties(channelCount, sampleRate))
		{
			hasAudioInfo = true;

			if (channelCount > 0)
			{
				audioIsStarted = true;
			
				audioOutput.Initialize(channelCount, sampleRate, 256);
				audioOutput.Play(&mp);
			}
		}
		
		// update presentation time stamp for the video

		if (audioIsStarted)
			mp.presentTime = mp.audioTime;
		else if (hasAudioInfo)
			mp.presentTime += dt;
		
		// update video

		mp.tick(mp.context, true);

		int videoSx;
		int videoSy;
		double duration;
		double sampleAspectRatio;

		const bool hasVideoInfo = mp.getVideoProperties(videoSx, videoSy, duration, sampleAspectRatio);

		// check if the video ended and needs to be looped

		if (mp.presentedLastFrame(mp.context))
		{
			auto openParams = mp.context->openParams;

			mp.close(false);
			mp.presentTime = 0.0;
			mp.openAsync(openParams);

			audioOutput.Shutdown();

			hasAudioInfo = false;
			audioIsStarted = false;
		}
		
		//
		
		clearSurface(0, 0, 0, 0);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		auto texture = mp.getTexture();
		
		if (texture != 0 && hasVideoInfo)
		{
			videoSx *= sampleAspectRatio;
			
			pushBlend(BLEND_OPAQUE);
			{
				const float scaleX = sx / float(videoSx);
				const float scaleY = sy / float(videoSy);
				const float scale = fminf(1.f, fminf(scaleX, scaleY));
				
				gxPushMatrix();
				{
					gxTranslatef((sx - videoSx * scale) / 2, (sy - videoSy * scale) / 2, 0);
					gxScalef(scale, scale, 1.f);
					
					gxSetTexture(texture);
					setColor(colorWhite);
					drawRect(0, 0, videoSx, videoSy);
					gxSetTexture(0);
				}
				gxPopMatrix();
			}
			popBlend();
		}
	}
};

struct FileEditor_Jgmod : FileEditor
{
	AllegroTimerApi timerApi;
	AllegroVoiceApi voiceApi;
	
	JGMOD * jgmod = nullptr;
	
	JGMOD_PLAYER player;
	
	AudioOutput_PortAudio audioOutput;
	AudioStream_AllegroVoiceMixer voiceMixer;
	
	FileEditor_Jgmod(const char * path)
		: timerApi(AllegroTimerApi::kMode_Manual)
		, voiceApi(192000, false)
		, voiceMixer(&voiceApi, &timerApi)
	{
		jgmod = jgmod_load(path);
		
		player.init(64, &timerApi, &voiceApi);
		player.play(jgmod, true);
		
		audioOutput.Initialize(2, 192000, 32);
		audioOutput.Play(&voiceMixer);
	}
	
	virtual ~FileEditor_Jgmod() override
	{
		audioOutput.Shutdown();
		
		jgmod_destroy(jgmod);
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		clearSurface(0, 0, 0, 0);
		
		setFont("calibri.ttf");
		pushFontMode(FONT_SDF);
		{
			gxTranslatef(12, 12, 0);
			jgmod_draw(player, true);
		}
		popFontMode();
	}
};

struct FileEditor_ParticleSystem : FileEditor
{
	ParticleEditor particleEditor;
	
	FileEditor_ParticleSystem(const char * path)
	{
		particleEditor.load(path);
	}
	
	virtual ~FileEditor_ParticleSystem() override
	{
		
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		clearSurface(0, 0, 0, 0);
		
		particleEditor.tick(inputIsCaptured == false, sx, sy, dt);
		
		particleEditor.draw(inputIsCaptured == false, sx, sy);
	}
};

#define MIDI_OFF 0x80
#define MIDI_ON 0x90

struct MidiBuffer
{
	static const int kMaxBytes = 1000 * 3;

	uint8_t bytes[kMaxBytes];
	int numBytes = 0;

	bool append(const uint8_t * in_bytes, const int in_numBytes)
	{
		if (numBytes + in_numBytes > MidiBuffer::kMaxBytes)
			return false;

		for (int i = 0; i < in_numBytes; ++i)
			bytes[numBytes++] = in_bytes[i];

		return true;
	}
};

struct MidiKeyboard
{
	static const int kNumKeys = 16;

	struct Key
	{
		bool isDown;
	};

	Key keys[kNumKeys];

	int octave = 4;

	MidiKeyboard()
	{
		for (int i = 0; i < kNumKeys; ++i)
		{
			auto & key = keys[i];

			key.isDown = false;
		}
	}

	int getNote(const int keyIndex) const
	{
		return octave * 8 + keyIndex;
	}

	void changeOctave(const int direction)
	{
		octave = clamp(octave + direction, 0, 10);

		for (int i = 0; i < kNumKeys; ++i)
		{
			auto & key = keys[i];

			key.isDown = false;
		}
	}
};

void doMidiKeyboard(MidiKeyboard & kb, const int mouseX, const int mouseY, MidiBuffer & midiBuffer, const bool doTick, const bool doDraw, int & out_sx, int & out_sy, bool & inputIsCaptured)
{
	const int keySx = 16;
	const int keySy = 64;
	const int octaveSx = 24;
	const int octaveSy = 24;

	const int hoverIndex = mouseY >= 0 && mouseY <= keySy ? mouseX / keySx : -1;
	const float velocity = clamp(mouseY / float(keySy), 0.f, 1.f);

	const int octaveX = keySx * MidiKeyboard::kNumKeys + 4;
	const int octaveY = 4;
	const int octaveHoverIndex = mouseX >= octaveX && mouseX <= octaveX + octaveSx ? (mouseY - octaveY) / octaveSy : -1;

	const int sx = std::max(keySx * MidiKeyboard::kNumKeys, octaveX + octaveSx);
	const int sy = std::max(keySy, octaveSy * 2);

	out_sx = sx;
	out_sy = sy;

	if (doTick)
	{
		bool isDown[MidiKeyboard::kNumKeys];
		memset(isDown, 0, sizeof(isDown));

		if (inputIsCaptured == false)
		{
			if (mouse.isDown(BUTTON_LEFT) && hoverIndex >= 0 && hoverIndex < MidiKeyboard::kNumKeys)
				isDown[hoverIndex] = true;
		}

		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
		{
			auto & key = kb.keys[i];

			if (key.isDown != isDown[i])
			{
				if (isDown[i])
				{
					key.isDown = true;

					const uint8_t message[3] = { MIDI_ON, (uint8_t)kb.getNote(i), uint8_t(velocity * 127) };
					
					midiBuffer.append(message, 3);
				}
				else
				{
					const uint8_t message[3] = { MIDI_OFF, (uint8_t)kb.getNote(i), uint8_t(velocity * 127) };

					midiBuffer.append(message, 3);

					key.isDown = false;
				}
			}
		}

		if (inputIsCaptured == false && mouse.wentDown(BUTTON_LEFT))
		{
			if (octaveHoverIndex == 0)
				kb.changeOctave(+1);
			if (octaveHoverIndex == 1)
				kb.changeOctave(-1);
		}

		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
			inputIsCaptured |= kb.keys[i].isDown;
	}

	if (doDraw)
	{
		setColor(colorBlack);
		drawRect(0, 0, sx, sy);

		const Color colorKey(200, 200, 200);
		const Color colorkeyHover(255, 255, 255);
		const Color colorKeyDown(100, 100, 100);

		hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(float(M_PI)/2.f).Scale(1.f, 1.f / keySy, 1.f), Color(140, 180, 220), colorWhite, COLOR_MUL);

		for (int i = 0; i < MidiKeyboard::kNumKeys; ++i)
		{
			auto & key = kb.keys[i];

			gxPushMatrix();
			{
				gxTranslatef(keySx * i, 0, 0);

				setColor(key.isDown ? colorKeyDown : i == hoverIndex ? colorkeyHover : colorKey);

				hqBegin(HQ_FILLED_RECTS);
				hqFillRect(0, 0, keySx, keySy);
				hqEnd();
			}
			gxPopMatrix();
		}

		hqClearGradient();

		{
			gxPushMatrix();
			{
				gxTranslatef(octaveX, octaveY, 0);

				//setLumi(200);
				//drawText(octaveSx + 4, 4, 12, +1, +1, "octave: %d", kb.octave);

				setColor(0 == octaveHoverIndex ? colorkeyHover : colorKey);
				hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(float(M_PI)/2.f).Scale(1.f, 1.f / (octaveSy * 2), 1.f).Translate(-octaveX, -octaveY, 0), Color(140, 180, 220), colorWhite, COLOR_MUL);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(0, 0, octaveSx, octaveSy, 2.f);
				hqEnd();
				hqClearGradient();

				setLumi(40);
				hqBegin(HQ_FILLED_TRIANGLES);
				hqFillTriangle(octaveSx/2, 6, octaveSx - 6, octaveSy - 6, 6, octaveSy - 6);
				hqEnd();

				gxTranslatef(0, octaveSy, 0);

				setColor(1 == octaveHoverIndex ? colorkeyHover : colorKey);
				hqSetGradient(GRADIENT_LINEAR, Mat4x4(true).RotateZ(float(M_PI)/2.f).Scale(1.f, 1.f / (octaveSy * 2), 1.f).Translate(-octaveX, -octaveY, 0), Color(140, 180, 220), colorWhite, COLOR_MUL);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				hqFillRoundedRect(0, 0, octaveSx, octaveSy, 2.f);
				hqEnd();
				hqClearGradient();

				setLumi(40);
				hqBegin(HQ_FILLED_TRIANGLES);
				hqFillTriangle(octaveSx/2, octaveSy - 6, octaveSx - 6, 6, 6, 6);
				hqEnd();
			}
			gxPopMatrix();
		}
		hqClearGradient();
	}
}

struct FileEditor_JsusFx : FileEditor, PortAudioHandler
{
	JsusFx_Framework jsusFx;

	JsusFxGfx_Framework gfx;
	JsusFxFileAPI_Basic fileApi;
	JsusFxPathLibrary_Basic pathLibary;

	Surface * surface = nullptr;

	bool isValid = false;

	bool firstFrame = true;

	int offsetX = 0;
	int offsetY = 0;
	bool isDragging = false;
	
	bool sliderIsActive[JsusFx::kMaxSliders] = { };
	
	PortAudioObject paObject;
	SDL_mutex * mutex = nullptr;

	MidiBuffer midiBuffer;
	MidiKeyboard midiKeyboard;

	FileEditor_JsusFx(const char * path)
		: jsusFx(pathLibary)
		, gfx(jsusFx)
		, pathLibary(".")
	{
		jsusFx.init();

		fileApi.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileApi;

		gfx.init(jsusFx.m_vm);
		jsusFx.gfx = &gfx;

		isValid = jsusFx.compile(pathLibary, path, JsusFx::kCompileFlag_CompileGraphicsSection);

		if (isValid)
		{
			mutex = SDL_CreateMutex();

			jsusFx.prepare(44100, 256);
			
			paObject.init(44100, 2, 0, 256, this);
		}
	}
	
	virtual ~FileEditor_JsusFx() override
	{
		paObject.shut();

		if (mutex != nullptr)
		{
			SDL_DestroyMutex(mutex);
			mutex = nullptr;
		}

		delete surface;
		surface = nullptr;
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		Assert(framesPerBuffer == 256);
		
		float inputSamples[2][256];
		
		if (false)
			memset(inputSamples, 0, sizeof(inputSamples));
		else
		{
			const float s = .1f;

			for (int i = 0; i < 256; ++i)
			{
				inputSamples[0][i] = random(-1.f, +1.f) * s;
				inputSamples[1][i] = random(-1.f, +1.f) * s;
			}
		}
		
		float outputSamples[2][256];
		
		const float * inputs[2] = { inputSamples[0], inputSamples[1] };
		float * outputs[2] = { outputSamples[0], outputSamples[1] };
		
		MidiBuffer midiBufferCopy;

		SDL_LockMutex(mutex);
		{
			midiBufferCopy = midiBuffer;

			midiBuffer.numBytes = 0;
		}
		SDL_UnlockMutex(mutex);

		jsusFx.setMidi(midiBufferCopy.bytes, midiBufferCopy.numBytes);
		
		jsusFx.process(inputs, outputs, framesPerBuffer, 2, 2);
		
		float * outputBufferf = (float*)outputBuffer;
		
		for (int i = 0; i < 256; ++i)
		{
			outputBufferf[i * 2 + 0] = outputSamples[0][i];
			outputBufferf[i * 2 + 1] = outputSamples[1][i];
		}
	}

	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (surface == nullptr || surface->getWidth() != sx || surface->getHeight() != sy)
		{
			delete surface;
			surface = nullptr;

			surface = new Surface(sx, sy, false);
		}
		
		clearSurface(0, 0, 0, 0);

		pushSurface(surface);
		{
			setColor(colorWhite);
			drawUiRectCheckered(0, 0, sx, sy, 8);
		
			int midiSx;
			int midiSy;

			doMidiKeyboard(midiKeyboard, mouse.x, mouse.y, midiBuffer, true, false, midiSx, midiSy, inputIsCaptured);

			if (isValid == false)
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				setColor(colorWhite);
				drawText(sx/2, sy/2, 16, 0, 0, "Failed to load jsfx file");
				popFontMode();
			}
			else if (jsusFx.hasGraphicsSection())
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				setColorClamp(true);
				{
					const int gfxSx = jsusFx.gfx_w;
					const int gfxSy = jsusFx.gfx_h;

					if (firstFrame)
					{
						firstFrame = false;

						offsetX = (sx - gfxSx) / 2;
						offsetY = (sy - gfxSy) / 2;
					}

					if (inputIsCaptured == false)
					{
						const bool isOutside =
							mouse.x < offsetX || mouse.x > offsetX + jsusFx.gfx_w ||
							mouse.y < offsetY || mouse.y > offsetY + jsusFx.gfx_h;

						if (isOutside && mouse.wentDown(BUTTON_LEFT))
						{
							isDragging = true;
						}

						if (mouse.wentUp(BUTTON_LEFT))
						{
							isDragging = false;
						}
					}
					else
					{
						isDragging = false;
					}
				
					if (isDragging)
					{
						inputIsCaptured = true;

						offsetX = mouse.x - gfxSx/2;
						offsetY = mouse.y;
					}
				
					if (inputIsCaptured == false)
					{
						if (keyboard.wentDown(SDLK_f) && keyboard.isDown(SDLK_LSHIFT))
						{
							inputIsCaptured = true;
						
							jsusFx.gfx_w = sx;
							jsusFx.gfx_h = sy;
						
							offsetX = 0;
							offsetY = 0;
						}
					}

					mouse.x -= offsetX;
					mouse.y -= offsetY;
					{
						setDrawRect(offsetX, offsetY, gfxSx, gfxSy);
						gfx.drawTransform.MakeTranslation(offsetX, offsetY, 0.f);

						gfx.setup(surface, gfxSx, gfxSy, mouse.x, mouse.y, inputIsCaptured == false);

						jsusFx.draw();

						clearDrawRect();
					}
					mouse.x += offsetX;
					mouse.y += offsetY;
				}
				setColorClamp(false);
				popFontMode();
			}
			else
			{
				setFont("calibri.ttf");
				pushFontMode(FONT_SDF);
				{
					const int margin = 10;
				
					int x = margin;
					int y = margin;

					int sliderIndex = 0;
				
					const int sx = 200;
					const int sy = 20;
					const int advanceY = 22;
				
					auto doSlider = [&](JsusFx & fx, JsusFx_Slider & slider, int x, int y, bool & isActive)
						{
							const bool isInside =
								x >= 0 && x <= sx &&
								y >= 0 && y <= sy;
						
							if (isInside && mouse.wentDown(BUTTON_LEFT))
								isActive = true;
						
							if (mouse.wentUp(BUTTON_LEFT))
								isActive = false;
						
							if (isActive)
							{
								const float t = x / float(sx);
								const float v = slider.min + (slider.max - slider.min) * t;
								fx.moveSlider(&slider - fx.sliders, v);
							}
						
							setColor(0, 0, 255, 127);
							const float t = (slider.getValue() - slider.min) / (slider.max - slider.min);
							drawRect(0, 0, sx * t, sy);
						
							if (slider.isEnum)
							{
								const int enumIndex = (int)slider.getValue();
							
								if (enumIndex >= 0 && enumIndex < slider.enumNames.size())
								{
									setColor(colorWhite);
									drawText(sx/2.f, sy/2.f, 14.f, 0.f, 0.f, "%s", slider.enumNames[enumIndex].c_str());
								}
							}
							else
							{
								setColor(colorWhite);
								drawText(sx/2.f, sy/2.f, 14.f, 0.f, 0.f, "%s", slider.desc);
							}
						
							setColor(63, 31, 255, 127);
							drawRectLine(0, 0, sx, sy);
						};

					int numSliders = 0;
				
 					for (auto & slider : jsusFx.sliders)
						if (slider.exists && slider.desc[0] != '-')
							numSliders++;
				
					if (numSliders > 0)
					{
						const int totalSx = sx + margin * 2;
						const int totalSy = advanceY * numSliders + margin * 2;
					
						setColor(0, 0, 0, 127);
						drawRect(0, 0, totalSx, totalSy);
					
						for (auto & slider : jsusFx.sliders)
						{
							if (slider.exists && slider.desc[0] != '-')
							{
								gxPushMatrix();
								gxTranslatef(x, y, 0);
								doSlider(jsusFx, slider, mouse.x - x, mouse.y - y, sliderIsActive[sliderIndex]);
								gxPopMatrix();

								y += advanceY;
							}

							sliderIndex++;
						}
					}
				}
				popFontMode();
			}

			doMidiKeyboard(midiKeyboard, mouse.x, mouse.y, midiBuffer, false, true, midiSx, midiSy, inputIsCaptured);
		}
		popSurface();

		surface->blit(BLEND_OPAQUE);
	}
};

struct FileEditor_Font : FileEditor
{
	std::string path;
	
	FileEditor_Font(const char * in_path)
		: path(in_path)
	{
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		clearSurface(0, 0, 0, 0);
		
		setFont(path.c_str());
		pushFontMode(FONT_BITMAP);
		setColor(colorWhite);
		
		const int fontSize = std::max(4, std::min(sx, sy) / 16);
		
		for (int xi = 0; xi < 16; ++xi)
		{
			for (int yi = 0; yi < 8; ++yi)
			{
				const int x = (xi + .5f) * sx / 16;
				const int y = (yi + .5f) * sy / 16 + sy/4;
				
				const char c = xi + yi * 16;
				
				if (isprint(c))
					drawText(x, y, fontSize, 0, -1, " %c", c);
			}
		}
		
		popFontMode();
	}
};

struct EditorWindow
{
	Window window;

	FileEditor * editor = nullptr;
	
	Surface * surface = nullptr;

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
		if (surface == nullptr || surface->getWidth() != window.getWidth() || surface->getHeight() != window.getHeight())
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
						window.hasFocus(),
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
	
	fileBrowser.onFileSelected = [&](const std::string & path)
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
	};
	
	std::vector<EditorWindow*> editorWindows;

	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
		// update editor windows

		if (editor != nullptr)
		{
			if (keyboard.wentDown(SDLK_e) && keyboard.isDown(SDLK_LSHIFT))
			{
				EditorWindow * window = new EditorWindow(editor);

				editorWindows.push_back(window);

				editor = nullptr;
			}
		}

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
