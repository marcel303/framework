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

/*

todo : draw windows in 3d using framework

todo : tick events for windows in 3d
	- let the app or framework determine pointer direction and button presses
	- intersect pointer with windows. determine nearest intersection
	- on button press, make the intersecting window active
	- for all windows, generate mouse.x, mouse.y and button presses

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

struct WindowTest
{
	Window window;
	FrameworkImGuiContext guiContext;
	ParameterMgr parameterMgr;
	ParameterMgr parameterMgr_A;
	ParameterMgr parameterMgr_B;

	ImGuiID lastHoveredId = 0;
	bool hoveredIdChanged = false;

	WindowTest(const float angle)
		: window("Window", 340, 340)
	{
		const Mat4x4 transform = Mat4x4(true).RotateY(angle).Translate(0, 1.5f, -.45f);
		window.setTransform(transform);

		guiContext.init(false);
		parameterMgr.addString("name", "");
		parameterMgr.addInt("count", 0)->setLimits(0, 100);
		parameterMgr.addFloat("speed", 0.f)->setLimits(0.f, 10.f);

		parameterMgr_A.setPrefix("Group A");
		parameterMgr_A.addVec3("color", Vec3(1.f))->setLimits(Vec3(0.f), Vec3(1.f));
		parameterMgr_A.addFloat("strength", 1.f)->setLimits(0.f, 1.f);
		parameterMgr.addChild(&parameterMgr_A);

		parameterMgr_B.setPrefix("Group B");
		parameterMgr_B.addEnum("type", 0, {{ "Box", 0 }, { "Sphere", 1 }});
		parameterMgr_B.addVec3("color", Vec3(1.f))->setLimits(Vec3(0.f), Vec3(1.f));
		parameterMgr_B.addFloat("strength", 1.f)->setLimits(0.f, 1.f);
		parameterMgr.addChild(&parameterMgr_B);
	}

	~WindowTest()
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
				ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
				ImGui::SetNextWindowSize(ImVec2(window.getWidth(), window.getHeight()), ImGuiCond_Once);

				if (ImGui::Begin("This is a window", nullptr, ImGuiWindowFlags_NoCollapse))
				{
					ImGui::Button("This is a button");

					parameterUi::doParameterUi_recursive(parameterMgr, nullptr);
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
	bool created = false;

	Vec3 playerLocation;

	std::vector<WindowTest*> windows;

	PointerObject pointers[2];

	VrHand hands[VrHand_COUNT];

	void create();
	void destroy();

	void tick(const float dt, const double predictedDisplayTime);

	void drawOnce() const;
	void drawOpaque() const;
	void drawTranslucent() const;
};

void Scene::create()
{
	for (int i = 0; i < 2; ++i)
	{
		hands[i].init((VrHands)i);
	}

	for (int i = 0; i < 3; ++i)
	{
		WindowTest * window = new WindowTest(i - 1);

		windows.push_back(window);
	}

	created = true;
}

void Scene::destroy()
{
	for (auto & hand : hands)
		hand.shut();

	for (auto *& window : windows)
	{
		delete window;
		window = nullptr;
	}

	windows.clear();

	created = false;
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

#if false
	const Mat4x4 viewToWorld = Mat4x4(true).Translate(playerLocation).Mul(pointers[0].transform);
	const int buttonMask =
		(pointers[0].isDown[0] << 0) |
		(pointers[0].isDown[1] << 1);
#else
	// todo : getFingerTransform may return false. handle this case
	Mat4x4 viewToWorld;
	hands[VrHand_Left].getFingerTransform(VrFinger_Index, playerLocation, viewToWorld);
	//const int buttonMask = (hands[0].fingers[VrFinger_Index].isPinching << 0);
	const int buttonMask = 1 << 0;
#endif

	framework.tickVirtualDesktop(viewToWorld, buttonMask, true);

	for (auto * window : windows)
		window->tick();

// todo : update controller input before updating windows
	uint32_t index = 0;

	for (;;)
	{
		ovrInputCapabilityHeader header;

		if (vrapi_EnumerateInputDevices(ovr, index++, &header) < 0)
			break;

		if (header.Type == ovrControllerType_TrackedRemote)
		{
			bool vibrate = false;

			for (auto * window : windows)
				vibrate |= window->hoveredIdChanged && window->lastHoveredId != 0;

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
	for (auto * window : windows)
		window->draw();
}

void Scene::drawOpaque() const
{
	ovrMobile * ovr = getOvrMobile();

	const double time = frameworkVr.PredictedDisplayTime;

	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);

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
}

int main(int argc, char * argv[])
{
	framework.manualVrMode = true;

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
					pushBlend(BLEND_ADD);
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
