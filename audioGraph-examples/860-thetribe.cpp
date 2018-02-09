#include "framework.h"
#include "objects/audioSourceVorbis.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "Quat.h"
#include "soundmix.h"
#include "soundVolume.h"
#include "vfxGraph.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "video.h"
#include <atomic>

#define MAX_VOLUMES 6
#define MAX_SAMPLELOCATIONS_TOTAL (MAX_SAMPLELOCATIONS_PER_VOLUME * MAX_VOLUMES)

const int GFX_SX = 1024;
const int GFX_SY = 768;

//

#define NUM_VIDEOCLIP_SOURCES 3
#define NUM_VIDEOCLIPS MAX_VOLUMES
#define NUM_VFXCLIPS 1

#define DRAW_GRIDS 1
#define DO_SPOKENWORD 0
#define DO_CONTROLWINDOW 0

static const float timeSeed = 1234.f;

static const char * audioFilenames[NUM_VIDEOCLIP_SOURCES] =
{
	"0.1.ogg",
	"1.1.ogg",
	"2.1.ogg",
};

static const float audioGains[NUM_VIDEOCLIP_SOURCES] =
{
	1.f,
	.3f,
	1.f
};

static const char * videoFilenames[NUM_VIDEOCLIP_SOURCES] =
{
	"0.1280px.mp4",
	"1.1280px.mp4",
	"2.1280px.mp4",
};

//

struct MyMutex : binaural::Mutex
{
	SDL_mutex * mutex;
	
	MyMutex(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
	}
	
	virtual void lock() override
	{
		SDL_LockMutex(mutex);
	}
	
	virtual void unlock() override
	{
		SDL_UnlockMutex(mutex);
	}
};

struct MyPortAudioHandler : PortAudioHandler
{
	SDL_mutex * mutex;
	
	std::vector<MultiChannelAudioSource_SoundVolume*> volumeSources;
	std::vector<AudioSource*> pointSources;
	
	MyPortAudioHandler()
		: PortAudioHandler()
		, mutex(nullptr)
		, volumeSources()
		, pointSources()
	{
	}
	
	~MyPortAudioHandler()
	{
		Assert(mutex == nullptr);
	}
	
	void init(SDL_mutex * _mutex)
	{
		mutex = _mutex;
	}
	
	void shut()
	{
		mutex = nullptr;
	}
	
	void addVolumeSource(MultiChannelAudioSource_SoundVolume * source)
	{
		SDL_LockMutex(mutex);
		{
			volumeSources.push_back(source);
		}
		SDL_UnlockMutex(mutex);
	}
	
	void addPointSource(AudioSource * source)
	{
		SDL_LockMutex(mutex);
		{
			pointSources.push_back(source);
		}
		SDL_UnlockMutex(mutex);
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		ALIGN16 float channelL[AUDIO_UPDATE_SIZE];
		ALIGN16 float channelR[AUDIO_UPDATE_SIZE];
		
		memset(channelL, 0, sizeof(channelL));
		memset(channelR, 0, sizeof(channelR));
		
		SDL_LockMutex(mutex);
		{
			for (auto volumeSource : volumeSources)
			{
				volumeSource->generate(0, channelL, AUDIO_UPDATE_SIZE);
				volumeSource->generate(1, channelR, AUDIO_UPDATE_SIZE);
			}
			
			for (auto & pointSource : pointSources)
			{
				ALIGN16 float channel[AUDIO_UPDATE_SIZE];
				
				pointSource->generate(channel, AUDIO_UPDATE_SIZE);
				
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				{
					channelL[i] += channel[i];
					channelR[i] += channel[i];
				}
			}
		}
		SDL_UnlockMutex(mutex);
		
		float * __restrict destinationBuffer = (float*)outputBuffer;

		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			destinationBuffer[i * 2 + 0] = channelL[i];
			destinationBuffer[i * 2 + 1] = channelR[i];
		}
	}
};

