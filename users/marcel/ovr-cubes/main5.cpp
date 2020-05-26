#include "gltf.h"
#include "gltf-draw.h"

#include "parameter.h"
#include "parameterUi.h"

#include "data/engine/ShaderCommon.txt" // VS_ constants
#include "framework.h"
#include "framework-ovr.h"
#include "gx_mesh.h"
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

todo : experiment with drawing window contents in 3d directly (no window surface). this will give higher quality results on outlines

 */

#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include <math.h>
#include <vector> // for hand skeleton

// -- VrApi

ovrMobile * getOvrMobile()
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

enum VrHands
{
	VrHand_Left,
	VrHand_Right,
	VrHand_COUNT
};

enum VrFingers
{
	VrFinger_Thumb,
	VrFinger_Index,
	VrFinger_Middle,
	VrFinger_Ring,
	VrFinger_Pinky,
	VrFinger_COUNT
};

struct VrHandBase
{
	bool hasSkeleton = false;
	struct
	{
		int numBones = 0;

		std::vector<int> boneParentIndices; // bone hierarchy

		std::vector<Mat4x4> localBoneTransforms;  // bind pose in local space
		std::vector<Mat4x4> globalBoneTransforms; // bind pose in global space

		int fingerNameToBoneIndex[VrFinger_COUNT];
	} skeleton;

	bool hasDeform = false;
	struct
	{
		std::vector<Mat4x4> localBoneTransforms;
		std::vector<Mat4x4> globalBoneTransforms;
	} deform;

	Mat4x4 rootPose = Mat4x4(true);

	struct
	{
		bool isPinching = false;
	} fingers[VrFinger_COUNT];

	VrHandBase()
	{
		for (auto & index : skeleton.fingerNameToBoneIndex)
			index = -1;
	}

	virtual void init(VrHands hand) = 0;
	virtual void shut() = 0;

	virtual void updateInputState() = 0;
	virtual void updateSkinningMatrices() const = 0;
	virtual ShaderBuffer & getSkinningMatrices(const bool update) const = 0;
	virtual void drawMesh() const = 0;

	Mat4x4 getTransform(const Vec3 worldOrigin) const
	{
		return Mat4x4(true)
			.Translate(worldOrigin)
			.Mul(rootPose);
	}

	/**
	 * Returns the world-space pointer transform for a finger.
	 * @param finger The finger for which to get the transform.
	 * @return The world-space pointer transform.
	 */
	bool getFingerTransform(const VrFingers finger, const Vec3 worldOrigin, Mat4x4 & out_transform) const
	{
		if (!hasSkeleton || !hasDeform)
			return false;

		const int boneIndex = skeleton.fingerNameToBoneIndex[finger];
		if (boneIndex < 0)
			return false;

		out_transform = Mat4x4(true)
			.Translate(worldOrigin)
			.Mul(rootPose)
			.Mul(deform.globalBoneTransforms[boneIndex])
			.RotateY(float(M_PI)/2.f); // todo : make rotation depend on the hand?
		return true;
	}

	void resizeSkeleton(const int numBones)
	{
		skeleton.numBones = numBones;
		skeleton.boneParentIndices.resize(numBones);
		skeleton.localBoneTransforms.resize(numBones);
		skeleton.globalBoneTransforms.resize(numBones);

		deform.localBoneTransforms.resize(numBones);
		deform.globalBoneTransforms.resize(numBones);
	}

	static void localPoseToGlobal(
		const Mat4x4 * __restrict localMatrices,
		const int * __restrict parentIndices,
		const int numMatrices,
		Mat4x4 * __restrict globalMatrices)
	{
		for (int i = 0; i < numMatrices; ++i)
		{
			const Mat4x4 & transform = localMatrices[i];
			const int parentIndex = parentIndices[i];
			if (parentIndex < 0)
				globalMatrices[i] = transform;
			else
				globalMatrices[i] = globalMatrices[parentIndex] * transform;
		}
	}

	void calculateGlobalBindPose()
	{
		// Calculate global bone transforms (bind pose).
		localPoseToGlobal(
			skeleton.localBoneTransforms.data(),
			skeleton.boneParentIndices.data(),
			skeleton.numBones,
			skeleton.globalBoneTransforms.data());
	}

