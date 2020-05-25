#include "ovr-egl.h"
#include "ovr-framebuffer.h"
#include "ovr-glext.h"

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

// -- VrApi

ovrMobile * getOvrMobile()
{
	return frameworkVr.Ovr;
}

// -- scene

struct PointerObject
{
	Mat4x4 transform = Mat4x4(true);
	bool isValid = false;

	bool isDown[2] = { };
};

struct WindowTest
{
    Window window;
    FrameworkImGuiContext guiContext;
	ParameterMgr parameterMgr;
	ParameterMgr parameterMgr_A;
	ParameterMgr parameterMgr_B;

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

struct Scene
{
    bool created = false;

	Vec3 playerLocation;

	std::vector<WindowTest*> windows;

	// hand mesh
	struct HandMesh
	{
		GxMesh mesh;
		GxVertexBuffer vb;
		GxIndexBuffer ib;
		ShaderBuffer skinningData;
	} handMeshes[2];

	PointerObject pointers[2];

    void create();
    void destroy();

    void tick(ovrMobile * ovr, const float dt, const double predictedDisplayTime);

    void draw() const;
    void drawEye(ovrMobile * ovr) const;
};

void Scene::create()
{
	for (int i = 0; i < 3; ++i)
	{
		WindowTest * window = new WindowTest(i - 1);

		windows.push_back(window);
	}

	for (int i = 0; i < 2; ++i)
	{
		ovrHandMesh handMesh;
		handMesh.Header.Version = ovrHandVersion_1;
		if (vrapi_GetHandMesh(getOvrMobile(), i == 0 ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT, &handMesh.Header) == ovrSuccess)
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

			handMeshes[i].vb.alloc(&handMesh, sizeof(handMesh));
			handMeshes[i].ib.alloc(handMesh.Indices, sizeof(handMesh.Indices) / sizeof(handMesh.Indices[0]), GX_INDEX_16);
			handMeshes[i].mesh.setVertexBuffer(&handMeshes[i].vb, vsInputs, numVsInputs, 0);
			handMeshes[i].mesh.setIndexBuffer(&handMeshes[i].ib);
			handMeshes[i].mesh.addPrim(GX_TRIANGLES, handMesh.NumIndices, true);

			handMeshes[i].skinningData.alloc(sizeof(Mat4x4) * 32);
		}
	}

    created = true;
}

void Scene::destroy()
{
	for (auto *& window : windows)
	{
		delete window;
		window = nullptr;
	}

	windows.clear();

    created = false;
}

