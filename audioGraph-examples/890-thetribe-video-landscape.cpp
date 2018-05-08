#include "890-performance.h"
#include "audioGraph.h"
#include "Noise.h"
#include "objects/audioSourceVorbis.h"
#include "Path.h"
#include "soundVolume.h"
#include "textScroller.h"
#include "vectorLines.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "video.h"
#include <cmath>

#define NUM_AUDIOCLIP_SOURCES 16
#define NUM_VIDEOCLIP_SOURCES 3
#define NUM_VIDEOCLIPS 32
#define NUM_VFXCLIPS 0
#define NUM_INTERVIEW_SOURCES 6

#define NUM_SPOKENWORD_SOURCES 3

#define ENABLE_INTERACTIVITY 0

#define PARTICLE_SCALE (GFX_SX / 1024.f)

static const float timeSeed = 1234.f;

static const char * audioFilenames[NUM_AUDIOCLIP_SOURCES] =
{
    "welcome/01 Welcome Intro alleeeen zang loop.ogg",
    "welcome/02 Welcome Intro zonder zang loop.ogg",
    "welcome/03 Welcome couplet 1 alleeen zang loop.ogg",
    "welcome/04 Welcome couplet 1 zonder zang loop.ogg",
    "welcome/05 Welcome couplet 2 alleen zang loop.ogg",
    "welcome/06 Welcome couplet 2 zonder zang loop.ogg",
    "welcome/07 Welcome refrein 1 alleen zang loop.ogg",
    "welcome/08 Welcome refrein 1 zonder zang loop.ogg",
    "welcome/09 Welcome brug alleeeen zang loop.ogg",
    "welcome/10 Welcome brug zonder zang loop.ogg",
    "welcome/11 Welcome 2e refrein alleeeen zang loop.ogg",
    "welcome/12 Welcome 2e refrein zonder zang loop.ogg",
    "welcome/13 Welcome Rap Gitaar lick loop.ogg",
    "welcome/14 Welcome Rap Alles loop.ogg",
    "welcome/15 Welcome Refrein 3 alleeen zang loop.ogg",
    "welcome/16 Welcome Refrein 3 zonder zang loop.ogg"
};