static void drawSoundVolume(const SoundVolume & volume)
{
	gxPushMatrix();
	{
		gxMultMatrixf(volume.transform.m_v);
		
		gxPushMatrix(); { gxTranslatef(-1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(+1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, -1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, +1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, -1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, +1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
	}
	gxPopMatrix();
}

float videoClipBlend[3] =
{
	1.f, 0.f, 1.f
};

struct Videoclip
{
	enum EvalMode
	{
		EvalMode_Floating,
		EvalMode_Line,
		EvalMode_Circle
	};
	
	Mat4x4 transform;
	AudioSourceVorbis soundSource;
	SoundVolume soundVolume;
	MediaPlayer mp;
	float gain;
	
	int index;
	
	Videoclip()
		: transform()
		, soundSource()
		, soundVolume()
		, mp()
		, gain(0.f)
		, index(-1)
	{
	}
	
	void init(
		const binaural::HRIRSampleSet * sampleSet,
		binaural::Mutex * mutex,
		const int _index, const char * audio, const char * video, const float _gain)
	{
		soundVolume.audioSource.init(sampleSet, mutex, &soundSource);
		
		//
		
		index = _index;
		
		soundSource.open(audio, true);
		
		mp.openAsync(video, MP::kOutputMode_RGBA);
		
		gain = _gain;
	}
	
	float intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p) const
	{
		auto & soundToWorld = soundVolume.transform;
		const Mat4x4 worldToSound = soundToWorld.CalcInv();
	
		const Vec3 pos_sound = worldToSound.Mul4(pos);
		const Vec3 dir_sound = worldToSound.Mul3(dir);
	
		const float d = pos_sound[2];
		const float dd = dir_sound[2];
	
		const float t = - d / dd;
		
		p = pos_sound + dir_sound * t;
		
		return t;
	}
	
	bool isInside(Vec3Arg pos_sound) const
	{
		return
			pos_sound[0] >= -1.f && pos_sound[0] <= +1.f &&
			pos_sound[1] >= -1.f && pos_sound[1] <= +1.f;
	}
	
	void evalVideoClipParams(const EvalMode mode, Vec3 & scale, Quat & rotation, Vec3 & position, float & opacity) const
	{
		const float t = (index + .5f) / float(NUM_VIDEOCLIPS) - .5f;
		
		switch (mode)
		{
		case EvalMode_Floating:
			{
				const float time = timeSeed + framework.time;
		
				const float moveSpeed = (1.f + index / float(NUM_VIDEOCLIPS)) * .2f;
				const float moveAmount = 4.f / (index / float(NUM_VIDEOCLIPS) + 1);
				const float x = std::sin(moveSpeed * time / 11.234f) * moveAmount;
				const float y = std::sin(moveSpeed * time / 13.456f) * moveAmount;
				const float z = std::sin(moveSpeed * time / 15.678f) * moveAmount;
			
				const float scaleSpeed = 1.f + index / 5.f;
				const float scaleY = lerp(.5f, 1.f, (std::cos(scaleSpeed * time / 4.567f) + 1.f) / 2.f);
				const float scaleX = scaleY * 4.f/3.f;
				//const float scaleZ = lerp(.05f, .5f, (std::cos(scaleSpeed * time / 8.765f) + 1.f) / 2.f);
				const float scaleZ = lerp(.05f, .1f, (std::cos(scaleSpeed * time / 8.765f) + 1.f) / 2.f);
			
				const float rotateSpeed = 1.f + index / 5.f;
				
				const Mat4x4 transform = Mat4x4(true).RotateX(rotateSpeed * time / 3.456f).RotateY(rotateSpeed * time / 4.567f);
				
				scale = Vec3(scaleX, scaleY, scaleZ);
				rotation.fromMatrix(transform);
				position = Vec3(x, y, z);
				opacity = 1.f;
			}
			break;
			
		case EvalMode_Line:
			{
				scale = Vec3(1, 1, 1);
				rotation.makeIdentity();
				const float x = t * 4.f;
				const float y = 0.f;
				position = Vec3(x, y, 0);
				opacity = 1.f;
			}
			break;
			
		case EvalMode_Circle:
			{
				scale = Vec3(1, 1, 1);
				rotation.makeIdentity();
				const float x = std::cos(t * M_PI * 2.f) * 4.f;
				const float y = std::sin(t * M_PI * 2.f) * 4.f;
				position = Vec3(x, y, 0);
				opacity = 1.f;
			}
			break;
		}
	}
	
	void tick(const float dt)
	{
		Vec3 totalScale(0.f, 0.f, 0.f);
		Quat totalRotation(0.f, 0.f, 0.f, 0.f);
		Vec3 totalTranslation(0.f, 0.f, 0.f);
		float totalOpacity = 0.f;
		float totalBlend = 0.f;
		
		for (int i = 0; i < 3; ++i) // todo : count
		{
			const EvalMode evalMode = (EvalMode)i;
			const float blend = videoClipBlend[i];
			
			Vec3 scale;
			Quat rotation;
			Vec3 position;
			float opacity;
			evalVideoClipParams(evalMode, scale, rotation, position, opacity);
			
			totalScale += scale * blend;
			totalRotation += rotation * blend;
			totalTranslation += position * blend;
			totalOpacity += opacity * blend;
			
			totalBlend += blend;
		}
		
		if (totalBlend != 0.f)
		{
			totalScale /= totalBlend;
			totalRotation /= totalBlend;
			totalTranslation /= totalBlend;
			totalOpacity /= totalBlend;
		}
		
		transform = Mat4x4(true).
			Translate(totalTranslation).
			Rotate(totalRotation).
			Scale(totalScale);
	
		// update media player
		
		const double newPresentTime = soundSource.samplePosition / float(soundSource.sampleRate);
		
		if (newPresentTime < mp.presentTime)
		{
			auto openParams = mp.context->openParams;
			
			mp.close(false);
			
			mp.openAsync(openParams);
		}
		
		mp.presentTime = newPresentTime;
		
		mp.tick(mp.context, true);
	}
	
	void drawSolid(const bool hover)
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			gxSetTexture(mp.getTexture());
			{
				setLumi(hover ? 255 : 200);
				drawRect(-1, -1, +1, +1);
			}
			gxSetTexture(0);
		}
		gxPopMatrix();
	}
	
	void drawTranslucent()
	{
	#if DRAW_GRIDS
		setColor(200, 200, 200, 63);
		drawSoundVolume(soundVolume);
	#endif
	}
};

