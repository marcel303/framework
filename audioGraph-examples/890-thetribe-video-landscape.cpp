#include "890-performance.h"
#include "objects/audioSourceVorbis.h"
#include "soundVolume.h"
#include "textScroller.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "video.h"
#include <cmath>

#define NUM_AUDIOCLIP_SOURCES 16
#define NUM_VIDEOCLIP_SOURCES 3
#define NUM_VIDEOCLIPS 32
#define NUM_VFXCLIPS 0

#define NUM_SPOKENWORD_SOURCES 3

#define ENABLE_INTERACTIVITY 0

static bool enableNearest = true;
static bool enableVertices = true;

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
	"0.320px.mp4",
	"1.320px.mp4",
	"2.320px.mp4",
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

static MediaPlayer mediaPlayers[NUM_VIDEOCLIP_SOURCES];

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
static float s_wobbleSpeedX = 1.f;
static float s_wobbleSpeedY = 1.f;
static float s_videoRetain = 0.f;

struct Videoclip
{
	Mat4x4 transform;
	AudioSourceVorbis soundSource;
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
		
		soundSource.open(audio, true);
		
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
		
		soundVolume.generateSampleLocations(worldToViewMatrix, cameraPosition_world, enableNearest, enableVertices, gain);
	}
	
	void drawSolid()
	{
		gxPushMatrix();
		{
			gxMultMatrixf(soundVolume.transform.m_v);
			
			gxSetTexture(mediaPlayers[index % NUM_VIDEOCLIP_SOURCES].getTexture());
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
	
	void init(const char * filename)
	{
		mp.openAsync(filename, MP::kOutputMode_RGBA);
		
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
		
		audioSource.open(spokenAudio[0], false);
		
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
		kCompositionMode_Free,
		kCompositionMode_Grid,
		kCompositionMode_Weave,
		kCompositionMode_Focus,
		kCompositionMode_COUNT
	};
	
	std::vector<FaceTile*> tiles;
	
	CompositionMode compositionMode = kCompositionMode_Focus;
	
	int focusIndex = -1;
	
	double time = 0.0;
	
	Surface * mask = nullptr;
	
	void init()
	{
		// create mask texture
		
		mask = new Surface(512, 512, false, false, SURFACE_RGBA8);
		
		pushSurface(mask);
		{
			mask->clear();
			
			setColor(colorWhite);
			pushBlend(BLEND_OPAQUE);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(0, 0, mask->getWidth(), mask->getHeight(), 20);
			hqEnd();
			popBlend();
		}
		popSurface();
		
		// create tiles
		
		const int numTiles = kGridSx * kGridSy;
		
		for (int i = 0; i < numTiles; ++i)
		{
			FaceTile * tile = new FaceTile();
			
			tile->init(videoFilenames[rand() % NUM_VIDEOCLIP_SOURCES]);
			
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
		tile.desiredAngleX = 180.f;
		tile.desiredAngleY = 0.f;
	}
	
	void tick(const float dt)
	{
		time += dt;
		
		// todo : update tile mode
		
		if ((rand() % 200) == 0)
		{
			//compositionMode = (CompositionMode)(rand() % kCompositionMode_COUNT);
		}
		
		if ((rand() % 200) == 0)
		{
			if (focusIndex >= 0)
			{
				tiles[focusIndex]->endFocus();
			}
			
			focusIndex = rand() % (kGridSx * kGridSy);
			
			tiles[focusIndex]->beginFocus();
		}
		
		// todo : update tile locations
		
		if (compositionMode == kCompositionMode_Free)
		{
		
		}
		else if (compositionMode == kCompositionMode_Grid)
		{
			int index = 0;
			
			for (auto tile : tiles)
			{
				calcGridProperties(index, *tile);
				
				index++;
			}
		}
		else if (compositionMode == kCompositionMode_Weave)
		{
		
		}
		else if (compositionMode == kCompositionMode_Focus)
		{
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
		
		// todo : update tile selection
		
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
			tileShader.setImmediate("opacity", 1.f);
			tileShader.setImmediate("range", s_videoNearDistance, s_videoDistance);
			
			gxPushMatrix();
			{
				const float rsy = GFX_SY * s_videoScale;
				const float rsx = rsy * tile->aspectRatio;
				
				gxTranslatef(
					tile->currentPos[0],
					tile->currentPos[1],
					tile->currentPos[2]);
				
				gxRotatef(90, 0, 0, 1); // fixme ! just to make it appear in portrait mode!
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
	
	Vfxclip vfxclips[NUM_VFXCLIPS];
	
	std::vector<SpokenWord*> spokenWords;
	
	Faces faces;
	
	HitTestResult hitTestResult;
	
	World()
		: camera()
		, videoclips()
		, vfxclips()
		, spokenWords()
		, faces()
		, hitTestResult()
	{
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
            const float audioGain = 1.f;
			
			videoclips[i].init(sampleSet, mutex, i, audioFilename, videoFilename, audioGain);
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
		kInput_VideoClipTimeMultiplier,
		kInput_VideoClipDrawGrid,
		kInput_VideoClipGridColor,
		kInput_VideoClipGridColor2,
		kInput_SpokenWordBegin,
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
		addInput(kInput_VideoClipTimeMultiplier, kVfxPlugType_Float);
		addInput(kInput_VideoClipDrawGrid, kVfxPlugType_Bool);
		addInput(kInput_VideoClipGridColor, kVfxPlugType_Color);
		addInput(kInput_VideoClipGridColor2, kVfxPlugType_Color);
		addInput(kInput_SpokenWordBegin, kVfxPlugType_Trigger);
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
		
		s_videoRetain = getInputFloat(kInput_VideoRetain, 0.f);
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
	in("video.tmult", "float", "1");
	in("video.dgrid", "bool", "1");
	in("gridcolor", "color", "ffffffff");
	in("gridcolor2", "color", "ffffffff");
	in("word.begin!", "trigger");
	in("video.begin!", "trigger");
	in("video.end!", "trigger");
	in("video.aspect!", "float", "1");
	in("video.scale", "float", "0.2");
	in("video.spacing", "float", "200");
	in("video.fdist", "float", "400");
	in("video.ndist", "float", "200");
	in("video.awob.x", "float");
	in("video.awob.y", "float");
	in("wobble.x", "float", "1");
	in("wobble.y", "float", "1");
	in("video.retain", "float");
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

void VideoLandscape::init()
{
	world = new World();
	world->init(g_sampleSet, g_binauralMutex);
    s_world = world;
}

void VideoLandscape::shut()
{
	world->shut();
	
	delete world;
	world = nullptr;
}

void VideoLandscape::tick(const float dt)
{
#if ENABLE_INTERACTIVITY
	// process input
	
	if (keyboard.wentDown(SDLK_n))
		enableNearest = !enableNearest;
	
	if (keyboard.wentDown(SDLK_v))
		enableVertices = !enableVertices;
	
	if (keyboard.wentDown(SDLK_TAB))
		showUi = !showUi;
#endif

	SDL_LockMutex(g_audioMutex);
	{
		g_audioMixer->voiceGroups[kVoiceGroup_Videoclips].desiredGain =
			keyboard.isDown(SDLK_w) ? .1f : 1.f;
	}
	SDL_UnlockMutex(g_audioMutex);
	
	// update video clips
	
	world->tick(dt);
}

void VideoLandscape::draw()
{
	projectPerspective3d(fov, near, far);
	{
		world->draw3d();
	}
	projectScreen2d();

	world->draw2d();

	if (showUi)
	{
		gxPushMatrix();
		
		setColor(255, 255, 255, 127);
		drawText(GFX_SX - 16, 24, 32, -1, +1, "GROOOP PERFORMANCE APP");

	#if 0
		gxTranslatef(0, GFX_SY - 100, 0);
		setColor(colorWhite);
		drawText(10, 40, kFontSize, +1, +1, "N: toggle use nearest point (%s)", enableNearest ? "on" : "off");
		drawText(10, 60, kFontSize, +1, +1, "V: toggle use vertices (%s)", enableVertices ? "on" : "off");
	#endif
	
		gxPopMatrix();
	}
}