static const char * videoFilenames[NUM_VIDEOCLIP_SOURCES] =
{
	"0.640px.mp4",
	"1.640px.mp4",
	"2.640px.mp4",
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

static const char * interviewFilenames[NUM_INTERVIEW_SOURCES] =
{
	"interviews/Daan-small.mp4",
	"interviews/Jasmin-small.mp4",
	"interviews/Jur-small.mp4",
	"interviews/Lisa-small.mp4",
	"interviews/Roos-small.mp4",
	"interviews/Wiek-small.mp4"
};

//

static MediaPlayer mediaPlayers[NUM_VIDEOCLIP_SOURCES];

static VectorMemory vectorMemory;

static VectorParticleSystem vectorParticleSystem;

//

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

static double s_videoClipSpeedMultiplier = 1.0;
static bool s_videoClipDrawGrid = true;
static Color s_videoClipGridColor(1.f, 1.f, 1.f, 1.f);
static Color s_videoClipGridColor2(1.f, 1.f, 1.f, 1.f);
static float s_videoAspect = 1.f;
static float s_videoScale = 1.f;
static float s_videoSpacing = 200.f;
static float s_videoDistance = 400.f;
static float s_videoNearDistance = 200.f;
static float s_videoAngleWobbleX = 0.f;
static float s_videoAngleWobbleY = 0.f;
static float s_videoSaturation = 1.f;
static float s_videoPerlinScale = 1.f;
static float s_videoPerlinThreshold = 0.f;
static float s_wobbleSpeedX = 1.f;
static float s_wobbleSpeedY = 1.f;
static float s_videoRetain = 0.f;
static float s_particlesLineOpacity = 0.f;
static float s_particlesCoronaOpacity = 0.f;

struct Videoclip
{
	Mat4x4 transform;
#if ENABLE_WELCOME
#if USE_STREAMING
	AudioSourceVorbis soundSource;
#else
	AudioSourcePcm soundSource;
#endif
#else
	AudioSourceMix soundSource;
#endif
	SoundVolume soundVolume;
	float gain;
	
	int index;
	
	bool hover;
	
	bool isPaused;
	
	bool isPlaying;
	
	double time;
	
	Videoclip()
		: transform()
		, soundSource()
		, soundVolume()
		, gain(0.f)
		, index(-1)
		, hover(false)
		, isPaused(true)
		, isPlaying(false)
		, time(timeSeed)
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
	
	#if ENABLE_WELCOME
	#if USE_STREAMING
		soundSource.open(audio, true);
	#else
		const PcmData * pcmData = getPcmData(audio);
		
		soundSource.init(pcmData, 0);
		soundSource.play();
		soundSource.loop = true;
	#endif
	#endif
		
		gain = _gain;
	}
	
	void shut()
	{
		if (isPlaying)
		{
			g_audioMixer->removeVolumeSource(&soundVolume.audioSource);
			isPlaying = false;
		}
	}
	
	bool intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p, float & t) const
	{
		return intersectSoundVolume(soundVolume, pos, dir, p, t);
	}
	
	void evalVideoClipParams(Vec3 & scale, Quat & rotation, Vec3 & position, float & opacity) const
	{
        const float moveSpeed = (1.f + index / float(NUM_VIDEOCLIPS)) * .1f;
        const float moveAmount = 1.f;
        const float x = std::sin(moveSpeed * time / 15.678f) * moveAmount * .2f - index * 2.7f - 8.f;
        const float y = std::sin(moveSpeed * time / 13.456f) * moveAmount;
        const float z = std::sin(moveSpeed * time / 11.234f) * moveAmount;
    
        const float scaleSpeed = 1.f + index / 5.f;
        const float scaleY = lerp(.5f, 1.f, (std::cos(scaleSpeed * time / 4.567f) + 1.f) / 2.f);
        const float scaleX = scaleY * 4.f/3.f;
        //const float scaleZ = lerp(.05f, .5f, (std::cos(scaleSpeed * time / 8.765f) + 1.f) / 2.f);
        const float scaleZ = lerp(.05f, .1f, (std::cos(scaleSpeed * time / 8.765f) + 1.f) / 2.f);
        const float rotateSpeed = 1.f + index / 10.f;
		
        scale = Vec3(scaleX, scaleY, scaleZ);
        rotation =
            Quat(Vec3(0.f, 0.f, 1.f), rotateSpeed * time / 3.456f) *
            Quat(Vec3(0.f, 1.f, 0.f), rotateSpeed * time / 4.567f);
        position = Vec3(x, y, z);
        opacity = 1.f;
	}
	
	void tick(const Mat4x4 & worldToViewMatrix, const Vec3 & cameraPosition_world, const float dt)
	{
		if (isPaused)
		{
			if (isPlaying)
			{
				g_audioMixer->removeVolumeSource(&soundVolume.audioSource);
				isPlaying = false;
			}
		}
		else
		{
			if (!isPlaying)
			{
				g_audioMixer->addVolumeSource(&soundVolume.audioSource);
				isPlaying = true;
			}
		}
		
		time += dt * s_videoClipSpeedMultiplier;
		
        Vec3 scale;
        Quat rotation;
        Vec3 position;
        float opacity;
        evalVideoClipParams(scale, rotation, position, opacity);

        transform = Mat4x4(true).
			Translate(position).
			Rotate(rotation).
			Scale(scale);
		
		// update sample locations for binauralization given the new transform
		
		soundVolume.transform = transform;
		
		const bool enableNearest = true;
		const bool enableVertices = true;
		soundVolume.generateSampleLocations(worldToViewMatrix, cameraPosition_world, enableNearest, enableVertices, gain);
	}
	
	void drawSolid()
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			const GLuint texture = mediaPlayers[index % NUM_VIDEOCLIP_SOURCES].getTexture();
			const float perlin1 = scaled_octave_noise_1d(8, .5f, s_videoPerlinScale, 0.f, 1.f, index);
			const float perlin2 = s_videoPerlinThreshold;
			
			Shader shader("videoclip");
			setShader(shader);
			shader.setTexture("image", 0, texture);
			shader.setImmediate("saturation", s_videoSaturation);
			shader.setImmediate("perlin", perlin1, perlin2);
			gxSetTexture(texture);
			{
				setLumi(hover ? 255 : 200);
				drawRect(-1, -1, +1, +1);
			}
			gxSetTexture(0);
			clearShader();
		}
		gxPopMatrix();
	}
	
	void drawTranslucent()
	{
		if (s_videoClipDrawGrid)
		{
			setColor(s_videoClipGridColor2);
			drawSoundVolume(soundVolume);
		}
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
		if (s_videoClipDrawGrid)
		{
			setColor(s_videoClipGridColor2);
			drawSoundVolume(soundVolume);
		}
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
		const char * text,
		const char * textFile, const char * audioFile,
		Vec3Arg _pos)
	{
		if (text != nullptr)
			textScroller.initFromText(text);
		else
			textScroller.initFromFile(textFile);
		
		if (audioFile != nullptr)
		{
			soundSource.open(audioFile, false);
		}
		
		//g_audioMixer->addPointSource(&soundSource);
	
		soundVolume.init(sampleSet, mutex, &soundSource);
		g_audioMixer->addVolumeSource(&soundVolume.audioSource);
		
		pos = _pos;
	}
	
	void shut()
	{
		g_audioMixer->removeVolumeSource(&soundVolume.audioSource);
	}
	
	bool intersectRayWithPlane(Vec3Arg pos, Vec3Arg dir, Vec3 & p, float & t) const
	{
		return intersectSoundVolume(soundVolume, pos, dir, p, t);
	}
	
	void toActive()
	{
		soundVolume.audioSource.mutex->lock();
		{
			const std::string filename = soundSource.filename;
			soundSource.open(filename.c_str(), false);
		}
		soundVolume.audioSource.mutex->unlock();

		state = kState_Active;
	}
	
	void toInactive()
	{
		state = kState_Inactive;
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
			
		#if ENABLE_INTERACTIVITY
			if (hover && (mouse.wentDown(BUTTON_LEFT) || gamepad[0].wentDown(GAMEPAD_A)))
			{
				toActive();
				break;
			}
		#endif
			break;
		
		case kState_Active:
			desiredColor = Color(255, 0, 0);
			desiredGain = 1.f;
			desiredScale = 1.f;
			desiredAngle = 180.f;
			
			if (soundSource.hasEnded)
			{
				toInactive();
				break;
			}
			
		#if ENABLE_INTERACTIVITY
			if (hover && (mouse.wentDown(BUTTON_LEFT) || gamepad[0].wentDown(GAMEPAD_A)))
			{
				toInactive();
				break;
			}
		#endif
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
	
	void drawSolid() const
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			setColor(colorWhite);
			drawCircle(0, 0, 1, 200);
		}
		gxPopMatrix();
	}
	
	void drawTranslucent() const
	{
		Color color = currentColor;
		if (hover)
			color = color.interp(colorWhite, .5f);
		
		setColor(color);
		drawSoundVolume(soundVolume);
	}
	
	void draw2d()
	{
		textScroller.draw();
	}
};

struct VfxNodeDrawGrid : VfxNodeBase
{
	enum Input
	{
		kInput_Draw,
		kInput_Scale1,
		kInput_Scale2,
		kInput_NumSegments1,
		kInput_NumSegments2,
		kInput_Axis1,
		kInput_Axis2,
		kInput_Color,
		kInput_Optimized,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Draw,
		kOutput_COUNT
	};
	
	VfxNodeDrawGrid()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		
		addInput(kInput_Draw, kVfxPlugType_Draw);
		addInput(kInput_Scale1, kVfxPlugType_Float);
		addInput(kInput_Scale2, kVfxPlugType_Float);
		addInput(kInput_NumSegments1, kVfxPlugType_Float);
		addInput(kInput_NumSegments2, kVfxPlugType_Float);
		addInput(kInput_Axis1, kVfxPlugType_Int);
		addInput(kInput_Axis2, kVfxPlugType_Int);
		addInput(kInput_Color, kVfxPlugType_Color);
		addInput(kInput_Optimized, kVfxPlugType_Bool);
		addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
	}
	
	virtual void draw() const override
	{
		const VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
		
		const float scale1 = getInputFloat(kInput_Scale1, 1.f);
		const float scale2 = getInputFloat(kInput_Scale1, 1.f);
		const int numSegments1 = getInputFloat(kInput_NumSegments1, 10);
		const int numSegments2 = getInputFloat(kInput_NumSegments2, 10);
		const int axis1 = clamp(getInputInt(kInput_Axis1, 0), 0, 2);
		const int axis2 = clamp(getInputInt(kInput_Axis2, 2), 0, 2);
		const VfxColor * color = getInputColor(kInput_Color, &defaultColor);
		const bool optimized = getInputBool(kInput_Optimized, true);
		
		float scale[3] = { 1.f, 1.f, 1.f };
		scale[axis1] = scale1;
		scale[axis2] = scale2;
		
		gxPushMatrix();
		{
			gxScalef(scale[0], scale[1], scale[2]);
			setColorf(color->r, color->g, color->b, color->a);
			drawGrid3dLine(numSegments1, numSegments2, axis1, axis2, optimized);
		}
		gxPopMatrix();
	}
};

