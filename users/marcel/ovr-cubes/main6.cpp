#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"

#include "parameter.h"
#include "parameterUi.h"

#include "data/engine/ShaderCommon.txt" // VS_ constants
#include "framework.h"
#include "framework-ovr.h"
#include "framework-vr-hands.h"
#include "framework-vr-pointer.h"
#include "gx_mesh.h"
#include "gx_render.h"
#include "internal.h"

#include "imgui-framework.h"
#include "imgui_internal.h"

#include "watersim.h"

#include "spatialAudioSystem-binaural.h"

#include "audiooutput/AudioOutput_Native.h"

#include "objects/audioSourceVorbis.h"
#include "objects/delayLine.h"

#include "particle.h"
#include "particle_editor.h"
#include "ui.h"

#include "Path.h" // GetExtension for jgmod file listing
#include "Quat.h"

#include <algorithm>

/*

new challenges!

todo : use finger gesture or controller button to open/close control panel
    - check finger pinching amount for the index finger
    - have pinch enter/leave thresholds (where leave > enter), to avoid rapid transitions
    - attach control panel to hand location on pinch

todo : experiment with drawing window contents in 3d directly (no window surface). this will give higher quality results on outlines

todo : refactor controller state similar to hand state

todo : add some particles drifting in the air
	- add ephimeral particles drifting in the air
	+ particle effect : spawn particles on movement. spawn particles when objects move
	- particle effect : add manual spawn trigger. spawn(100) ?
	+ particle effect : add ability to enable/disable spawn by time. or give it some limited amount of time to spawn. allows for spawning on some event over time for some while

todo : make NdotV abs(..) inside the gltf pbr shaders. this would make lighting act double-sidedly

todo : add 'assetcopy-filelist.txt' or something, which stores the hashes for copied files
    - during assetcopy: optionally enable filelist check
    - calculate hash for assets inside apk
    - check if changed from filelist

todo : window3d : draw magnification bubble at cursor location
todo : framework : draw pointer beams when manual vr mode is off
    - requires a unified place to store pointer transforms, and active pointer index

todo : experiment with more geometrically interesting mesh shapes for the jgmod voices
    - what kind of effects can be achieved?

todo : grab jgmod voice cubes using left pointer
    - add jgmod voices to raycast
    - manually set the transform

DONE : automatically switch between controller and hands for virtual desktop interaction

DONE : draw virtual desktop using a custom shader, to give the windows some specular reflection
    # issue : no way to control which texture uniform gets set. pass texture name to drawVirtualDesktop ?
    + maybe just custom draw all windows (it's easy enough)
    + problem : need to iterate windows

DONE : project hand position down onto watersim, to give the user an exaggered impression of how high up they are
    - calculate inverse of the watersim transform
    - use the inverse transform, to transform hand location from world-space into watersim-space
    - use xz coordinates to lookup height (xyz) position
    - transform xyz position back into world-space and draw a circle or something

DONE : apply a texture or color to the watersim quads

DONE : determine nice watersim params and update defaults

DONE : binauralize some sounds

DONE : polish virtual desktop interaction when using hands,
    + tell virtual desktop which type of interaction is used (beam or finger),
    + let virtual desktop have detection of false finger presses

DONE : add UI for showing spatial audio system status

DONE : add the moon

DONE : add some stars

DONE : spawn particles and vibrate when controller beams intersect each other

DONE : add spatial audio source parameters
	- make headroom a spatial audio source parameter
	- make recorded distance a spatial audio source parameter

DONE : optimize matrix multiplication for neon

 */

#include <math.h>

#define SAMPLE_RATE 48000

#define AUDIO_BUFFER_VALIDATION 0

struct Scene;

//

static bool calculate_nearest_points_for_ray_vs_ray(
	Vec3Arg p1,
	Vec3Arg d1,
	Vec3Arg p2,
	Vec3Arg d2,
	Vec3 & out_closest1,
	Vec3 & out_closest2);

//

struct ControlPanel
{
	enum Tab
	{
		kTab_Scene,
		kTab_Audio,
		kTab_Tracker,
		kTab_ParticleEditor
	};
	
	Window window;
	FrameworkImGuiContext guiContext;

	Scene * scene = nullptr;
	SpatialAudioSystem_Binaural * spatialAudioSystem = nullptr;
	
	ParticleEditor particleEditor;

	ImGuiID lastHoveredId = 0;
	bool hoveredIdChanged = false;
	
	Tab activeTab = kTab_Scene;

	bool doWindowResize = true;
	
	ControlPanel(const Vec3 position, const float angle, Scene * in_scene, SpatialAudioSystem_Binaural * in_spatialAudioSystem)
		: window("Window", 340, 340, true)
	{
	#if WINDOW_IS_3D
		const Mat4x4 transform =
			Mat4x4(true)
				.RotateY(angle)
				.Translate(position);
		window.setTransform(transform);
	#endif

		guiContext.init(false);

		scene = in_scene;
		spatialAudioSystem = in_spatialAudioSystem;
	}

	~ControlPanel()
	{
		guiContext.shut();
	}

	void tick(const float dt);

	void draw()
	{
		pushWindow(window);
		{
			framework.beginDraw(20, 20, 20, 255);
			{
				guiContext.draw();
				
				if (activeTab == kTab_ParticleEditor)
				{
				// todo : remove particle preview from particle editor, or make it optional
					pushFontMode(FONT_SDF);
					particleEditor.draw(true, window.getWidth(), window.getHeight());
					popFontMode();
				}
			}
			framework.endDraw();
		}
		popWindow();
	}
};

// -- watersim object

static void calculateNormal(const Watersim & watersim, const int x, const int z, float * out_normal)
{
	const int x1 = x < 0 ? 0 : x > watersim.numElems - 1 ? watersim.numElems - 1 : x;
	const int z1 = z < 0 ? 0 : z > watersim.numElems - 1 ? watersim.numElems - 1 : z;
	const int x2 = x1 + 1 < watersim.numElems ? x1 + 1 : x1;
	const int z2 = z1 + 1 < watersim.numElems ? z1 + 1 : z1;
	
	const float y1 = watersim.p[x1][z1];
	const float y2 = watersim.p[x2][z1];
	const float y3 = watersim.p[x2][z2];
	const float y4 = watersim.p[x1][z2];

	const float dx = -(y2 - y1);
	const float dy = 1.f;
	const float dz = -(y4 - y1);

	const float ds = sqrtf(dx * dx + dy * dy + dz * dz);

	const float nx = dx / ds;
	const float ny = dy / ds;
	const float nz = dz / ds;

	out_normal[0] = nx;
	out_normal[1] = ny;
	out_normal[2] = nz;
}

// -- collision system

struct CollisionSystemInterface
{
	virtual bool raycast(Vec3Arg origin, Vec3Arg direction, const int collisionMask, float & out_distance, Vec3 & out_normal) const = 0;
};

static const int kCollisionFlag_Watersim = 1 << 0;

static CollisionSystemInterface * collisionSystem = nullptr;

// -- spatial audio system

static SpatialAudioSystemInterface * spatialAudioSystem = nullptr;

// -- forward lighting system

struct ForwardLightingSystemInterface
{
	virtual void beginScene(const Mat4x4 & worldToView) = 0;

	virtual void setSunProperties(const bool enabled, Vec3Arg position, Vec3Arg color, const float intensity) = 0;
	virtual bool getSunProperties(Vec3 & position, Vec3 & color, const bool viewSpace) = 0;

	virtual void setLightingForGltfMaterialShader(Shader & materialShader) = 0;
	virtual void setLightingForGltfMaterialShaders(gltf::MaterialShaders & materialShaders) = 0;
};

static ForwardLightingSystemInterface * forwardLightingSystem = nullptr;

// -- particle effect system

static ParticleEffectSystem particleEffectSystem;

// -- moon object

struct MoonObject
{
	float angle = 0.f;
	Vec3 axis = Vec3(1, 0, 0);

	Vec3 getPosition() const
	{
		Quat q;
		q.fromAngleAxis(angle * float(M_PI / 180.0), axis);

		return q.toMatrix().GetAxis(2) * 100.f;
	}

