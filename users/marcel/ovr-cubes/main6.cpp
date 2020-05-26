#include "gltf.h"
#include "gltf-draw.h"

#include "parameter.h"
#include "parameterUi.h"

#include "framework.h"
#include "framework-ovr.h"
#include "framework-vr-hands.h"
#include "internal.h"

#include "imgui-framework.h"
#include "imgui_internal.h"

#include "watersim.h"

/*

new challenges!

todo : automatically switch between controller and hands for virtual desktop interaction

todo : use finger gesture or controller button to open/close control panel

todo : for testing : save/load parameters on startup/exit

todo : experiment with drawing window contents in 3d directly (no window surface). this will give higher quality results on outlines

todo : draw virtual desktop using a custom shader, to give the windows some specular reflection

todo : refactor controller state similar to hand state

todo : project hand position down onto watersim, to give the user an exaggered impression of how high up they are

todo : determine nice watersim params and update defaults

todo : binauralize some sounds

todo : polish virtual desktop interaction when using hands,
    - tell virtual desktop which type of interaction is used (beam or finger),
    - let virtual desktop have detection of false finger presses

 */

#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include <math.h>
#include <vector> // for hand skeleton

// -- VrApi

static ovrMobile * getOvrMobile()
{
	return frameworkVr.Ovr;
}

struct ControlPanel
{
	Window window;
	FrameworkImGuiContext guiContext;
	ParameterMgr * parameterMgr = nullptr;

	ImGuiID lastHoveredId = 0;
	bool hoveredIdChanged = false;

	ControlPanel(const Vec3 position, const float angle, ParameterMgr * in_parameterMgr)
		: window("Window", 340, 340)
	{
		const Mat4x4 transform =
			Mat4x4(true)
				.RotateY(angle)
				.Translate(position);
		window.setTransform(transform);

		guiContext.init(false);

		parameterMgr = in_parameterMgr;
	}

	~ControlPanel()
	{
		guiContext.shut();
	}

	void tick()
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

// -- scene

struct Scene
{
	Vec3 playerLocation;

	PointerObject pointers[2];

	VrHand hands[VrHand_COUNT];

	ControlPanel * controlPanel = nullptr;

	ParameterMgr parameterMgr;
	ParameterInt * resolution = nullptr;
	ParameterFloat * tension = nullptr;
	ParameterFloat * velocityRetain = nullptr;
	ParameterFloat * positionRetain = nullptr;
	ParameterFloat * materialMetallic = nullptr;
	ParameterFloat * materialRoughness = nullptr;
	ParameterFloat * transformHeight = nullptr;
	ParameterFloat * transformScale = nullptr;

	Mat4x4 watersim_transform = Mat4x4(true);
	Watersim watersim;

	void create();
	void destroy();

	void tick(const float dt, const double predictedDisplayTime);

	void drawOnce() const;
	void drawOpaque() const;
	void drawTranslucent() const;

	void drawWatersim() const;
};

void Scene::create()
{
	for (int i = 0; i < 2; ++i)
	{
		hands[i].init((VrHands)i);
	}

	controlPanel = new ControlPanel(Vec3(0, 1.5f, -.45f), 0.f, &parameterMgr);

	// add watersim object
	resolution = parameterMgr.addInt("resolution", 16);
	resolution->setLimits(2, Watersim::kMaxElems);
	tension = parameterMgr.addFloat("tension", 10.f);
	tension->setLimits(1.f, 100.f);
	velocityRetain = parameterMgr.addFloat("velocityRetain", .1f);
	velocityRetain->setLimits(.01f, .99f);
	positionRetain = parameterMgr.addFloat("positionRetain", .1f);
	positionRetain->setLimits(.01f, .99f);
	materialMetallic = parameterMgr.addFloat("materialMetallic", .8f);
	materialMetallic->setLimits(0.f, 1.f);
	materialRoughness = parameterMgr.addFloat("materialRoughness", .8f);
	materialRoughness->setLimits(0.f, 1.f);
	transformHeight = parameterMgr.addFloat("transformHeight", 0.f);
	transformHeight->setLimits(-2.f, +2.f);
	transformScale = parameterMgr.addFloat("transformScale", 1.f);
	transformScale->setLimits(.1f, 2.f);
	watersim.init(resolution->get());
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

	// update windows

#if true
	const Mat4x4 viewToWorld = Mat4x4(true).Translate(playerLocation).Mul(pointers[0].transform);
	const int buttonMask =
		(pointers[0].isDown[0] << 0) |
		(pointers[0].isDown[1] << 1);
	const bool useProximityForInteractivity = false;
#else
	// todo : getFingerTransform may return false. handle this case
	Mat4x4 viewToWorld;
	hands[VrHand_Left].getFingerTransform(VrFinger_Index, playerLocation, viewToWorld);
	//const int buttonMask = (hands[0].fingers[VrFinger_Index].isPinching << 0);
	const int buttonMask = 1 << 0;
	const bool useProximityForInteractivity = true;
#endif

	framework.tickVirtualDesktop(viewToWorld, buttonMask, useProximityForInteractivity);

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
	//drawWatersim();
#endif

	setColor(colorWhite);
	framework.drawVirtualDesktop();

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
				gltf::setDefaultMaterialLighting(metallicRoughnessShader, worldToView);

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
			const float a = lerp<float>(.1f, .4f, (sin(time) + 1.0) / 2.0);
			setColorf(1, 1, 1, a);
			fillCube(Vec3(0, 0, -100), Vec3(.01f, .01f, 100));
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
				gltf::setDefaultMaterialLighting(metallicRoughnessShader, worldToView);

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

	drawWatersim();
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
			gltf::setDefaultMaterialLighting(metallicRoughnessShader, worldToView, Vec3(0.f, -1.f, 4.f).CalcNormalized());

			gltf::MetallicRoughnessParams params;
			params.init(metallicRoughnessShader);

			gltf::Material material;
			gltf::Scene scene;
			int nextTextureUnit = 0;
			params.setShaderParams(metallicRoughnessShader, material, scene, false, nextTextureUnit);

			params.setUseVertexColors(metallicRoughnessShader, true);
			params.setMetallicRoughness(metallicRoughnessShader, materialMetallic->get(), materialRoughness->get());

			setColor(255, 200, 180, 180);
			gxBegin(GX_QUADS);
			{
				for (int x = 0; x < watersim.numElems - 1; ++x)
				{
					for (int z = 0; z < watersim.numElems - 1; ++z)
					{
						const float y1 = watersim.p[x + 0][z + 0];
						const float y2 = watersim.p[x + 1][z + 0];
						const float y3 = watersim.p[x + 1][z + 1];
						const float y4 = watersim.p[x + 0][z + 1];

						const float nx = -(y2 - y1);
						const float ny = 1.f;
						const float nz = -(y4 - y1);

						const Vec3 normal = Vec3(nx, ny, nz).CalcNormalized();

						gxNormal3f(normal[0], normal[1], normal[2]);
						gxVertex3f(x + 0, y1, z + 0);
						gxVertex3f(x + 1, y2, z + 0);
						gxVertex3f(x + 1, y3, z + 1);
						gxVertex3f(x + 0, y4, z + 1);
					}
				}
			}
			gxEnd();
		}
		clearShader();
	}
	gxPopMatrix();
}

int main(int argc, char * argv[])
{
	if (!framework.init(0, 0))
		return -1;

	Scene scene;

	scene.create();

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		// Tick the simulation
		scene.tick(frameworkVr.TimeStep, frameworkVr.PredictedDisplayTime);

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
