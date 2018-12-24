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
		
		inputIsCaptured |= vfxGraphMgr.tickEditor(sx, sy, dt, inputIsCaptured);
		
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
		
		vfxGraphMgr.drawEditor(sx, sy);
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
		
		inputIsCaptured |= audioGraphMgr.tickEditor(sx, sy, dt, inputIsCaptured);
		
		// tick audio graph
		
		audioGraphMgr.tickMain();
		
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
								
								// fixme : remember path !
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
				
				textEditor.Render(filename.c_str(), ImVec2(sx - 20, sy - 40), false);
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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		sprite.update(dt);
		
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
			ImGui::SetNextWindowPos(ImVec2(4, 4));
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
		
		if (inputIsCaptured == false && mouse.isDown(BUTTON_LEFT))
		{
			inputIsCaptured = true;

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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
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

		const bool hasVideoInfo = mp.getVideoProperties(videoSx, videoSy, duration);

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
		
		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		auto texture = mp.getTexture();
		
		if (texture != 0 && hasVideoInfo)
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
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		particleEditor.tick(inputIsCaptured == false, sx, sy, dt);
		
		particleEditor.draw(inputIsCaptured == false, sx, sy);
	}
};

struct FileEditor_JsusFx : FileEditor
{
	JsusFx_Framework jsusFx;

	JsusFxGfx_Framework gfx;
	JsusFxFileAPI_Basic fileApi;
	JsusFxPathLibrary_Basic pathLibary;

	Surface *& surface;

	bool isValid = false;

	bool firstFrame = true;

	int offsetX = 0;
	int offsetY = 0;
	bool isDragging = false;
	
	bool sliderIsActive[JsusFx::kMaxSliders] = { };

	FileEditor_JsusFx(const char * path, Surface *& in_surface)
		: jsusFx(pathLibary)
		, gfx(jsusFx)
		, pathLibary(".")
		, surface(in_surface)
	{
		jsusFx.init();

		fileApi.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileApi;

		gfx.init(jsusFx.m_vm);
		jsusFx.gfx = &gfx;

		isValid = jsusFx.compile(pathLibary, path, JsusFx::kCompileFlag_CompileGraphicsSection);

		if (isValid)
		{
			jsusFx.prepare(44100, 256);
		}
	}

	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
		if (isValid == false)
			return;

		jsusFx.process(nullptr, nullptr, 256, 0, 0);

		setColor(colorWhite);
		drawUiRectCheckered(0, 0, sx, sy, 8);
		
		if (jsusFx.hasGraphicsSection())
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
				
				auto doSlider = [](JsusFx & fx, JsusFx_Slider & slider, int x, int y, bool & isActive)
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
	}
};

struct FileEditor_Font : FileEditor
{
	std::string path;
	
	FileEditor_Font(const char * in_path)
		: path(in_path)
	{
	}
	
	virtual void tick(const int sx, const int sy, const float dt, bool & inputIsCaptured) override
	{
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
		if (!window.hasFocus())
			return;

		pushWindow(window);
		{
			framework.beginDraw(0, 0, 0, 0);
			{
				bool inputIsCaptured = false;

				editor->tick(window.getWidth(), window.getHeight(), dt, inputIsCaptured);
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
	
	s_dataFolder = getDirectory();
	framework.fillCachesWithPath(".", true);
	
// todo : create ImGui context

	FrameworkImGuiContext guiContext;
	guiContext.init();
	guiContext.pushImGuiContext();
	ImGuiIO& io = ImGui::GetIO();
	io.FontDefault = io.Fonts->AddFontFromFileTTF("calibri.ttf", 16);
	guiContext.popImGuiContext();
	guiContext.updateFontTexture();

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
	
		auto directory = Path::GetDirectory(path);
	#ifndef WIN32
		directory = "/" + directory;
	#endif
		changeDirectory(directory.c_str());
		
		auto filename = Path::GetFileName(path);
		auto extension = Path::GetExtension(filename, true);
		
		if (extension == "ps" ||
			extension == "vs" ||
			extension == "txt" ||
			extension == "ini" ||
			extension == "cpp" ||
			extension == "h" ||
			extension == "py" ||
			extension == "jsfx-inc" ||
			extension == "inc" ||
			extension == "md" ||
			extension == "bat" ||
			extension == "sh")
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
		else if (extension == "pfx")
		{
			editor = new FileEditor_ParticleSystem(filename.c_str());
		}
		else if (extension == "jsfx")
		{
			editor = new FileEditor_JsusFx(filename.c_str(), editorSurface);
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
	
	changeDirectory(s_dataFolder.c_str());
	
	guiContext.shut();

	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