	void tick(const float dt)
	{
		angle -= dt * .2f;
	}

	void drawOpaque() const
	{
		Mat4x4 worldToView;
		gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
		const Mat4x4 & viewToWorld = worldToView.CalcInv();
		const Vec3 origin = viewToWorld.GetTranslation();
		const Vec3 position = origin + getPosition();

		const Vec3 drawPosition = position + origin;
		const Mat4x4 lookatMatrix = Mat4x4(true).Lookat(position, origin, Vec3(0, 1, 0));

		gxPushMatrix();
		{
			gxMultMatrixf(lookatMatrix.m_v);

			setColor(colorWhite);
			fillCircle(0, 0, 10.f, 40);
		}
		gxPopMatrix();
	}
};

// -- celestrial sphere object

struct CelestialSphereObject
{
	GxVertexBuffer vb;
	GxIndexBuffer ib;
	GxMesh mesh;

	float distance = 0.f;

	void init(const int numStars, const float in_distance)
	{
		distance = in_distance;

		struct Vertex
		{
			Vec4 position;
			Vec4 color;
		};

		std::vector<Vertex> vertices;
		vertices.resize(numStars * 6);

		for (int i = 0; i < numStars; ++i)
		{
			const float a1 = random<float>(-M_PI, +M_PI);
			const float a2 = random<float>(-M_PI, +M_PI);

			const Mat4x4 rotation = Mat4x4(true).RotateX(a1).RotateY(a2);
			const Vec4 position = rotation * Vec4(0, 0, 1, 1);

			for (int j = 0; j < 6; ++j)
			{
				vertices[i * 6 + j].position = position;
				vertices[i * 6 + j].color.Set(1, 1, 1, 1);
			}
		}

		vb.alloc(vertices.data(), numStars * 6 * sizeof(Vec3));

		const GxVertexInput inputs[2] =
			{
				{ VS_POSITION, 4, GX_ELEMENT_FLOAT32, 0, 0*4, sizeof(Vec4) },
				{ VS_COLOR,    4, GX_ELEMENT_FLOAT32, 0, 4*4, sizeof(Vec4) }
			};

		mesh.setVertexBuffer(&vb, inputs, sizeof(inputs) / sizeof(inputs[0]), sizeof(Vertex));
		mesh.addPrim(GX_TRIANGLES, numStars * 6, false);
		//mesh.addPrim(GX_POINTS, numStars * 6, true);
	}

	void drawOpaque() const
	{
		gxPushMatrix();
		{
			gxScalef(distance, distance, distance);
			gxRotatef(framework.time, 1, 1, 1);

			Shader shader("stars");
			setShader(shader);
			{
				shader.setTexture("source", 0, getTexture("star.jpg"), true, true);
				shader.setImmediate("u_time", framework.time);

				mesh.draw();
			}
			clearShader();
		}
		gxPopMatrix();
	}
};

// -- drifter object

struct DrifterObject
{
	Mat4x4 transform = Mat4x4(true);

	Vec3 speed;
	
	ParticleEffectLibrary effectLibrary;

	void init()
	{
		effectLibrary.loadFromFile("drifter-particles1.pfx");
	// todo : add a dedicated particle effect for drifters, and remove the hack below
		for (auto & effectInfo : effectLibrary.effectInfos)
		{
			effectInfo.particleInfo.emissionType = ParticleInfo::kEmissionType_DistanceTraveled;
			effectInfo.particleInfo.rate /= 100.f;
		}
		
		effectLibrary.createEffects(particleEffectSystem);
		
		Vec3 position = transform.GetTranslation();
		position[1] += .2f;
		position *= 100.f; // fixme : particle effect scaling
		effectLibrary.setPosition(position[0], position[1], position[2], true);
	}
	
	void shut()
	{
		effectLibrary.removeEffects(particleEffectSystem);
	}
	
	void tick(const float dt)
	{
		Vec3 position = transform.GetTranslation();

		Vec3 normal(false);
		float distance;
		if (collisionSystem->raycast(position, Vec3(0, -1, 0), kCollisionFlag_Watersim, distance, normal))
		{
			speed[0] += normal[0] * 1.f * dt;
			speed[2] += normal[2] * 1.f * dt;

			position[1] = position[1] - distance + .1f;
		}

		speed *= powf(.97f, dt);

		position += speed * dt;
		
		transform.SetTranslation(position);
		
		//
		
		position[1] += .2f;
		position *= 100.f; // fixme : particle effect scaling
		effectLibrary.setPosition(position[0], position[1], position[2], false);
	}

	void drawOpaque() const
	{
		gxPushMatrix();
		{
			gxMultMatrixf(transform.m_v);

			setColor(200, 230, 255);
			drawCircle(0, 0, .2f, 100);
		}
		gxPopMatrix();
	}
};

// -- audio streamer object

struct AudioStreamerAudioSource : AudioSource
{
	AudioSourceVorbis audioSourceVorbis;
	std::atomic<float> rms;

	AudioStreamerAudioSource()
		: rms(0.f)
	{
	}

	void open(const char * filename)
	{
		filename = framework.resolveResourcePath(filename);
		
		audioSourceVorbis.open(filename, true);
	}

	virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override final
	{
		audioSourceVorbis.generate(samples, numSamples);

		float squaredSum = 0.f;

		for (int i = 0; i < numSamples; ++i)
		{
			squaredSum += samples[i] * samples[i];
		}

		rms.store(sqrtf(squaredSum / numSamples));
	}
};

struct AudioStreamerObject
{
	Vec3 initialPosition;

	Mat4x4 transform = Mat4x4(true);
	AudioStreamerAudioSource * audioSource = nullptr;
	void * spatialAudioSource = nullptr;

	~AudioStreamerObject()
	{
		shut();
	}

	void init(const char * filename)
	{
		audioSource = new AudioStreamerAudioSource();
		audioSource->open(filename);

		spatialAudioSource = spatialAudioSystem->addSource(transform, audioSource, 4.f, 12.f);
	}

	void shut()
	{
		delete audioSource;
		audioSource = nullptr;
	}

	void tick(const float dt)
	{
		//transform = Mat4x4(true).Translate(initialPosition).Translate(0.f, sinf(framework.time / 2.34f) * .06f, 0.f);
		transform = Mat4x4(true).Translate(initialPosition).Translate(sinf(framework.time / 2.34f) * 10.f, sinf(framework.time / 2.34f) * .06f, 0.f);

		spatialAudioSystem->setSourceTransform(spatialAudioSource, transform);
	}

	void drawOpaque() const
	{
		gxPushMatrix();
		{
			gxMultMatrixf(transform.m_v);

			const float value = clamp<float>(audioSource->rms.load(), 0.f, 1.f);
			const float scale = lerp<float>(.2f, 1.f, value);
			gxScalef(scale, scale, scale);

			setColor(colorWhite);
			lineCube(Vec3(), Vec3(.3f));
			fillCube(Vec3(), Vec3(.1f));
		}
		gxPopMatrix();
	}
};

// -- model object

struct ModelObject
{
	Mat4x4 transform = Mat4x4(true);

	gltf::Scene scene;
	gltf::BufferCache bufferCache;

	ParameterMgr parameterMgr;
	ParameterVec3 * position = nullptr;

	void init(Vec3Arg in_position)
	{
		parameterMgr.setPrefix("model");
		position = parameterMgr.addVec3("position", Vec3(0.f));
		position->setLimits(Vec3(-4.f), Vec3(+4.f));
		position->setEditingCurveExponential(4.f);
		position->set(in_position);
		position->setDirty();

		//gltf::loadScene("Suzanne/glTF/Suzanne.gltf", scene);
		gltf::loadScene("deus_ex_pbr/scene.gltf", scene);
		bufferCache.init(scene);
		
		for (auto & material : scene.materials)
			material.pbrMetallicRoughness.roughnessFactor = .25f;
	}
	