struct Vfxclip
{
	SoundVolume soundVolume;
	VfxGraph * vfxGraph;
	
	Vfxclip()
		: soundVolume()
		, vfxGraph(nullptr)
	{
	}
	
	~Vfxclip()
	{
		delete vfxGraph;
		vfxGraph = nullptr;
	}
	
	void open(const char * filename)
	{
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		createVfxTypeDefinitionLibrary(typeDefinitionLibrary);
		
		Graph graph;
		
		if (graph.load(filename, &typeDefinitionLibrary))
		{
			vfxGraph = constructVfxGraph(graph, &typeDefinitionLibrary);
		}
	}
	
	float intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p) const
	{
		auto & soundToWorld = soundVolume.transform;
		const Mat4x4 worldToSound = soundToWorld.CalcInv();
	
		const Vec3 pos_sound = worldToSound.Mul4(pos);
		const Vec3 dir_sound = worldToSound.Mul3(dir);
	
		const float d = pos_sound[2];
		const float dd = dir_sound[2];
	
		const float t = - d / dd;
		
		p = pos_sound + dir_sound * t;
		
		return t;
	}
	
	bool isInside(Vec3Arg pos_sound) const
	{
		return
			pos_sound[0] >= -1.f && pos_sound[0] <= +1.f &&
			pos_sound[1] >= -1.f && pos_sound[1] <= +1.f;
	}
	
	void tick(const float dt)
	{
		vfxGraph->tick(1024, 1024, dt);
		
		vfxGraph->traverseDraw(1024, 1024);
	}
	
	void drawSolid(const bool hover)
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			const VfxNodeDisplay * displayNode = vfxGraph->getMainDisplayNode();
			
			const GLuint texture = displayNode ? displayNode->getImage()->getTexture() : 0;
			
			gxSetTexture(texture);
			{
				setLumi(255);
				drawRect(-1, -1, +1, +1);
			}
			gxSetTexture(0);
		}
		gxPopMatrix();
	}
	
	void drawTranslucent()
	{
	#if DRAW_GRIDS
		setColor(0, 0, 0, 100);
		drawSoundVolume(soundVolume);
	#endif
	}
};

#if DO_SPOKENWORD

#include "FileStream.h"
#include "StreamReader.h"
#include "StringEx.h"
#include "textScroller.h"

struct SpokenWord
{
	TextScroller textScroller;
	
	AudioSourceVorbis soundSource;
	
	SpokenWord()
		: textScroller()
		, soundSource()
	{
	}
	
	void open(const char * text, const char * audio)
	{
		textScroller.open(text);
		
		soundSource.open(audio, false);
	}
	