VFX_NODE_TYPE(VfxNodeDrawGrid)
{
	typeName = "draw.grid";
	
	in("draw", "draw");
	in("scale1", "float", "1");
	in("scale2", "float", "1");
	in("numSegs1", "float", "10");
	in("numSegs2", "float", "10");
	in("axis1", "int", "0");
	in("axis2", "int", "2");
	in("color", "color", "ffffffff");
	in("optimized", "bool", "1");
	out("draw", "draw");
}

//

const char * getRandomFaceTileFilename()
{
	const char * filename = interviewFilenames[rand() % NUM_INTERVIEW_SOURCES];
	//const char * filename = videoFilenames[rand() % NUM_VIDEOCLIP_SOURCES];
	
	return filename;
}

struct FaceTile
{
	Vec3 currentPos;
	Vec3 desiredPos;
	
	Vec3 finalPos;
	
	float wobbleSpeed;
	float wobbleMag;
	
	float wobbleX;
	float wobbleY;
	
	float currentAngleX = 0.f;
	float currentAngleY = 0.f;
	float desiredAngleX = 0.f;
	float desiredAngleY = 0.f;
	
	float aspectRatio;
	
	double time = 0.0;
	
	MediaPlayer mp;
	
	AudioSourceVorbis audioSource;
	
	std::string audioFilename;
	
	void init(const char * videoFilename, const char * _audioFilename)
	{
		mp.openAsync(videoFilename, MP::kOutputMode_RGBA);
		
		audioFilename = _audioFilename;
		
		wobbleSpeed = 1.f;
		//wobbleMag = 50.f;
		wobbleMag = 0.f;
		
		aspectRatio = 1.f;
		
		wobbleX = random(0.f, 1000.f);
		wobbleY = random(0.f, 1000.f);
	}
	
	void shut()
	{
		mp.close(true);
	}
	
	void beginFocus()
	{
		auto openParams = mp.context->openParams;
		
		mp.close(false);
	
		mp.presentTime = 0.0;
	
		mp.openAsync(openParams);
		
		audioSource.open(audioFilename.c_str(), false);
		
		g_audioMixer->addPointSource(&audioSource);
	}
	
	void endFocus()
	{
		g_audioMixer->removePointSource(&audioSource);
		
		audioSource.close();
	}
	
	void tick(const float dt)
	{
		time += dt;
		
		const double retainPerSecond = s_videoRetain;
		const double retain = pow(retainPerSecond, dt);
		
		currentPos = lerp(desiredPos, currentPos, retain);
		
		wobbleX += wobbleSpeed * s_wobbleSpeedX * dt;
		wobbleY += wobbleSpeed * s_wobbleSpeedY * dt;
		
		const Vec3 wobblePos(
			sinf(wobbleX) * wobbleMag,
			sinf(wobbleY) * wobbleMag,
			0.f);
		
		currentAngleX = lerp(desiredAngleX, currentAngleX, retain);
		currentAngleY = lerp(desiredAngleY, currentAngleY, retain);
		
		finalPos = currentPos + wobblePos;
		
		aspectRatio = s_videoAspect;
		
		if (audioSource.isOpen())
		{
			// advance video in sync with audio
			
			mp.presentTime = audioSource.samplePosition / double(audioSource.sampleRate);
			
			mp.tick(mp.context, true);
		}
		else
		{
			// advance video
			
			mp.presentTime += dt;
			
			mp.tick(mp.context, true);
			
			// loop when finished
			
			if (mp.isActive(mp.context) && mp.presentedLastFrame(mp.context))
			{
				auto openParams = mp.context->openParams;
				
				mp.close(false);
				
				mp.presentTime = 0.0;
				
				mp.openAsync(openParams);
			}
		}
	}
};

struct Faces
{
	static const int kGridSx = 5;
	static const int kGridSy = 3;
	
	enum CompositionMode
	{
		kCompositionMode_Grid,
		kCompositionMode_Focus,
		kCompositionMode_COUNT
	};
	
	std::vector<FaceTile*> tiles;
	
	CompositionMode compositionMode = kCompositionMode_Focus;
	
	float opacity = 0.f;
	
	int focusIndex = -1;
	
	double time = 0.0;
	
	float focus = 0.f;
	
	Surface * mask = nullptr;
	
	void init()
	{
		// create mask texture
		
		mask = new Surface(400, 500, false, false, SURFACE_R8);
		
		pushSurface(mask);
		{
			mask->clear();
			
			setColor(colorWhite);
			pushBlend(BLEND_OPAQUE);
			pushColorPost(POST_PREMULTIPLY_RGB_WITH_ALPHA);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(0, 0, mask->getWidth(), mask->getHeight(), 24);
			hqEnd();
			popColorPost();
			popBlend();
		}
		popSurface();
		
		// create tiles
		
		const int numTiles = kGridSx * kGridSy;
		
		for (int i = 0; i < numTiles; ++i)
		{
			FaceTile * tile = new FaceTile();
			
			const char * videoFilename = getRandomFaceTileFilename();
			const std::string audioFilename = Path::ReplaceExtension(videoFilename, "ogg");
			
			tile->init(videoFilename, audioFilename.c_str());
			
			tile->wobbleSpeed = random(.5f, 1.f);
			
			tiles.push_back(tile);
		}
	}
	
	void shut()
	{
		for (auto & tile : tiles)
		{
			tile->shut();
			
			delete tile;
			tile = nullptr;
		}
		
		tiles.clear();
		
		delete mask;
		mask = nullptr;
	}
	