	void tick(const float dt)
	{
		if (position->isDirty)
		{
			position->isDirty = false;

			transform = Mat4x4(true)
				.Translate(position->get())
				.Scale(.03f);
		}

		transform = transform.Rotate(dt * .4f, Vec3(0, 1, 0));
	}

	void drawOpaque() const
	{
		Mat4x4 worldToView;
		gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);

		gxPushMatrix();
		{
			gxMultMatrixf(transform.m_v);

			gltf::MaterialShaders materialShaders;
			gltf::setDefaultMaterialShaders(materialShaders);

			forwardLightingSystem->setLightingForGltfMaterialShaders(materialShaders);

			gltf::DrawOptions drawOptions;
			drawOptions.defaultMaterial.pbrMetallicRoughness.metallicFactor = 1.f;
			drawOptions.defaultMaterial.pbrMetallicRoughness.roughnessFactor = .2f;

			gltf::drawScene(scene, &bufferCache, materialShaders, true, &drawOptions);
		}
		gxPopMatrix();
	}
};

// -- jgplayer object

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"
#include "audiooutput/AudioOutput_OpenSL.h"
#include "framework-allegro2.h"
#include "jgmod.h"

struct JgplayerObject
{
	//static const int kMaxVoices = 12;
	static const int kMaxVoices = 32;
	static const int kMaxSamplesForVisualization = 256;
	
	struct MyAudioSource : AudioSource
	{
		AllegroTimerApi * timerApi = nullptr;
		AllegroVoiceApi * voiceApi = nullptr;
		int voiceId = -1;
		
		void * spatialAudioSource = nullptr;
		
		bool isVisible = false;
		Mat4x4 transform = Mat4x4(true);

		float samplesForVisualization[kMaxSamplesForVisualization];
		int numSamplesForVisualization = 0;
		
		void init(AllegroVoiceApi * in_voiceApi, const int in_voiceId)
		{
			voiceApi = in_voiceApi;
			voiceId = in_voiceId;
			
			spatialAudioSource = spatialAudioSystem->addSource(
				transform,
				this,
				4.f,
				12.f);
		}
		
		void shut()
		{
			spatialAudioSystem->removeSource(spatialAudioSource);
			spatialAudioSource = nullptr;
			
			timerApi = nullptr;
			voiceApi = nullptr;
			voiceId = -1;
		}
		
		virtual void generate(SAMPLE_ALIGN16 float * __restrict samples, const int numSamples) override final
		{
			if (timerApi != nullptr)
			{
				timerApi->processInterrupts(int64_t(numSamples) * 1000000 / voiceApi->sampleRate);
			}

			float stereoPanning;
			voiceApi->generateSamplesForVoice(voiceId, samples, numSamples, stereoPanning);

		// todo : use a circular history buffer or something
			const int numSamplesToCopyForVisualization = numSamples < kMaxSamplesForVisualization ? numSamples : kMaxSamplesForVisualization;
			memcpy(samplesForVisualization, samples, numSamplesToCopyForVisualization * sizeof(float));
			for (int i = numSamplesToCopyForVisualization; i < kMaxSamplesForVisualization; ++i)
				samplesForVisualization[i] = 0.f;
			numSamplesForVisualization = numSamplesToCopyForVisualization;
		}
	};
	
	AllegroTimerApi * timerApi = nullptr;
	AllegroVoiceApi * voiceApi = nullptr;

	JGMOD_PLAYER * player = nullptr;
	JGMOD * mod = nullptr;
	
	MyAudioSource audioSources[kMaxVoices];
	
	gltf::Scene box_scene;
	gltf::BufferCache box_bufferCache;
	
	ParameterMgr parameterMgr;
	ParameterInt * speed = nullptr;
	ParameterInt * tempo = nullptr;
	ParameterInt * pitch = nullptr;
	ParameterEnum * file = nullptr;
	
	void init(const char * filename)
	{
		timerApi = new AllegroTimerApi(AllegroTimerApi::kMode_Manual);
		voiceApi = new AllegroVoiceApi(SAMPLE_RATE, true);

		filename = framework.resolveResourcePath(filename);
		mod = jgmod_load(filename);
		
		player = new JGMOD_PLAYER();
		
	    if (player->init(JGMOD_MAX_VOICES, timerApi, voiceApi) < 0)
		{
	        logError("unable to allocate %d voices", JGMOD_MAX_VOICES);
		}
		else if (mod != nullptr)
		{
			player->enable_lasttrk_loop = true;
			player->play(mod, true);
		}
		
		for (int i = 0; i < kMaxVoices; ++i)
		{
		// todo : register pre-update with spatial audio system
			if (i == 0)
				audioSources[i].timerApi = timerApi;
			
			audioSources[i].init(voiceApi, i);
		}
		
		//
		
		gltf::loadScene("hollow-cube.gltf", box_scene);
		box_bufferCache.init(box_scene);
		
		//
		
		parameterMgr.setPrefix("jgplayer");

		std::vector<std::string> modFiles;
		{
			// list all of the module files
			auto files = listResourceFiles(".", true);
			for (auto & file : files)
			{
				auto ext = Path::GetExtension(file, true);
				if (ext == "mod" || ext == "xm" || ext == "s3m" || ext == "it")
					modFiles.push_back(file);
			}
			std::sort(modFiles.begin(), modFiles.end());
		}

		std::vector<ParameterEnum::Elem> fileElems;
		for (size_t i = 0; i < modFiles.size(); ++i)
			fileElems.push_back({ modFiles[i].c_str(), (int)i });
		file = parameterMgr.addEnum("file", 0, fileElems);

		speed = parameterMgr.addInt("speed", 100);
		speed->setLimits(0, 200);
		tempo = parameterMgr.addInt("tempo", 100);
		tempo->setLimits(0, 200);
		pitch = parameterMgr.addInt("pitch", 100);
		pitch->setLimits(0, 200);
	}

	void shut()
	{
		for (int i = 0; i < kMaxVoices; ++i)
		{
			audioSources[i].shut();
		}
		
		player->stop();
		
		delete player;
		player = nullptr;
		
		if (mod != nullptr)
		{
			jgmod_destroy(mod);
			mod = nullptr;
		}

		delete timerApi;
		delete voiceApi;
		timerApi = nullptr;
		voiceApi = nullptr;
	}
	
	void tick(const float dt)
	{
		if (file->isDirty)
		{
			file->isDirty = false;

			player->destroy_mod();
			mod = nullptr;

			const char * filename = file->translateValueToKey(file->get());
			filename = framework.resolveResourcePath(filename);
			mod = jgmod_load(filename);
			player->play(mod, true);
		}

		player->set_speed((speed->get() * tempo->get()) / 100);
		player->set_pitch((speed->get() * pitch->get()) / 100);
		
		for (int i = 0; i < kMaxVoices; ++i)
		{
			audioSources[i].isVisible = voiceApi->voice_is_playing(audioSources[i].voiceId);

			const int panning = voiceApi->voice_get_pan(audioSources[i].voiceId);
			const int pitch = voiceApi->voice_get_frequency(audioSources[i].voiceId);
			const int volume = voiceApi->voice_get_volume(audioSources[i].voiceId);
			
			// update transform based on panning, pitch, etc
			audioSources[i].transform =
				Mat4x4(true)
					.RotateY(framework.time / 100.f)
					.Translate(
						((audioSources[i].voiceId + .5f) / kMaxVoices - .5f) * 2.f * 4.f,
						1.f + (panning/255.f - .5f) * 2.f * 1.f,
						pitch / 4000.f - 2.f)
					.Scale(volume / 255.f);

			spatialAudioSystem->setSourceTransform(
				audioSources[i].spatialAudioSource,
				audioSources[i].transform);
		}
	}
	