void Scene::tick(ovrMobile * ovr, const float dt, const double predictedDisplayTime)
{
static bool isPinching = false; // todo : remove hack

	// update windows

#if true
	const Mat4x4 viewToWorld = Mat4x4(true).Translate(playerLocation).Mul(pointers[0].transform);
	const int buttonMask =
		(pointers[0].isDown[0] << 0) |
		(pointers[0].isDown[1] << 1);
#else
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	Mat4x4 viewToWorld = worldToView.CalcInv();
	const int buttonMask = (isPinching << 0);
#endif

	framework.tickVirtualDesktop(viewToWorld, buttonMask);

	for (auto * window : windows)
		window->tick();

	uint32_t index = 0;

isPinching = false; // todo : remove hack

	for (;;)
	{
		ovrInputCapabilityHeader header;

		const auto result = vrapi_EnumerateInputDevices(ovr, index++, &header);

		if (result < 0)
			break;

		if (header.Type == ovrControllerType_TrackedRemote)
		{
			bool vibrate = false;

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
		else if (header.Type == ovrControllerType_Hand)
		{
			ovrInputStateHand hand;
			hand.Header.ControllerType = ovrControllerType_Hand;
			if (vrapi_GetCurrentInputState(ovr, header.DeviceID, &hand.Header) == ovrSuccess)
			{
				if ((hand.InputStateStatus & ovrInputStateHandStatus_IndexPinching) && hand.PinchStrength[ovrHandPinchStrength_Index] >= .5f)
					isPinching = true;
			}
		}
	}
}

void Scene::draw() const
{
	for (auto * window : windows)
		window->draw();
}

void Scene::drawEye(ovrMobile * ovr) const
{
	const double time = frameworkVr.getTimeInSeconds();

    gxTranslatef(
	    -playerLocation[0],
	    -playerLocation[1],
	    -playerLocation[2]);

	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);

    pushDepthTest(true, DEPTH_LESS);
    pushBlend(BLEND_OPAQUE);

	// Adjust for floor level.
	float ground_y = 0.f;

#if false
    {
	    ovrPosef boundaryPose;
	    ovrVector3f boundaryScale;
	    if (vrapi_GetBoundaryOrientedBoundingBox(ovr, &boundaryPose, &boundaryScale) == ovrSuccess)
	        ground_y = boundaryPose.Translation.y;
    }
#endif

    gxPushMatrix();
    {
    	gxTranslatef(0, ground_y, 0);

	    // draw world-space things
    }
    gxPopMatrix(); // Undo ground level adjustment.

	setColor(colorWhite);
	framework.drawVirtualDesktop();

	// Draw hands.
#if true
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

#if true
	if (ovr != nullptr)
	{
		for (int i = 0; true; i++)
		{
			// Describe this input device.
			ovrInputCapabilityHeader cap;
			const ovrResult result = vrapi_EnumerateInputDevices(ovr, i, &cap);
			if (result < 0)
				break;

			// Is it a hand?
			if (cap.Type == ovrControllerType_Hand)
			{
				// Describe hand.
				ovrInputHandCapabilities handCapabilities;
	            handCapabilities.Header = cap;
	            if (vrapi_GetInputDeviceCapabilities(ovr, &handCapabilities.Header) != ovrSuccess)
		            continue;

				// Left hand or right hand.
		        const int index = (handCapabilities.HandCapabilities & ovrHandCaps_LeftHand) ? 0 : 1;

				// Fetch the current pose for the hand.
				ovrHandPose handPose;
				handPose.Header.Version = ovrHandVersion_1;
				if (vrapi_GetHandPose(ovr, cap.DeviceID, time, &handPose.Header) == ovrSuccess &&
					handPose.Status == ovrHandTrackingStatus_Tracked &&
					(handPose.HandConfidence == ovrConfidence_HIGH))
				{
					// Fetch the initial pose for the hand.
					// We will need to combine it with the current pose, which only defines the new orientations for the bones.
					// And we will need it to perform skinning as well.
					ovrHandSkeleton handSkeleton;
					handSkeleton.Header.Version = ovrHandVersion_1;
					if (vrapi_GetHandSkeleton(ovr, index == 0 ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT, &handSkeleton.Header) == ovrSuccess)
					{
					// todo : Check with the SDK documentation, if we can optimize global transform calculations, by assuming parent matrices are always processed before a child.

						// Calculate global bone transforms (bind pose).
						ovrMatrix4f globalBoneTransforms_bind[ovrHand_MaxBones];
						for (int i = 0; i < handSkeleton.NumBones; ++i)
						{
							const ovrPosef & pose = handSkeleton.BonePoses[i];
							const ovrMatrix4f transform = vrapi_GetTransformFromPose(&pose);
							const int parentIndex = handSkeleton.BoneParentIndices[i];
							if (parentIndex < 0)
								globalBoneTransforms_bind[i] = transform;
							else
								globalBoneTransforms_bind[i] = ovrMatrix4f_Multiply(&globalBoneTransforms_bind[parentIndex], &transform);
						}

						// Update local bone poses.
						ovrPosef localBonePoses[ovrHand_MaxBones];
						for (int i = 0; i < handSkeleton.NumBones; ++i)
						{
							localBonePoses[i] = handSkeleton.BonePoses[i];
							localBonePoses[i].Orientation = handPose.BoneRotations[i];
						}

						// Calculate global bone transforms (animated).
						ovrMatrix4f globalBoneTransforms[ovrHand_MaxBones];
						for (int i = 0; i < handSkeleton.NumBones; ++i)
						{
							const ovrPosef & pose = localBonePoses[i];
							const ovrMatrix4f transform = vrapi_GetTransformFromPose(&pose);
							const int parentIndex = handSkeleton.BoneParentIndices[i];
							if (parentIndex < 0)
								globalBoneTransforms[i] = transform;
							else
								globalBoneTransforms[i] = ovrMatrix4f_Multiply(&globalBoneTransforms[parentIndex], &transform);
						}

						// Calculate skinning matrices. Skinning matrices will first transform a vertex into 'bind space',
						// by applying the inverse of the initial pose transform. It will then apply the global bone transform.
						ovrMatrix4f skinningTransforms[ovrHand_MaxBones];
						for (int i = 0; i < handSkeleton.NumBones; ++i)
						{
							const ovrMatrix4f objectToBind = ovrMatrix4f_Inverse(&globalBoneTransforms_bind[i]);
							const ovrMatrix4f & bindToObject = globalBoneTransforms[i];
							skinningTransforms[i] = ovrMatrix4f_Multiply(&bindToObject, &objectToBind);
							skinningTransforms[i] = ovrMatrix4f_Transpose(&skinningTransforms[i]);
						}

						ovrMatrix4f rootPose = vrapi_GetTransformFromPose(&handPose.RootPose);
						rootPose = ovrMatrix4f_Transpose(&rootPose);

						gxPushMatrix();
						{
							// Apply the root pose.
							gxMultMatrixf((float*)rootPose.M);
							gxScalef(handPose.HandScale, handPose.HandScale, handPose.HandScale);

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

								const_cast<ShaderBuffer&>(handMeshes[index].skinningData).setData(skinningTransforms, sizeof(skinningTransforms));
								metallicRoughnessShader.setBuffer("SkinningData", handMeshes[index].skinningData);

								setColor(255, 127, 63, 255);
								handMeshes[index].mesh.draw();
							}
							clearShader();
						}
						gxPopMatrix();
					}
				}
			}
		}
	}