	void beginFocus(const int index)
	{
		if (index == focusIndex)
		{
			return;
		}
		
		if (focusIndex >= 0)
		{
			tiles[focusIndex]->endFocus();
			
			focusIndex = -1;
		}
		
		if (index >= 0 && index < tiles.size())
		{
			focusIndex = index;
		
			tiles[focusIndex]->beginFocus();
			
			compositionMode = kCompositionMode_Focus;
		}
		else
		{
			compositionMode = kCompositionMode_Grid;
		}
	}
	
	void endFocus()
	{
		beginFocus(-1);
	}
	
	static void calcGridProperties(const int index, FaceTile & tile)
	{
		const int tileX = index % kGridSx;
		const int tileY = index / kGridSx;

		const float spacing = s_videoSpacing;

		const float x = lerp(-1.f, +1.f, (tileX + .5f) / float(kGridSx)) * kGridSx * spacing;
		const float y = lerp(-1.f, +1.f, (tileY + .5f) / float(kGridSy)) * kGridSy * spacing * s_videoAspect;

		tile.desiredPos = Vec3(x, y, s_videoDistance);
		tile.desiredAngleX = 0.f;
		tile.desiredAngleY = 0.f;
		
		tile.desiredAngleX += sin(tile.time + index) * s_videoAngleWobbleX;
		tile.desiredAngleY += sin(tile.time + index) * s_videoAngleWobbleY;
	}
	
	static void calcFocusProperties(const int index, FaceTile & tile)
	{
		tile.desiredPos = Vec3(0, 0, s_videoNearDistance);
		tile.desiredAngleX = 0.f;
		tile.desiredAngleY = 180.f;
	}
	
	void tick(const float dt)
	{
		time += dt;
		
	#if 0
		if ((rand() % 1000) == 0)
		{
			const int index = rand() % (kGridSx * kGridSy);
			
			beginFocus(index);
		}
	#endif
		
		// check if focus tile has ended audio playback
		
		if (focusIndex >= 0)
		{
			FaceTile * tile = tiles[focusIndex];
			
			if (!tile->audioSource.isOpen() || tile->audioSource.hasEnded)
			{
				endFocus();
			}
		}
		
		// update tile composition and properties
		
		float desiredFocus = 0.f;
		
		if (compositionMode == kCompositionMode_Grid)
		{
			int index = 0;
			
			for (auto tile : tiles)
			{
				calcGridProperties(index, *tile);
				
				index++;
			}
		}
		else if (compositionMode == kCompositionMode_Focus)
		{
			desiredFocus = 1.f;
			
			int index = 0;
			
			for (auto tile : tiles)
			{
				if (index == focusIndex)
				{
					calcFocusProperties(index, *tile);
				}
				else
				{
					calcGridProperties(index, *tile);
				}
				
				index++;
			}
		}
		else
		{
			Assert(false);
		}
		
		const double retainPerSecond = s_videoRetain;
		const double retain = pow(retainPerSecond, dt);
		
		focus = lerp(desiredFocus, focus, retain);
		
		// update tiles
		
		for (auto tile : tiles)
		{
			tile->tick(dt);
		}
	}
	
	void draw2d()
	{
		projectPerspective3d(90.f, 1.f, 1000.f);
		gxPushMatrix();
		gxScalef(1, -1, 1);
		
		auto sortedTiles = tiles;
		
		std::sort(sortedTiles.begin(), sortedTiles.end(), [](FaceTile * t1, FaceTile * t2) { return t1->currentPos[2] > t2->currentPos[2]; });
		
		for (auto tile : sortedTiles)
		{
			auto texture = tile->mp.getTexture();
			
			if (texture == 0)
				continue;
			
			int sx;
			int sy;
			double duration;
			
			if (!tile->mp.getVideoProperties(sx, sy, duration))
				continue;
		
			Shader tileShader("face-tile");
			setShader(tileShader);
			tileShader.setTexture("image", 0, texture);
			tileShader.setTexture("mask", 1, mask->getTexture());
			tileShader.setImmediate("range", s_videoNearDistance, s_videoDistance);
			tileShader.setImmediate("focus", focus);
			tileShader.setImmediate("opacity", opacity);
			
			gxPushMatrix();
			{
				const float rsy = 768 * s_videoScale;
				const float rsx = rsy / tile->aspectRatio;
				
				gxTranslatef(
					tile->currentPos[0],
					tile->currentPos[1],
					tile->currentPos[2]);
				
				gxScalef(rsx/2.f, rsy/2.f, 1.f);
				
				gxRotatef(tile->currentAngleX, 1, 0, 0);
				gxRotatef(tile->currentAngleY, 0, 1, 0);
				
				setColor(colorWhite);
				drawRect(-1, -1, +1, +1);
			}
			gxPopMatrix();
			
			clearShader();
		}
		
		gxPopMatrix();
		projectScreen2d();
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
	
	float videoclipsOpacity;
	
	Vfxclip vfxclips[NUM_VFXCLIPS];
	
	std::vector<SpokenWord*> spokenWords;
	
	Faces faces;
	
	HitTestResult hitTestResult;
	
	World()
		: camera()
		, videoclips()
		, videoclipsOpacity(0.f)
		, vfxclips()
		, spokenWords()
		, faces()
		, hitTestResult()
	{
		vectorParticleSystem.spawn = [](VectorParticleSystem & ps, const float life)
		{
			const int spawnIndex = ps.nextSpawnIndex();
			
			const float x1 = -100.f;
			const float y1 = -1.f;
			const float z1 = -1.f;
			const float x2 = +0.f;
			const float y2 = +1.f;
			const float z2 = +1.f;

			ps.lt[spawnIndex] = life;
			ps.lr[spawnIndex] = 1.f / life;
			ps.x[spawnIndex] = random(x1, x2);
			ps.y[spawnIndex] = random(y1, y2);
			ps.z[spawnIndex] = random(z1, z2);
			ps.vx[spawnIndex] = random(-1.f, +1.f);
			ps.vy[spawnIndex] = random(-1.f, +2.f);
			ps.vz[spawnIndex] = random(-1.f, +1.f);
		};
	}

	void init(binaural::HRIRSampleSet * sampleSet, binaural::Mutex * mutex)
	{
		for (int i = 0; i < NUM_VIDEOCLIP_SOURCES; ++i)
		{
			const char * videoFilename = videoFilenames[i];
			
			mediaPlayers[i].openAsync(videoFilename, MP::kOutputMode_RGBA);
		}
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
            const int audioIndex = i % NUM_AUDIOCLIP_SOURCES;
            const int videoIndex = i % NUM_VIDEOCLIP_SOURCES;
			
			const char * audioFilename = audioFilenames[audioIndex];
			const char * videoFilename = videoFilenames[videoIndex];
			
			videoclips[i].init(sampleSet, mutex, i, audioFilename, videoFilename, videoclipsOpacity);
		}
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			vfxclips[i].open("groooplogo.xml");
		}
		