	void drawOpaque() const
	{
		pushCullMode(CULL_BACK, CULL_CCW);
		{
			for (int i = 0; i < kMaxVoices; ++i)
			{
				if (!audioSources[i].isVisible)
					continue;
				
				gxPushMatrix();
				{
					gxMultMatrixf(audioSources[i].transform.m_v);

					const Color color = Color::fromHSL(i / float(kMaxVoices), .5f, .5f);
					
					setColor(color);
					//lineCube(Vec3(), Vec3(.2f));
					
					gxPushMatrix();
					gxScalef(.3f, .3f, .3f);
					{
						gltf::MaterialShaders materialShaders;
						gltf::setDefaultMaterialShaders(materialShaders);
						forwardLightingSystem->setLightingForGltfMaterialShaders(materialShaders);
						
						gltf::DrawOptions drawOptions;
						drawOptions.defaultMaterial.doubleSided = true;
						drawOptions.defaultMaterial.pbrMetallicRoughness.baseColorFactor = color;
						drawOptions.defaultMaterial.emissiveFactor = Vec3(color.r, color.g, color.b) * .3f;
						drawOptions.defaultMaterial.pbrMetallicRoughness.metallicFactor = 1.f;
						drawOptions.defaultMaterial.pbrMetallicRoughness.roughnessFactor = .3f;
						
						gltf::drawScene(box_scene, &box_bufferCache, materialShaders, true, &drawOptions);
					}
					gxPopMatrix();

					const int numSamplesForVisualization = audioSources[i].numSamplesForVisualization;
					if (numSamplesForVisualization > 0)
					{
						const float scale = 1.f / numSamplesForVisualization;
						gxTranslatef(-.5f, 0, 0);
						gxScalef(scale, 1, 1);
						gxTranslatef(+.5f, 0, 0);

						gxBegin(GX_LINE_STRIP);
						{
							for (int s = 0; s < numSamplesForVisualization; ++s)
							{
								gxVertex3f(
									s,
									audioSources[i].samplesForVisualization[s],
									0.f);
							}
						}
						gxEnd();
					}
				}
				gxPopMatrix();
			}
		}
		popCullMode();
	}
};

// -- particle effect object

struct ParticleEffectObject
{
	std::vector<ParticleEffectInfo> infos;
	std::vector<ParticleEffect*> effects;
	
	void init(const char * filename)
	{
		if (loadParticleEffectLibrary(filename, infos))
		{
			for (auto & info : infos)
			{
				auto * effect = particleEffectSystem.createEffect(&info);
				
				effects.push_back(effect);
			}
		}
	}
	
	void shut()
	{
		for (auto *& effect : effects)
		{
			particleEffectSystem.removeEffect(effect);
			
			effect = nullptr;
		}
		
		effects.clear();
	}
	
	void tick(const float dt)
	{
		for (auto * effect : effects)
		{
			effect->emitter.active = mouse.isDown(BUTTON_LEFT) || vrPointer[1].isDown(VrButton_A);
		}
	}
};

// -- scene

struct Scene : CollisionSystemInterface, ForwardLightingSystemInterface
{
	Vec3 playerLocation;

	ParticleEffectLibrary pointerBeamsEffect;

	ControlPanel * controlPanel = nullptr;
	bool showControlPanel = true;
	AudioSourceVorbis controlPanel_audioSource;
	void * controlPanel_spatialAudioSource = nullptr;

	ParameterMgr parameterMgr;
	
	ParameterMgr parameterMgr_watersim;
	ParameterInt * resolution = nullptr;
	ParameterFloat * tension = nullptr;
	ParameterFloat * velocityRetain = nullptr;
	ParameterFloat * positionRetain = nullptr;
	ParameterFloat * transformHeight = nullptr;
	ParameterFloat * transformScale = nullptr;

	ParameterMgr parameterMgr_material;
	ParameterFloat * materialMetallic = nullptr;
	ParameterFloat * materialRoughness = nullptr;

	ParameterMgr parameterMgr_lighting;
	ParameterFloat * lightIntensity = nullptr;

	Mat4x4 watersim_transform = Mat4x4(true);
	Watersim watersim;

	std::vector<MoonObject> moons;

	std::vector<CelestialSphereObject> celestialSpheres;

	std::vector<DrifterObject> drifters;

	std::vector<AudioStreamerObject> audioStreamers;

	std::vector<ModelObject> models;
	
	std::vector<JgplayerObject> jgplayers;
	
	std::vector<ParticleEffectObject> particleEffects;

	void create();
	void destroy();

	void tick(const float dt);

	void drawOnce() const;
	void drawOpaque() const;
	void drawTranslucent() const;

	void drawWatersim() const;
	void drawWatersimHandProjections() const;

	// -- collision system interface

// todo : should be part of a collision system
	virtual bool raycast(Vec3Arg origin, Vec3Arg direction, const int collisionMask, float & out_distance, Vec3 & out_normal) const override final
	{
		bool result = false;

		out_distance = FLT_MAX;

		if (collisionMask & kCollisionFlag_Watersim)
		{
			const Mat4x4 & watersimToWorld = watersim_transform;
			const Mat4x4 worldToWatersim = watersimToWorld.CalcInv();
			const Vec3 position_watersim = worldToWatersim.Mul4(origin);

			{
				const float height = watersim.sample(
					position_watersim[0],
					position_watersim[2]);

				const float distance = position_watersim[1] - height;

				if (distance < out_distance)
				{
					float normal[3];
					calculateNormal(
						watersim,
						(int)floorf(position_watersim[0]),
						(int)floorf(position_watersim[2]),
						normal);

					out_distance = distance;
					out_normal.Set(normal[0], normal[1], normal[2]);

					result = true;
				}
			}
		}

		return result;
	}

	// -- forward lighting system interface

	Mat4x4 worldToView = Mat4x4(true);

	bool sunEnabled = false;
	Vec3 sunPosition;
	Vec3 sunColor;

	virtual void beginScene(const Mat4x4 & in_worldToView) override final
	{
		worldToView = in_worldToView;
	}

	virtual void setSunProperties(const bool enabled, Vec3Arg position, Vec3Arg color, const float intensity) override final
	{
		sunEnabled = enabled;
		sunPosition = position;
		sunColor = color * intensity;
	}

	virtual bool getSunProperties(Vec3 & position, Vec3 & color, const bool viewSpace) override final
	{
		if (viewSpace)
			position = worldToView.Mul4(sunPosition);
		else
			position = sunPosition;

		color = sunColor;

		return sunEnabled;
	}

	virtual void setLightingForGltfMaterialShader(Shader & materialShader) override final
	{
		gltf::setDefaultMaterialLighting(
			materialShader,
			worldToView,
			-sunPosition.CalcNormalized(),
			sunColor);
	}

	virtual void setLightingForGltfMaterialShaders(gltf::MaterialShaders & materialShaders) override final
	{
		gltf::setDefaultMaterialLighting(
			materialShaders,
			worldToView,
			-sunPosition.CalcNormalized(),
			sunColor);
	}
};