	void tick(const float dt)
	{
		textScroller.progress += dt / 530.f;
	}
	
	void draw()
	{
		textScroller.draw();
	}
};

#endif

struct World
{
	Camera3d camera;
	
	Videoclip videoclips[NUM_VIDEOCLIPS];
	
	Vfxclip vfxclips[NUM_VFXCLIPS];
	
	void init(binaural::HRIRSampleSet * sampleSet, binaural::Mutex * mutex)
	{
		camera.gamepadIndex = 0;
		
		//const float kMoveSpeed = .2f;
		const float kMoveSpeed = 1.f;
		camera.maxForwardSpeed *= kMoveSpeed;
		camera.maxUpSpeed *= kMoveSpeed;
		camera.maxStrafeSpeed *= kMoveSpeed;
		
		camera.position[0] = 0;
		camera.position[1] = +.3f;
		camera.position[2] = -1.f;
		camera.pitch = 10.f;
		
		//
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			const int index = i % NUM_VIDEOCLIP_SOURCES;
			
			const char * audioFilename = audioFilenames[index];
			const char * videoFilename = videoFilenames[index];
			const float audioGain = audioGains[index];
			
			videoclips[i].init(sampleSet, mutex, i, audioFilename, videoFilename, audioGain);
		}
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			vfxclips[i].open("groooplogo.xml");
		}
	}
	
	int hitTest(Vec3Arg pos, Vec3Arg dir) const
	{
		int result = -1;
		float bestDistance = std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			Vec3 p;
			
			const float t = videoclips[i].intersectRayWithPlane(pos, dir, p);
			
			if (t >= 0.f)
			{
				if (videoclips[i].isInside(p))
				{
					//const float distance = pos_sound.CalcSize();
					const float distance = p.CalcSize(); // fixme : this is skewed by scale
					
					if (distance < bestDistance)
					{
						bestDistance = distance;
						result = i;
					}
				}
			}
		}
		
		return result;
	}
	
	void tick(const float dt)
	{
		// update the camera
		
		const bool doCamera = !(keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT));
		
		camera.tick(dt, doCamera);
		
		// update video clips
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			videoclips[i].tick(dt);
		}
		
		// update vfx clips
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			vfxclips[i].tick(dt);
		}
	}
	
	void draw(const Camera3d & camera)
	{
		const Vec3 origin = camera.getWorldMatrix().GetTranslation();
		const Vec3 direction = camera.getWorldMatrix().GetAxis(2);
		
		const int hoverIndex = hitTest(origin, direction);
		
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		{
			pushBlend(BLEND_OPAQUE);
			{
				for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
				{
					const bool hover = (i == hoverIndex);
					
					videoclips[i].drawSolid(hover);
				}
				
				for (int i = 0; i < NUM_VFXCLIPS; ++i)
				{
					vfxclips[i].drawSolid(false);
				}
			}
			popBlend();
		}
		glDisable(GL_DEPTH_TEST);
		
		glEnable(GL_DEPTH_TEST);
		{
			gxPushMatrix();
			{
				gxScalef(10, 10, 10);
				setColor(200, 200, 200, 60);
				drawGrid3dLine(100, 100, 0, 2, true);
			}
			gxPopMatrix();
			
			for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
			{
				videoclips[i].drawTranslucent();
			}
		
			for (int i = 0; i < NUM_VFXCLIPS; ++i)
			{
				vfxclips[i].drawTranslucent();
			}
		}
		glDisable(GL_DEPTH_TEST);
	}
};

#if DO_CONTROLWINDOW

struct ControlWindow
{
	Window window;
	
	World & world;
	
	ControlWindow(World & _world)
		: window("Control", 300, 300, false)
		, world(_world)
	{
		window.setPosition(0, 200);
	}
	
	void tick()
	{
		pushWindow(window);
		{
		}
		popWindow();
	}
	
	void draw()
	{
		pushWindow(window);
		{
			framework.beginDraw(255, 0, 0, 0);
			{
				gxPushMatrix();
				{
					gxTranslatef(window.getWidth()/2, window.getHeight()/2, 0);
					gxScalef(10, 10, 10);
					
					hqBegin(HQ_FILLED_CIRCLES, true);
					{
						for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
						{
							auto & videoclip = world.videoclips[i];
							auto pos = videoclip.soundVolume.transform.GetTranslation();
							
							setColor(colorWhite);
							hqFillCircle(pos[0], pos[2], 5.f);
						}
					}
					hqEnd();
				}
				gxPopMatrix();
			}
			framework.endDraw();
		}
		popWindow();
	}
};