		faces.init();
	}
	
	void shut()
	{
		Assert(spokenWords.empty());
		
		faces.shut();
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			videoclips[i].shut();
		}
		
		for (int i = 0; i < NUM_VIDEOCLIP_SOURCES; ++i)
		{
			mediaPlayers[i].close(true);
		}
	}
	
	void beginVideoClips()
	{
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			videoclips[i].isPaused = false;
		}
	}
	
	void pauseVideoClips()
	{
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			videoclips[i].isPaused = true;
		}
	}
	
	SpokenWord * createSpokenWord()
	{
		SpokenWord * spokenWord = new SpokenWord();
		
		spokenWords.push_back(spokenWord);
		
		return spokenWord;
	}
	
	void freeSpokenWord(SpokenWord *& spokenWord)
	{
		if (spokenWord == nullptr)
			return;
		
		auto i = std::find(spokenWords.begin(), spokenWords.end(), spokenWord);
		Assert(i != spokenWords.end());
		
		spokenWords.erase(i);
		
		//
		
		spokenWord->shut();
		
		delete spokenWord;
		spokenWord = nullptr;
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
		
		for (auto spokenWord : spokenWords)
		{
			Vec3 p;
			float t;
			
			if (spokenWord->intersectRayWithPlane(pos, dir, p, t) && t >= 0.f)
			{
				if (t < bestDistance)
				{
					bestDistance = t;
					result = HitTestResult();
					result.spokenWord = spokenWord;
				}
			}
		}
		
		return result;
	}
	
	void tick(const float dt)
	{
		// update the camera
		
	#if ENABLE_INTERACTIVITY
		const bool doCamera = !(keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT));
	#else
		const bool doCamera = false;
	#endif
		
		camera.tick(dt, doCamera);
		
		// pause/resume video clips
		
		const float opacity = clamp(videoclipsOpacity, 0.f, 1.f);
		
		if (opacity <= 1.f / 100.f)
			pauseVideoClips();
		else
			beginVideoClips();
		
		//
		
		camera.pushViewMatrix();
		
		Mat4x4 worldToViewMatrix;
		gxGetMatrixf(GL_MODELVIEW, worldToViewMatrix.m_v);
		
		const Vec3 cameraPosition_world = worldToViewMatrix.CalcInv().GetTranslation();
		
		// update media players
		
		for (int i = 0; i < NUM_VIDEOCLIP_SOURCES; ++i)
		{
			mediaPlayers[i].presentTime += dt;
			
			mediaPlayers[i].tick(mediaPlayers[i].context, true);
			
			if (mediaPlayers[i].presentedLastFrame(mediaPlayers[i].context))
			{
				auto openParams = mediaPlayers[i].context->openParams;
				
				mediaPlayers[i].close(false);
				
				mediaPlayers[i].presentTime = 0.0;
				
				mediaPlayers[i].openAsync(openParams);
			}
		}
		
		// update video clips
		
		for (int i = 0; i < NUM_VIDEOCLIPS; ++i)
		{
			videoclips[i].gain = videoclipsOpacity;
			
			videoclips[i].tick(worldToViewMatrix, cameraPosition_world, dt);
		}
		
		// update vfx clips
		
		for (int i = 0; i < NUM_VFXCLIPS; ++i)
		{
			vfxclips[i].tick(dt);
		}
		
		// update spoken words
		
		for (auto spokenWord : spokenWords)
		{
			spokenWord->tick(worldToViewMatrix, cameraPosition_world, dt);
		}
		
		// update faces
		
		faces.tick(dt);
		
		// updated vector lines
		
		auto previousState = vectorParticleSystem;
		
		vectorParticleSystem.tick(-.5f, dt);
		
		for (int i = (vectorParticleSystem.spawnIndex + 1) % VectorParticleSystem::kMaxParticles; i != previousState.spawnIndex; i = (i + 1) % VectorParticleSystem::kMaxParticles)
		{
			if (previousState.lt[i] <= 0.f || vectorParticleSystem.lt[i] <= 0.f)
				continue;
			
			vectorMemory.addLine(
				previousState.x[i],
				previousState.y[i],
				previousState.z[i],
				vectorParticleSystem.x[i],
				vectorParticleSystem.y[i],
				vectorParticleSystem.z[i], 4.f);
		}
		
		vectorMemory.tick(dt);
		
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
		
		camera.popViewMatrix();
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
				
				for (auto spokenWord : spokenWords)
				{
					spokenWord->drawSolid();
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
				//gxTranslatef(camera.position[0], 0.f, camera.position[2]);
				gxTranslatef(-45.f, 0, 0);
				
				gxScalef(40, 40, 40);
				setColor(s_videoClipGridColor);
				drawGrid3dLine(400, 400, 0, 2, true);
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
			
			for (auto spokenWord : spokenWords)
			{
				spokenWord->drawTranslucent();
			}
			
			pushBlend(BLEND_ALPHA);
			{
				vectorMemory.draw(s_particlesLineOpacity);
			}
			popBlend();
			
			pushBlend(BLEND_ADD);
			{
				setColorf(1.f, 1.f, 1.f, s_particlesCoronaOpacity);
				Shader shader("particles");
				shader.setTexture("source", 0, getTexture("particle.jpg"), true, true);
				shader.setImmediate("pointScale", PARTICLE_SCALE);
				vectorParticleSystem.draw();
				clearShader();
			}
			popBlend();
		}
		glDepthMask(GL_TRUE);
		glDisable(GL_DEPTH_TEST);
		
		camera.popViewMatrix();
	}
	
	void draw2d()
	{
		faces.draw2d();
		
		for (auto spokenWord : spokenWords)
		{
			spokenWord->draw2d();
		}
	}
};

static World * s_world = nullptr;