void Scene::create()
{
	collisionSystem = this;
	forwardLightingSystem = this;
	
	pointerBeamsEffect.loadFromFile("drifter-particles1.pfx");
	pointerBeamsEffect.createEffects(particleEffectSystem);

	controlPanel = new ControlPanel(Vec3(0, 1.5f, -.45f), 0.f, this, (SpatialAudioSystem_Binaural*)spatialAudioSystem);
#if WINDOW_IS_3D && 0
	controlPanel_audioSource.open("180328-004.ogg", true);
	controlPanel_spatialAudioSource = spatialAudioSystem->addSource(controlPanel->window.getTransform(), &controlPanel_audioSource, 4.f, 12.f);
#endif

	// add watersim object
	parameterMgr_watersim.setPrefix("watersim");
	resolution = parameterMgr_watersim.addInt("resolution", Watersim::kMaxElems);
	resolution->setLimits(2, Watersim::kMaxElems);
	tension = parameterMgr_watersim.addFloat("tension", 4.f);
	tension->setLimits(1.f, 100.f);
	velocityRetain = parameterMgr_watersim.addFloat("velocityRetain", .99f);
	velocityRetain->setLimits(.01f, .99f);
	positionRetain = parameterMgr_watersim.addFloat("positionRetain", .99f);
	positionRetain->setLimits(.01f, .99f);
	transformHeight = parameterMgr_watersim.addFloat("transformHeight", 0.f);
	transformHeight->setLimits(-8.f, +8.f);
	transformScale = parameterMgr_watersim.addFloat("transformScale", 1.f);
	transformScale->setLimits(.1f, 2.f);
	watersim.init(resolution->get());
	parameterMgr.addChild(&parameterMgr_watersim);

	parameterMgr_material.setPrefix("material");
	materialMetallic = parameterMgr_material.addFloat("materialMetallic", .8f);
	materialMetallic->setLimits(0.f, 1.f);
	materialRoughness = parameterMgr_material.addFloat("materialRoughness", .4f);
	materialRoughness->setLimits(0.f, 1.f);
	parameterMgr.addChild(&parameterMgr_material);

	parameterMgr_lighting.setPrefix("lighting");
	lightIntensity = parameterMgr_lighting.addFloat("lightIntensity", 1.f);
	lightIntensity->setLimits(0.f, 4.f);
	parameterMgr.addChild(&parameterMgr_lighting);

	moons.resize(1);

	for (auto & moon : moons)
	{
		moon.angle = .2f;
		moon.axis = Vec3(1, .2f, .2f).CalcNormalized();
	}

	celestialSpheres.resize(1);

	for (auto & celestialSphere : celestialSpheres)
	{
		celestialSphere.init(10000, 100.f);
	}

	drifters.resize(32);

	for (auto & drifter : drifters)
	{
		const float x = random<float>(-4.f, +4.f);
		const float z = random<float>(-4.f, +4.f);

		drifter.transform.SetTranslation(x, 0, z);
		
		drifter.init();
	}

	audioStreamers.resize(1); // works with up to 20 in release

	int index = 0;

	for (auto & audioStreamer : audioStreamers)
	{
		const float x = random<float>(-8.f, +8.f);
		const float z = random<float>(-8.f, +8.f);

		audioStreamer.initialPosition.Set(x, .4f, z);

		const char * filenames[3] =
			{
				"veemsounds.ogg",
				"180328-004.ogg",
				"180327-001.ogg"
			};

		audioStreamer.init(filenames[index % 3]);

		index++;
	}

	models.resize(1);
	int modelIdx = 0;

	for (auto & model : models)
	{
		model.init(Vec3((modelIdx + .5f - models.size() / 2.f), 0, 0));

		parameterMgr.addChild(&model.parameterMgr, ++modelIdx);
	}
	
	jgplayers.resize(1);
	int jgplayerIdx = 0;
	
	for (auto & jgplayer : jgplayers)
	{
		//jgplayer.init("point_of_departure.s3m");
		//jgplayer.init("K_vision.s3m");
		//jgplayer.init("25 - Surfacing.s3m");
		//jgplayer.init("05 - Shared Dig.s3m");
		jgplayer.init("UNATCO.it");
		
		parameterMgr.addChild(&jgplayer.parameterMgr, ++jgplayerIdx);
	}
	
	particleEffects.resize(1);
	
	for (auto & particleEffect : particleEffects)
	{
		particleEffect.init("particles3.pfx");
	}
}

void Scene::destroy()
{
	for (auto & particleEffect : particleEffects)
	{
		particleEffect.shut();
	}
	
	for (auto & drifter : drifters)
	{
		drifter.shut();
	}
	
	watersim.shut();

	delete controlPanel;
	controlPanel = nullptr;
	
	pointerBeamsEffect.removeEffects(particleEffectSystem);
}

void Scene::tick(const float dt)
{
	// update movement

	if (vrPointer[1].hasTransform && vrPointer[1].wentDown(VrButton_Trigger))
	{
		playerLocation += vrPointer[1].getTransform(Vec3()).GetAxis(2);
	}

	// update windows

	Mat4x4 viewToWorld(true);
	int buttonMask = 0;
	bool isHand = false;

	const VrSide interactiveSide = VrSide_Right;
// todo : automatically switch between left and right hand. or allow both to interact at the 'same' time ?
	if (vrHand[interactiveSide].getFingerTransform(VrFinger_Index, playerLocation, viewToWorld))
	{
		buttonMask = 1 << 0;
		isHand = true;
	}
	else
	{
		viewToWorld = vrPointer[0].getTransform(playerLocation);
		buttonMask =
			(vrPointer[0].isDown(VrButton_Trigger) << 0) |
			(vrPointer[0].isDown(VrButton_GripTrigger) << 1);
		isHand = false;

	#if WINDOW_IS_3D
	#if false
		controlPanel->window.setTransform(
			Mat4x4(true)
				.Translate(playerLocation)
				.Mul(vrPointer[1].transform));
	#else
		if (vrPointer[1].isDown(VrButton_GripTrigger))
		{
			controlPanel->window.setTransform(
				vrPointer[1].getTransform(playerLocation)
					.Translate(0, .2f, .4f)
					.RotateX(float(M_PI/180.0) * -15));
		}
	#endif
	#endif
		controlPanel->window.show();
	}

#if WINDOW_IS_3D
	// Update registered spatial audio source transform for the control panel.
	if (controlPanel_spatialAudioSource != nullptr)
		spatialAudioSystem->setSourceTransform(controlPanel_spatialAudioSource, controlPanel->window.getTransform());
#endif

	// Update the virtual desktop.
	framework.tickVirtualDesktop(viewToWorld, true, buttonMask, isHand);

	controlPanel->tick(dt);

// todo : use active pointer
	vrPointer[0].wantsToVibrate |= controlPanel->hoveredIdChanged && (controlPanel->lastHoveredId != 0);

	// Check if pointer beams are intersecting. If so, give some feedback (vibration, particles)
	pointerBeamsEffect.setActive(false);
	if (vrPointer[0].hasTransform && vrPointer[1].hasTransform)
	{
		const Mat4x4 transform1 = vrPointer[0].getTransform(playerLocation);
		const Mat4x4 transform2 = vrPointer[1].getTransform(playerLocation);
		
		Vec3 closest1;
		Vec3 closest2;
		if (calculate_nearest_points_for_ray_vs_ray(
			transform1.GetTranslation(), transform1.GetAxis(2),
			transform2.GetTranslation(), transform2.GetAxis(2),
			closest1, closest2))
		{
			const float distance = (closest2 - closest1).CalcSize();

			if (distance <= .02f)
			{
				vrPointer[0].wantsToVibrate = true;
				vrPointer[1].wantsToVibrate = true;

				if (models.empty() == false)
				{
					const Vec3 midpoint = (closest1 + closest2) / 2.f;

					pointerBeamsEffect.setActive(true);
					Vec3 position = midpoint;
					position *= 100.f; // fixme : scale particle effects
					pointerBeamsEffect.setPosition(position[0], position[1], position[2], false);
				}
			}
		}
	}

	// update watersim object
	{
		// update resolution

		if (resolution->get() != watersim.numElems)
		{
			watersim.shut();
			watersim.init(resolution->get());

			watersim.randomize();
		}

		watersim_transform = Mat4x4(true)
			.Translate(0, transformHeight->get(), 0)
			.Scale(transformScale->get(), 1.f, transformScale->get())
			.Translate(
				-(watersim.numElems - 1) / 2.f,
				0.f,
				-(watersim.numElems - 1) / 2.f);

		// update interaction with hands

		if ((rand() % 100) == 0)
		{
			watersim.doGaussianImpact(
				rand() % watersim.numElems,
				rand() % watersim.numElems,
				3,
				1.f, 1.f);
		}

		// update physics

		watersim.tick(
			dt,                    // time step
			tension->get(),        // tension
			velocityRetain->get(), // velocity retain
			positionRetain->get(), // position retain
			true);                 // closed ends (non-wrapped at the edges)
	}

	for (auto & moon : moons)
	{
		moon.tick(dt);
	}

	for (auto & drifter : drifters)
	{
		drifter.tick(dt);
	}

	for (auto & audioStreamer : audioStreamers)
	{
		audioStreamer.tick(dt);
	}
	
	for (auto & model : models)
	{
		model.tick(dt);
	}
	
	for (auto & jgplayer : jgplayers)
	{
		jgplayer.tick(dt);
	}
	
	for (auto & particleEffect : particleEffects)
	{
		particleEffect.tick(dt);
	}
	
	particleEffectSystem.tick(0.f, -10.f, 0.f, dt);
}

