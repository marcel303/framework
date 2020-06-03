#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"

#include "parameter.h"
#include "parameterUi.h"

#include "data/engine/ShaderCommon.txt" // VS_ constants
#include "framework.h"
#include "framework-ovr.h"
#include "framework-vr-hands.h"
#include "gx_render.h"
#include "internal.h"

#include "imgui-framework.h"
#include "imgui_internal.h"

#include "watersim.h"

#include "audiooutput/AudioOutput_Native.h"
#include "binauralizer.h"
#include "binaural_oalsoft.h"

#include "objects/audioSourceVorbis.h"
#include "objects/delayLine.h"

#include "Quat.h"

/*

new challenges!

DONE : automatically switch between controller and hands for virtual desktop interaction

todo : use finger gesture or controller button to open/close control panel
    - check finger pinching amount for the index finger
    - have pinch enter/leave thresholds (where leave > enter), to avoid rapid transitions
    - attach control panel to hand location on pinch

todo : for testing : save/load parameters on startup/exit

todo : experiment with drawing window contents in 3d directly (no window surface). this will give higher quality results on outlines

todo : draw virtual desktop using a custom shader, to give the windows some specular reflection
    - issue : no way to control which texture uniform gets set. pass texture name to drawVirtualDesktop ?
    - maybe just custom draw all windows (it's easy enough)
    - problem : need to iterate windows

todo : refactor controller state similar to hand state

DONE : project hand position down onto watersim, to give the user an exaggered impression of how high up they are
    - calculate inverse of the watersim transform
    - use the inverse transform, to transform hand location from world-space into watersim-space
    - use xz coordinates to lookup height (xyz) position
    - transform xyz position back into world-space and draw a circle or something

todo : apply a texture or color to the watersim quads

todo : determine nice watersim params and update defaults

DONE : binauralize some sounds

DONE : polish virtual desktop interaction when using hands,
    + tell virtual desktop which type of interaction is used (beam or finger),
    + let virtual desktop have detection of false finger presses

todo : add UI for showing spatial audio system status

DONE : add the moon

todo : add some stars

todo : add some particles drifiting in the air
	- particles everywhere

todo : make NdotV abs(..) inside the gltf pbr shaders. this would make lighting act double-sidedly

todo : add 'assetcopy-filelist.txt' or something, which stores the hashes for copied files
    - during assetcopy: optionally enable filelist check
    - calculate hash for assets inside apk
    - check if changed from filelist

todo : window3d : draw zoom bubble at cursor location
todo : framework : draw pointer beams when manual vr mode is off
    - requires a unified place to store pointer transforms, and active pointer index

 */

#if FRAMEWORK_USE_OVR_MOBILE
	#include <VrApi_Helpers.h>
	#include <VrApi_Input.h>
#endif

#include <algorithm> // std::find
#include <math.h>
#include <mutex>

struct Scene;

struct ControlPanel
{
	Window window;
	FrameworkImGuiContext guiContext;

	ParameterMgr * parameterMgr = nullptr;
	Scene * scene = nullptr;

	ImGuiID lastHoveredId = 0;
	bool hoveredIdChanged = false;

	ControlPanel(const Vec3 position, const float angle, ParameterMgr * in_parameterMgr, Scene * in_scene)
		: window("Window", 340, 340)
	{
	#if WINDOW_IS_3D
		const Mat4x4 transform =
			Mat4x4(true)
				.RotateY(angle)
				.Translate(position);
		window.setTransform(transform);
	#endif

		guiContext.init(false);

		parameterMgr = in_parameterMgr;
		scene = in_scene;
	}

	~ControlPanel()
	{
		guiContext.shut();
	}

	void tick();

	void draw()
	{
		pushWindow(window);
		{
			framework.beginDraw(20, 20, 20, 255);
			{
				guiContext.draw();
			}
			framework.endDraw();
		}
		popWindow();
	}
};

struct PointerObject
{
	Mat4x4 transform = Mat4x4(true);
	bool isValid = false;

	bool isDown[2] = { };
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

struct SpatialAudioSystemInterface
{
	virtual void * addSource(const Mat4x4 & transform, AudioSource * audioSource) = 0;
	virtual void removeSource(void * source) = 0;

