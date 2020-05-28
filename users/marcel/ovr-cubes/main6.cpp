#include "gltf.h"
#include "gltf-draw.h"

#include "parameter.h"
#include "parameterUi.h"

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

todo : binauralize some sounds

DONE : polish virtual desktop interaction when using hands,
    + tell virtual desktop which type of interaction is used (beam or finger),
    + let virtual desktop have detection of false finger presses

 */

#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include <math.h>

// -- VrApi

static ovrMobile * getOvrMobile()
{
	return frameworkVr.Ovr;
}

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
		const Mat4x4 transform =
			Mat4x4(true)
				.RotateY(angle)
				.Translate(position);
		window.setTransform(transform);

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
	const int x1 = x >= 0 ? x : 0;
	const int z1 = z >= 0 ? z : 0;
	const int x2 = x + 1 < watersim.numElems ? x + 1 : x;
	const int z2 = z + 1 < watersim.numElems ? z + 1 : z;

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

// --

struct CollisionSystemInterface
{
	virtual bool raycast(Vec3Arg origin, Vec3Arg direction, const int collisionMask, float & out_distance, Vec3 & out_normal) const = 0;
};

static const int kCollisionFlag_Watersim = 1 << 0;

static CollisionSystemInterface * collisionSystem = nullptr;

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

// -- scene

struct Scene : CollisionSystemInterface
{
	Vec3 playerLocation;

	PointerObject pointers[2];

	VrHand hands[VrHand_COUNT];

	ControlPanel * controlPanel = nullptr;
	bool showControlPanel = true;

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
	ParameterFloat * binauralElevationScale = nullptr;
	ParameterFloat * binauralAzimuthScale = nullptr;
	ParameterVec3 * binauralSourcePos_listener = nullptr;

	Mat4x4 watersim_transform = Mat4x4(true);
	Watersim watersim;

	std::vector<DrifterObject> drifters;

	void create();
	void destroy();

	void tick(const float dt, const double predictedDisplayTime);

	void drawOnce() const;
	void drawOpaque() const;
	void drawTranslucent() const;

	void drawWatersim() const;
	void drawWatersimHandProjections() const;

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
			const float x = floorf(position_watersim[0]);
			const float z = floorf(position_watersim[2]);

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
};

void Scene::create()
{
	collisionSystem = this;

	for (int i = 0; i < 2; ++i)
	{
		hands[i].init((VrHands)i);
	}

	controlPanel = new ControlPanel(Vec3(0, 1.5f, -.45f), 0.f, &parameterMgr, this);

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
	binauralElevationScale = parameterMgr_binaural.addFloat("elevationScale", 0.f);
	binauralElevationScale->setLimits(0.f, 90.f);
	binauralAzimuthScale = parameterMgr_binaural.addFloat("azimuthScale", 0.f);
	binauralAzimuthScale->setLimits(0.f, 180.f);
	binauralSourcePos_listener = parameterMgr_binaural.addVec3("sourcePos_listener", Vec3(0.f));
	parameterMgr.addChild(&parameterMgr_binaural);

	drifters.resize(32);

	for (auto & drifter : drifters)
	{
		const float x = random<float>(-4.f, +4.f);
		const float z = random<float>(-4.f, +4.f);

		drifter.transform.SetTranslation(x, 0, z);
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

void Scene::tick(const float dt, const double predictedDisplayTime)
{
	ovrMobile * ovr = getOvrMobile();

	// update input state

	for (auto & hand : hands)
	{
		hand.updateInputState();
	}

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
			if (vrapi_GetInputTrackingState(ovr, header.DeviceID, predictedDisplayTime, &tracking) != ovrSuccess)
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
						ovrMatrix4f m = ovrMatrix4f_CreateIdentity();
						const auto & p = tracking.HeadPose.Pose;
						const auto & t = p.Translation;

						m = ovrMatrix4f_CreateTranslation(t.x, t.y, t.z);
						ovrMatrix4f r = ovrMatrix4f_CreateFromQuaternion(&p.Orientation);
						m = ovrMatrix4f_Multiply(&m, &r);
						m = ovrMatrix4f_Transpose(&m);

						memcpy(pointer.transform.m_v, (float*)m.M, sizeof(Mat4x4));
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
								playerLocation += pointer.transform.GetAxis(2) * -6.f;
							}
						}
					}
				}
			}

			vrapi_SetHapticVibrationSimple(ovr, header.DeviceID, vibrate ? .5f : 0.f);
		}
	}

	// update windows

	Mat4x4 viewToWorld(true);
	int buttonMask = 0;
	bool isHand = false;

// todo : automatically switch between left and right hand. or allow both to interact at the 'same' time ?
	if (hands[VrHand_Left].getFingerTransform(VrFinger_Index, playerLocation, viewToWorld))
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

	#if false
		controlPanel->window.setTransform(Mat4x4(true).Translate(playerLocation).Mul(pointers[1].transform));
	#else
		if (pointers[1].isDown[1])
			controlPanel->window.setTransform(Mat4x4(true).Translate(playerLocation).Mul(pointers[1].transform).Translate(0, .2f, -.1f).RotateX(float(M_PI/180.0) * -15));
	#endif
		controlPanel->window.show();
	}

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

	for (auto & drifter : drifters)
	{
		drifter.tick(dt);
	}
}