struct VfxNodeWorld : VfxNodeBase
{
	enum Input
	{
		kInput_CameraX,
		kInput_CameraY,
		kInput_CameraZ,
		kInput_CameraYaw,
		kInput_CameraRoll,
		kInput_WorldOpacity,
		kInput_FacesOpacity,
		kInput_VideoClipTimeMultiplier,
		kInput_VideoClipDrawGrid,
		kInput_VideoClipGridColor,
		kInput_VideoClipGridColor2,
		kInput_VideoBegin,
		kInput_VideoEnd,
		kInput_VideoAspect,
		kInput_VideoScale,
		kInput_VideoSpacing,
		kInput_VideoDistance,
		kInput_VideoNearDistance,
		kInput_VideoAngleWobbleX,
		kInput_VideoAngleWobbleY,
		kInput_VideoWobbleX,
		kInput_VideoWobbleY,
		kInput_VideoRetain,
		kInput_VideoSaturation,
		kInput_VideoNoisePerlinScale,
		kInput_VideoNoisePerlinThreshold,
		kInput_VideoFocus,
		kInput_VideoFocusIndex,
		kInput_ParticleLineOpacity,
		kInput_ParticleCoronaOpacity,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	VfxNodeWorld()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		
		addInput(kInput_CameraX, kVfxPlugType_Float);
		addInput(kInput_CameraY, kVfxPlugType_Float);
		addInput(kInput_CameraZ, kVfxPlugType_Float);
		addInput(kInput_CameraYaw, kVfxPlugType_Float);
		addInput(kInput_CameraRoll, kVfxPlugType_Float);
		addInput(kInput_WorldOpacity, kVfxPlugType_Float);
		addInput(kInput_FacesOpacity, kVfxPlugType_Float);
		addInput(kInput_VideoClipTimeMultiplier, kVfxPlugType_Float);
		addInput(kInput_VideoClipDrawGrid, kVfxPlugType_Bool);
		addInput(kInput_VideoClipGridColor, kVfxPlugType_Color);
		addInput(kInput_VideoClipGridColor2, kVfxPlugType_Color);
		addInput(kInput_VideoBegin, kVfxPlugType_Trigger);
		addInput(kInput_VideoEnd, kVfxPlugType_Trigger);
		addInput(kInput_VideoAspect, kVfxPlugType_Float);
		addInput(kInput_VideoScale, kVfxPlugType_Float);
		addInput(kInput_VideoSpacing, kVfxPlugType_Float);
		addInput(kInput_VideoDistance, kVfxPlugType_Float);
		addInput(kInput_VideoNearDistance, kVfxPlugType_Float);
		addInput(kInput_VideoAngleWobbleX, kVfxPlugType_Float);
		addInput(kInput_VideoAngleWobbleY, kVfxPlugType_Float);
		addInput(kInput_VideoWobbleX, kVfxPlugType_Float);
		addInput(kInput_VideoWobbleY, kVfxPlugType_Float);
		addInput(kInput_VideoRetain, kVfxPlugType_Float);
		addInput(kInput_VideoSaturation, kVfxPlugType_Float);
		addInput(kInput_VideoNoisePerlinScale, kVfxPlugType_Float);
		addInput(kInput_VideoNoisePerlinThreshold, kVfxPlugType_Float);
		addInput(kInput_VideoFocus, kVfxPlugType_Trigger);
		addInput(kInput_VideoFocusIndex, kVfxPlugType_Float);
		addInput(kInput_ParticleLineOpacity, kVfxPlugType_Float);
		addInput(kInput_ParticleCoronaOpacity, kVfxPlugType_Float);
	}
	
	virtual ~VfxNodeWorld() override
	{
	}
	
	virtual void tick(const float dt) override
	{
		s_world->camera.position = Vec3(
			getInputFloat(kInput_CameraX, 0.f),
			getInputFloat(kInput_CameraY, 0.f),
			getInputFloat(kInput_CameraZ, 0.f));
		
		s_world->camera.yaw = getInputFloat(kInput_CameraYaw, 0.f);
		s_world->camera.roll = getInputFloat(kInput_CameraRoll, 0.f);
		
		s_world->videoclipsOpacity = getInputFloat(kInput_WorldOpacity, 0.f);
		s_world->faces.opacity = getInputFloat(kInput_FacesOpacity, 0.f);
		
		s_videoClipSpeedMultiplier = getInputFloat(kInput_VideoClipTimeMultiplier, 1.f);
		s_videoClipDrawGrid = getInputBool(kInput_VideoClipDrawGrid, true);
		
		VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
		
		const VfxColor * gridColor = getInputColor(kInput_VideoClipGridColor, &defaultColor);
		s_videoClipGridColor.r = gridColor->r;
		s_videoClipGridColor.g = gridColor->g;
		s_videoClipGridColor.b = gridColor->b;
		s_videoClipGridColor.a = gridColor->a;
		
		const VfxColor * gridColor2 = getInputColor(kInput_VideoClipGridColor2, &defaultColor);
		s_videoClipGridColor2.r = gridColor2->r;
		s_videoClipGridColor2.g = gridColor2->g;
		s_videoClipGridColor2.b = gridColor2->b;
		s_videoClipGridColor2.a = gridColor2->a;
		
		s_videoAspect = getInputFloat(kInput_VideoAspect, 1.f);
		s_videoScale = getInputFloat(kInput_VideoScale, .2f);
		s_videoSpacing = getInputFloat(kInput_VideoSpacing, 200.f);
		s_videoDistance = getInputFloat(kInput_VideoDistance, 400.f);
		s_videoNearDistance = getInputFloat(kInput_VideoNearDistance, 200.f);
		s_videoAngleWobbleX = getInputFloat(kInput_VideoAngleWobbleX, 0.f);
		s_videoAngleWobbleY = getInputFloat(kInput_VideoAngleWobbleY, 0.f);
		
		s_wobbleSpeedX = getInputFloat(kInput_VideoWobbleX, 1.f);
		s_wobbleSpeedY = getInputFloat(kInput_VideoWobbleY, 1.f);
		
		s_videoSaturation = getInputFloat(kInput_VideoSaturation, 1.f);
		s_videoPerlinScale = getInputFloat(kInput_VideoNoisePerlinScale, 1.f);
		s_videoPerlinThreshold = getInputFloat(kInput_VideoNoisePerlinThreshold, 0.f);
		
		s_videoRetain = getInputFloat(kInput_VideoRetain, 0.f);
		
		s_particlesLineOpacity = getInputFloat(kInput_ParticleLineOpacity, 0.f);
		s_particlesCoronaOpacity = getInputFloat(kInput_ParticleCoronaOpacity, 0.f);
	}
	