#endif

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		const std::string filename = args.getString("file", "");
		
		const GLuint texture = getTexture(filename.c_str());
		
		if (texture != 0)
		{
		
		}
	}
}

int main(int argc, char * argv[])
{
	framework.actionHandler = handleAction;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	const int kFontSize = 16;
	const int kFontSize2 = 10;
	
	bool enableNearest = true;
	bool enableVertices = true;
	
	bool showUi = false;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	MyMutex binauralMutex(audioMutex);
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("subject147", sampleSet);
	sampleSet.finalize();
	
	//
	
	World world;
	
	world.init(&sampleSet, &binauralMutex);
	
#if DO_SPOKENWORD
	SpokenWord spokenWord;
	
	// 8:49 ~= 530 seconds
	spokenWord.open("wiekspreekt.txt", "wiekspreekt.ogg");
#endif
	
	MyPortAudioHandler * paHandler = new MyPortAudioHandler();
	
	paHandler->init(audioMutex);
	
	for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
	{
		auto & videoClip = world.videoclips[i];
		
		paHandler->addVolumeSource(&videoClip.soundVolume.audioSource);
	}
	
#if DO_SPOKENWORD
	paHandler->addPointSource(&spokenWord.soundSource);
#endif

	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, paHandler);
	
#if DO_CONTROLWINDOW
	ControlWindow controlWindow(world);
#endif
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;
		
		// process input
		
		if (keyboard.wentDown(SDLK_n))
			enableNearest = !enableNearest;
		
		if (keyboard.wentDown(SDLK_v))
			enableVertices = !enableVertices;
		
		if (keyboard.wentDown(SDLK_TAB))
			showUi = !showUi;
		
	#if 0
		videoClipBlend[0] = (1.f + std::cos(framework.time / 3.4f)) / 2.f;
		videoClipBlend[1] = (1.f + std::cos(framework.time / 4.56f)) / 2.f;
		videoClipBlend[2] = (1.f + std::cos(framework.time / 5.67f)) / 2.f;
	#elif 1
		videoClipBlend[0] = mouse.x / float(GFX_SX);
		videoClipBlend[1] = mouse.y / float(GFX_SY);
		videoClipBlend[2] = 1.f - videoClipBlend[0] - videoClipBlend[1];
	#endif
	
		// update video clips
		
		world.tick(dt);
		
		// update sound volume properties
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			auto & soundVolume = world.videoclips[i].soundVolume;
			
			soundVolume.transform = world.videoclips[i].transform;
		}
		
		// gather HRTF sampling points
		
		const Vec3 cameraPosition_world = world.camera.getWorldMatrix().GetTranslation();
		//const Vec3 soundPosition_world = world.videoclips[0].soundVolume.transform.GetTranslation();
		//const Vec3 soundPosition_view = camera.getViewMatrix().Mul4(pSoundWorld);
		const Mat4x4 worldToViewMatrix = world.camera.getViewMatrix();
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			auto & self = world.videoclips[i];
			
			self.soundVolume.generateSampleLocations(worldToViewMatrix, cameraPosition_world, enableNearest, enableVertices, self.gain);
		}
		
	#if DO_SPOKENWORD
		spokenWord.tick(dt);
	#endif
		
	#if DO_CONTROLWINDOW
		controlWindow.tick();
		
		controlWindow.draw();
	#endif
	
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(fov, near, far);
			
			world.camera.pushViewMatrix();
			{
				world.draw(world.camera);
			}
			world.camera.popViewMatrix();
			
			projectScreen2d();
			
		#if DO_SPOKENWORD
			spokenWord.draw();
		#endif
			
			if (showUi)
			{
				gxTranslatef(0, GFX_SY - 100, 0);
				setColor(colorWhite);
				drawText(10, 40, kFontSize, +1, +1, "N: toggle use nearest point (%s)", enableNearest ? "on" : "off");
				drawText(10, 60, kFontSize, +1, +1, "V: toggle use vertices (%s)", enableVertices ? "on" : "off");
			}
			
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	paHandler->shut();
	delete paHandler;
	paHandler = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
