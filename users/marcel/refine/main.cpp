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

		//const char * rootFolder = "/Users/thecat/framework/vfxGraph-examples/data";
		const char * rootFolder = "/Users/thecat/framework/";
		
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
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		const float scaleX = sx / float(sprite.getWidth());
		const float scaleY = sy / float(sprite.getHeight());
		const float scale = fminf(1.f, fminf(scaleX, scaleY));
		
		setColor(colorWhite);
		sprite.scale = scale;
		sprite.draw();
	}
};

#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"

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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		audioOutput.Update();
		
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
	
	FileEditor_Model(const char * path)
		: model(path)
	{
		model.calculateAABB(min, max, false);
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		projectPerspective3d(60.f, .01f, 100.f);
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			
			const Vec3 delta = max - min;
			const Vec3 middle = (max + min) / 2.f;
			const float maxAxis = fmaxf(delta[0], fmaxf(delta[1], delta[2]));
			
			if (maxAxis > 0.f)
			{
				gxPushMatrix();
				gxTranslatef(0, 0, 1.f);
				gxScalef(1.f / maxAxis, 1.f / maxAxis, 1.f / maxAxis);
				gxTranslatef(-middle[0], -middle[1], -middle[2]);
				model.draw(DrawMesh | DrawColorNormals);
				gxPopMatrix();
			}
			
			glDisable(GL_DEPTH_TEST);
		}
		projectScreen2d();
	}
};

struct FileEditor_Spriter : FileEditor
{
	Spriter spriter;
	SpriterState spriterState;
	
	bool firstFrame = true;
	
	FileEditor_Spriter(const char * path)
		: spriter(path)
	{
		spriterState.startAnim(spriter, "Idle");
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		spriterState.updateAnim(spriter, dt);
		
		if (firstFrame)
		{
			firstFrame = false;
			
			spriterState.x = sx/2.f;
			spriterState.y = sy/2.f;
		}
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			spriterState.x = mouse.x;
			spriterState.y = mouse.y;
		}
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		setColor(colorWhite);
		spriter.draw(spriterState);
	}
};

#include "video.h"

struct FileEditor_Video : FileEditor
{
	MediaPlayer mp;
	AudioOutput_PortAudio audioOutput;
	
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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		int channelCount;
		int sampleRate;
		
		if (audioIsStarted == false && mp.getAudioProperties(channelCount, sampleRate))
		{
			audioIsStarted = true;
			
			audioOutput.Initialize(channelCount, sampleRate, 256);
			audioOutput.Play(&mp);
		}
		
		mp.presentTime = mp.audioTime;
		
		mp.tick(mp.context, true);
		
		//
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		auto texture = mp.getTexture();
		
		int videoSx;
		int videoSy;
		double duration;
		
		if (texture != 0 && mp.getVideoProperties(videoSx, videoSy, duration))
		{
			pushBlend(BLEND_OPAQUE);
			{
				gxSetTexture(texture);
				setColor(colorWhite);
				drawRect(0, 0, videoSx, videoSy);
				gxSetTexture(0);
			}
			popBlend();
		}
	}
};

#include "framework-allegro2.h"
#include "jgmod.h"

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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
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
	Surface * editorSurface = new Surface(VIEW_SX - 300, VIEW_SY, false, true);
	
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
		else if (extension == "bmp" || extension == "jpg" || extension == "jpeg" || extension == "png" || extension == "psd" || extension == "tga")
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
							editorSurface->clear();
							editorSurface->clearDepth(1.f);
							
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