void Scene::drawOnce() const
{
	controlPanel->draw();
}

void Scene::drawOpaque() const
{
	const double time = framework.time;

	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);

	forwardLightingSystem->beginScene(worldToView);

	if (moons.empty())
	{
		forwardLightingSystem->setSunProperties(
			true,
			Vec3(0, -100, 0),
			Vec3(1.f),
			1.f);
	}
	else
	{
		auto & moon = moons.front();

		forwardLightingSystem->setSunProperties(
			true,
			moon.getPosition(),
			Vec3(1.f),
			lightIntensity->get());
	}

#if true
	drawWatersim();
	drawWatersimHandProjections();
#endif

	for (auto & moon : moons)
	{
		moon.drawOpaque();
	}

	for (auto & celestialSphere : celestialSpheres)
	{
		celestialSphere.drawOpaque();
	}

	for (auto & drifter : drifters)
	{
		drifter.drawOpaque();
	}

	for (auto & audioStreamer : audioStreamers)
	{
		audioStreamer.drawOpaque();
	}

	for (auto & model : models)
	{
		model.drawOpaque();
	}
	
	for (auto & jgplayer : jgplayers)
	{
		jgplayer.drawOpaque();
	}

#if WINDOW_IS_3D
#if true
	auto windows = framework.getAllWindows();
	
	Shader metallicRoughnessShader("pbr-metallicRoughness-simple");
	setShader(metallicRoughnessShader);
	{
		forwardLightingSystem->setLightingForGltfMaterialShader(metallicRoughnessShader);

		gltf::MetallicRoughnessParams params;
		params.init(metallicRoughnessShader);

		gltf::Material material;
		gltf::Scene scene;
		int nextTextureUnit = 0;
		params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);
		
		for (auto * window : windows)
		{
			if (window->hasSurface() && window->isHidden() == false)
			{
				params.setMetallicRoughness(metallicRoughnessShader, materialMetallic->get(), materialRoughness->get());
				params.setBaseColorTexture(metallicRoughnessShader, window->getColorTarget()->getTextureId(), 0, nextTextureUnit);

				gxPushMatrix();
				{
					gxMultMatrixf(window->getTransformForDraw().m_v);
					gxTranslatef(0, 0, .01f);

					pushCullMode(CULL_BACK, CULL_CCW);
					{
						setColor(colorWhite);
						fillCube(
							Vec3(window->getWidth()/2.f, window->getHeight()/2.f, 0),
							Vec3(window->getWidth()/2.f + 10, window->getHeight()/2.f + 10, .008f));
					}
					popCullMode();
				}
				gxPopMatrix();

				metallicRoughnessShader.setImmediate("scene_ambientLightColor",
					window->hasFocus() ? .3f : 0.f,
					window->hasFocus() ? .3f : 0.f,
					window->hasFocus() ? .3f : 0.f);

				window->draw3d();
			}
		}
	}
	clearShader();

	for (auto * window : windows)
	{
		if (window->hasSurface() && window->isHidden() == false && window->hasFocus())
		{
		#if false
			// draw magnification bubble around cursor position
			float mouseX;
			float mouseY;
			if (window->getMousePosition(mouseX, mouseY))
			{
				gxPushMatrix();
				{
					const float sampleRadius = 10.f;
					const float drawRadius = 20.f;

					gxMultMatrixf(window->getTransformForDraw().m_v);
					gxTranslatef(0, 0, .001f);
					gxTranslatef(mouseX, window->getHeight() - mouseY, 0);
					gxScalef(drawRadius, drawRadius, 1);
					
					setColor(colorWhite);
					gxSetTexture(window->getColorTarget()->getTextureId());
					
					const float x1 = mouseX - sampleRadius;
					const float y1 = mouseY - sampleRadius;
					const float x2 = mouseX + sampleRadius;
					const float y2 = mouseY + sampleRadius;
					
					const float u1 = x1 / window->getWidth();
					const float v1 = y1 / window->getHeight();
					const float u2 = x2 / window->getWidth();
					const float v2 = y2 / window->getHeight();
					
					gxBegin(GX_QUADS);
					{
						gxNormal3f(0, 0, 1);
						
						gxTexCoord2f(u1, v1); gxVertex2f(-1, +1);
						gxTexCoord2f(u2, v1); gxVertex2f(+1, +1);
						gxTexCoord2f(u2, v2); gxVertex2f(+1, -1);
						gxTexCoord2f(u1, v2); gxVertex2f(-1, -1);
					}
					gxEnd();
					
					gxSetTexture(0);
				}
				gxPopMatrix();
			}
		#endif
			
			setColor(colorWhite);
			window->draw3dCursor();
		}
	}
#else
	setColor(colorWhite);
	framework.drawVirtualDesktop();
#endif
#endif

#if true
	// Draw controllers.
	for (int i = 0; i < VrSide_COUNT; ++i)
	{
		auto & pointer = vrPointer[i];

		if (pointer.hasTransform == false)
			continue;

		// Draw cube.
		gxPushMatrix();
		{
			gxMultMatrixf(pointer.getTransform(playerLocation).m_v);

			pushCullMode(CULL_BACK, CULL_CCW);
			Shader metallicRoughnessShader("pbr-metallicRoughness-simple");
			setShader(metallicRoughnessShader);
			{
				forwardLightingSystem->setLightingForGltfMaterialShader(metallicRoughnessShader);

				gltf::MetallicRoughnessParams params;
				params.init(metallicRoughnessShader);

				gltf::Material material;
				material.pbrMetallicRoughness.baseColorFactor = Color(255, 127, 63, 255);

				gltf::Scene scene;
				int nextTextureUnit = 0;
				params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);

			#if 1
				params.setMetallicRoughness(metallicRoughnessShader, .1f, .2f);

				params.setBaseColor(metallicRoughnessShader, Color(255, 0, 0));
				fillCube(Vec3(0, 0, 0), Vec3(.04f, .01f, .01f));

				params.setBaseColor(metallicRoughnessShader, Color(0, 255, 0));
				fillCube(Vec3(0, 0, 0), Vec3(.01f, .04f, .01f));

				params.setBaseColor(metallicRoughnessShader, Color(0, 0, 255));
				fillCube(Vec3(0, 0, .1f), Vec3(.01f, .01f, .1f));
			#else
				params.setMetallicRoughness(metallicRoughnessShader, .8f, .2f);
				fillCube(Vec3(0, 0, .05f), Vec3(.02f, .02f, .1f));
			#endif
			}
			clearShader();
			popCullMode();

		// todo : draw in translucent pass. todo : process and remember pointer state during tick
			// Draw pointer ray.
			pushCullMode(CULL_BACK, CULL_CCW);
			pushBlend(BLEND_ADD);
			const float a = lerp<float>(.02f, .04f, (sin(time) + 1.0) / 2.0);
			setColorf(.5f, .8f, 1.f, a);
			fillCube(Vec3(0, 0, 100), Vec3(.004f, .004f, 100));
			popBlend();
			popCullMode();
		}
		gxPopMatrix();
	}
#endif

#if true
	// Draw hands.
	for (int i = 0; i < VrSide_COUNT; ++i)
	{
		auto & hand = vrHand[i];

		if (!hand.hasDeform)
			continue;

		gxPushMatrix();
		{
			// Apply the root pose.
			gxMultMatrixf(hand.getTransform(playerLocation).m_v);

			pushCullMode(CULL_BACK, CULL_CCW);
			Shader metallicRoughnessShader("pbr-metallicRoughness-skinned");
			setShader(metallicRoughnessShader);
			{
				forwardLightingSystem->setLightingForGltfMaterialShader(metallicRoughnessShader);

				gltf::MetallicRoughnessParams params;
				params.init(metallicRoughnessShader);

				gltf::Material material;
				material.pbrMetallicRoughness.baseColorFactor = Color(255, 127, 63, 255);

				gltf::Scene scene;
				int nextTextureUnit = 0;
				params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);
				params.setMetallicRoughness(metallicRoughnessShader, .8f, .2f);

				auto & skinningData = vrHand[i].getSkinningMatrices(true);
				metallicRoughnessShader.setBuffer("SkinningData", skinningData);

				vrHand[i].drawMesh();
			}
			clearShader();
			popCullMode();
		}
		gxPopMatrix();
	}
