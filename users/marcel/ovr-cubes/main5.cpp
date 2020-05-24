#include "ovr-egl.h"
#include "ovr-framebuffer.h"
#include "ovr-glext.h"

#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"

#include "parameter.h"
#include "parameterUi.h"

#include "data/engine/ShaderCommon.txt" // VS_ constants
#include "framework.h"
#include "gx_render.h"
#include "internal.h"

#include "imgui-framework.h"

#include "android-assetcopy.h"

#include "StringEx.h"

/*

todo : draw windows in 3d using framework

todo : tick events for windows in 3d
	- let the app or framework determine pointer direction and button presses
	- intersect pointer with windows. determine nearest intersection
	- on button press, make the intersecting window active
	- for all windows, generate mouse.x, mouse.y and button presses

 */

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include <VrApi_SystemUtils.h>

#include <android/input.h>
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON

#include <math.h>
#include <time.h>
#include <unistd.h>

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;

// -- system clock time

static double GetTimeInSeconds()
{
    // note : this time must match the time used internally by the VrApi. this is due to us having
    //        to simulate the time step since the last update until the predicted (by the vr system) display time

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

// -- VrApi

ovrMobile * getOvrMobile();

// -- scene

#include "gx_mesh.h"
#include "image.h"
#include "../../../3rdparty/ovr-mobile/VrApi/Include/VrApi_Input.h"

struct PointerObject
{
	Mat4x4 transform = Mat4x4(true);
	bool isValid = false;

	bool isDown = false;
	float pictureTimer = 0.f;
};

struct Scene
{
    bool created = false;

	Vec3 playerLocation;

    // ImGui test
    Window * guiWindow = nullptr;
    FrameworkImGuiContext guiContext;
    ParameterMgr parameterMgr;

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
	guiWindow = new Window("window", 300, 300);
	guiContext.init(false);
	parameterMgr.addString("name", "");
	parameterMgr.addInt("count", 0)->setLimits(0, 100);
	parameterMgr.addFloat("speed", 0.f)->setLimits(0.f, 10.f);

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
	// destroy imgui related objects

	guiContext.shut();

	delete guiWindow;
	guiWindow = nullptr;

    created = false;
}

void Scene::tick(ovrMobile * ovr, const float dt, const double predictedDisplayTime)
{
static bool isPinching = false; // todo : remove hack

	// update window positions

	Mat4x4 transform = Mat4x4(true).Translate(0, 0, -.3f);//.RotateY(float(M_PI));
	guiWindow->setTransform(transform);

	// update windows

// todo : move this to framework

// todo : let the user specify the pointer origin and direction. or the pointer transform
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	Mat4x4 viewToWorld = worldToView.CalcInv();
	const Vec3 pointerOrigin = viewToWorld.GetTranslation();
	const Vec3 pointerDirection = viewToWorld.GetAxis(2).CalcNormalized();

	auto * windowData = guiWindow->getWindowData();
	windowData->beginProcess();
	windowData->endProcess();

	Vec2 pixelPos;
	float distance;
	if (guiWindow->intersectRay(pointerOrigin, pointerDirection, pixelPos, distance))
	{
		windowData->mouseData.mouseX = pixelPos[0];
		windowData->mouseData.mouseY = pixelPos[1];
		windowData->mouseData.mouseDown[0] = isPinching;
	}

	// update gui window

	pushWindow(*guiWindow);
	{
		bool inputIsCaptured = false;
		guiContext.processBegin(.01f, guiWindow->getWidth(), guiWindow->getHeight(), inputIsCaptured);
		{
			ImGui::SetNextWindowPos(ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(guiWindow->getWidth(), guiWindow->getHeight()));

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

	uint32_t index = 0;

isPinching = false; // todo : remove hack

	for (;;)
	{
		ovrInputCapabilityHeader header;

		const auto result = vrapi_EnumerateInputDevices(ovr, index++, &header);

		if (result < 0)
			break;

// todo : vrapi_SetHapticVibrationSimple(ovrMobile* ovr, const ovrDeviceID deviceID, const float intensity)
		if (header.Type == ovrControllerType_TrackedRemote)
		{
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

					const bool wasDown = pointer.isDown;

					if (state.Buttons & ovrButton_Trigger)
						pointer.isDown = true;
					else
						pointer.isDown = false;

					if (pointer.isDown != wasDown)
					{
						if (pointer.isDown && pointer.isValid && index == 1)
						{
							playerLocation += pointer.transform.GetAxis(2) * -6.f;
						}
					}

					pointer.pictureTimer = clamp<float>(pointer.pictureTimer + (pointer.isDown ? +1 : -1) * .01f / .4f, 0.f, 1.f);
				}
			}
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
	setFont("calibri.ttf");

	pushWindow(*guiWindow);
	{
		framework.beginDraw(0, 0, 0, 255);
		{
			guiContext.draw();
		}
		framework.endDraw();
	}
	popWindow();
}

void Scene::drawEye(ovrMobile * ovr) const
{
    setFont("calibri.ttf");

	const double time = GetTimeInSeconds();

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

    {
	    ovrPosef boundaryPose;
	    ovrVector3f boundaryScale;
	    if (vrapi_GetBoundaryOrientedBoundingBox(ovr, &boundaryPose, &boundaryScale) == ovrSuccess)
	        ground_y = boundaryPose.Translation.y;
    }

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

		// Draw picture.

		gxSetTexture(guiWindow->getColorTarget()->getTextureId());
		setColor(colorWhite);
		{
			gxPushMatrix();
			gxMultMatrixf(pointer.transform.m_v);
			gxTranslatef(0, 0, -3);
			gxRotatef(180 + sin(time) * 15, 0, 1, 0);
			gxScalef(pointer.pictureTimer, pointer.pictureTimer, pointer.pictureTimer);
			drawRect(+1, +1, -1, -1);
			gxPopMatrix();
		}
		gxSetTexture(0);

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

		#if true
		    handMeshes[0].mesh.draw();
		#endif
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
				if (vrapi_GetHandPose(ovr, cap.DeviceID, GetTimeInSeconds(), &handPose.Header) == ovrSuccess &&
					handPose.Status == ovrHandTrackingStatus_Tracked &&
					(handPose.HandConfidence == ovrConfidence_HIGH || true))
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

// -- FrameworkVr

#include "framework-android-app.h"
#include <android_native_app_glue.h>

class FrameworkVr
{
    ovrJava Java;
    ovrEgl * Egl = nullptr;

public:
    ANativeWindow * NativeWindow = nullptr;
    bool Resumed = false;

    ovrMobile * Ovr = nullptr;

    long long FrameIndex = 1;
    double PredictedDisplayTime = 0.0;

	ovrMobile * getOvrMobile()
	{
		return Ovr;
	}

private:
    int SwapInterval = 1;
    int CpuLevel = CPU_LEVEL;
    int GpuLevel = GPU_LEVEL;
    int MainThreadTid = 0;
    int RenderThreadTid = 0;
    bool BackButtonDownLastFrame = false;
    bool GamePadBackButtonDown = false;

	// Rendering.
	ovrFramebuffer FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
	int NumBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
    bool UseMultiview = true;

    double StartTime = 0.0;

private:
	// Frame state.
	ovrTracking2 Tracking;
	Mat4x4 ProjectionMatrices[2];
	Mat4x4 ViewMatrices[2];
	ovrLayerProjection2 WorldLayer;

private:
	// Draw state.
	int currentEyeIndex = -1;

public:
    bool init(ovrEgl * egl);
    void shutdown();

	void process();

private:
    void nextFrame();
public:
    void submitFrameAndPresent();

	int getEyeCount() const
    {
        return NumBuffers;
    }

	void beginEye(const int eyeIndex, const Color & clearColor);
	void endEye();

private:
	void pushBlackFinal();

private:
	void processEvents();
	void handleVrModeChanges();
	void handleDeviceInput();
	int handleKeyEvent(const int keyCode, const int action);
	void handleVrApiEvents();

public:
	float TimeStep = 0.f;
};

//

FrameworkVr frameworkVr;

ovrMobile * getOvrMobile()
{
	return frameworkVr.getOvrMobile();
}

//

bool FrameworkVr::init(ovrEgl * egl)
{
	android_app * app = get_android_app();

	ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	Java.Vm = app->activity->vm;
	Java.Vm->AttachCurrentThread(&Java.Env, nullptr);
	Java.ActivityObject = app->activity->clazz;

	const ovrInitParms initParms = vrapi_DefaultInitParms(&Java);
	const int32_t initResult = vrapi_Initialize(&initParms);

	if (initResult != VRAPI_INITIALIZE_SUCCESS)
	{
		// If intialization failed, vrapi_* function calls will not be available.
		logError("failed to initialize VrApi: %d", initResult);
		return false;
	}

	Egl = egl;

// todo : the native activity example doesn't do this. why? and what does it do anyway?
    // This app will handle android gamepad events itself.
    vrapi_SetPropertyInt(&Java, VRAPI_EAT_NATIVE_GAMEPAD_EVENTS, 0);

	MainThreadTid = gettid();

    const bool useMultiview = false;

    logDebug("useMultiview: %d", useMultiview ? 1 : 0);

	NumBuffers = useMultiview ? 1 : VRAPI_FRAME_LAYER_EYE_MAX;

    // Create the frame buffers.
	for (int eyeIndex = 0; eyeIndex < NumBuffers; ++eyeIndex)
    {
        FrameBuffer[eyeIndex].init(
            useMultiview,
            GL_RGBA8,
            vrapi_GetSystemPropertyInt(&Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
            vrapi_GetSystemPropertyInt(&Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
            NUM_MULTI_SAMPLES);
    }

	StartTime = GetTimeInSeconds();

	return true;
}

void FrameworkVr::shutdown()
{
    for (int eyeIndex = 0; eyeIndex < NumBuffers; ++eyeIndex)
        FrameBuffer[eyeIndex].shut();

	Java.Vm->DetachCurrentThread();
}

void FrameworkVr::process()
{
	// process app events

	processEvents();

	// update head tracking info and prepare for drawing the next frame

	nextFrame();

	// calculate the time step, based on the current and previous predicted display time

    TimeStep = PredictedDisplayTime - StartTime;

    StartTime = PredictedDisplayTime;
}

void FrameworkVr::nextFrame()
{
	// This is the only place the frame index is incremented, right before calling vrapi_GetPredictedDisplayTime().
    FrameIndex++;

    // Get the HMD pose, predicted for the middle of the time period during which
    // the new eye images will be displayed. The number of frames predicted ahead
    // depends on the pipeline depth of the engine and the synthesis rate.
    // The better the prediction, the less black will be pulled in at the edges.
	PredictedDisplayTime = vrapi_GetPredictedDisplayTime(Ovr, FrameIndex);
    Tracking = vrapi_GetPredictedTracking2(Ovr, PredictedDisplayTime);

	// Get the projection and view matrices using the predicted tracking info.
	ovrMatrix4f eyeViewMatrixTransposed[2];
	eyeViewMatrixTransposed[0] = ovrMatrix4f_Transpose(&Tracking.Eye[0].ViewMatrix);
	eyeViewMatrixTransposed[1] = ovrMatrix4f_Transpose(&Tracking.Eye[1].ViewMatrix);

	ovrMatrix4f projectionMatrixTransposed[2];
	projectionMatrixTransposed[0] = ovrMatrix4f_Transpose(&Tracking.Eye[0].ProjectionMatrix);
	projectionMatrixTransposed[1] = ovrMatrix4f_Transpose(&Tracking.Eye[1].ProjectionMatrix);

	memcpy(ProjectionMatrices, projectionMatrixTransposed, sizeof(ProjectionMatrices));
	memcpy(ViewMatrices, eyeViewMatrixTransposed, sizeof(ViewMatrices));

	// Setup the projection layer we want to display.
	ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
	layer.HeadPose = Tracking.HeadPose;

	for (int eyeIndex = 0; eyeIndex < VRAPI_FRAME_LAYER_EYE_MAX; ++eyeIndex)
	{
		ovrFramebuffer * frameBuffer = &FrameBuffer[NumBuffers == 1 ? 0 : eyeIndex];
		layer.Textures[eyeIndex].ColorSwapChain = frameBuffer->ColorTextureSwapChain;
		layer.Textures[eyeIndex].SwapChainIndex = frameBuffer->TextureSwapChainIndex;
		layer.Textures[eyeIndex].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&Tracking.Eye[eyeIndex].ProjectionMatrix);
	}
	layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

	WorldLayer = layer;
}

void FrameworkVr::submitFrameAndPresent()
{
	const ovrLayerHeader2 * layers[] = { &WorldLayer.Header };

	ovrSubmitFrameDescription2 frameDesc = { };
	frameDesc.Flags = 0;
	frameDesc.SwapInterval = SwapInterval;
	frameDesc.FrameIndex = FrameIndex;
	frameDesc.DisplayTime = PredictedDisplayTime;
	frameDesc.LayerCount = 1;
	frameDesc.Layers = layers;

	// Hand over the eye images to the time warp.
	vrapi_SubmitFrame2(Ovr, &frameDesc);
}

void FrameworkVr::beginEye(const int eyeIndex, const Color & clearColor)
{
	Assert(currentEyeIndex == -1);
	currentEyeIndex = eyeIndex;

	// NOTE: In the non-mv case, latency can be further reduced by updating the sensor
	// prediction for each eye (updates orientation, not position)
	ovrFramebuffer * frameBuffer = &FrameBuffer[eyeIndex];
	ovrFramebuffer_SetCurrent(frameBuffer);

	gxSetMatrixf(GX_PROJECTION, ProjectionMatrices[eyeIndex].m_v);
	gxSetMatrixf(GX_MODELVIEW, ViewMatrices[eyeIndex].m_v);

	glViewport(0, 0, frameBuffer->Width, frameBuffer->Height);
	glScissor(0, 0, frameBuffer->Width, frameBuffer->Height);
	checkErrorGL();

	glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
	glClearDepthf(1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	checkErrorGL();

	glEnable(GL_SCISSOR_TEST);
	checkErrorGL();
}

void FrameworkVr::endEye()
{
	Assert(currentEyeIndex != -1);
	ovrFramebuffer * frameBuffer = &FrameBuffer[currentEyeIndex];
	currentEyeIndex = -1;

	// Explicitly clear the border texels to black when GL_CLAMP_TO_BORDER is not available.
	if (ovrOpenGLExtensions.EXT_texture_border_clamp == false)
		ovrFramebuffer_ClearBorder(frameBuffer);

	glDisable(GL_SCISSOR_TEST);
	checkErrorGL();

	ovrFramebuffer_Resolve(frameBuffer);
	ovrFramebuffer_Advance(frameBuffer);

	ovrFramebuffer_SetNone();
}

void FrameworkVr::pushBlackFinal()
{
    const int frameFlags = VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL;

    ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

    const ovrLayerHeader2 * layers[] = { &layer.Header };

    ovrSubmitFrameDescription2 frameDesc = { };
    frameDesc.Flags = frameFlags;
    frameDesc.SwapInterval = 1;
    frameDesc.FrameIndex = FrameIndex;
    frameDesc.DisplayTime = PredictedDisplayTime;
    frameDesc.LayerCount = 1;
    frameDesc.Layers = layers;

    vrapi_SubmitFrame2(Ovr, &frameDesc);
}

void FrameworkVr::processEvents()
{
	android_app * app = get_android_app();

	for (;;)
	{
		// process app events

		for (;;)
		{
			Assert(Resumed || Ovr == nullptr); // Ovr must be nullptr when not Resumed

			const bool waitForEvents = (Ovr == nullptr && app->destroyRequested == 0);

			int events;
			struct android_poll_source *source;
	        const int ident = ALooper_pollAll(waitForEvents ? -1 : 0, NULL, &events, (void **) &source);

	        if (ident < 0)
	            break;

            if (ident == LOOPER_ID_MAIN)
            {
                auto cmd = android_app_read_cmd(app);

                android_app_pre_exec_cmd(app, cmd);

				switch (cmd)
				{
				case APP_CMD_START:
                    logDebug("APP_CMD_START");
                    break;

				case APP_CMD_STOP:
                    logDebug("APP_CMD_STOP");
                    break;

				case APP_CMD_RESUME:
                    logDebug("APP_CMD_RESUME");
                    Resumed = true;
                    break;

				case APP_CMD_PAUSE:
                    logDebug("APP_CMD_PAUSE");
                    Resumed = false;
                    break;

				case APP_CMD_INIT_WINDOW:
                    logDebug("APP_CMD_INIT_WINDOW");
                    NativeWindow = app->window;
                    break;

				case APP_CMD_TERM_WINDOW:
                    logDebug("APP_CMD_TERM_WINDOW");
                    NativeWindow = nullptr;
                    break;

				case APP_CMD_CONTENT_RECT_CHANGED:
                    if (ANativeWindow_getWidth(app->window) < ANativeWindow_getHeight(app->window))
                    {
                        // An app that is relaunched after pressing the home button gets an initial surface with
                        // the wrong orientation even though android:screenOrientation="landscape" is set in the
                        // manifest. The choreographer callback will also never be called for this surface because
                        // the surface is immediately replaced with a new surface with the correct orientation.
                        logError("- Surface not in landscape mode!");
                    }

                    if (app->window != NativeWindow)
                    {
                        if (NativeWindow != nullptr)
                        {
                            // todo : perform actions due to window being destroyed
                            NativeWindow = nullptr;
                        }

                        if (app->window != nullptr)
                        {
                            // todo : perform actions due to window being created
                            NativeWindow = app->window;
                        }
                    }
                    break;

				default:
                    break;
				}

                android_app_post_exec_cmd(app, cmd);
            }
            else if (ident == LOOPER_ID_INPUT)
            {
                AInputEvent * event = nullptr;
                while (AInputQueue_getEvent(app->inputQueue, &event) >= 0)
                {
                    //if (AInputQueue_preDispatchEvent(app->inputQueue, event))
                    //    continue;

                    int32_t handled = 0;

                    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
                    {
                        const int keyCode = AKeyEvent_getKeyCode(event);
                        const int action = AKeyEvent_getAction(event);

                        if (action != AKEY_EVENT_ACTION_DOWN &&
                            action != AKEY_EVENT_ACTION_UP)
                        {
                            // dispatch
                        }
                        else if (keyCode == AKEYCODE_VOLUME_UP)
                        {
                            // dispatch
                        }
                        else if (keyCode == AKEYCODE_VOLUME_DOWN)
                        {
                            // dispatch
                        }
                        else
                        {
                            if (handleKeyEvent(
                                keyCode,
                                action))
                            {
                                handled = 1;
                            }
                        }
                    }

                    AInputQueue_finishEvent(app->inputQueue, event, handled);
                }
            }

            // Update vr mode based on the current app state.
	        handleVrModeChanges();
        }

        // We must read from the event queue with regular frequency.
        handleVrApiEvents();

        // Handle device input, to see if we should present the system ui.
        handleDeviceInput();


        // When we're no in vr mode.. rather than letting the app simulate
        // and draw the next frame, we will wait for events until we are back
        // in vr mode again.
        if (Ovr == nullptr)
            continue;

        // Done!
        break;
    }
}

void FrameworkVr::handleVrModeChanges()
{
    if (Resumed != false && NativeWindow != nullptr)
    {
        if (Ovr == nullptr)
        {
            ovrModeParms parms = vrapi_DefaultModeParms(&Java);

            // Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView
            parms.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

            parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
            parms.Display = (size_t)Egl->Display;
            parms.WindowSurface = (size_t)NativeWindow;
            parms.ShareContext = (size_t)Egl->Context;

            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            logDebug("- vrapi_EnterVrMode()");
            Ovr = vrapi_EnterVrMode(&parms);

            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            // If entering VR mode failed then the ANativeWindow was not valid.
            if (Ovr == nullptr)
            {
                logError("Invalid ANativeWindow!");
                NativeWindow = nullptr;
            }

            // Set performance parameters once we have entered VR mode and have a valid ovrMobile.
            if (Ovr != nullptr)
            {
                vrapi_SetClockLevels(Ovr, CpuLevel, GpuLevel);
                logDebug("- vrapi_SetClockLevels( %d, %d )", CpuLevel, GpuLevel);

                vrapi_SetPerfThread(Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, MainThreadTid);
                logDebug("- vrapi_SetPerfThread( MAIN, %d )", MainThreadTid);

                vrapi_SetPerfThread(Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, RenderThreadTid);
                logDebug("- vrapi_SetPerfThread( RENDERER, %d )", RenderThreadTid);
            }
        }
    }
    else
    {
        if (Ovr != nullptr)
        {
            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));
            logDebug("- vrapi_LeaveVrMode()");
	        {
                vrapi_LeaveVrMode(Ovr);
                Ovr = nullptr;
	        }
            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));
        }
    }
}

void FrameworkVr::handleDeviceInput()
{
    bool backButtonDownThisFrame = false;

    for (int i = 0; true; i++)
    {
        ovrInputCapabilityHeader cap;
        const ovrResult result = vrapi_EnumerateInputDevices(Ovr, i, &cap);
        if (result < 0)
            break;

        if (cap.Type == ovrControllerType_Headset)
        {
            ovrInputStateHeadset headsetInputState;
            headsetInputState.Header.ControllerType = ovrControllerType_Headset;
            const ovrResult result = vrapi_GetCurrentInputState(Ovr, cap.DeviceID, &headsetInputState.Header);
            if (result == ovrSuccess)
                backButtonDownThisFrame |= headsetInputState.Buttons & ovrButton_Back;
        }
        else if (cap.Type == ovrControllerType_TrackedRemote)
        {
            ovrInputStateTrackedRemote trackedRemoteState;
            trackedRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
            const ovrResult result = vrapi_GetCurrentInputState(Ovr, cap.DeviceID, &trackedRemoteState.Header);
            if (result == ovrSuccess)
            {
                backButtonDownThisFrame |= trackedRemoteState.Buttons & ovrButton_Back;
                backButtonDownThisFrame |= trackedRemoteState.Buttons & ovrButton_B;
                backButtonDownThisFrame |= trackedRemoteState.Buttons & ovrButton_Y;
            }
        }
        else if (cap.Type == ovrControllerType_Gamepad)
        {
            ovrInputStateGamepad gamepadState;
            gamepadState.Header.ControllerType = ovrControllerType_Gamepad;
            const ovrResult result = vrapi_GetCurrentInputState(Ovr, cap.DeviceID, &gamepadState.Header);
            if (result == ovrSuccess)
                backButtonDownThisFrame |= gamepadState.Buttons & ovrButton_Back;
        }
    }

    backButtonDownThisFrame |= GamePadBackButtonDown;

    if (BackButtonDownLastFrame && !backButtonDownThisFrame)
    {
        logDebug("back button short press");
        logDebug("- pushBlackFinal()");
        pushBlackFinal();
        logDebug("- vrapi_ShowSystemUI( confirmQuit )");
        vrapi_ShowSystemUI(&Java, VRAPI_SYS_UI_CONFIRM_QUIT_MENU);
    }

	BackButtonDownLastFrame = backButtonDownThisFrame;
}

int FrameworkVr::handleKeyEvent(const int keyCode, const int action)
{
    // Handle back button.
    if (keyCode == AKEYCODE_BACK || keyCode == AKEYCODE_BUTTON_B)
    {
        if (action == AKEY_EVENT_ACTION_DOWN)
            GamePadBackButtonDown = true;
        else if (action == AKEY_EVENT_ACTION_UP)
            GamePadBackButtonDown = false;
        return 1;
    }

    return 0;
}

void FrameworkVr::handleVrApiEvents()
{
    ovrEventDataBuffer eventDataBuffer;

    // Poll for VrApi events
    for (;;)
    {
        ovrEventHeader * eventHeader = (ovrEventHeader*)&eventDataBuffer;
        const ovrResult res = vrapi_PollEvent(eventHeader);
        if (res != ovrSuccess)
            break;

        switch (eventHeader->EventType)
        {
            case VRAPI_EVENT_DATA_LOST:
                logDebug("vrapi_PollEvent: Received VRAPI_EVENT_DATA_LOST");
                break;

            case VRAPI_EVENT_VISIBILITY_GAINED:
                logDebug("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_GAINED");
                break;

            case VRAPI_EVENT_VISIBILITY_LOST:
                logDebug("vrapi_PollEvent: Received VRAPI_EVENT_VISIBILITY_LOST");
                break;

            case VRAPI_EVENT_FOCUS_GAINED:
                // FOCUS_GAINED is sent when the application is in the foreground and has
                // input focus. This may be due to a system overlay relinquishing focus
                // back to the application.
                logDebug("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_GAINED");
                break;

            case VRAPI_EVENT_FOCUS_LOST:
                // FOCUS_LOST is sent when the application is no longer in the foreground and
                // therefore does not have input focus. This may be due to a system overlay taking
                // focus from the application. The application should take appropriate action when
                // this occurs.
                logDebug("vrapi_PollEvent: Received VRAPI_EVENT_FOCUS_LOST");
                break;

            default:
                logDebug("vrapi_PollEvent: Unknown event");
                break;
        }
    }
}

/*
================================================================================

Android Main (android_native_app_glue)

================================================================================
*/

#include "framework-android-app.h"
#include <android_native_app_glue.h>
#include <android/native_activity.h>

int main(int argc, char * argv[])
{
    android_app * app = get_android_app();

    const double t1 = GetTimeInSeconds();
    const bool copied_files =
	    chdir(app->activity->internalDataPath) == 0 &&
        assetcopy::recursively_copy_assets_to_filesystem(
		    app->activity->vm,
		    app->activity->clazz,
		    app->activity->assetManager,
		    "") &&
	    chdir(app->activity->internalDataPath) == 0;
    const double t2 = GetTimeInSeconds();
    logInfo("asset copying took %.2f seconds", (t2 - t1));

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

    while (!app->destroyRequested)
    {
        //framework.process();
	    frameworkVr.process();

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
