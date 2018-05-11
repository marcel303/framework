#include "FileStream.h"
#include "StreamReader.h"
#include "StringEx.h"
#include "textScroller.h"

namespace Video360
{
#define NUM_VIDEOCLIP_SOURCES 3
#define NUM_VIDEOCLIPS 3
#define NUM_VFXCLIPS 1

#define NUM_SPOKENWORD_SOURCES 3
#define NUM_SPOKENWORDS 3

#define DRAW_GRIDS 1
#define ENABLE_TRANSFORM_MIXING 0

static bool enableNearest = true;
static bool enableVertices = true;

static const float timeSeed = 1234.f;

static const char * audioFilenames[NUM_VIDEOCLIP_SOURCES] =
{
    "welcome/01 Welcome Intro alleeeen zang loop.ogg",
    "welcome/04 Welcome couplet 1 zonder zang loop.ogg",
    "welcome/08 Welcome refrein 1 zonder zang loop.ogg",
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

static const char * spokenText[NUM_SPOKENWORD_SOURCES] =
{
	"albert-tekst1.txt",
	"albert-tekst2.txt",
	"wiekspreekt.txt"
};

static const char * spokenAudio[NUM_SPOKENWORD_SOURCES] =
{
	"albert-tekst1.ogg",
	"albert-tekst2.ogg",
	"wiekspreekt.ogg" // 8:49 ~= 530 seconds
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

enum VoiceGroup
{
	kVoiceGroup_Videoclips,
	kVoiceGroup_SpokenWord
};

struct VoiceGroupData
{
	float currentGain;
	float desiredGain;
	float gainSpeed;
	
	VoiceGroupData()
		: currentGain(1.f)
		, desiredGain(1.f)
		, gainSpeed(.5f)
	{
	}
	
	void tick(const float dt)
	{
		const float retain = std::pow(1.f - gainSpeed, dt);
		const float falloff = 1.f - retain;
		
		currentGain = retain * currentGain + falloff * desiredGain;
	}
};

struct MyPortAudioHandler : PortAudioHandler
{
	const static int kMaxVoiceGroups = 4;
	
	SDL_mutex * mutex;
	
	VoiceGroupData voiceGroups[kMaxVoiceGroups];
	
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
		
		const float dt = AUDIO_UPDATE_SIZE / float(SAMPLE_RATE);
		
		SDL_LockMutex(mutex);
		{
			for (auto & voiceGroup : voiceGroups)
			{
				voiceGroup.tick(dt);
			}
			
			for (auto volumeSource : volumeSources)
			{
				const float gain = voiceGroups[kVoiceGroup_Videoclips].currentGain;
				
				volumeSource->generate(0, channelL, AUDIO_UPDATE_SIZE, gain);
				volumeSource->generate(1, channelR, AUDIO_UPDATE_SIZE, gain);
			}
			
			for (auto & pointSource : pointSources)
			{
				const float gain = voiceGroups[kVoiceGroup_SpokenWord].currentGain;
				
				ALIGN16 float channel[AUDIO_UPDATE_SIZE];
				
				pointSource->generate(channel, AUDIO_UPDATE_SIZE);
				
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				{
					channelL[i] += channel[i] * gain;
					channelR[i] += channel[i] * gain;
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

static MyPortAudioHandler * s_paHandler = nullptr;

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

static bool intersectSoundVolume(const SoundVolume & soundVolume, Vec3Arg pos, Vec3Arg dir, Vec3 & p, float & t)
{
	auto & soundToWorld = soundVolume.transform;
	const Mat4x4 worldToSound = soundToWorld.CalcInv();

	const Vec3 pos_sound = worldToSound.Mul4(pos);
	const Vec3 dir_sound = worldToSound.Mul3(dir);

	const float d = pos_sound[2];
	const float dd = dir_sound[2];

	t = - d / dd;
	
	p = pos_sound + dir_sound * t;
	
	return
		p[0] >= -1.f && p[0] <= +1.f &&
		p[1] >= -1.f && p[1] <= +1.f;
}

float videoClipBlend[3] =
{
	1.f, 0.f, 0.f
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
	
	bool hover;
	
	Videoclip()
		: transform()
		, soundSource()
		, soundVolume()
		, mp()
		, gain(0.f)
		, index(-1)
		, hover(false)
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
		
		//
		
		s_paHandler->addVolumeSource(&soundVolume.audioSource);
	}
	
	bool intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p, float & t) const
	{
		return intersectSoundVolume(soundVolume, pos, dir, p, t);
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
				
				scale = Vec3(scaleX, scaleY, scaleZ);
				rotation =
					Quat(Vec3(1.f, 0.f, 0.f), rotateSpeed * time / 3.456f) *
					Quat(Vec3(0.f, 1.f, 0.f), rotateSpeed * time / 4.567f);
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
	
	void tick(const Mat4x4 & worldToViewMatrix, const Vec3 & cameraPosition_world, const float dt)
	{
		Vec3 totalScale(0.f, 0.f, 0.f);
		Quat totalRotation;
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
			// todo : what's the best way to blend between rotations ?
			//totalRotation = totalRotation.slerp(rotation, blend);
			totalRotation = totalRotation * rotation;
			totalTranslation += position * blend;
			totalOpacity += opacity * blend;
			
			totalBlend += blend;
		}
		
		if (totalBlend != 0.f)
		{
			totalScale /= totalBlend;
			totalTranslation /= totalBlend;
			totalOpacity /= totalBlend;
		}
		
		transform = Mat4x4(true).
			Translate(totalTranslation).
			Rotate(totalRotation).
			Scale(totalScale);
		
		// update sample locations for binauralization given the new transform
		
		soundVolume.transform = transform;
		
		soundVolume.generateSampleLocations(worldToViewMatrix, cameraPosition_world, enableNearest, enableVertices, gain);
		
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
	
	void drawSolid()
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
	
	bool hover;
	
	Vfxclip()
		: soundVolume()
		, vfxGraph(nullptr)
		, hover(false)
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
	
	bool intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p, float & t) const
	{
		return intersectSoundVolume(soundVolume, pos, dir, p, t);
	}
	
	void tick(const float dt)
	{
		vfxGraph->tick(1024, 1024, dt);
		
		vfxGraph->traverseDraw(1024, 1024);
	}
	
	void drawSolid()
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
			
			if (hover)
			{
				setLumi(255);
				drawRectLine(-1, -1, +1, +1);
			}
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

struct SpokenWord
{
	enum State
	{
		kState_Inactive,
		kState_Active
	};
	
	TextScroller textScroller;
	
	AudioSourceVorbis soundSource;
	SoundVolume soundVolume;
	
	Vec3 pos;
	
	State state;
	
	bool hover;
	
	Color currentColor;
	Color desiredColor;
	
	float currentGain;
	float desiredGain;
	
	float currentScale;
	float desiredScale;
	
	float currentAngle;
	float desiredAngle;
	
	SpokenWord()
		: textScroller()
		, soundSource()
		, soundVolume()
		, state(kState_Inactive)
		, hover(false)
		, currentColor(50, 50, 50)
		, desiredColor(50, 50, 50)
		, currentGain(0.f)
		, desiredGain(0.f)
		, currentScale(0.f)
		, desiredScale(.1f)
		, currentAngle(0.f)
		, desiredAngle(0.f)
	{
	}
	
	void init(
		const binaural::HRIRSampleSet * sampleSet,
		binaural::Mutex * mutex,
		const char * text, const char * audio,
		Vec3Arg _pos)
	{
		textScroller.initFromFile(text);
		
		soundSource.open(audio, false);
		//s_paHandler->addPointSource(&soundSource);
		
		soundVolume.init(sampleSet, mutex, &soundSource);
		s_paHandler->addVolumeSource(&soundVolume.audioSource);
		
		pos = _pos;
	}
	
	bool intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p, float & t) const
	{
		return intersectSoundVolume(soundVolume, pos, dir, p, t);
	}
	
	void tick(const Mat4x4 & worldToViewMatrix, Vec3Arg cameraPosition_world, const float dt)
	{
		//const float y = std::sin(framework.time / 3.45f) * 1.f;
		const float y = 0.f;
		//const float angleY = std::sin(framework.time / 4.56f) * 1.f;
		const float angleY = currentAngle * M_PI / 180.f;
		
		soundVolume.transform = Mat4x4(true)
			.Translate(pos)
			.Translate(0.f, 0.f, y)
			.RotateY(angleY)
			.Scale(currentScale, currentScale, currentScale)
			.Scale(1.f, 1.f, .1f);
		soundVolume.generateSampleLocations(worldToViewMatrix, cameraPosition_world, true, true, currentGain);
		
		textScroller.progress += dt / 530.f;
		
		//
		
		switch (state)
		{
		case kState_Inactive:
			desiredColor = Color(50, 50, 50);
			desiredGain = 0.f;
			desiredScale = .4f;
			desiredAngle = 0.f;
			
			if (hover && (mouse.wentDown(BUTTON_LEFT) || gamepad[0].wentDown(GAMEPAD_A)))
			{
				soundVolume.audioSource.mutex->lock();
				{
					const std::string filename = soundSource.filename;
					soundSource.open(filename.c_str(), false);
				}
				soundVolume.audioSource.mutex->unlock();
				
				state = kState_Active;
				break;
			}
			break;
		
		case kState_Active:
			desiredColor = Color(255, 0, 0);
			desiredGain = 1.f;
			desiredScale = 1.f;
			desiredAngle = 180.f;
			
			if (hover && (mouse.wentDown(BUTTON_LEFT) || gamepad[0].wentDown(GAMEPAD_A)))
			{
				state = kState_Inactive;
				break;
			}
			break;
		}
		
		//
		
		const float speed = .7f;
		const float retain = std::pow(1.f - speed, dt);
		
		currentColor = desiredColor.interp(currentColor, retain);
		currentGain = lerp(desiredGain, currentGain, retain);
		currentScale = lerp(desiredScale, currentScale, retain);
		currentAngle = lerp(desiredAngle, currentAngle, retain);
	}
	
	void drawSolid()
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			setColor(colorWhite);
			drawCircle(0, 0, 1, 200);
		}
		gxPopMatrix();
	}
	
	void drawTranslucent()
	{
		Color color = currentColor;
		if (hover)
			color = color.interp(colorWhite, .5f);
		
		setColor(color);
		drawSoundVolume(soundVolume);
	}
	
	void draw2d()
	{
	#if 0
		textScroller.draw();
	#endif
	}
};

struct World
{
	struct HitTestResult
	{
		Videoclip * videoclip;
		
		Vfxclip * vfxclip;
		
		SpokenWord * spokenWord;
		
		HitTestResult()
			: videoclip(nullptr)
			, vfxclip(nullptr)
			, spokenWord(nullptr)
		{
		}
	};
	
	Camera3d camera;
	
	Videoclip videoclips[NUM_VIDEOCLIPS];
	
	Vfxclip vfxclips[NUM_VFXCLIPS];
	
	SpokenWord spokenWords[NUM_SPOKENWORDS];
	
	HitTestResult hitTestResult;
	
	World()
		: camera()
		, videoclips()
		, vfxclips()
		, spokenWords()
		, hitTestResult()
	{
	}

	void init(binaural::HRIRSampleSet * sampleSet, binaural::Mutex * mutex)
	{
		camera.gamepadIndex = 0;
		
		const float kMoveSpeed = .2f;
		//const float kMoveSpeed = 1.f;
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
		
		for (int i = 0; i < NUM_SPOKENWORDS; ++i)
		{
			auto & spokenWord = spokenWords[i];
			
			const char * textFilename = spokenText[i];
			const char * audioFilename = spokenAudio[i];
			
			const Vec3 p(0.f, 0.f, i + 1.f);
			
			spokenWord.init(sampleSet, mutex, textFilename, audioFilename, p);
		}
	}
	
	HitTestResult hitTest(Vec3Arg pos, Vec3Arg dir)
	{
		HitTestResult result;
		
		float bestDistance = std::numeric_limits<float>::infinity();
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			auto & videoclip = videoclips[i];
			
			Vec3 p;
			float t;
			
			if (videoclip.intersectRayWithPlane(pos, dir, p, t) && t >= 0.f)
			{
				if (t < bestDistance)
				{
					bestDistance = t;
					result = HitTestResult();
					result.videoclip = &videoclip;
				}
			}
		}
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			auto & vfxclip = vfxclips[i];
			
			Vec3 p;
			float t;
			
			if (vfxclip.intersectRayWithPlane(pos, dir, p, t) && t >= 0.f)
			{
				if (t < bestDistance)
				{
					bestDistance = t;
					result = HitTestResult();
					result.vfxclip = &vfxclip;
				}
			}
		}
		
		for (int i = 0; i < NUM_SPOKENWORDS; ++i)
		{
			auto & spokenWord = spokenWords[i];
			
			Vec3 p;
			float t;
			
			if (spokenWord.intersectRayWithPlane(pos, dir, p, t) && t >= 0.f)
			{
				if (t < bestDistance)
				{
					bestDistance = t;
					result = HitTestResult();
					result.spokenWord = &spokenWord;
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
		
		//
		
		const Vec3 cameraPosition_world = camera.getWorldMatrix().GetTranslation();
		const Mat4x4 worldToViewMatrix = camera.getViewMatrix();
		
		// update video clips
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			videoclips[i].tick(worldToViewMatrix, cameraPosition_world, dt);
		}
		
		// update vfx clips
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			vfxclips[i].tick(dt);
		}
		
		for (int i = 0; i < NUM_SPOKENWORDS; ++i)
		{
			spokenWords[i].tick(worldToViewMatrix, cameraPosition_world, dt);
		}
		
		//
		
		if (hitTestResult.videoclip)
			hitTestResult.videoclip->hover = false;
		if (hitTestResult.vfxclip)
			hitTestResult.vfxclip->hover = false;
		if (hitTestResult.spokenWord)
			hitTestResult.spokenWord->hover = false;
		
		const Vec3 origin = camera.getWorldMatrix().GetTranslation();
		const Vec3 direction = camera.getWorldMatrix().GetAxis(2);
		hitTestResult = hitTest(origin, direction);
		
		if (hitTestResult.videoclip)
			hitTestResult.videoclip->hover = true;
		if (hitTestResult.vfxclip)
			hitTestResult.vfxclip->hover = true;
		if (hitTestResult.spokenWord)
			hitTestResult.spokenWord->hover = true;
	}
	
	void draw3d()
	{
		camera.pushViewMatrix();
		
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		{
			pushBlend(BLEND_OPAQUE);
			{
				for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
				{
					videoclips[i].drawSolid();
				}
				
				for (int i = 0; i < NUM_VFXCLIPS; ++i)
				{
					vfxclips[i].drawSolid();
				}
				
				for (int i = 0; i < NUM_SPOKENWORDS; ++i)
				{
					spokenWords[i].drawSolid();
				}
			}
			popBlend();
		}
		glDisable(GL_DEPTH_TEST);
		
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
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
			
			for (int i = 0; i < NUM_SPOKENWORDS; ++i)
			{
				spokenWords[i].drawTranslucent();
			}
		}
		glDepthMask(GL_TRUE);
		glDisable(GL_DEPTH_TEST);
		
		camera.popViewMatrix();
	}
	
	void draw2d()
	{
		for (int i = 0; i < NUM_SPOKENWORDS; ++i)
		{
			spokenWords[i].draw2d();
		}
	}
};

static World * s_world = nullptr;

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

static void drawCubeFace(Vec3Arg side, Vec3Arg up)
{
    Mat4x4 cameraMatrix;
    cameraMatrix.MakeLookat(Vec3(0,0,0), side, up);
    
    float fov = 90.f;
    float near = .01f;
    float far = 100.f;
    
    projectPerspective3d(fov, near, far);
    gxPushMatrix();
    {
        Mat4x4 worldToView = cameraMatrix.CalcInv();
        gxMultMatrixf(worldToView.m_v);
        
        const auto oldPitch = s_world->camera.pitch;
        s_world->camera.pitch = 0.f;
        {
            s_world->draw3d();
        }
        s_world->camera.pitch = oldPitch;
    }
    gxPopMatrix();
    projectScreen2d();
}

static const int kCubeSx = 512*2;
static const int kCubeSy = 512*2;

static void initCube(GLuint & cubemap)
{
    glGenTextures(1, &cubemap);
    checkErrorGL();
    
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    checkErrorGL();
    {
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        checkErrorGL();
        
        glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, kCubeSx, kCubeSy);
        checkErrorGL();
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    checkErrorGL();
}

static void shutCube(GLuint & cubemap)
{
    glDeleteTextures(1, &cubemap);
    checkErrorGL();
    cubemap = 0;
}

static void drawCube()
{
    GLuint cubemap = 0;
    initCube(cubemap);
    
    struct FaceInfo
    {
        GLint id;
        Vec3 side;
        Vec3 up;
    };
    
    FaceInfo faceInfos[6] =
    {
        { GL_TEXTURE_CUBE_MAP_POSITIVE_X, Vec3(-1,0,0), Vec3(0,+1,0) },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_X, Vec3(+1,0,0), Vec3(0,+1,0) },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Y, Vec3(0,+1,0), Vec3(0,0,-1) },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, Vec3(0,-1,0), Vec3(0,0,+1) },
        { GL_TEXTURE_CUBE_MAP_POSITIVE_Z, Vec3(0,0,+1), Vec3(0,+1,0) },
        { GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, Vec3(0,0,-1), Vec3(0,+1,0) }
    };
    
    //glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    
    Surface surface(kCubeSx, kCubeSy, true, false, SURFACE_RGBA8);
    pushSurface(&surface);
    
    for (int i = 0; i < 6; ++i)
    {
        auto & faceInfo = faceInfos[i];
        
        surface.clear();
        surface.clearDepth(1.f);
        
        drawCubeFace(faceInfo.side, faceInfo.up);
        
        //glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
        checkErrorGL();
        glCopyTexSubImage2D(faceInfo.id, 0, 0, 0, 0, 0, kCubeSx, kCubeSy);
        checkErrorGL();
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        checkErrorGL();
    }
    
    popSurface();
    
    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
    {
        setColor(colorWhite);
        Shader shader("cubeproj");
        setShader(shader);
        
        //shader.setTexture("source", 0, cubemap);
        const GLint index = glGetUniformLocation(shader.getProgram(), "source");
        checkErrorGL();
        
        if (index != -1)
        {
            const int unit = 0;
            
            glUniform1i(index, unit);
            glActiveTexture(GL_TEXTURE0 + unit);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
            glActiveTexture(GL_TEXTURE0);
            checkErrorGL();
        }
        
        gxBegin(GL_QUADS);
        {
            drawRect(0, 0, GFX_SX, GFX_SY);
        }
        gxEnd();
        clearShader();
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    
    shutCube(cubemap);
}