#endif
}

void Scene::drawTranslucent() const
{
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);

	forwardLightingSystem->beginScene(worldToView);
	
	gxPushMatrix();
	{
		gxScalef(1.f / 100.f, 1.f / 100.f, 1.f / 100.f);
		
		particleEffectSystem.draw();
	}
	gxPopMatrix();
}

void Scene::drawWatersim() const
{
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);

	// todo : convert watersim values to texture, create texture, and use a static mesh to sample from it
	//        using a shader. this avoids using the GX api to draw dynamic geometry

	// Draw watersim object.
	gxPushMatrix();
	{
		gxMultMatrixf(watersim_transform.m_v);

		Shader metallicRoughnessShader("pbr-metallicRoughness-simple");
		setShader(metallicRoughnessShader);
		{
			forwardLightingSystem->setLightingForGltfMaterialShader(metallicRoughnessShader);

			gltf::MetallicRoughnessParams params;
			params.init(metallicRoughnessShader);

			gltf::Material material;
			gltf::Scene scene;
			int nextTextureUnit = 0;
			params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);

			params.setMetallicRoughness(metallicRoughnessShader, materialMetallic->get(), materialRoughness->get());
			params.setBaseColorTexture(metallicRoughnessShader, getTexture("texture.png"), 0, nextTextureUnit);

			setColor(255, 200, 180, 180);
			gxBegin(GX_QUADS);
			{
				for (int x = 0; x < watersim.numElems - 1; ++x)
				{
					for (int z = 0; z < watersim.numElems - 1; ++z)
					{
						float normal[3];
						calculateNormal(watersim, x, z, normal);

						const float y1 = watersim.p[x + 0][z + 0];
						const float y2 = watersim.p[x + 1][z + 0];
						const float y3 = watersim.p[x + 1][z + 1];
						const float y4 = watersim.p[x + 0][z + 1];

						gxNormal3fv(normal);
						gxTexCoord2f(0, 0); gxVertex3f(x + 0, y1, z + 0);
						gxTexCoord2f(1, 0); gxVertex3f(x + 1, y2, z + 0);
						gxTexCoord2f(1, 1); gxVertex3f(x + 1, y3, z + 1);
						gxTexCoord2f(0, 1); gxVertex3f(x + 0, y4, z + 1);
					}
				}
			}
			gxEnd();
		}
		clearShader();
	}
	gxPopMatrix();
}

void Scene::drawWatersimHandProjections() const
{
	for (int i = 0; i < VrSide_COUNT; ++i)
	{
		if (!vrHand[i].hasDeform)
			continue;

		const Vec3 position_world = vrHand[i].getTransform(playerLocation).GetTranslation();
		const Vec3 direction_world(0, -1, 0);

		float distance;
		Vec3 normal(false);
		if (!raycast(position_world, direction_world, kCollisionFlag_Watersim, distance, normal))
			continue;

		const Vec3 projected_world = position_world + direction_world * distance;

		gxPushMatrix();
		{
			const Vec3 drawOffset(0.f, .1f, 0.f);
			const Vec3 drawPosition = projected_world + drawOffset;
			gxTranslatef(drawPosition[0], drawPosition[1], drawPosition[2]);
			gxRotatef(90, 1, 0, 0);

			setColor(colorWhite);
			drawCircle(0, 0, lerp<float>(.1f, .2f, fmodf(framework.time / 2.4f, 1.f)), 100);
		}
		gxPopMatrix();
	}
}

// -- SpatialAudioSystem

// todo : use HD audio output and stream

#include "limiter.h"

struct SpatialAudioSystemAudioStream : AudioStream
{
	Limiter limiter;
	
	virtual int Provide(int numSamples, AudioSample * __restrict samples) override final
	{
		ALIGN16 float outputSamplesL[numSamples];
		ALIGN16 float outputSamplesR[numSamples];

		// note : we update the audio in steps of 32 samples, to improve the timing for notes when the sample size is
		//        large. hopefully this fixes strange timing issues during Oculus Vr movie recordings. also this will
		//        reduce (the maximum) binauralization latency by a little and make it more stable/less jittery
		for (int i = 0; i < numSamples; )
		{
			const int kUpdateSize = 32;
			const int numSamplesRemaining = numSamples - i;
			const int numSamplesThisUpdate =
				numSamplesRemaining < kUpdateSize
					? numSamplesRemaining
					: kUpdateSize;

			spatialAudioSystem->generateLR(
				outputSamplesL + i,
				outputSamplesR + i,
				numSamplesThisUpdate);
			
			i += numSamplesThisUpdate;
		}
		
	#if AUDIO_BUFFER_VALIDATION
		for (int i = 0; i < numSamples; ++i)
			Assert(isfinite(outputSamplesL[i]) && isfinite(outputSamplesR[i]));
	#endif
		
		limiter.chunk_analyze(outputSamplesL, numSamples, 16);
		limiter.chunk_analyze(outputSamplesR, numSamples, 16);
		limiter.chunk_applyInPlace(outputSamplesL, numSamples, 16, 1.f);
		limiter.chunk_applyInPlace(outputSamplesR, numSamples, 16, 1.f);
		limiter.chunk_end(powf(.995f, numSamples/256.f));
		
		const float scale_s16 = (1 << 15) - 1;
		
		for (int i = 0; i < numSamples; ++i)
		{
			const int32_t valueL = int32_t(outputSamplesL[i] * scale_s16);
			const int32_t valueR = int32_t(outputSamplesR[i] * scale_s16);
			
			Assert(valueL >= -((1 << 15) - 1) && valueL <= +((1 << 15) - 1));
			Assert(valueR >= -((1 << 15) - 1) && valueR <= +((1 << 15) - 1));
			
			samples[i].channel[0] = valueL;
			samples[i].channel[1] = valueR;
		}

		return numSamples;
	}
};

// -- ControlPanel implementation