	virtual void setSourceTransform(void * source, const Mat4x4 & transform) = 0;

	virtual void setListenerTransform(const Mat4x4 & transform) = 0;

	virtual void updatePanning() = 0;

	virtual void generateLR(
		float * __restrict outputSamplesL,
		float * __restrict outputSamplesR,
		const int numSamples) = 0;
};

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

		transform.SetTranslation(position + speed * dt);
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

		spatialAudioSource = spatialAudioSystem->addSource(transform, audioSource);
	}

	void shut()
	{
		delete audioSource;
		audioSource = nullptr;
	}

	void tick(const float dt)
	{
		transform = Mat4x4(true).Translate(initialPosition).Translate(0.f, sinf(framework.time / 2.34f) * .06f, 0.f);

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

	void init()
	{
		transform.SetTranslation(
			random<float>(-4.f, +4.f),
			.5f,
			random<float>(-4.f, +4.f));

		gltf::loadScene("Suzanne/glTF/Suzanne.gltf", scene);
		bufferCache.init(scene);
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

			gltf::drawScene(scene, &bufferCache, materialShaders, true, &drawOptions);
		}
		gxPopMatrix();
	}
};

// -- scene

struct Scene : CollisionSystemInterface, ForwardLightingSystemInterface
{
	Vec3 playerLocation;

	PointerObject pointers[2];

	VrHand hands[VrHand_COUNT];

	ControlPanel * controlPanel = nullptr;
	bool showControlPanel = true;
	AudioSourceVorbis controlPanel_audioSource;
	void * controlPanel_spatialAudioSource = nullptr;

	ParameterMgr parameterMgr;
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

	ParameterMgr parameterMgr_binaural;
	ParameterFloat * binauralElevationBase = nullptr;
	ParameterFloat * binauralAzimuthBase = nullptr;
	ParameterVec3 * binauralSourcePos_listener = nullptr;

	Mat4x4 watersim_transform = Mat4x4(true);
	Watersim watersim;

	std::vector<MoonObject> moons;

	std::vector<CelestialSphereObject> celestialSpheres;

	std::vector<DrifterObject> drifters;

	std::vector<AudioStreamerObject> audioStreamers;

	std::vector<ModelObject> models;

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

	for (int i = 0; i < 2; ++i)
	{
		hands[i].init((VrHands)i);
	}

	controlPanel = new ControlPanel(Vec3(0, 1.5f, -.45f), 0.f, &parameterMgr, this);
#if WINDOW_IS_3D
	controlPanel_audioSource.open("180328-004.ogg", true);
	controlPanel_spatialAudioSource = spatialAudioSystem->addSource(controlPanel->window.getTransform(), &controlPanel_audioSource);
#endif

	// add watersim object
	resolution = parameterMgr.addInt("resolution", 16);
	resolution->setLimits(2, Watersim::kMaxElems);
	tension = parameterMgr.addFloat("tension", 10.f);
	tension->setLimits(1.f, 100.f);
	velocityRetain = parameterMgr.addFloat("velocityRetain", .1f);
	velocityRetain->setLimits(.01f, .99f);
	positionRetain = parameterMgr.addFloat("positionRetain", .1f);
	positionRetain->setLimits(.01f, .99f);
	transformHeight = parameterMgr.addFloat("transformHeight", 0.f);
	transformHeight->setLimits(-8.f, +8.f);
	transformScale = parameterMgr.addFloat("transformScale", 1.f);
	transformScale->setLimits(.1f, 2.f);
	watersim.init(resolution->get());

	parameterMgr_material.setPrefix("material");
	materialMetallic = parameterMgr_material.addFloat("materialMetallic", .8f);
	materialMetallic->setLimits(0.f, 1.f);
	materialRoughness = parameterMgr_material.addFloat("materialRoughness", .8f);
	materialRoughness->setLimits(0.f, 1.f);
	parameterMgr.addChild(&parameterMgr_material);

	parameterMgr_lighting.setPrefix("lighting");
	lightIntensity = parameterMgr_lighting.addFloat("lightIntensity", 1.f);
	lightIntensity->setLimits(0.f, 2.f);
	parameterMgr.addChild(&parameterMgr_lighting);