void main()
{
	const int kFontSize = 16;
	
	bool showUi = true;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	MyMutex binauralMutex(audioMutex);
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("subject147", sampleSet);
	sampleSet.finalize();
	
	MyPortAudioHandler * paHandler = new MyPortAudioHandler();
	paHandler->init(audioMutex);
	s_paHandler = paHandler;
	
	//
	
	World world;
	
	world.init(&sampleSet, &binauralMutex);
    s_world = &world;
	
	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, paHandler);
	
    Surface preview(GFX_SX/4, GFX_SY/4, true, false, SURFACE_RGBA8);
    
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
		
		SDL_LockMutex(audioMutex);
		{
			paHandler->voiceGroups[kVoiceGroup_Videoclips].desiredGain =
				keyboard.isDown(SDLK_w) ? .1f : 1.f;
		}
		SDL_UnlockMutex(audioMutex);
		
	#if ENABLE_TRANSFORM_MIXING
	#if 0
		videoClipBlend[0] = (1.f + std::cos(framework.time / 3.4f)) / 2.f;
		videoClipBlend[1] = (1.f + std::cos(framework.time / 4.56f)) / 2.f;
		videoClipBlend[2] = (1.f + std::cos(framework.time / 5.67f)) / 2.f;
	#elif 1
		videoClipBlend[0] = mouse.x / float(GFX_SX);
		videoClipBlend[1] = mouse.y / float(GFX_SY);
		videoClipBlend[2] = std::max(0.f, 1.f - videoClipBlend[0] - videoClipBlend[1]);
	#endif
	#endif
	
		// update video clips
		
		world.tick(dt);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
            
			// draw cube map
            
            drawCube();
            
            // draw small preview which is non-cubemapped
            
			pushSurface(&preview);
            {
                preview.clear(255, 255, 255, 0);
                preview.clearDepth(1.f);
                
                float fov = 60.f;
                float near = .01f;
                float far = 100.f;
                
                projectPerspective3d(fov, near, far);
                {
                    world.draw3d();
                }
                projectScreen2d();
            }
            popSurface();
            
            pushBlend(BLEND_OPAQUE);
            gxSetTexture(preview.getTexture());
            {
                setColor(colorWhite);
                //drawRect(10, 10, 10 + preview.getWidth(), 10 + preview.getHeight());
				
				const float x1 = 10;
				const float y1 = 10;
				const float x2 = 10 + preview.getWidth();
				const float y2 = 10 + preview.getHeight();
				
				gxBegin(GL_QUADS);
				{
					gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y1);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y1);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y2);
					gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y2);
				}
				gxEnd();
            }
            gxSetTexture(0);
            popBlend();
            
			world.draw2d();
			
			if (showUi)
			{
				setColor(255, 255, 255, 127);
				drawText(GFX_SX - 10, 40, 32, -1, +1, "360 VIDEO");

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
	
	s_paHandler = nullptr;
	paHandler->shut();
	delete paHandler;
	paHandler = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
}

}