void Scene::drawOnce() const
{
	controlPanel->draw();
}

void Scene::drawOpaque() const
{
	ovrMobile * ovr = getOvrMobile();

	const double time = frameworkVr.PredictedDisplayTime;

	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);

#if true
	drawWatersim();
	drawWatersimHandProjections();
#endif

#if true
	for (auto & drifter : drifters)
	{
		drifter.drawOpaque();
	}
#endif

#if true
	Shader metallicRoughnessShader("gltf/shaders/pbr-metallicRoughness");
	setShader(metallicRoughnessShader);
	{
	// todo : use a single sun location and direction vector for the entire scene .. this is becoming unmanagable
		gltf::setDefaultMaterialLighting(
			metallicRoughnessShader,
			worldToView,
			Vec3(0.f, -1.f, -3.f).CalcNormalized(),
			Vec3(lightIntensity->get()),
			Vec3(.2f));

		gltf::MetallicRoughnessParams params;
		params.init(metallicRoughnessShader);

		gltf::Material material;
		gltf::Scene scene;
		int nextTextureUnit = 0;
		params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);

		params.setUseVertexColors(metallicRoughnessShader, false);
		params.setMetallicRoughness(metallicRoughnessShader, materialMetallic->get(), materialRoughness->get());
		params.setBaseColorTexture(metallicRoughnessShader, controlPanel->window.getColorTarget()->getTextureId(), 0, nextTextureUnit);

		setColor(colorWhite);
		if (controlPanel->window.isHidden() == false)
		{
			gxPushMatrix();
			{
				gxMultMatrixf(controlPanel->window.getTransformForDraw().m_v);
				gxTranslatef(0, 0, -.01f);
				fillCube(Vec3(controlPanel->window.getWidth()/2.f, controlPanel->window.getHeight()/2.f, 0), Vec3(controlPanel->window.getWidth()/2.f + 10, controlPanel->window.getHeight()/2.f + 10, .008f));
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
			Shader metallicRoughnessShader("gltf/shaders/pbr-metallicRoughness");
			setShader(metallicRoughnessShader);
			{
				gltf::setDefaultMaterialLighting(metallicRoughnessShader, worldToView,
					Vec3(0.f, -1.f, -3.f).CalcNormalized(),
					Vec3(lightIntensity->get()));

				gltf::MetallicRoughnessParams params;
				params.init(metallicRoughnessShader);

				gltf::Material material;
				gltf::Scene scene;
				int nextTextureUnit = 0;
				params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);

				params.setUseVertexColors(metallicRoughnessShader, true);
				params.setMetallicRoughness(metallicRoughnessShader, .8f, .2f);
				setColor(255, 127, 63, 255);
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

			Shader metallicRoughnessShader("pbr-metallicRoughness-skinned");
			setShader(metallicRoughnessShader);
			{
				gltf::setDefaultMaterialLighting(metallicRoughnessShader, worldToView,
					Vec3(0.f, -1.f, -3.f).CalcNormalized(),
					Vec3(lightIntensity->get()));

				gltf::MetallicRoughnessParams params;
				params.init(metallicRoughnessShader);

				gltf::Material material;
				material.pbrMetallicRoughness.baseColorFactor = Color(255, 127, 63, 255);

				gltf::Scene scene;
				int nextTextureUnit = 0;
				params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);

				params.setUseVertexColors(metallicRoughnessShader, true);
				params.setMetallicRoughness(metallicRoughnessShader, .8f, .2f);

				auto & skinningData = hands[i].getSkinningMatrices(true);
				metallicRoughnessShader.setBuffer("SkinningData", skinningData);

				setColor(255, 127, 63, 255);
				hands[i].drawMesh();
			}
			clearShader();
		}
		gxPopMatrix();
	}
#endif
}