	virtual void handleTrigger(const int index) override
	{
		if (index == kInput_VideoBegin)
		{
			s_world->beginVideoClips();
		}
		else if (index == kInput_VideoEnd)
		{
			s_world->pauseVideoClips();
		}
		else if (index == kInput_VideoFocus)
		{
			const int focusIndex = getInputFloat(kInput_VideoFocusIndex, 0.f);
			
			if (focusIndex >= 0 && focusIndex < s_world->faces.tiles.size())
			{
				if (focusIndex == s_world->faces.focusIndex)
					s_world->faces.endFocus();
				else
					s_world->faces.beginFocus(focusIndex);
			}
		}
	}
};

VFX_NODE_TYPE(VfxNodeWorld)
{
	typeName = "world";
	
	in("camera.x", "float");
	in("camera.y", "float");
	in("camera.z", "float");
	in("camera.yaw", "float");
	in("camera.roll", "float");
	in("world.a", "float");
	in("faces.a", "float");
	in("video.tmult", "float", "1");
	in("video.dgrid", "bool", "1");
	in("gridcolor", "color", "ffffffff");
	in("gridcolor2", "color", "ffffffff");
	in("video.begin!", "trigger");
	in("video.end!", "trigger");
	in("video.aspect", "float", "1");
	in("video.scale", "float", "0.2");
	in("video.spacing", "float", "200");
	in("video.fdist", "float", "400");
	in("video.ndist", "float", "200");
	in("video.awob.x", "float");
	in("video.awob.y", "float");
	in("wobble.x", "float", "1");
	in("wobble.y", "float", "1");
	in("video.retain", "float");
	in("video.sat", "float", "1");
	in("video.p.scale", "float", "1");
	in("video.p.thres", "float");
	in("video.focus", "trigger");
	in("video.findex", "float");
	in("p.opacity", "float", "p.line");
	in("p.corona.opacity", "float", "p.corona");
}

//

struct VfxNodeSpokenWord : VfxNodeBase
{
	enum Input
	{
		kInput_Text,
		kInput_Sound,
		kInput_Time,
		kInput_X,
		kInput_Y,
		kInput_Z,
		kInput_Begin,
		kInput_End,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	SpokenWord * spokenWord;
	
	std::string currentText;
	std::string currentSoundFilename;
	float currentTime;
	
	VfxNodeSpokenWord()
		: VfxNodeBase()
		, spokenWord(nullptr)
		, currentText()
		, currentSoundFilename()
		, currentTime(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		
		addInput(kInput_Text, kVfxPlugType_String);
		addInput(kInput_Sound, kVfxPlugType_String);
		addInput(kInput_Time, kVfxPlugType_Float);
		addInput(kInput_X, kVfxPlugType_Float);
		addInput(kInput_Y, kVfxPlugType_Float);
		addInput(kInput_Z, kVfxPlugType_Float);
		addInput(kInput_Begin, kVfxPlugType_Trigger);
		addInput(kInput_End, kVfxPlugType_Trigger);
	}
	
	virtual ~VfxNodeSpokenWord() override
	{
		s_world->freeSpokenWord(spokenWord);
	}
	
	virtual void tick(const float dt) override
	{
		if (isPassthrough)
		{
			s_world->freeSpokenWord(spokenWord);
			
			currentText.clear();
			currentSoundFilename.clear();
			currentTime = 0.f;
			
			return;
		}
		
		const char * text = getInputString(kInput_Text, "");
		const char * soundFilename = getInputString(kInput_Sound, "");
		const float time = getInputFloat(kInput_Time, 0.f);
		const float x = getInputFloat(kInput_X, 0.f);
		const float y = getInputFloat(kInput_Y, 0.f);
		const float z = getInputFloat(kInput_Z, 0.f);
		
		if (spokenWord == nullptr || text != currentText || soundFilename != currentSoundFilename || time != currentTime)
		{
			currentText = text;
			currentSoundFilename = soundFilename;
			currentTime = time;
			
			s_world->freeSpokenWord(spokenWord);
			
			spokenWord = s_world->createSpokenWord();
			
			spokenWord->init(g_sampleSet, g_binauralMutex, text, nullptr, soundFilename, Vec3(0.f, 0.f, 0.f));
		}
		
		spokenWord->pos = Vec3(x, y, z);
	}
	
	virtual void handleTrigger(const int index) override
	{
		if (isPassthrough)
			return;
		
		if (index == kInput_Begin)
		{
			spokenWord->toActive();
		}
		else if (index == kInput_End)
		{
			spokenWord->toInactive();
		}
	}
	
	virtual void draw() const override
	{
		if (isPassthrough)
			return;
		if (spokenWord == nullptr)
			return;
		
		spokenWord->drawSolid();
		
		spokenWord->drawTranslucent();
	}
};

VFX_NODE_TYPE(VfxNodeSpokenWord)
{
	typeName = "spokenWord";
	
	in("text", "string");
	in("sound", "string");
	in("time", "float");
	in("x", "float");
	in("y", "float");
	in("z", "float");
	in("begin!", "trigger");
	in("end!", "trigger");
}

//

struct Starfield
{
	VectorParticleSystem ps;
	
	VectorMemory vm;
	
	float starOpacity = 1.f;
	float lineOpacity = 1.f;
	float speedMultiplier = 1.f;
	
	void init()
	{
		ps.spawn = [&](VectorParticleSystem & ps, const float life)
		{
			const int spawnIndex = ps.nextSpawnIndex();
			
			const float x1 = -1.f;
			const float y1 = -1.f;
			const float z1 = +10.f;
			const float x2 = +1.f;
			const float y2 = +1.f;
			const float z2 = +10.f;
			
			const float speedMultiplier = .2f;
			
			ps.lt[spawnIndex] = life;
			ps.lr[spawnIndex] = 1.f / life;
			ps.x[spawnIndex] = random(x1, x2);
			ps.y[spawnIndex] = random(y1, y2);
			ps.z[spawnIndex] = random(z1, z2);
			ps.vx[spawnIndex] = speedMultiplier * random(-1.f, +1.f);
			ps.vy[spawnIndex] = speedMultiplier * random(-1.f, +1.f);
			ps.vz[spawnIndex] = speedMultiplier * random(-10.f, -10.f);
		};
	}
	