	parameterMgr_binaural.setPrefix("binaural");
	binauralElevationBase = parameterMgr_binaural.addFloat("elevationBase", 0.f);
	binauralElevationBase->setLimits(-90.f, +90.f);
	binauralAzimuthBase = parameterMgr_binaural.addFloat("azimuthBase", 0.f);
	binauralAzimuthBase->setLimits(-180.f, +180.f);
	binauralSourcePos_listener = parameterMgr_binaural.addVec3("sourcePos_listener", Vec3(0.f));
	parameterMgr.addChild(&parameterMgr_binaural);

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

	//models.resize(1);

	for (auto & model : models)
	{
		model.init();
	}
}

void Scene::destroy()
{
	watersim.shut();

	for (auto & hand : hands)
		hand.shut();

	delete controlPanel;
	controlPanel = nullptr;
}

void Scene::tick(const float dt)
{
	// update input state

	for (auto & hand : hands)
	{
		hand.updateInputState();
	}

#if FRAMEWORK_USE_OVR_MOBILE
	ovrMobile * ovr = frameworkOvr.Ovr;
	
// todo : add VrController to framework-vr shared library
	uint32_t index = 0;

	for (;;)
	{
		ovrInputCapabilityHeader header;

		if (vrapi_EnumerateInputDevices(ovr, index++, &header) < 0)
			break;

		if (header.Type == ovrControllerType_TrackedRemote)
		{
			bool vibrate = false;

			vibrate |= controlPanel->hoveredIdChanged && controlPanel->lastHoveredId != 0;

			ovrTracking tracking;
			if (vrapi_GetInputTrackingState(ovr, header.DeviceID, frameworkOvr.PredictedDisplayTime, &tracking) != ovrSuccess)
				tracking.Status = 0;

			ovrInputStateTrackedRemote state;
			state.Header.ControllerType = ovrControllerType_TrackedRemote;
			if (vrapi_GetCurrentInputState(ovr, header.DeviceID, &state.Header ) >= 0)
			{
				int index = -1;

				ovrInputTrackedRemoteCapabilities remoteCaps;
				remoteCaps.Header.Type = ovrControllerType_TrackedRemote;
				remoteCaps.Header.DeviceID = header.DeviceID;
				if (vrapi_GetInputDeviceCapabilities(ovr, &remoteCaps.Header) == ovrSuccess)
				{
					if (remoteCaps.ControllerCapabilities & ovrControllerCaps_LeftHand)
						index = 0;
					if (remoteCaps.ControllerCapabilities & ovrControllerCaps_RightHand)
						index = 1;
				}

				if (index != -1)
				{
					auto & pointer = pointers[index];

					if (tracking.Status & VRAPI_TRACKING_STATUS_POSITION_VALID)
					{
						ovrMatrix4f transform = vrapi_GetTransformFromPose(&tracking.HeadPose.Pose);
						transform = ovrMatrix4f_Transpose(&transform);

						memcpy(&pointer.transform, (float*)transform.M, sizeof(Mat4x4));
						pointer.isValid = true;
					}
					else
					{
						pointer.transform.MakeIdentity();
						pointer.isValid = false;
					}

					const int buttonMasks[2] = { ovrButton_Trigger, ovrButton_GripTrigger };

					for (int i = 0; i < 2; ++i)
					{
						const bool wasDown = pointer.isDown[i];

						if (state.Buttons & buttonMasks[i])
							pointer.isDown[i] = true;
						else
							pointer.isDown[i] = false;

						if (pointer.isDown[i] != wasDown)
						{
							// todo : vrapi_SetHapticVibrationSimple(ovrMobile* ovr, const ovrDeviceID deviceID, const float intensity)
							vibrate = true;

							if (i == 0 && index == 1 && pointer.isDown[i] && pointer.isValid)
							{
								playerLocation += pointer.transform.GetAxis(2) * -1.f;
							}
						}
					}
				}
			}

			vrapi_SetHapticVibrationSimple(ovr, header.DeviceID, vibrate ? .5f : 0.f);
		}
	}
#endif

	// update windows

	Mat4x4 viewToWorld(true);
	int buttonMask = 0;
	bool isHand = false;

	const VrHands interactiveHand = VrHand_Right;
// todo : automatically switch between left and right hand. or allow both to interact at the 'same' time ?
	if (hands[interactiveHand].getFingerTransform(VrFinger_Index, playerLocation, viewToWorld))
	{
		buttonMask = 1 << 0;
		isHand = true;
	}
	else
	{
		viewToWorld = Mat4x4(true).Translate(playerLocation).Mul(pointers[0].transform);
		buttonMask =
			(pointers[0].isDown[0] << 0) |
			(pointers[0].isDown[1] << 1);
		isHand = false;

	#if WINDOW_IS_3D
	#if false
		controlPanel->window.setTransform(
			Mat4x4(true)
				.Translate(playerLocation)
				.Mul(pointers[1].transform));
	#else
		if (pointers[1].isDown[1])
		{
			controlPanel->window.setTransform(
				Mat4x4(true)
					.Translate(playerLocation)
					.Mul(pointers[1].transform)
					.Translate(0, .2f, -.1f)
					.RotateX(float(M_PI/180.0) * -15));
		}
	#endif
	#endif
		controlPanel->window.show();
	}

#if WINDOW_IS_3D
	// Update registered spatial audio source transform for the control panel.
	spatialAudioSystem->setSourceTransform(controlPanel_spatialAudioSource, controlPanel->window.getTransform());
#endif

	// Update the virtual desktop.
	framework.tickVirtualDesktop(viewToWorld, buttonMask, isHand);

	controlPanel->tick();

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
			.Scale(transformScale->get())
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

#if WINDOW_IS_3D
#if true
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
		params.setBaseColorTexture(metallicRoughnessShader, controlPanel->window.getColorTarget()->getTextureId(), 0, nextTextureUnit);

		setColor(colorWhite);
		if (controlPanel->window.isHidden() == false)
		{
			gxPushMatrix();
			{
				gxMultMatrixf(controlPanel->window.getTransformForDraw().m_v);
				gxTranslatef(0, 0, -.01f);
				pushCullMode(CULL_BACK, CULL_CCW);
				fillCube(Vec3(controlPanel->window.getWidth()/2.f, controlPanel->window.getHeight()/2.f, 0), Vec3(controlPanel->window.getWidth()/2.f + 10, controlPanel->window.getHeight()/2.f + 10, .008f));
				popCullMode();
			}
			gxPopMatrix();

			controlPanel->window.draw3d();
		}

		//framework.drawVirtualDesktop();
	}
	clearShader();

	if (controlPanel->window.isHidden() == false && controlPanel->window.hasFocus())
	{
		setColor(colorWhite);
		controlPanel->window.draw3dCursor();
	}