void ControlPanel::tick(const float dt)
{
	if (doWindowResize)
	{
		doWindowResize = false;
			
		int sx = 0;
		int sy = 0;
		
		if (activeTab == kTab_Scene || activeTab == kTab_Audio || activeTab == kTab_Tracker)
		{
			sx = 340;
			sy = 340;
		}
		else if (activeTab == kTab_ParticleEditor)
		{
			sx = 1024;
			sy = 1024;
		}
		else
		{
			Assert(false);
		}
		
		window.setSize(sx, sy);
	}
	
	pushWindow(window);
	{
		bool inputIsCaptured = false;
		guiContext.processBegin(.01f, window.getWidth(), window.getHeight(), inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(window.getWidth(), window.getHeight()));

			if (ImGui::Begin("Control Panel", nullptr,
				ImGuiWindowFlags_MenuBar |
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoCollapse))
			{
				if (ImGui::BeginMenuBar())
				{
					auto previousTab = activeTab;
					
					if (ImGui::Button("Scene"))
						activeTab = kTab_Scene;
					if (ImGui::Button("Audio"))
						activeTab = kTab_Audio;
					if (ImGui::Button("Tracker"))
						activeTab = kTab_Tracker;
					if (ImGui::Button("Particle Editor"))
						activeTab = kTab_ParticleEditor;
					
					if (activeTab != previousTab && (activeTab == kTab_ParticleEditor || previousTab == kTab_ParticleEditor))
					{
						doWindowResize = true;
					}
					
					ImGui::EndMenuBar();
				}
				
				if (activeTab == kTab_Scene)
				{
					if (ImGui::Button("(R) drifters"))
					{
						for (auto & drifter : scene->drifters)
						{
							const float x = random<float>(-4.f, +4.f);
							const float z = random<float>(-4.f, +4.f);

							drifter.transform.SetTranslation(x, 0, z);

							drifter.effectLibrary.setPositionDiscontinuity();
						}
					}
					ImGui::SameLine();

					if (ImGui::Button("(R) streamers"))
					{
						for (auto & audioStreamer : scene->audioStreamers)
						{
							const float x = random<float>(-8.f, +8.f);
							const float z = random<float>(-8.f, +8.f);

							audioStreamer.initialPosition.Set(x, .4f, z);
						}
					}

					parameterUi::doParameterUi_recursive(scene->parameterMgr, nullptr);
				}
				else if (activeTab == kTab_Audio)
				{
					parameterUi::doParameterUi_recursive(spatialAudioSystem->parameterMgr, nullptr);
					
					auto * spatialAudioSystemImpl = (SpatialAudioSystem_Binaural*)spatialAudioSystem;
					spatialAudioSystemImpl->sources_mutex.lock();
					{
						for (auto * source = spatialAudioSystemImpl->sources; source != nullptr; source = source->next)
						{
							ImGui::Text("el: %+07.2f, az: %+07.2f, in: %.2f, rd: %04.2f, hr: %05.2f",
								source->elevation.load(),
								source->azimuth.load(),
								source->intensity.load(),
								source->recordedDistance,
								source->headroomInDb);
						}
					}
					spatialAudioSystemImpl->sources_mutex.unlock();
				}
				else if (activeTab == kTab_Tracker)
				{
					for (auto & jgplayer : scene->jgplayers)
					{
						if (ImGui::Button("Prev track"))
							jgplayer.player->prev_track();
						ImGui::SameLine();
						if (ImGui::Button("Next next"))
							jgplayer.player->next_track();

						if (jgplayer.player->is_paused())
						{
							if (ImGui::Button("Resume"))
								jgplayer.player->resume();
						}
						else
						{
							if (ImGui::Button("Pause"))
								jgplayer.player->pause();
						}
					}
				}
			}
			ImGui::End();

			hoveredIdChanged = (GImGui->HoveredId != lastHoveredId);
			lastHoveredId = GImGui->HoveredId;
		}
		guiContext.processEnd();
		
		if (activeTab == kTab_ParticleEditor)
		{
			particleEditor.tick(true, window.getWidth(), window.getHeight(), dt);
		}
	}
	popWindow();
}

// todo : move to a separate file
struct FastDelayLineForSmallChunks
{
	float * samples = nullptr;
	int numSamples = 0;

	void alloc(const int in_numSamples)
	{
		samples = new float[in_numSamples];
		numSamples = in_numSamples;
	}

	void free()
	{
		delete [] samples;
		samples = nullptr;

		numSamples = 0;
	}

	float * prepareNextChunk(const int numSamplesToProvide)
	{
		const int numSamplesToKeep = numSamples - numSamplesToProvide;

		memcpy(
			samples,
			samples + numSamples - numSamplesToKeep,
			numSamplesToKeep * sizeof(float));

		return samples + numSamplesToKeep;
	}

	void readChunkWithInterpolation(const float delayInSamples, const int numSamplesToRead, float * out_samples)
	{
		// note : the -eps is because we read +1 sample during linear interpolation, and we don't want to read
		//        past our own sample array. this ensured the sample index, when rounded down, is one less
		//        than the last sample index, and the 't' value used during interpolation is correct (0.99..)
		const float eps = 1e-3f;
		const float indexOfFirstSampleToRead = ((numSamples - numSamplesToRead) - delayInSamples) - eps;
		Assert(indexOfFirstSampleToRead >= 0 && indexOfFirstSampleToRead < numSamples - numSamplesToRead);

		int firstIndexRoundedDownToInt = int(indexOfFirstSampleToRead);
		Assert(firstIndexRoundedDownToInt >= 0 && firstIndexRoundedDownToInt <= numSamples - numSamplesToRead - 1);

		const float t = indexOfFirstSampleToRead - firstIndexRoundedDownToInt;
		Assert(t >= 0.f && t <= 1.f);

		const float t1 = 1.f - t;
		const float t2 = t;

		for (int i = 0; i < numSamplesToRead; ++i)
		{
			const int index1 = firstIndexRoundedDownToInt + i;
			const int index2 = firstIndexRoundedDownToInt + i + 1;

			Assert(index1 >= 0 && index1 < numSamples);
			Assert(index2 >= 0 && index2 < numSamples);

			const float sample1 = samples[index1];
			const float sample2 = samples[index2];

			out_samples[i] = sample1 * t1 + sample2 * t2;
		}
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.vrMode = true;
	framework.enableVrMovement = FRAMEWORK_IS_NATIVE_VR ? false : true;
	framework.enableDepthBuffer = true;

	if (!framework.init(800, 600))
		return -1;
	
	initUi();
	
	auto * spatialAudioSystemImpl = new SpatialAudioSystem_Binaural("binaural");
	spatialAudioSystem = spatialAudioSystemImpl;

	Scene scene;

	scene.create();

	AudioOutput_Native audioOutput;
	audioOutput.Initialize(2, SAMPLE_RATE, 256);

	SpatialAudioSystemAudioStream audioStream;
	audioOutput.Play(&audioStream);

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		const float dt = fminf(framework.timeStep, 1/30.f);
		
		// Tick the simulation
		scene.tick(dt);

		// Update the listener transform for the spatial audio system.
		const Mat4x4 listenerTransform =
			Mat4x4(true)
				.Translate(scene.playerLocation)
				.Mul(framework.getHeadTransform());
		spatialAudioSystem->setListenerTransform(listenerTransform);

		// Update the panning for the spatial audio system. This basically tells the spatial audio system we're done making a batch of modifications to the audio source and listener transforms, and tell it to update panning.
		spatialAudioSystem->updatePanning();

		// Render the stuff we need to draw only once (shared for each eye).
		scene.drawOnce();

		// Render the eye images.
		for (int eyeIndex = 0; eyeIndex < framework.getEyeCount(); ++eyeIndex)
		{
			framework.beginEye(eyeIndex, colorBlack);
			{
				gxPushMatrix();
				gxTranslatef(
					-scene.playerLocation[0],
					-scene.playerLocation[1],
					-scene.playerLocation[2]);
				{
					pushDepthTest(true, DEPTH_LESS);
					pushBlend(BLEND_OPAQUE);
					{
						scene.drawOpaque();
					}
					popBlend();
					popDepthTest();

					pushDepthTest(true, DEPTH_LESS, false);
					pushBlend(BLEND_ALPHA);
					{
						scene.drawTranslucent();
					}
					popBlend();
					popDepthTest();
				}
				gxPopMatrix();
			}
			framework.endEye();
		}

		framework.present();
	}

	scene.destroy();

	shutUi();
	
	Font("calibri.ttf").saveCache();

	framework.shutdown();

	return 0;
}

static bool calculate_nearest_points_for_ray_vs_ray(
	Vec3Arg p1,
	Vec3Arg d1,
	Vec3Arg p2,
	Vec3Arg d2,
	Vec3 & out_closest1,
	Vec3 & out_closest2)
{
	const float SMALL_NUM = 1e-6f;

	//const Vec3 u = L1.P1 - L1.P0;
	//const Vec3 v = L2.P1 - L2.P0;
	const Vec3 & u = d1;
	const Vec3 & v = d2;
	//const Vec3 w = L1.P0 - L2.P0;
	const Vec3 w = p1 - p2;
    const float a = u * u;         // always >= 0
	const float b = u * v;
	const float c = v * v;         // always >= 0
	const float d = u * w;
	const float e = v * w;
	const float D = a*c - b*b;     // always >= 0

	float sc;
	float tc;

    // compute the line parameters of the two closest points
    if (D < SMALL_NUM) {           // the lines are almost parallel
        sc = 0.0;
        tc = (b>c ? d/b : e/c);    // use the largest denominator
    }
    else {
        sc = (b*e - c*d) / D;
        tc = (a*e - b*d) / D;
    }

	out_closest1 = p1 + u * sc;
	out_closest2 = p2 + v * tc;

    return sc >= 0.f && tc >= 0.f;
}