#endif

    gxPopMatrix();
#endif

	// -- End opaque pass.
	popBlend();
    popDepthTest();

    // -- Begin translucent pass.
    pushDepthTest(true, DEPTH_LESS, false);
    pushBlend(BLEND_ADD);
    {
    	// todo : draw translucent objects
    }
    popBlend();
    popDepthTest();
    // -- End translucent pass.
}

int main(int argc, char * argv[])
{
	ovrEgl egl;

	egl.createContext();

	ovrOpenGLExtensions.init();

	FrameworkVr frameworkVr;

	if (!framework.init(0, 0) ||
		!frameworkVr.init(&egl))
	{
		return -1;
	}

	Scene scene;

    for (;;)
    {
        //framework.process();
	    frameworkVr.process();

	    if (framework.quitRequested)
	        break;

        // Create the scene if not yet created.
        // The scene is created here to be able to show a loading icon.
        if (!scene.created)
        {
            // Show a loading icon.
            const int frameFlags = VRAPI_FRAME_FLAG_FLUSH;

            ovrLayerProjection2 blackLayer = vrapi_DefaultLayerBlackProjection2();
            blackLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

            ovrLayerLoadingIcon2 iconLayer = vrapi_DefaultLayerLoadingIcon2();
            iconLayer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

            const ovrLayerHeader2 * layers[] =
                {
                    &blackLayer.Header,
                    &iconLayer.Header,
                };

            ovrSubmitFrameDescription2 frameDesc = { };
            frameDesc.Flags = frameFlags;
            frameDesc.SwapInterval = 1;
            frameDesc.FrameIndex = frameworkVr.FrameIndex;
            frameDesc.DisplayTime = frameworkVr.PredictedDisplayTime;
            frameDesc.LayerCount = 2;
            frameDesc.Layers = layers;

            vrapi_SubmitFrame2(frameworkVr.Ovr, &frameDesc);

            // Create the scene.
            scene.create();
        }

	    // Tick the simulation
        scene.tick(frameworkVr.Ovr, frameworkVr.TimeStep, frameworkVr.PredictedDisplayTime);

		// Render the stuff we need to draw only once (shared for each eye).
	    scene.draw();

		// Render the eye images.
	    for (int eyeIndex = 0; eyeIndex < frameworkVr.getEyeCount(); ++eyeIndex)
	    {
		    frameworkVr.beginEye(eyeIndex, colorBlack);
		    {
		        scene.drawEye(frameworkVr.Ovr);
		    }
		    frameworkVr.endEye();
	    }

	    frameworkVr.submitFrameAndPresent();
    }

    Font("calibri.ttf").saveCache();

	scene.destroy();

	framework.shutdown();
	frameworkVr.shutdown();

    egl.destroyContext();

    vrapi_Shutdown();

	return 0;
}