#else
	setColor(colorWhite);
	framework.drawVirtualDesktop();
#endif
#endif

#if true
	// Draw controllers.
	gxPushMatrix();
	gxTranslatef(
		playerLocation[0],
		playerLocation[1],
		playerLocation[2]);
	for (int i = 0; i < 2; ++i)
	{
		auto & pointer = pointers[i];

		if (pointer.isValid == false)
			continue;

		// Draw cube.
		gxPushMatrix();
		{
			gxMultMatrixf(pointer.transform.m_v);

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

				params.setMetallicRoughness(metallicRoughnessShader, .8f, .2f);
				fillCube(Vec3(), Vec3(.02f, .02f, .1f));
			}
			clearShader();
			popCullMode();

		// todo : draw in translucent pass. todo : process and remember pointer state during tick
			// Draw pointer ray.
			pushCullMode(CULL_BACK, CULL_CCW);
			pushBlend(BLEND_ADD);
			const float a = lerp<float>(.02f, .04f, (sin(time) + 1.0) / 2.0);
			setColorf(.5f, .8f, 1.f, a);
			fillCube(Vec3(0, 0, -100), Vec3(.004f, .004f, 100));
			popBlend();
			popCullMode();
		}
		gxPopMatrix();
	}
	gxPopMatrix();
#endif