	void calculateGlobalDeformPose()
	{
		// Calculate global bone transforms (animated).
		localPoseToGlobal(
			deform.localBoneTransforms.data(),
			skeleton.boneParentIndices.data(),
			skeleton.numBones,
			deform.globalBoneTransforms.data());
	}

	void calculateSkinnningMatrices(Mat4x4 * skinningMatrices, const int maxBones) const
	{
		// Calculate skinning matrices. Skinning matrices will first transform a vertex into 'bind space',
		// by applying the inverse of the initial pose transform. It will then apply the global bone transform.
		for (int i = 0; i < skeleton.numBones && i < maxBones; ++i)
		{
			const Mat4x4 objectToBind = skeleton.globalBoneTransforms[i].CalcInv();
			const Mat4x4 & bindToDeform = deform.globalBoneTransforms[i];
			skinningMatrices[i] = bindToDeform * objectToBind;
		}
	}
};

class VrHand : public VrHandBase
{
	VrHands hand;

	GxMesh mesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;

	mutable ShaderBuffer skinningData;

public:
	virtual void init(VrHands in_hand) override
	{
		hand = in_hand;

		const auto ovrHand = (hand == VrHand_Left) ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT;

		ovrHandMesh handMesh;
		handMesh.Header.Version = ovrHandVersion_1;
		if (vrapi_GetHandMesh(getOvrMobile(), ovrHand, &handMesh.Header) == ovrSuccess)
		{
			GxVertexInput vsInputs[] =
			{
				{ VS_POSITION,      3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexPositions), sizeof(ovrVector3f) },
				{ VS_NORMAL,        3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexNormals),   sizeof(ovrVector3f) },
				//{ VS_COLOR,         3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexNormals),   sizeof(ovrVector3f) },
				{ VS_TEXCOORD0,     2, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexUV0),       sizeof(ovrVector2f) },
				{ VS_BLEND_INDICES, 4, GX_ELEMENT_UINT16,  0, offsetof(ovrHandMesh, BlendIndices),    sizeof(ovrVector4s) },
				{ VS_BLEND_WEIGHTS, 4, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, BlendWeights),    sizeof(ovrVector4f) }
			};
			const int numVsInputs = sizeof(vsInputs) / sizeof(vsInputs[0]);

			vb.alloc(&handMesh, sizeof(handMesh));
			ib.alloc(handMesh.Indices, sizeof(handMesh.Indices) / sizeof(handMesh.Indices[0]), GX_INDEX_16);
			mesh.setVertexBuffer(&vb, vsInputs, numVsInputs, 0);
			mesh.setIndexBuffer(&ib);
			mesh.addPrim(GX_TRIANGLES, handMesh.NumIndices, true);

			skinningData.alloc(sizeof(Mat4x4) * 32);
		}
	}

	virtual void shut() override
	{
		mesh.clear();
		vb.free();
		ib.free();

		skinningData.free();
	}

	virtual void updateInputState() override
	{
		for (int i = 0; i < VrFinger_COUNT; ++i)
			fingers[i].isPinching = false;
		hasSkeleton = false;
		hasDeform = false;

		// loop through input device and find our hand

		auto * ovr = getOvrMobile();

		uint32_t deviceIndex = 0;

		for (;;)
		{
			ovrInputCapabilityHeader header;
			if (vrapi_EnumerateInputDevices(ovr, deviceIndex++, &header) < 0)
				break;

			// Is it a hand?
			if (header.Type != ovrControllerType_Hand)
				continue;

			// Describe hand.
			ovrInputHandCapabilities handCapabilities;
			handCapabilities.Header = header;
			if (vrapi_GetInputDeviceCapabilities(ovr, &handCapabilities.Header) != ovrSuccess)
				continue;

			// Is it our hand?
			if (hand == VrHand_Left && (handCapabilities.HandCapabilities & ovrHandCaps_LeftHand) == 0)
				continue;
			if (hand == VrHand_Right && (handCapabilities.HandCapabilities & ovrHandCaps_RightHand) == 0)
				continue;

			for (;;)
			{
				// Fetch the input state for the hand.
				ovrInputStateHand hand;
				hand.Header.ControllerType = ovrControllerType_Hand;
				if (vrapi_GetCurrentInputState(ovr, header.DeviceID, &hand.Header) != ovrSuccess)
					break;

				// todo : pinching state for all five fingers
				if ((hand.InputStateStatus & ovrInputStateHandStatus_IndexPinching) && hand.PinchStrength[ovrHandPinchStrength_Index] >= .5f)
					fingers[VrFinger_Index].isPinching = true;

				break;
			}

			for (;;)
			{
				// Fetch the initial pose for the hand.
				// We will need to combine it with the current pose, which only defines the new orientations for the bones.
				// And we will need it to perform skinning as well.
				ovrHandSkeleton handSkeleton;
				handSkeleton.Header.Version = ovrHandVersion_1;
				const auto ovrHand = (hand == VrHand_Left) ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT;
				if (vrapi_GetHandSkeleton(ovr, ovrHand, &handSkeleton.Header) != ovrSuccess)
					break;

				hasSkeleton = true;

				resizeSkeleton(handSkeleton.NumBones);

				// Fill in bone hierarchy.
				for (int i = 0; i < handSkeleton.NumBones; ++i)
					skeleton.boneParentIndices[i] = handSkeleton.BoneParentIndices[i];

				// Fill in bone indices for finger tips (for pointer positions and directions).
				skeleton.fingerNameToBoneIndex[VrFinger_Thumb]  = ovrHandBone_ThumbTip;
				skeleton.fingerNameToBoneIndex[VrFinger_Index]  = ovrHandBone_IndexTip;
				skeleton.fingerNameToBoneIndex[VrFinger_Middle] = ovrHandBone_MiddleTip;
				skeleton.fingerNameToBoneIndex[VrFinger_Ring]   = ovrHandBone_RingTip;
				skeleton.fingerNameToBoneIndex[VrFinger_Pinky]  = ovrHandBone_PinkyTip;

				// Fill in local bone transforms (bind pose).
				for (int i = 0; i < handSkeleton.NumBones; ++i)
				{
					const ovrPosef & pose = handSkeleton.BonePoses[i];
					const ovrMatrix4f transform = vrapi_GetTransformFromPose(&pose);
					const ovrMatrix4f transform_transposed = ovrMatrix4f_Transpose(&transform);
					memcpy(skeleton.localBoneTransforms[i].m_v, transform_transposed.M, sizeof(Mat4x4));
				}

				// Calculate global bind pose.
				calculateGlobalBindPose();

				// Fetch the current pose for the hand.
				ovrHandPose handPose;
				handPose.Header.Version = ovrHandVersion_1;
				if (vrapi_GetHandPose(ovr, header.DeviceID, frameworkVr.PredictedDisplayTime, &handPose.Header) != ovrSuccess)
					break;

				// Is the hand being tracked, with high confidence?
				if (handPose.Status != ovrHandTrackingStatus_Tracked ||
					handPose.HandConfidence != ovrConfidence_HIGH)
					break;

				hasDeform = true;

				// Update local bone poses.
				for (int i = 0; i < handSkeleton.NumBones; ++i)
				{
					ovrPosef pose = handSkeleton.BonePoses[i];
					pose.Orientation = handPose.BoneRotations[i];
					const ovrMatrix4f transform = vrapi_GetTransformFromPose(&pose);
					const ovrMatrix4f transform_transposed = ovrMatrix4f_Transpose(&transform);

					memcpy(deform.localBoneTransforms[i].m_v, transform_transposed.M, sizeof(Mat4x4));
				}

				calculateGlobalDeformPose();

				ovrMatrix4f ovrRootPose = vrapi_GetTransformFromPose(&handPose.RootPose);
				ovrRootPose = ovrMatrix4f_Transpose(&ovrRootPose);
				memcpy(rootPose.m_v, ovrRootPose.M, sizeof(Mat4x4));
				rootPose = rootPose.Scale(handPose.HandScale);

				break;
			}
		}
	}

	virtual void updateSkinningMatrices() const override
	{
		Mat4x4 skinningTransforms[32];
		calculateSkinnningMatrices(skinningTransforms, 32);

		skinningData.setData(skinningTransforms, sizeof(skinningTransforms));
	}

	virtual ShaderBuffer & getSkinningMatrices(const bool update) const override
	{
		if (update)
			updateSkinningMatrices();

		return skinningData;
	}

	virtual void drawMesh() const override
	{
		mesh.draw();
	}
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

	for (auto & hand : hands)
	{
		hand.updateInputState();
	}

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