	void shut()
	{
	}
	
	void tick(const float dt)
	{
		auto previousState = ps;
		
		ps.tick(0.f, dt * speedMultiplier);
		
		for (int i = (ps.spawnIndex + 1) % VectorParticleSystem::kMaxParticles; i != previousState.spawnIndex; i = (i + 1) % VectorParticleSystem::kMaxParticles)
		{
			if (previousState.lt[i] <= 0.f || ps.lt[i] <= 0.f)
				continue;
			
			vm.addLine(
				previousState.x[i],
				previousState.y[i],
				previousState.z[i],
				ps.x[i],
				ps.y[i],
				ps.z[i], 2.3f);
		}
		
		vm.tick(dt);
	}
	
	void draw3d()
	{
		draw3dTranslucent();
	}
	
	void draw3dTranslucent()
	{
		pushBlend(BLEND_ALPHA);
		{
			vm.draw(lineOpacity);
		}
		popBlend();
		
		pushBlend(BLEND_ADD);
		{
			setColorf(1.f, 1.f, 1.f, starOpacity);
			Shader shader("particles");
			shader.setTexture("source", 0, getTexture("particle.jpg"), true, true);
			shader.setImmediate("pointScale", PARTICLE_SCALE);
			{
				ps.draw();
			}
			clearShader();
		}
		popBlend();
	}
};

static Starfield * s_starfield = nullptr;

struct VfxNodeStarfield : VfxNodeBase
{
	enum Input
	{
		kInput_StarOpacity,
		kInput_LineOpacity,
		kInput_Speed,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	VfxNodeStarfield()
		: VfxNodeBase()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_StarOpacity, kVfxPlugType_Float);
		addInput(kInput_LineOpacity, kVfxPlugType_Float);
		addInput(kInput_Speed, kVfxPlugType_Float);
	}
	
	virtual void tick(const float dt) override
	{
		s_starfield->starOpacity = getInputFloat(kInput_StarOpacity, 1.f);
		s_starfield->lineOpacity = getInputFloat(kInput_LineOpacity, 1.f);
		s_starfield->speedMultiplier = getInputFloat(kInput_Speed, 1.f);
	}
};

VFX_NODE_TYPE(VfxNodeStarfield)
{
	typeName = "starfield";
	in("star.opacity", "float", "1");
	in("line.opacity", "float", "1");
	in("speed", "float", "1");
}

//

#include "vfxNodeBase.h"

struct VfxNodeScalarSmoothe : VfxNodeBase
{
	enum SmoothingUnit
	{
		kSmoothingUnit_PerSecond,
		kSmoothingUnit_PerMillisecond
	};
	
	enum Input
	{
		kInput_Value,
		kInput_InitialValue,
		kInput_SmoothingUnit,
		kInput_Smoothness,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Result,
		kOutput_COUNT
	};
	
	float resultOutput;
	
	double currentValue;
	
	VfxNodeScalarSmoothe()
		: VfxNodeBase()
		, resultOutput()
		, currentValue(0.0)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Value, kVfxPlugType_Float);
		addInput(kInput_InitialValue, kVfxPlugType_Float);
		addInput(kInput_SmoothingUnit, kVfxPlugType_Int);
		addInput(kInput_Smoothness, kVfxPlugType_Float);
		addOutput(kOutput_Result, kVfxPlugType_Float, &resultOutput);
	}
	
	virtual void init(const GraphNode & node) override
	{
		currentValue = getInputFloat(kInput_InitialValue, 0.f);
	}
	
	virtual void tick(const float dt) override;
};

VFX_ENUM_TYPE(smoothing_mode)
{
	elem("perSecond");
	elem("perMillisecond");
}

VFX_NODE_TYPE(VfxNodeScalarSmoothe)
{
	typeName = "scalar.smoothe";
	
	in("value", "float");
	in("value.init", "float");
	inEnum("smoothing", "smoothing_mode");
	in("smoothness", "float", "0.5");
	out("result", "float");
}

void VfxNodeScalarSmoothe::tick(const float _dt)
{
	vfxCpuTimingBlock(VfxNodeScalarSmoothe);
	
	const float value = getInputFloat(kInput_Value, 0.f);
	const SmoothingUnit smoothingUnit = (SmoothingUnit)getInputInt(kInput_SmoothingUnit, 0);
	const float retain = getInputFloat(kInput_Smoothness, .5f);
	
	const double dt = (smoothingUnit == kSmoothingUnit_PerSecond) ? _dt : _dt * 1000.0;
	
	if (isPassthrough)
	{
		resultOutput = value;
	}
	else
	{
		const double retainPerSecond = fminf(1.f, fmaxf(0.f, retain));
		const double retainPerSample = pow(retainPerSecond, dt);
		const double followPerSample = 1.0 - retainPerSample;
		
		resultOutput = currentValue;
		
		currentValue = currentValue * retainPerSample + value * followPerSample;
	}
}

//

void VideoLandscape::init()
{
	world = new World();
	world->init(g_sampleSet, g_binauralMutex);
    s_world = world;
	
    starfield = new Starfield();
    starfield->init();
    s_starfield = starfield;
}

void VideoLandscape::shut()
{
	starfield->shut();
	delete starfield;
	starfield = nullptr;
	s_starfield = nullptr;
	
	world->shut();
	delete world;
	world = nullptr;
	s_world = nullptr;
}

void VideoLandscape::tick(const float dt)
{
	SDL_LockMutex(g_audioMutex);
	{
		g_audioMixer->voiceGroups[kVoiceGroup_Videoclips].desiredGain =
			keyboard.isDown(SDLK_w) ? .1f : 1.f;
	}
	SDL_UnlockMutex(g_audioMutex);
	
	// update video clips
	
	world->tick(dt);
	
	// update starfield
	
	starfield->tick(dt);
}

void VideoLandscape::draw()
{
	projectPerspective3d(fov, near, far);
	{
		world->draw3d();
	}
	projectScreen2d();

	const float opacity = clamp(world->videoclipsOpacity, 0.f, 1.f);

	setColorf(0.f, 0.f, 0.f, 1.f - opacity);
	drawRect(0, 0, GFX_SX, GFX_SY);
	
	world->draw2d();
	
	projectPerspective3d(fov, near, far);
	{
		starfield->draw3d();
	}
	projectScreen2d();
}