void Scene::drawTranslucent() const
{
	// todo : draw translucent objects

	//drawWatersim();
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

		Shader metallicRoughnessShader("gltf/shaders/pbr-metallicRoughness");
		setShader(metallicRoughnessShader);
		{
			gltf::setDefaultMaterialLighting(metallicRoughnessShader, worldToView,
				Vec3(0.f, -1.f, -3.f).CalcNormalized(),
				Vec3(lightIntensity->get()));

			gltf::MetallicRoughnessParams params;
			params.init(metallicRoughnessShader);

			gltf::Material material;
			gltf::Scene scene;
			int nextTextureUnit = 0;
			params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);

			params.setUseVertexColors(metallicRoughnessShader, true);
			params.setMetallicRoughness(metallicRoughnessShader, materialMetallic->get(), materialRoughness->get());
			params.setBaseColorTexture(metallicRoughnessShader, getTexture("texture.bmp"), 0, nextTextureUnit);
			//params.setMetallicRoughnessTexture(metallicRoughnessShader, getTexture("sabana.jpg"), 0, nextTextureUnit);

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

int main(int argc, char * argv[])
{
	if (!framework.init(0, 0))
		return -1;

	Scene scene;

	scene.create();

	struct BinauralAudioStream : AudioStream
	{
		AudioSourceVorbis audioSource;

		binaural::HRIRSampleSet sampleSet;
		binaural::Mutex_Dummy mutex;
		binaural::Binauralizer binauralizer;

		std::atomic<float> elevationBase;
		std::atomic<float> azimuthBase;
		std::atomic<float> elevationScale;
		std::atomic<float> azimuthScale;
		std::atomic<float> intensity;

		double time = 0.0;

		BinauralAudioStream()
			: elevationBase(0.f)
			, azimuthBase(0.f)
			, elevationScale(30.f)
			, azimuthScale(60.f)
			, intensity(0.f)
		{
		}

		void init()
		{
			audioSource.open("button1.ogg", true);

			loadHRIRSampleSet_Oalsoft("binaural/Default HRTF.mhr", sampleSet);
			sampleSet.finalize();
			binauralizer.init(&sampleSet, &mutex);
		}

		virtual int Provide(int numSamples, AudioSample * __restrict samples) override
		{
			float inputSamples[numSamples];

			// generate source audio
			audioSource.generate(inputSamples, numSamples);

			// distance attenuation
			const float intensity_value = intensity.load();
			for (int i = 0; i < numSamples; ++i)
				inputSamples[i] *= intensity_value;

			binauralizer.provide(inputSamples, numSamples);

			binauralizer.setSampleLocation(
				elevationBase.load() + sin(time / 1.23) * elevationScale.load(),
				azimuthBase.load() + sin(time / 2.34) * azimuthScale.load());

			time += numSamples / 44100.0;

			float outputSamplesL[numSamples];
			float outputSamplesR[numSamples];
			binauralizer.generateLR(
				outputSamplesL,
				outputSamplesR,
				numSamples);

			for (int i = 0; i < numSamples; ++i)
			{
				if (outputSamplesL[i] < -1.f) outputSamplesL[i] = -1.f;
				if (outputSamplesR[i] < -1.f) outputSamplesR[i] = -1.f;
				if (outputSamplesL[i] > +1.f) outputSamplesL[i] = +1.f;
				if (outputSamplesR[i] > +1.f) outputSamplesR[i] = +1.f;

				samples[i].channel[0] = outputSamplesL[i] * 16000.f;
				samples[i].channel[1] = outputSamplesR[i] * 16000.f;
			}

			return numSamples;
		}
	};

	BinauralAudioStream audioStream;
	audioStream.init();

	AudioOutput_Native audioOutput;
	audioOutput.Initialize(2, 44100, 256);
	audioOutput.Play(&audioStream);

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		// Tick the simulation
		scene.tick(frameworkVr.TimeStep, frameworkVr.PredictedDisplayTime);

	#if true
		// Update binauralization parameters from listener and audio source transforms.
		const Mat4x4 listenerToWorld = Mat4x4(true).Translate(scene.playerLocation).Mul(frameworkVr.HeadTransform);
		const Mat4x4 worldToListener = listenerToWorld.CalcInv();

		const Mat4x4 & audioStreamToWorld = scene.controlPanel->window.getTransform();
		const Vec3 audioStreamPosition_world = audioStreamToWorld.GetTranslation();
		const Vec3 audioStreamPosition_listener = worldToListener.Mul4(audioStreamPosition_world);
		float elevation;
		float azimuth;
		binaural::cartesianToElevationAndAzimuth(
				audioStreamPosition_listener[2],
				audioStreamPosition_listener[1],
				audioStreamPosition_listener[0],
				elevation,
				azimuth);
		const float distance = audioStreamPosition_listener.CalcSize();
		const float intensity = fminf(16.f, 1.f / (distance * distance + 1e-6f));

		azimuth = -azimuth;
		//azimuth += 180.f; // fixme : openal is shifted?
		if (azimuth > 180.f)
			azimuth -= 360.f;

		scene.binauralElevationBase->set(elevation);
		scene.binauralAzimuthBase->set(azimuth);
		scene.binauralSourcePos_listener->set(audioStreamPosition_listener);

		audioStream.intensity = intensity;
	#endif

		// Update audio parameters.
		audioStream.elevationBase = scene.binauralElevationBase->get();
		audioStream.azimuthBase = scene.binauralAzimuthBase->get();
		audioStream.elevationScale = scene.binauralElevationScale->get();
		audioStream.azimuthScale = scene.binauralAzimuthScale->get();

		// Render the stuff we need to draw only once (shared for each eye).
		scene.drawOnce();

		// Render the eye images.
		for (int eyeIndex = 0; eyeIndex < frameworkVr.getEyeCount(); ++eyeIndex)
		{
			frameworkVr.beginEye(eyeIndex, colorBlack);
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
			frameworkVr.endEye();
		}

		frameworkVr.submitFrameAndPresent();
	}

	scene.destroy();

	Font("calibri.ttf").saveCache();

	framework.shutdown();

	return 0;
}
