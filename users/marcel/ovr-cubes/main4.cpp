#include "ovr-egl.h"
#include "ovr-framebuffer.h"
#include "ovr-glext.h"

#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "magicavoxel-framework.h"

#include "parameter.h"
#include "parameterUi.h"

#include "audioTypeDB.h"
#include "graphEdit.h"

#include "data/engine/ShaderCommon.txt" // VS_ constants
#include "framework.h"
#include "gx_render.h"

#include "imgui-framework.h"

#include "android-assetcopy.h"

#include "StringEx.h"

/*

 DONE : integrate MagicaVoxel model drawing

 DONE : integrate gltf model drawing

 todo : integrate libbullet3 for terrain collisions

 DONE : integrate audiograph for some audio processing: first step towards 4dworld in vr

 todo : integrate vfxgraph for some vfx processing: let it modulate lines or points and draw them in 3d

 DONE : check if Window class works conveniently with render passes and color/depth targets
 todo : integrate 3d virtual pointer with Window objects. create a small 3d Window picking system? should this be part of framework? perhaps just prototype it here and see what feels right

 DONE : integrate imgui drawing to a Window. composite the result in 3d

 DONE : test if drawing text in 3d using sdf fonts works

 DONE : integrate jgmod

 todo : integrate libparticle

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

#define USE_FRAMEWORK_DRAWING 1

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

	// gltf
// todo : add gltf::BufferedScene ? something is needed to make gltf scenes more usable. now always requires a separate buffer cache object ..
	gltf::Scene gltf_scene;
	gltf::BufferCache gltf_bufferCache;

	// MagicaVoxel
	MagicaWorld magica_world;
	GxMesh magica_mesh;
    GxVertexBuffer magica_vb;
    GxIndexBuffer magica_ib;

	// terrain
    GxMesh terrain_mesh;
    GxVertexBuffer terrain_vb;
    GxIndexBuffer terrain_ib;

    // ImGui test
    Window * guiWindow = nullptr;
    FrameworkImGuiContext guiContext;
    ParameterMgr parameterMgr;

    // graph editor
    Window * graphEditWindow = nullptr;
	Graph_TypeDefinitionLibrary * audioTypes = nullptr;
	GraphEdit * graphEdit = nullptr;

	// hand mesh
	struct HandMesh
	{
		GxMesh mesh;
		GxVertexBuffer vb;
		GxIndexBuffer ib;
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
	//gltf::loadScene("lain_2.0/scene.gltf", gltf_scene);
	gltf::loadScene("Suzanne/glTF/Suzanne.gltf", gltf_scene);
	gltf_bufferCache.init(gltf_scene);

	if (!readMagicaWorld("room.vox", magica_world))
		logError("failed to read MagicaVoxel world");

	gxCaptureMeshBegin(magica_mesh, magica_vb, magica_ib);
	{
		drawMagicaWorld(magica_world);
	}
	gxCaptureMeshEnd();

	guiWindow = new Window("window", 300, 300);
	guiContext.init(false);
	parameterMgr.addString("name", "");
	parameterMgr.addInt("count", 0)->setLimits(0, 100);
	parameterMgr.addFloat("speed", 0.f)->setLimits(0.f, 10.f);

	graphEditWindow = new Window("Graph Editor", 300, 300);
    audioTypes = createAudioTypeDefinitionLibrary();
    graphEdit = new GraphEdit(graphEditWindow->getWidth(), graphEditWindow->getHeight(), audioTypes); // todo : remove width/heigth from editor ctor. or make optional. sx/sy should be passed on tick instead
	graphEdit->load("sweetStuff6.xml");

	for (int i = 0; i < 2; ++i)
	{
		ovrHandMesh handMesh;
		handMesh.Header.Version = ovrHandVersion_1;
		if (vrapi_GetHandMesh(getOvrMobile(), i == 0 ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT, &handMesh.Header) == ovrSuccess)
		{
			GxVertexInput vsInputs[] =
			{
				{ VS_POSITION,      3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexPositions), sizeof(ovrVector3f) },
				//{ VS_NORMAL,        3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexNormals),   sizeof(ovrVector3f) },
				{ VS_COLOR,         3, GX_ELEMENT_FLOAT32, 0, offsetof(ovrHandMesh, VertexNormals),   sizeof(ovrVector3f) },
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
		}
	}

    auto * image = loadImage("Hokkaido8.png");
    if (image != nullptr)
    {
        gxCaptureMeshBegin(terrain_mesh, terrain_vb, terrain_ib);
	    {
	        setColor(colorWhite);
	        gxBegin(GX_QUADS);
		    //gxBegin(GX_POINTS);
	        const int stride = 3; // 2 when points, 3 when quads
		    for (int y = 0; y < image->sy - stride; y += stride)
		    {
			    ImageData::Pixel * line1 = image->getLine(y + 0);
			    ImageData::Pixel * line2 = image->getLine(y + stride);
			    ImageData::Pixel * line[2] = { line1, line2 };
	            for (int x = 0; x < image->sx - stride; x += stride)
	            {
				#define emitVertex(ox, oy) \
		            gxVertex3f( \
				            ((x + ox) + .5f - image->sx / 2.f) * .1f, \
				            line[oy/stride][x + ox].r / 40.f - 3, \
				            ((y + oy) + .5f - image->sy / 2.f) * .1f);

		            gxColor3ub(line1[x].r + 127, line1[x].r, 255 - line1[x].r);

				#if false
		            emitVertex(0,           0);
				#else
		            emitVertex(0,           0);
		            emitVertex(stride,      0);
		            emitVertex(stride, stride);
		            emitVertex(0,      stride);
				#endif

				#undef emitVertex
	            }
	        }
	        gxEnd();
	    }
	    gxCaptureMeshEnd();

        delete image;
        image = nullptr;
    }

    created = true;
}

void Scene::destroy()
{
	// destroy terrain resources

	terrain_mesh.clear();
	terrain_vb.free();
	terrain_ib.free();

	// destroy graph editor related objects

	delete graphEdit;
	graphEdit = nullptr;

	delete audioTypes;
	audioTypes = nullptr;

	delete graphEditWindow;
	graphEditWindow = nullptr;

	// destroy imgui related objects

	guiContext.shut();

	delete guiWindow;
	guiWindow = nullptr;

	magica_world.free();

	// todo : clear gltf scene

    created = false;
}

void Scene::tick(ovrMobile * ovr, const float dt, const double predictedDisplayTime)
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

	uint32_t index = 0;

	for (;;)
	{
		ovrInputCapabilityHeader header;

		const auto result = vrapi_EnumerateInputDevices(ovr, index++, &header);

		if (result < 0)
			break;

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

	graphEdit->displaySx = graphEditWindow->getWidth();
	graphEdit->displaySy = graphEditWindow->getHeight();
	graphEdit->tick(.01f, false);

	pushWindow(*graphEditWindow);
	{
		framework.beginDraw(0, 0, 0, 0);
		{
			pushFontMode(FONT_SDF);
			graphEdit->draw();
			popFontMode();
		}
		framework.endDraw();
	}
	popWindow();
}

void Scene::drawEye(ovrMobile * ovr) const
{
    setFont("calibri.ttf");

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
    gxTranslatef(0, ground_y, 0);

    const double time = GetTimeInSeconds();

	// Draw gltf scene.
	gxPushMatrix();
    {
	    gxTranslatef(0, 2.5f, 0);

		gltf::MaterialShaders materialShaders;
		gltf::setDefaultMaterialShaders(materialShaders);
		gltf::setDefaultMaterialLighting(materialShaders, worldToView, Vec3(.3f, -1.f, .3f).CalcNormalized(), Vec3(10.f, .8f, .6f), Vec3(.2f));

		for (int i = 0; i < 10; ++i)
		{
			gxPushMatrix();
		    gxTranslatef(sinf(i * 1.23f) * 1.f, sinf(i / 1.23f) * .5f, sinf(i * 2.34f) * 1.f);
	        gxRotatef((time + i) * 40.f, .3f, 1, .2f);
	        gxScalef(.1f, .1f, .1f);
			gltf::DrawOptions drawOptions;
			gltf::drawScene(gltf_scene, &gltf_bufferCache, materialShaders, true, &drawOptions);
			gxPopMatrix();
		}
    }
	gxPopMatrix();

	// Draw MagicVoxel world
    gxPushMatrix();
    gxScalef(.01f, .01f, .01f);
    pushShaderOutputs("n");
	//drawMagicaWorld(magica_world);
	//magica_mesh.draw();
	popShaderOutputs();
	gxPopMatrix();

    // Draw terrain.
    terrain_mesh.draw();

	// Draw circles.
    for (int i = 0; i < 16; ++i)
    {
	    gxPushMatrix();

        const float t = float(sin(time / 4.0 + i) + 1.f) / 2.f;
	    const float size = lerp<float>(1.f, 4.f, t);
	    const float c = 1.f - t;

	    const float y_t = float(sin(time / 6.0 / 2.34f + i) + 1.f) / 2.f;
	    const float y = lerp<float>(0.f, 4.f, y_t);
	    gxTranslatef(0, y, 0);
	    gxRotatef(90, 1, 0, 0);

	    setColorf(c, c, c, 1.f);
	    drawCircle(0.f, 0.f, size, 100);

	    gxPopMatrix();
    }

    gxPopMatrix(); // Undo ground level adjustment.

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

		gxSetTexture(i == 0
			? guiWindow->getColorTarget()->getTextureId()
			: graphEditWindow->getColorTarget()->getTextureId());
		//gxSetTexture(graphEditWindow->getColorTarget()->getTextureId());
		//gxSetTexture(getTexture("sabana.jpg"));
		//gxSetTextureSampler(GX_SAMPLE_MIPMAP, true);
		setColor(colorWhite);
		{
			gxPushMatrix();
			gxMultMatrixf(pointer.transform.m_v);
			gxTranslatef(0, 0, -3);
			gxRotatef(180 + sinf(time) * 15, 0, 1, 0);
			gxScalef(pointer.pictureTimer, pointer.pictureTimer, pointer.pictureTimer);
			drawRect(+1, +1, -1, -1);
			gxPopMatrix();
		}
		gxSetTexture(0);

		// Draw cube.
		gxPushMatrix();
		gxMultMatrixf(pointer.transform.m_v);

		pushCullMode(CULL_BACK, CULL_CCW);
		pushShaderOutputs("n");
		setColor(colorWhite);
		fillCube(Vec3(), Vec3(.02f, .02f, .1f));
		popShaderOutputs();

		// Draw pointer ray.
		pushBlend(BLEND_ADD);
		const float a = lerp<float>(.1f, .4f, (sin(time) + 1.0) / 2.0);
		setColorf(1, 1, 1, a);
		fillCube(Vec3(0, 0, -100), Vec3(.01f, .01f, 100));
		popBlend();
		popCullMode();

	#if true
	    handMeshes[0].mesh.draw();
	#endif

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
					handPose.HandConfidence == ovrConfidence_HIGH)
				{
					// Fetch the initial pose for the hand.
					// We will need to combine it with the current pose, which only defines the new orientations for the bones.
					// And we will need it to perform skinning as well.
					ovrHandSkeleton handSkeleton;
					handSkeleton.Header.Version = ovrHandVersion_1;
					if (vrapi_GetHandSkeleton(ovr, index == 0 ? VRAPI_HAND_LEFT : VRAPI_HAND_RIGHT, &handSkeleton.Header) == ovrSuccess)
					{
						// Update local bone poses.
						ovrPosef localBonePoses[ovrHand_MaxBones];
						for (int i = 0; i < handSkeleton.NumBones; ++i)
						{
							localBonePoses[i] = handSkeleton.BonePoses[i];
							localBonePoses[i].Orientation = handPose.BoneRotations[i];
						}

						// Calculate global bone transforms.
						ovrMatrix4f globalBoneTransforms[ovrHand_MaxBones];
						for (int i = 0; i < handSkeleton.NumBones; ++i)
						{
							ovrMatrix4f globalBoneTransform = ovrMatrix4f_CreateIdentity();

							for (int index = i; index >= 0; index = handSkeleton.BoneParentIndices[index])
							{
								auto & pose = localBonePoses[index];
								ovrMatrix4f transform = vrapi_GetTransformFromPose(&pose);
								globalBoneTransform = ovrMatrix4f_Multiply(&transform, &globalBoneTransform);
							}

							globalBoneTransforms[i] = globalBoneTransform;
						}

						ovrMatrix4f rootPose = vrapi_GetTransformFromPose(&handPose.RootPose);
						rootPose = ovrMatrix4f_Transpose(&rootPose);

						// Draw bones.
						gxPushMatrix();
						gxMultMatrixf((float*)rootPose.M);
						gxBegin(GX_LINES);
						{
							for (int i = 0; i < handSkeleton.NumBones; ++i)
							{
								const int parentIndex = handSkeleton.BoneParentIndices[i];

								if (parentIndex >= 0)
								{
									ovrVector4f origin = { 0, 0, 0, 1 };
									ovrVector4f position1 = ovrVector4f_MultiplyMatrix4f(&globalBoneTransforms[i], &origin);
									ovrVector4f position2 = ovrVector4f_MultiplyMatrix4f(&globalBoneTransforms[parentIndex], &origin);

									setColor(colorGreen);
									gxVertex3f(position1.x, position1.y, position1.z);
									gxVertex3f(position2.x, position2.y, position2.z);
								}
							}
						}
						gxEnd();
						gxPopMatrix();
					}

					// Draw hand mesh.

					ovrMatrix4f rootPose = vrapi_GetTransformFromPose(&handPose.RootPose);
					rootPose = ovrMatrix4f_Transpose(&rootPose);

					gxPushMatrix();
					gxMultMatrixf((float*)rootPose.M);
					//handMeshes[index].mesh.draw();
					gxPopMatrix();
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

	// Draw text.
	gxPushMatrix();
    {
        gxTranslatef(0, 2, 0);
        gxRotatef(time * 10.f, 0, 1, 0);
        gxScalef(1, -1, 1);
        pushFontMode(FONT_SDF);
	    {
	    // todo : add a function to directly draw 3d msdf text. msdf or sdf is really a requirement for vr
	        setColor(Color::fromHSL(time / 4.f, .5f, .5f));
	        drawTextArea(0, 0, 1.f, .25f, .1f, 0, 0, "Hello World!\nThis is some text, drawn using Framework's multiple signed-distance field method! Doesn't it look crisp? :-)", 2.f, 2.f);
	    }
	    popFontMode();

		setColor(40, 30, 20);
	    fillCircle(0, 0, 1.2f/2.f, 100);
    }
    gxPopMatrix();

#if false
	// Draw filled circles.
    for (int i = 0; i < 16; ++i)
    {
        if ((i % 3) != 0)
            continue;

	    gxPushMatrix();

	    const float y_t = float(sin(time / 6.0 / 2.34f + i) + 1.f) / 2.f;
	    const float y = lerp<float>(0.f, 4.f, y_t);

	    gxTranslatef(0, y, 0);
	    gxRotatef(90, 1, 0, 0);

	    const float t = float(sin(time / 4.0 + i) + 1.f) / 2.f;
	    const float size = lerp<float>(1.f, 4.f, t);

	    const float c = fabsf(y_t - .5f) * 2.f;

	    setColorf(1, 1, 1, c * .2f);
	    fillCircle(0.f, 0.f, size, 100);

	    gxPopMatrix();
    }
#endif

	// -- End translucent pass.
    popBlend();
    popDepthTest();
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

#if USE_FRAMEWORK_DRAWING
    const bool useMultiview = false;
#else
	const bool useMultiview &= (ovrOpenGLExtensions.multi_view && vrapi_GetSystemPropertyInt(&Java, VRAPI_SYS_PROP_MULTIVIEW_AVAILABLE));
#endif

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
	glClearDepth(1.f);
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

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"
#include "audiooutput/AudioOutput_OpenSL.h"
#include "framework-allegro2.h"
#include "jgmod.h"

#define DIGI_SAMPLERATE 48000

struct JgmodTest
{
	AllegroTimerApi * timerApi = nullptr;
	AllegroVoiceApi * voiceApi = nullptr;

	AudioOutput_OpenSL * audioOutput = nullptr;
	AudioStream * audioStream = nullptr;

	JGMOD_PLAYER player;
	JGMOD * mod = nullptr;

	void init()
	{
		timerApi = new AllegroTimerApi(AllegroTimerApi::kMode_Manual);
		voiceApi = new AllegroVoiceApi(DIGI_SAMPLERATE, true);

		audioOutput = new AudioOutput_OpenSL();
		audioOutput->Initialize(2, DIGI_SAMPLERATE, 256);

		audioStream = new AudioStream_AllegroVoiceMixer(voiceApi, timerApi);
		audioOutput->Play(audioStream);

	    if (player.init(JGMOD_MAX_VOICES, timerApi, voiceApi) < 0)
		{
	        logError("unable to allocate %d voices", JGMOD_MAX_VOICES);
	        return;
		}

		player.enable_lasttrk_loop = true;

		const char * filename = "point_of_departure.s3m";
		mod = jgmod_load(filename);

		if (mod != nullptr)
		{
			player.play(mod, true);
		}
	}

	void shut()
	{
		player.stop();

		if (mod != nullptr)
		{
			jgmod_destroy(mod);
			mod = nullptr;
		}

		if (audioOutput != nullptr)
		{
			audioOutput->Stop();
			delete audioStream;
			audioStream = nullptr;
		}

		if (audioOutput != nullptr)
		{
			audioOutput->Shutdown();
			delete audioOutput;
			audioOutput = nullptr;
		}

		delete timerApi;
		delete voiceApi;
		timerApi = nullptr;
		voiceApi = nullptr;
	}
};

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioVoiceManager.h"

#include "audiooutput/AudioOutput_Native.h"
#include "audiostream/AudioStream.h"

struct AudioStream_AudioGraph : AudioStream
{
	AudioVoiceManager * voiceMgr = nullptr;
	AudioGraphManager * audioGraphMgr = nullptr;

	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override
	{
		//audioGraphMgr->tickMain();

		audioGraphMgr->tickAudio(numSamples / 44100.f);

		float samples[numSamples * 2];

		voiceMgr->generateAudio(samples, numSamples, 2);

		for (int i = 0; i < numSamples; ++i)
		{
			float valueL = samples[i * 2 + 0];
			float valueR = samples[i * 2 + 1];

			valueL = valueL < -1.f ? -1.f : valueL > +1.f ? +1.f : valueL;
			valueR = valueR < -1.f ? -1.f : valueR > +1.f ? +1.f : valueR;

			const int valueLi = int(valueL * ((1 << 15) - 1));
			const int valueRi = int(valueR * ((1 << 15) - 1));

			buffer[i].channel[0] = valueLi;
			buffer[i].channel[1] = valueRi;
		}

		return numSamples;
	}
};

struct AudiographTest
{
	AudioMutex mutex;
	AudioVoiceManagerBasic voiceMgr;
	AudioGraphManager_Basic audioGraphMgr;

	AudioStream_AudioGraph audioStream;
	AudioOutput_Native audioOutput;

	AudiographTest()
		: audioGraphMgr(true)
	{
	}

	void init()
	{
		// initialize audio related systems

		mutex.init();

		voiceMgr.init(&mutex, 16);
		voiceMgr.outputStereo = true;

		audioGraphMgr.init(&mutex, &voiceMgr);

		// create an audio graph instance

		auto * instance = audioGraphMgr.createInstance("sweetStuff6.xml");

		audioStream.voiceMgr = &voiceMgr;
		audioStream.audioGraphMgr = &audioGraphMgr;

		audioOutput.Initialize(2, 44100, 256);
		audioOutput.Play(&audioStream);
	}

	void shut()
	{
		audioOutput.Stop();
		audioOutput.Shutdown();

		audioGraphMgr.shut();

		voiceMgr.shut();

		mutex.shut();
	}
};

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

    JgmodTest jgmodTest;
    //jgmodTest.init();

    AudiographTest audiographTest;
    audiographTest.init();

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

    audiographTest.shut();

    jgmodTest.shut();

	scene.destroy();

	framework.shutdown();
	frameworkVr.shutdown();

    egl.destroyContext();

    vrapi_Shutdown();

	return 0;
}
