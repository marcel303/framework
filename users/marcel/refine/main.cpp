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

#include "nfd.h"

#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"

#include "video.h"

#include "framework-allegro2.h"
#include "jgmod.h"

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
		//const char * rootFolder = "/Users/thecat/";
		
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
	virtual ~FileEditor()
	{
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) = 0;
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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		// update real-time editing
		
		inputIsCaptured |= audioGraphMgr.tickEditor(dt, inputIsCaptured);
		
		// tick audio graph
		
		audioGraphMgr.tickMain();
		
		// update visualizers and draw editor
		
		audioGraphMgr.tickVisualizers();
		
		audioGraphMgr.drawEditor();
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
		
		isValid = loadIntoTextEditor(path.c_str(), lineEndings, textEditor);
		
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
								isValid = loadIntoTextEditor(filename, lineEndings, textEditor);
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
							
							if (NFD_SaveDialog(nullptr, nullptr, &filename) == NFD_OKAY)
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
	std::string path;
	char sheetFilename[64];
	
	FrameworkImGuiContext guiContext;
	
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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		sprite.update(dt);
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		const float scaleX = sx / float(sprite.getWidth());
		const float scaleY = sy / float(sprite.getHeight());
		const float scale = fminf(1.f, fminf(scaleX, scaleY));
		
		setColor(colorWhite);
		sprite.x = (sx - sprite.getWidth() * scale) / 2;
		sprite.y = (sy - sprite.getHeight() * scale) / 2;
		sprite.scale = scale;
		sprite.draw();
		
		guiContext.processBegin(dt, sx, sy, inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(4, 4));
			ImGui::SetNextWindowSize(ImVec2(240, 60));
			if (ImGui::Begin("Sprite", nullptr,
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoMove))
			{
				if (ImGui::InputText("Sheet", sheetFilename, sizeof(sheetFilename)))
				{
					if (sheetFilename[0] == 0)
						sprite = Sprite(path.c_str());
					else
						sprite = Sprite(path.c_str(), 0.f, 0.f, sheetFilename);
				}
				
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
	float rotationX = 0.f;
	float rotationY = 0.f;
	
	FileEditor_Model(const char * path)
		: model(path)
	{
		model.calculateAABB(min, max, true);
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			rotationX += mouse.dy;
			rotationY -= mouse.dx;
		}
			
		projectPerspective3d(60.f, .01f, 100.f);
		{
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			
			const Vec3 delta = max - min;
			const float maxAxis = fmaxf(delta[0], fmaxf(delta[1], delta[2]));
			
			if (maxAxis > 0.f)
			{
				gxPushMatrix();
				gxTranslatef(0, 0, 1.f);
				gxScalef(1.f / maxAxis, 1.f / maxAxis, 1.f / maxAxis);
				gxTranslatef(0.f, 0.f, 1.f);
				gxRotatef(rotationY, 0.f, 1.f, 0.f);
				gxRotatef(rotationX, 1.f, 0.f, 0.f);
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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
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
			ImGui::SetNextWindowPos(ImVec2(4, 4));
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
	guiContext.init();

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
					
					*type = 'a';
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