#if true
	// Draw hands.
	for (int i = 0; i < 2; ++i)
	{
		auto & hand = hands[i];

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

				auto & skinningData = hands[i].getSkinningMatrices(true);
				metallicRoughnessShader.setBuffer("SkinningData", skinningData);

				hands[i].drawMesh();
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
			params.setBaseColorTexture(metallicRoughnessShader, getTexture("texture.bmp"), 0, nextTextureUnit);

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
	for (int i = 0; i < 2; ++i)
	{
		if (!hands[i].hasDeform)
			continue;

		const Vec3 position_world = hands[i].getTransform(playerLocation).GetTranslation();
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

// -- ControlPanel implementation

void ControlPanel::tick()
{
	pushWindow(window);
	{
		bool inputIsCaptured = false;
		guiContext.processBegin(.01f, window.getWidth(), window.getHeight(), inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(window.getWidth(), window.getHeight()));

			if (ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoCollapse))
			{
				if (ImGui::Button("Restart drifters"))
				{
					for (auto & drifter : scene->drifters)
					{
						const float x = random<float>(-4.f, +4.f);
						const float z = random<float>(-4.f, +4.f);

						drifter.transform.SetTranslation(x, 0, z);
					}
				}

				if (ImGui::Button("Restart audio streamers"))
				{
					for (auto & audioStreamer : scene->audioStreamers)
					{
						const float x = random<float>(-8.f, +8.f);
						const float z = random<float>(-8.f, +8.f);

						audioStreamer.initialPosition.Set(x, .4f, z);
					}
				}

				parameterUi::doParameterUi_recursive(*parameterMgr, nullptr);
			}
			ImGui::End();

			hoveredIdChanged = (GImGui->HoveredId != lastHoveredId);
			lastHoveredId = GImGui->HoveredId;
		}
		guiContext.processEnd();
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

// -- SpatialAudioSystem

struct SpatialAudioSystem : SpatialAudioSystemInterface
{
	struct Source
	{
		Mat4x4 transform = Mat4x4(true);
		AudioSource * audioSource = nullptr;
		binaural::Binauralizer binauralizer;

		std::atomic<float> elevation;
		std::atomic<float> azimuth;
		std::atomic<float> intensity;

		Source()
			: elevation(0.f)
			, azimuth(0.f)
			, intensity(0.f)
		{
		}
	};

	binaural::HRIRSampleSet sampleSet;
	binaural::Mutex_Dummy mutex_binaural; // we use a dummy mutex for the binauralizer, since we change (elevation, azimuth) only from the audio thread
	std::mutex mutex_sources; // mutex for sources array

	std::vector<Source*> sources;

	Mat4x4 listenerTransform = Mat4x4(true);

	SpatialAudioSystem()
	{
		loadHRIRSampleSet_Oalsoft("binaural/irc_1047_44100.mhr", sampleSet);
		sampleSet.finalize();
	}

	virtual void * addSource(const Mat4x4 & transform, AudioSource * audioSource) override final
	{
		Source * source = new Source();
		source->transform = transform;
		source->audioSource = audioSource;
		source->binauralizer.init(&sampleSet, &mutex_binaural);

		mutex_sources.lock();
		{
			sources.push_back(source);
		}
		mutex_sources.unlock();

		return source;
	}

	virtual void removeSource(void * in_source) override final
	{
		Source * source = (Source*)in_source;

		auto i = std::find(sources.begin(), sources.end(), source);
		Assert(i != sources.end());

		mutex_sources.lock();
		{
			sources.erase(i);
		}
		mutex_sources.unlock();

		delete source;
		source = nullptr;
	}

	virtual void setSourceTransform(void * in_source, const Mat4x4 & transform) override final
	{
		Source * source = (Source*)in_source;

		source->transform = transform;
	}

	virtual void setListenerTransform(const Mat4x4 & transform) override final
	{
		listenerTransform = transform;
	}

	virtual void updatePanning() override final
	{
		// Update binauralization parameters from listener and audio source transforms.
		const Mat4x4 & listenerToWorld = listenerTransform;
		const Mat4x4 worldToListener = listenerToWorld.CalcInv();

		for (auto * source : sources)
		{
			const Mat4x4 & sourceToWorld = source->transform;
			const Vec3 sourcePosition_world = sourceToWorld.GetTranslation();
			const Vec3 sourcePosition_listener = worldToListener.Mul4(sourcePosition_world);
			float elevation;
			float azimuth;
			binaural::cartesianToElevationAndAzimuth(
					sourcePosition_listener[2],
					sourcePosition_listener[1],
					sourcePosition_listener[0],
					elevation,
					azimuth);
			const float distance = sourcePosition_listener.CalcSize();
			const float recordedDistance = 4.f;
			const float headroomInDb = 12.f;
			const float maxGain = powf(10.f, headroomInDb/20.f);
			const float normalizedDistance = distance / recordedDistance;
			const float intensity = fminf(maxGain, 1.f / (normalizedDistance * normalizedDistance + 1e-6f));

			source->elevation = elevation;
			source->azimuth = azimuth;
			source->intensity = intensity;
		}
	}

	virtual void generateLR(float * __restrict outputSamplesL, float * __restrict outputSamplesR, const int numSamples) override final
	{
		memset(outputSamplesL, 0, numSamples * sizeof(float));
		memset(outputSamplesR, 0, numSamples * sizeof(float));

		mutex_sources.lock();
		{
			for (auto * source : sources)
			{
				float inputSamples[numSamples];

				// generate source audio
				source->audioSource->generate(inputSamples, numSamples);

				// todo : delay line for doppler shift and propagation delay

				// distance attenuation
				const float intensity_value = source->intensity.load();
				for (int i = 0; i < numSamples; ++i)
					inputSamples[i] *= intensity_value;

				source->binauralizer.provide(inputSamples, numSamples);

				source->binauralizer.setSampleLocation(
					source->elevation.load(),
					source->azimuth.load());

				float samplesL[numSamples];
				float samplesR[numSamples];
				source->binauralizer.generateLR(
					samplesL,
					samplesR,
					numSamples);

				for (int i = 0; i < numSamples; ++i)
				{
					outputSamplesL[i] += samplesL[i];
					outputSamplesR[i] += samplesR[i];
				}
			}
		}
		mutex_sources.unlock();
	}
};

struct SpatialAudioSystemAudioStream : AudioStream
{
	virtual int Provide(int numSamples, AudioSample * __restrict samples) override final
	{
		float outputSamplesL[numSamples];
		float outputSamplesR[numSamples];
		spatialAudioSystem->generateLR(
			outputSamplesL,
			outputSamplesR,
			numSamples);

		for (int i = 0; i < numSamples; ++i)
		{
		// todo : implement soft clipping ?
			if (outputSamplesL[i] < -1.f) outputSamplesL[i] = -1.f;
			if (outputSamplesR[i] < -1.f) outputSamplesR[i] = -1.f;
			if (outputSamplesL[i] > +1.f) outputSamplesL[i] = +1.f;
			if (outputSamplesR[i] > +1.f) outputSamplesR[i] = +1.f;

			samples[i].channel[0] = outputSamplesL[i] * 32000.f;
			samples[i].channel[1] = outputSamplesR[i] * 32000.f;
		}

		return numSamples;
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.manualVrMode = true;
	framework.enableDepthBuffer = true;

	if (!framework.init(800, 600))
		return -1;

	spatialAudioSystem = new SpatialAudioSystem();

	Scene scene;

	scene.create();

	AudioOutput_Native audioOutput;
	audioOutput.Initialize(2, 44100, 256);

	SpatialAudioSystemAudioStream audioStream;
	audioOutput.Play(&audioStream);

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		// Tick the simulation
		scene.tick(framework.timeStep);

	// todo : show spatial audio source position in listener space, in spatial audio system status gui (under advanced.. ;-),
		//scene.binauralSourcePos_listener->set(audioStreamPosition_listener);

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
				#if false
					// depth pre-pass
					pushDepthTest(true, DEPTH_LESS);
					pushBlend(BLEND_OPAQUE);
					pushColorWriteMask(0, 0, 0, 0);
					pushShaderOutputs("");
					{
						scene.drawOpaque();
					}
					popShaderOutputs();
					popColorWriteMask();
					popBlend();
					popDepthTest();
				#endif

					pushDepthTest(true, DEPTH_LESS);
					pushBlend(BLEND_OPAQUE);
					{
						scene.drawOpaque();
					}
					popBlend();
					popDepthTest();

					pushDepthTest(true, DEPTH_LESS, false);
					//pushBlend(BLEND_ADD);
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

	Font("calibri.ttf").saveCache();

	framework.shutdown();

	return 0;
}
