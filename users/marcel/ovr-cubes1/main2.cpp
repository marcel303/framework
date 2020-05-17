/************************************************************************************

Filename	:	VrCubeWorld_SurfaceView.c
Content		:	This sample uses a plain Android SurfaceView and handles all
                Activity and Surface life cycle events in native code. This sample
                does not use the application framework.
                This sample only uses the VrApi.
Created		:	March, 2015
Authors		:	J.M.P. van Waveren

Copyright	:	Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

*************************************************************************************/

#include "opengl-ovr.h"
#include "ovrFramebuffer.h"

#include "framework.h"

#include "android-assetcopy.h"

#include "StringEx.h"

#include <VrApi.h>
#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include <VrApi_SystemUtils.h>

#include <android/input.h>
#include <android/log.h>
#include <android/native_window_jni.h> // for native window JNI
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON

#include <assert.h>
#include <math.h>
#include <sys/prctl.h> // for prctl( PR_SET_NAME )
#include <time.h>
#include <unistd.h>

#define USE_FRAMEWORK_DRAWING 1

#define ENABLE_AUDIOOUTPUT_TEST 1

#if ENABLE_AUDIOOUTPUT_TEST
#include "audiooutput/AudioOutput_OpenSL.h"
#include "audiostream/AudioStreamVorbis.h"

static AudioOutput_OpenSL s_audioOutput;
static AudioStream_Vorbis s_audioStream;
#endif

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;

#if false // multi view vertex shader reference

VERTEX_SHADER =

    #define NUM_VIEWS 2
    #extension GL_OVR_multiview2 : enable
    layout(num_views=NUM_VIEWS) in;
    #define VIEW_ID gl_ViewID_OVR

    in vec3 vertexPosition;
    in mat4 vertexTransform;

    uniform SceneMatrices
    {
    	uniform mat4 ViewMatrix[NUM_VIEWS];
    	uniform mat4 ProjectionMatrix[NUM_VIEWS];
    } sm;

    void main()
    {
		gl_Position = sm.ProjectionMatrix[VIEW_ID] * ( sm.ViewMatrix[VIEW_ID] * ( vertexTransform * vec4( vertexPosition * 0.1, 1.0 ) ) );
    }

#endif

/*
================================================================================

System Clock Time

================================================================================
*/

static double GetTimeInSeconds()
{
    // note : this time must match the time used internally by the VrApi. this is due to us having
    //        to simulate the time step since the last update until the predicted (by the vr system) display time

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

/*
================================================================================

OpenGL-ES Utility Functions

================================================================================
*/

// todo : move OpenGL extensions into a separate file

static void EglInitExtensions()
{
    const char * allExtensions = (const char*)glGetString(GL_EXTENSIONS);

    if (allExtensions != nullptr)
    {
        glExtensions.multi_view = strstr(allExtensions, "GL_OVR_multiview2") &&
            strstr(allExtensions, "GL_OVR_multiview_multisampled_render_to_texture");

        glExtensions.EXT_texture_border_clamp =
            strstr(allExtensions, "GL_EXT_texture_border_clamp") ||
            strstr(allExtensions, "GL_OES_texture_border_clamp");
    }
}

/*
================================================================================

ovrEgl

================================================================================
*/

// todo : GLES initialization routines to framework

struct ovrEgl
{
    EGLint MajorVersion = 0; // note : major version number returned by eglInitialize
    EGLint MinorVersion = 0; // note : minor version number returned by eglInitialize
    EGLDisplay Display = EGL_NO_DISPLAY;
    EGLConfig Config = 0;
    EGLSurface TinySurface = EGL_NO_SURFACE;
    EGLSurface MainSurface = EGL_NO_SURFACE;
    EGLContext Context = EGL_NO_CONTEXT;
};

static void ovrEgl_CreateContext(ovrEgl * egl, const ovrEgl * shareEgl)
{
    assert(egl->Display == EGL_NO_DISPLAY);

    egl->Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(egl->Display, &egl->MajorVersion, &egl->MinorVersion);

    // Do NOT use eglChooseConfig, because the Android EGL code pushes in multisample
    // flags in eglChooseConfig if the user has selected the "force 4x MSAA" option in
    // settings, and that is completely wasted for our warp target.

    const int MAX_CONFIGS = 1024;
    EGLConfig configs[MAX_CONFIGS];
    EGLint numConfigs = 0;

    if (eglGetConfigs(egl->Display, configs, MAX_CONFIGS, &numConfigs) == EGL_FALSE)
    {
        logError("eglGetConfigs() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    const EGLint configAttribs[] =
        {
            EGL_RED_SIZE,       8,
            EGL_GREEN_SIZE,     8,
            EGL_BLUE_SIZE,      8,
            EGL_ALPHA_SIZE,     8, // need alpha for the multi-pass timewarp compositor
            EGL_DEPTH_SIZE,     0,
            EGL_STENCIL_SIZE,   0,
            EGL_SAMPLES,        0,
            EGL_NONE
        };

    egl->Config = 0;

    for (int i = 0; i < numConfigs; i++)
    {
        EGLint value = 0;

        eglGetConfigAttrib(egl->Display, configs[i], EGL_RENDERABLE_TYPE, &value);
        if ((value & EGL_OPENGL_ES3_BIT_KHR) != EGL_OPENGL_ES3_BIT_KHR)
            continue;

        // The pbuffer config also needs to be compatible with normal window rendering
        // so it can share textures with the window context.
        eglGetConfigAttrib(egl->Display, configs[i], EGL_SURFACE_TYPE, &value);
        if ((value & (EGL_WINDOW_BIT | EGL_PBUFFER_BIT)) != (EGL_WINDOW_BIT | EGL_PBUFFER_BIT))
            continue;

        int j = 0;
        while (configAttribs[j] != EGL_NONE)
        {
            eglGetConfigAttrib(egl->Display, configs[i], configAttribs[j], &value);
            if (value != configAttribs[j + 1])
                break;
            j += 2;
        }

        if (configAttribs[j] == EGL_NONE)
        {
            egl->Config = configs[i];
            break;
        }
    }

    if (egl->Config == 0)
    {
        logError("eglChooseConfig() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    logDebug("Context = eglCreateContext( Display, Config, EGL_NO_CONTEXT, contextAttribs )");
    const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
    egl->Context = eglCreateContext(
        egl->Display,
        egl->Config,
        (shareEgl != NULL) ? shareEgl->Context : EGL_NO_CONTEXT,
        contextAttribs);

    if (egl->Context == EGL_NO_CONTEXT)
    {
        logError("eglCreateContext() failed: %s", EglErrorString(eglGetError()));
        return;
    }

    logDebug("TinySurface = eglCreatePbufferSurface( Display, Config, surfaceAttribs )");
    const EGLint surfaceAttribs[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
    egl->TinySurface = eglCreatePbufferSurface(egl->Display, egl->Config, surfaceAttribs);
    if (egl->TinySurface == EGL_NO_SURFACE)
    {
        logError("eglCreatePbufferSurface() failed: %s", EglErrorString(eglGetError()));
        eglDestroyContext(egl->Display, egl->Context);
        egl->Context = EGL_NO_CONTEXT;
        return;
    }

    logDebug("eglMakeCurrent( Display, TinySurface, TinySurface, Context )");
    if (eglMakeCurrent(egl->Display, egl->TinySurface, egl->TinySurface, egl->Context) == EGL_FALSE)
    {
        logError("eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
        eglDestroySurface(egl->Display, egl->TinySurface);
        eglDestroyContext(egl->Display, egl->Context);
        egl->Context = EGL_NO_CONTEXT;
        return;
    }
}

static void ovrEgl_DestroyContext(ovrEgl * egl)
{
    if (egl->Display != EGL_NO_DISPLAY)
    {
        logError("- eglMakeCurrent( Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT )");
        if (eglMakeCurrent(egl->Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) == EGL_FALSE)
            logError("- eglMakeCurrent() failed: %s", EglErrorString(eglGetError()));
    }

    if (egl->Context != EGL_NO_CONTEXT)
    {
        logError("- eglDestroyContext( Display, Context )");
        if (eglDestroyContext(egl->Display, egl->Context) == EGL_FALSE)
            logError("- eglDestroyContext() failed: %s", EglErrorString(eglGetError()));
        egl->Context = EGL_NO_CONTEXT;
    }

    if (egl->TinySurface != EGL_NO_SURFACE)
    {
        logError("- eglDestroySurface( Display, TinySurface )");
        if (eglDestroySurface(egl->Display, egl->TinySurface) == EGL_FALSE)
            logError("- eglDestroySurface() failed: %s", EglErrorString(eglGetError()));
        egl->TinySurface = EGL_NO_SURFACE;
    }

    if (egl->Display != EGL_NO_DISPLAY)
    {
        logError("- eglTerminate( Display )");
        if (eglTerminate(egl->Display) == EGL_FALSE)
            logError("- eglTerminate() failed: %s", EglErrorString(eglGetError()));
        egl->Display = EGL_NO_DISPLAY;
    }
}

/*
================================================================================

ovrScene

================================================================================
*/

#define NUM_INSTANCES 1500
#define NUM_ROTATIONS 16

struct ovrScene
{
    bool CreatedScene = false;
	unsigned int Random = 2;
    ovrVector3f Rotations[NUM_ROTATIONS];
    ovrVector3f CubePositions[NUM_INSTANCES];
    int CubeRotations[NUM_INSTANCES];
};

static bool ovrScene_IsCreated(ovrScene * scene)
{
    return scene->CreatedScene;
}

// Returns a random float in the range [0, 1].
static float ovrScene_RandomFloat(ovrScene * scene)
{
    scene->Random = 1664525L * scene->Random + 1013904223L;
    union
    {
        float f;
        uint32_t i;
    } r;
    r.i = 0x3F800000 | (scene->Random & 0x007FFFFF);
    return r.f - 1.0f;
}

static void ovrScene_Create(ovrScene * scene, bool useMultiview)
{
    // Setup random rotations.
    for (int i = 0; i < NUM_ROTATIONS; i++)
    {
        scene->Rotations[i].x = ovrScene_RandomFloat(scene);
        scene->Rotations[i].y = ovrScene_RandomFloat(scene);
        scene->Rotations[i].z = ovrScene_RandomFloat(scene);
    }

    // Setup random cube positions and rotations.
    for (int i = 0; i < NUM_INSTANCES; i++)
    {
        float rx, ry, rz;
        for (;;)
        {

            rx = (ovrScene_RandomFloat(scene) - 0.5f) * (50.0f + sqrt(NUM_INSTANCES));
            ry = (ovrScene_RandomFloat(scene) - 0.5f) * (50.0f + sqrt(NUM_INSTANCES));
            rz = (ovrScene_RandomFloat(scene) - 0.5f) * (50.0f + sqrt(NUM_INSTANCES));
            // If too close to 0,0,0
            if (fabsf(rx) < 1.0f && fabsf(ry) < 1.0f && fabsf(rz) < 1.0f)
                continue;
            // Test for overlap with any of the existing cubes.
            bool overlap = false;
            for (int j = 0; j < i; j++)
            {
                if (fabsf(rx - scene->CubePositions[j].x) < 1.0f &&
                    fabsf(ry - scene->CubePositions[j].y) < 1.0f &&
                    fabsf(rz - scene->CubePositions[j].z) < 1.0f)
                    overlap = true;
            }
            if (!overlap)
                break;
        }

        rx *= 0.1f;
        ry *= 0.1f;
        rz *= 0.1f;

        // Insert into list sorted based on distance.
        int insert = 0;
        const float distSqr = rx * rx + ry * ry + rz * rz;
        for (int j = i; j > 0; j--)
        {
            const ovrVector3f* otherPos = &scene->CubePositions[j - 1];
            const float otherDistSqr =
                otherPos->x * otherPos->x + otherPos->y * otherPos->y + otherPos->z * otherPos->z;
            if (distSqr > otherDistSqr)
            {
                insert = j;
                break;
            }
            scene->CubePositions[j] = scene->CubePositions[j - 1];
            scene->CubeRotations[j] = scene->CubeRotations[j - 1];
        }

        scene->CubePositions[insert].x = rx;
        scene->CubePositions[insert].y = ry;
        scene->CubePositions[insert].z = rz;

        scene->CubeRotations[insert] = (int)(ovrScene_RandomFloat(scene) * (NUM_ROTATIONS - 0.1f));
    }

    scene->CreatedScene = true;
}

static void ovrScene_Destroy(ovrScene * scene)
{
    scene->CreatedScene = false;
}

/*
================================================================================

ovrSimulation

================================================================================
*/

struct ovrSimulation
{
    ovrVector3f CurrentRotation = { 0.0f, 0.0f, 0.0f };
};

static void ovrSimulation_Advance(ovrSimulation * simulation, const double elapsedDisplayTime)
{
    // Update rotation.
    simulation->CurrentRotation.x = (float)(elapsedDisplayTime);
    simulation->CurrentRotation.y = (float)(elapsedDisplayTime);
    simulation->CurrentRotation.z = (float)(elapsedDisplayTime);
}

/*
================================================================================

ovrRenderer

================================================================================
*/

struct ovrRenderer
{
    ovrFramebuffer FrameBuffer[VRAPI_FRAME_LAYER_EYE_MAX];
    int NumBuffers = VRAPI_FRAME_LAYER_EYE_MAX;
};

static void ovrRenderer_Create(ovrRenderer * renderer, const ovrJava * java, const bool useMultiview)
{
    renderer->NumBuffers = useMultiview ? 1 : VRAPI_FRAME_LAYER_EYE_MAX;

    // Create the frame buffers.
    for (int eye = 0; eye < renderer->NumBuffers; eye++)
    {
        renderer->FrameBuffer[eye].init(
            useMultiview,
            GL_RGBA8,
            vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
            vrapi_GetSystemPropertyInt(java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
            NUM_MULTI_SAMPLES);
    }
}

static void ovrRenderer_Destroy(ovrRenderer * renderer)
{
    for (int eye = 0; eye < renderer->NumBuffers; eye++)
        renderer->FrameBuffer[eye].shut();
}

static ovrLayerProjection2 ovrRenderer_RenderFrame(
    ovrRenderer * renderer,
    const ovrJava * java,
    const ovrScene * scene,
    const ovrSimulation * simulation,
    const ovrTracking2 * tracking,
    ovrMobile * ovr)
{
	// todo : use rotation matrices when drawing cubes
	ovrMatrix4f rotationMatrices[NUM_ROTATIONS];
	for (int i = 0; i < NUM_ROTATIONS; i++)
	{
		rotationMatrices[i] = ovrMatrix4f_CreateRotation(
				scene->Rotations[i].x * simulation->CurrentRotation.x,
				scene->Rotations[i].y * simulation->CurrentRotation.y,
				scene->Rotations[i].z * simulation->CurrentRotation.z);
	}

    ovrTracking2 updatedTracking = *tracking;

    ovrMatrix4f eyeViewMatrixTransposed[2];
    eyeViewMatrixTransposed[0] = ovrMatrix4f_Transpose(&updatedTracking.Eye[0].ViewMatrix);
    eyeViewMatrixTransposed[1] = ovrMatrix4f_Transpose(&updatedTracking.Eye[1].ViewMatrix);

    ovrMatrix4f projectionMatrixTransposed[2];
    projectionMatrixTransposed[0] = ovrMatrix4f_Transpose(&updatedTracking.Eye[0].ProjectionMatrix);
    projectionMatrixTransposed[1] = ovrMatrix4f_Transpose(&updatedTracking.Eye[1].ProjectionMatrix);

    ovrLayerProjection2 layer = vrapi_DefaultLayerProjection2();
    layer.HeadPose = updatedTracking.HeadPose;
    for (int eye = 0; eye < VRAPI_FRAME_LAYER_EYE_MAX; eye++)
    {
        ovrFramebuffer * frameBuffer = &renderer->FrameBuffer[renderer->NumBuffers == 1 ? 0 : eye];
        layer.Textures[eye].ColorSwapChain = frameBuffer->ColorTextureSwapChain;
        layer.Textures[eye].SwapChainIndex = frameBuffer->TextureSwapChainIndex;
        layer.Textures[eye].TexCoordsFromTanAngles = ovrMatrix4f_TanAngleMatrixFromProjection(&updatedTracking.Eye[eye].ProjectionMatrix);
    }
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_CHROMATIC_ABERRATION_CORRECTION;

    // Render the eye images.
    for (int eye = 0; eye < renderer->NumBuffers; eye++)
    {
        // NOTE: In the non-mv case, latency can be further reduced by updating the sensor
        // prediction for each eye (updates orientation, not position)
        ovrFramebuffer * frameBuffer = &renderer->FrameBuffer[eye];
        ovrFramebuffer_SetCurrent(frameBuffer);

        glEnable(GL_SCISSOR_TEST);
        checkErrorGL();

        glViewport(0, 0, frameBuffer->Width, frameBuffer->Height);
        glScissor(0, 0, frameBuffer->Width, frameBuffer->Height);
        checkErrorGL();

        //glClearColor(0.125f, 0.0f, 0.125f, 1.0f);
	    glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        checkErrorGL();

		setDepthTest(true, DEPTH_LESS);

	    gxSetMatrixf(GX_MODELVIEW, (float*)eyeViewMatrixTransposed[eye].M);
	    gxSetMatrixf(GX_PROJECTION, (float*)projectionMatrixTransposed[eye].M);

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

	    // Draw cubes.
	    pushCullMode(CULL_BACK, CULL_CCW);
		pushShaderOutputs("n");
		beginCubeBatch();
	    {
	        for (int i = 0; i < NUM_INSTANCES; ++i)
		    {
			    const float x = (i & 2) == 0 ? 0.f : (float)sin(time + i);
			    const float z = (i & 2) == 1 ? 0.f : (float)sin(time + i);

			    const float size = lerp<float>(.02f, .08f, float(sin(time + i) + 1.f) / 2.f);

			    setColor(colorWhite);
			    fillCube(Vec3(scene->CubePositions[i].x + x, scene->CubePositions[i].y, scene->CubePositions[i].z + z), Vec3(size));
		    }
	    }
	    endCubeBatch();
	    popShaderOutputs();
	    popCullMode();

		// Draw circles.
		pushLineSmooth(true);
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
	    popLineSmooth();

		// Draw textured walls.
	    gxSetTexture(getTexture("sabana.jpg"));
	    gxSetTextureSampler(GX_SAMPLE_MIPMAP, true);
	    setColor(colorWhite);
	    {
		    gxPushMatrix();
		    gxTranslatef(+3, 0, 0);
		    gxRotatef(time * 20.f, 0, 1, 0);
		    drawRect(+1, +1, -1, -1);
		    gxPopMatrix();

		    gxPushMatrix();
		    gxTranslatef(-3, 0, 0);
		    gxRotatef(time * 20.f, 0, 1, 0);
		    drawRect(+1, +1, -1, -1);
		    gxPopMatrix();
	    }
	    gxSetTexture(0);

	    gxPopMatrix();

		// Draw hands.
	#if true
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
		        // todo : use predicted display time, not current time
		        if (vrapi_GetInputTrackingState(ovr, header.DeviceID, GetTimeInSeconds(), &tracking) != ovrSuccess)
			        tracking.Status = 0;

		        ovrMatrix4f m = ovrMatrix4f_CreateIdentity();
		        if (tracking.Status & VRAPI_TRACKING_STATUS_POSITION_VALID)
		        {
			        const auto & p = tracking.HeadPose.Pose;
			        const auto & t = p.Translation;

			        m = ovrMatrix4f_CreateTranslation(t.x, t.y, t.z);
			        ovrMatrix4f r = ovrMatrix4f_CreateFromQuaternion(&p.Orientation);
			        m = ovrMatrix4f_Multiply(&m, &r);
			        m = ovrMatrix4f_Transpose(&m);
		        }

		        ovrInputStateTrackedRemote state;
				state.Header.ControllerType = ovrControllerType_TrackedRemote;
				if (vrapi_GetCurrentInputState(ovr, header.DeviceID, &state.Header ) >= 0)
				{
					if ((state.Buttons & ovrButton_Trigger) && (tracking.Status & VRAPI_TRACKING_STATUS_POSITION_VALID))
					{
						// Draw textured walls.
						gxSetTexture(getTexture("sabana.jpg"));
						gxSetTextureSampler(GX_SAMPLE_MIPMAP, true);
						setColor(colorWhite);
						{
							gxPushMatrix();
							gxMultMatrixf((float*)m.M);
							gxTranslatef(0, 0, -3);
							gxRotatef(time * 20.f, 0, 1, 0);
							drawRect(+1, +1, -1, -1);
							gxPopMatrix();
						}
						gxSetTexture(0);
					}

					if (state.Buttons & ovrButton_X)
						s_audioOutput.Play(&s_audioStream);
					else
						s_audioOutput.Stop();
				}

		        if (tracking.Status & VRAPI_TRACKING_STATUS_POSITION_VALID)
				{
					gxPushMatrix();
					gxMultMatrixf((float*)m.M);

					pushCullMode(CULL_BACK, CULL_CCW);
					pushShaderOutputs("n");
					setColor(colorWhite);
					fillCube(Vec3(), Vec3(.02f, .02f, .1f));
					popShaderOutputs();

					pushBlend(BLEND_ADD);
					const float a = lerp<float>(.1f, .4f, (sin(time) + 1.0) / 2.0);
					setColorf(1, 1, 1, a);
					fillCube(Vec3(0, 0, -100), Vec3(.01f, .01f, 100));
					popBlend();
					popCullMode();

				#if false
					ovrHandMesh handMesh;
					handMesh.Header.Version = ovrHandVersion_1;
					if (vrapi_GetHandMesh(ovr, VRAPI_HAND_LEFT, &handMesh.Header) == ovrSuccess)
					{
						gxBegin(GX_POINTS);
						{
							setColor(colorWhite);
							for (int i = 0; i < handMesh.NumVertices; ++i)
							{
								gxVertex3fv(&handMesh.VertexPositions[i].x);
							}
						}
						gxEnd();
					}
				#endif

					gxPopMatrix();
				}
	        }
	    }
	#endif

		// Draw filled circles.
	    pushDepthTest(true, DEPTH_LESS, false);
		pushBlend(BLEND_ADD);
	    pushLineSmooth(true);
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
	    popLineSmooth();
	    popBlend();
	    popDepthTest();

        // Explicitly clear the border texels to black when GL_CLAMP_TO_BORDER is not available.
        if (glExtensions.EXT_texture_border_clamp == false)
           ovrFramebuffer_ClearBorder(frameBuffer);

        ovrFramebuffer_Resolve(frameBuffer);
        ovrFramebuffer_Advance(frameBuffer);
    }

    ovrFramebuffer_SetNone();

    return layer;
}

/*
================================================================================

ovrApp

================================================================================
*/

struct ovrApp
{
    ovrJava Java;
    ovrEgl Egl;
    ANativeWindow* NativeWindow = nullptr;
    bool Resumed = false;
    ovrMobile* Ovr = nullptr;
    ovrScene Scene;
    ovrSimulation Simulation;
    long long FrameIndex = 1;
    double DisplayTime = 0.0;
    int SwapInterval = 1;
    int CpuLevel = 2;
    int GpuLevel = 2;
    int MainThreadTid = 0;
    int RenderThreadTid = 0;
    bool BackButtonDownLastFrame = false;
    bool GamePadBackButtonDown = false;
    ovrRenderer Renderer;
    bool UseMultiview = true;

    ovrApp()
    {
        Java.Vm = nullptr;
        Java.Env = nullptr;
        Java.ActivityObject = nullptr;
    }
};

static void ovrApp_PushBlackFinal(ovrApp * app)
{
    const int frameFlags = VRAPI_FRAME_FLAG_FLUSH | VRAPI_FRAME_FLAG_FINAL;

    ovrLayerProjection2 layer = vrapi_DefaultLayerBlackProjection2();
    layer.Header.Flags |= VRAPI_FRAME_LAYER_FLAG_INHIBIT_SRGB_FRAMEBUFFER;

    const ovrLayerHeader2 * layers[] = { &layer.Header };

    ovrSubmitFrameDescription2 frameDesc = { };
    frameDesc.Flags = frameFlags;
    frameDesc.SwapInterval = 1;
    frameDesc.FrameIndex = app->FrameIndex;
    frameDesc.DisplayTime = app->DisplayTime;
    frameDesc.LayerCount = 1;
    frameDesc.Layers = layers;

    vrapi_SubmitFrame2(app->Ovr, &frameDesc);
}

static void ovrApp_HandleVrModeChanges(ovrApp * app)
{
    if (app->Resumed != false && app->NativeWindow != nullptr)
    {
        if (app->Ovr == nullptr)
        {
            ovrModeParms parms = vrapi_DefaultModeParms(&app->Java);

            // Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView
            parms.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

            parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
            parms.Display = (size_t)app->Egl.Display;
            parms.WindowSurface = (size_t)app->NativeWindow;
            parms.ShareContext = (size_t)app->Egl.Context;

            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            logDebug("- vrapi_EnterVrMode()");
            app->Ovr = vrapi_EnterVrMode(&parms);

            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            // If entering VR mode failed then the ANativeWindow was not valid.
            if (app->Ovr == nullptr)
            {
                logError("Invalid ANativeWindow!");
                app->NativeWindow = nullptr;
            }

            // Set performance parameters once we have entered VR mode and have a valid ovrMobile.
            if (app->Ovr != nullptr)
            {
                vrapi_SetClockLevels(app->Ovr, app->CpuLevel, app->GpuLevel);
                logDebug("- vrapi_SetClockLevels( %d, %d )", app->CpuLevel, app->GpuLevel);

                vrapi_SetPerfThread(app->Ovr, VRAPI_PERF_THREAD_TYPE_MAIN, app->MainThreadTid);
                logDebug("- vrapi_SetPerfThread( MAIN, %d )", app->MainThreadTid);

                vrapi_SetPerfThread(app->Ovr, VRAPI_PERF_THREAD_TYPE_RENDERER, app->RenderThreadTid);
                logDebug("- vrapi_SetPerfThread( RENDERER, %d )", app->RenderThreadTid);
            }
        }
    }
    else
    {
        if (app->Ovr != nullptr)
        {
            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));

            logDebug("- vrapi_LeaveVrMode()");
            vrapi_LeaveVrMode(app->Ovr);
            app->Ovr = nullptr;

            logDebug("- eglGetCurrentSurface( EGL_DRAW ) = %p", eglGetCurrentSurface(EGL_DRAW));
        }
    }
}

static void ovrApp_HandleInput(ovrApp * app)
{
    bool backButtonDownThisFrame = false;

    for (int i = 0; true; i++)
    {
        ovrInputCapabilityHeader cap;
        const ovrResult result = vrapi_EnumerateInputDevices(app->Ovr, i, &cap);
        if (result < 0)
            break;

        if (cap.Type == ovrControllerType_Headset)
        {
            ovrInputStateHeadset headsetInputState;
            headsetInputState.Header.ControllerType = ovrControllerType_Headset;
            const ovrResult result = vrapi_GetCurrentInputState(app->Ovr, cap.DeviceID, &headsetInputState.Header);
            if (result == ovrSuccess)
                backButtonDownThisFrame |= headsetInputState.Buttons & ovrButton_Back;
        }
        else if (cap.Type == ovrControllerType_TrackedRemote)
        {
            ovrInputStateTrackedRemote trackedRemoteState;
            trackedRemoteState.Header.ControllerType = ovrControllerType_TrackedRemote;
            const ovrResult result = vrapi_GetCurrentInputState(app->Ovr, cap.DeviceID, &trackedRemoteState.Header);
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
            const ovrResult result = vrapi_GetCurrentInputState(app->Ovr, cap.DeviceID, &gamepadState.Header);
            if (result == ovrSuccess)
                backButtonDownThisFrame |= gamepadState.Buttons & ovrButton_Back;
        }
    }

    backButtonDownThisFrame |= app->GamePadBackButtonDown;

    bool backButtonDownLastFrame = app->BackButtonDownLastFrame;
    app->BackButtonDownLastFrame = backButtonDownThisFrame;

    if (backButtonDownLastFrame && !backButtonDownThisFrame)
    {
        logDebug("back button short press");
        logDebug("- ovrApp_PushBlackFinal()");
        ovrApp_PushBlackFinal(app);
        logDebug("- vrapi_ShowSystemUI( confirmQuit )");
        vrapi_ShowSystemUI(&app->Java, VRAPI_SYS_UI_CONFIRM_QUIT_MENU);
    }
}

static int ovrApp_HandleKeyEvent(ovrApp * app, const int keyCode, const int action)
{
    // Handle back button.
    if (keyCode == AKEYCODE_BACK || keyCode == AKEYCODE_BUTTON_B)
    {
        if (action == AKEY_EVENT_ACTION_DOWN)
            app->GamePadBackButtonDown = true;
        else if (action == AKEY_EVENT_ACTION_UP)
            app->GamePadBackButtonDown = false;
        return 1;
    }

    return 0;
}

static void ovrApp_HandleVrApiEvents(ovrApp * app)
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

extern "C"
{
    #include <android_native_app_glue.h>
    #include <android/native_activity.h>

	static void test_audiooutput(JavaVM * vm)
	{
		JNIEnv * env = nullptr;
		vm->AttachCurrentThread(&env, nullptr);

		jclass audioSystem = env->FindClass("android/media/AudioSystem");
		jmethodID method = env->GetStaticMethodID(audioSystem, "getPrimaryOutputSamplingRate", "()I");
		jint nativeOutputSampleRate = env->CallStaticIntMethod(audioSystem, method);
		method = env->GetStaticMethodID(audioSystem, "getPrimaryOutputFrameCount", "()I");
		jint nativeBufferLength = env->CallStaticIntMethod(audioSystem, method);

		vm->DetachCurrentThread();

		logDebug("native sample rate: %d", nativeOutputSampleRate);
		logDebug("native buffer size: %d", nativeBufferLength);

		s_audioOutput.Initialize(2, nativeOutputSampleRate, nativeBufferLength);

		s_audioStream.Open("button1.ogg", true);
		//s_audioOutput.Play(&s_audioStream);
	}

    void android_main(android_app* app)
    {
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

	    test_audiooutput(app->activity->vm);

        ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

        ovrJava java;
        java.Vm = app->activity->vm;
        java.Vm->AttachCurrentThread(&java.Env, nullptr);
        java.ActivityObject = app->activity->clazz;

    // todo : set thread name
        // Note that AttachCurrentThread will reset the thread name.
        //prctl(PR_SET_NAME, (long)"OVR::Main", 0, 0, 0);

        const ovrInitParms initParms = vrapi_DefaultInitParms(&java);
        const int32_t initResult = vrapi_Initialize(&initParms);

        if (initResult != VRAPI_INITIALIZE_SUCCESS)
        {
            // If intialization failed, vrapi_* function calls will not be available.
            exit(0);
        }

        ovrApp appState;
        appState.Java = java;

    // todo : the native activity example doesn't do this. why? and what does it do anyway?
        // This app will handle android gamepad events itself.
        vrapi_SetPropertyInt(&appState.Java, VRAPI_EAT_NATIVE_GAMEPAD_EVENTS, 0);

        ovrEgl_CreateContext(&appState.Egl, nullptr);

        EglInitExtensions();

	    framework.init(0, 0);

	#if USE_FRAMEWORK_DRAWING
	    appState.UseMultiview = false;
	#else
        appState.UseMultiview &= (glExtensions.multi_view && vrapi_GetSystemPropertyInt(&appState.Java, VRAPI_SYS_PROP_MULTIVIEW_AVAILABLE));
	#endif

        logDebug("AppState UseMultiview : %d", appState.UseMultiview ? 1 : 0);

        appState.CpuLevel = CPU_LEVEL;
        appState.GpuLevel = GPU_LEVEL;
        appState.MainThreadTid = gettid();

        ovrRenderer_Create(&appState.Renderer, &java, appState.UseMultiview);

        const double startTime = GetTimeInSeconds();

        while (!app->destroyRequested)
        {
            int ident;
            int events;
            struct android_poll_source *source;
            //const int timeoutMilliseconds = (appState.Ovr == NULL && app->destroyRequested == 0) ? -1 : 0;
            const int timeoutMilliseconds = 0;

            while ((ident = ALooper_pollAll(timeoutMilliseconds, NULL, &events, (void **) &source)) >= 0)
            {
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
                        appState.Resumed = true;
                        break;

					case APP_CMD_PAUSE:
                        logDebug("APP_CMD_PAUSE");
                        appState.Resumed = false;
                        break;

					case APP_CMD_INIT_WINDOW:
                        logDebug("APP_CMD_INIT_WINDOW");
                        appState.NativeWindow = app->window;
                        break;

					case APP_CMD_TERM_WINDOW:
                        logDebug("APP_CMD_TERM_WINDOW");
                        appState.NativeWindow = nullptr;
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

                        if (app->window != appState.NativeWindow)
                        {
                            if (appState.NativeWindow != nullptr)
                            {
                                // todo : perform actions due to window being destroyed
                                appState.NativeWindow = nullptr;
                            }

                            if (app->window != nullptr)
                            {
                                // todo : perform actions due to window being created
                                appState.NativeWindow = app->window;
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
                    // todo : restore AInputQueue_preDispatchEvent
                        //if (AInputQueue_preDispatchEvent(app->inputQueue, event)) {
                        //    continue;
                        //}

                        int32_t handled = 0;

                        if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY)
                        {
                            int keyCode = AKeyEvent_getKeyCode(event);
                            int action = AKeyEvent_getAction(event);

                            if (action != AKEY_EVENT_ACTION_DOWN &&
                                action != AKEY_EVENT_ACTION_UP)
                            {
                                // dispatch
                            }
                            else if (keyCode == AKEYCODE_VOLUME_UP)
                            {
                                // todo : we get here twice due to up/down ?
                                //adjustVolume(1);
                                //handled = 1;
                            }
                            else if (keyCode == AKEYCODE_VOLUME_DOWN)
                            {
                                // todo : we get here twice due to up/down ?
                                //adjustVolume(-1);
                                //handled = 1;
                            }
                            else
                            {
                                if (action == AKEY_EVENT_ACTION_UP)
                                {
                                    //Log.v( TAG, "GLES3JNIActivity::dispatchKeyEvent( " + keyCode + ", " + action + " )" );
                                }

                                ovrApp_HandleKeyEvent(
                                    &appState,
                                    keyCode,
                                    action);

                                handled = 1;
                            }
                        }

                        AInputQueue_finishEvent(app->inputQueue, event, handled);
                    }
                }

                ovrApp_HandleVrModeChanges(&appState);
            }

            // We must read from the event queue with regular frequency.
            ovrApp_HandleVrApiEvents(&appState);

            ovrApp_HandleInput(&appState);

            if (appState.Ovr == nullptr)
                continue;

            // Create the scene if not yet created.
            // The scene is created here to be able to show a loading icon.
            if (!ovrScene_IsCreated(&appState.Scene))
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
                frameDesc.FrameIndex = appState.FrameIndex;
                frameDesc.DisplayTime = appState.DisplayTime;
                frameDesc.LayerCount = 2;
                frameDesc.Layers = layers;

                vrapi_SubmitFrame2(appState.Ovr, &frameDesc);

                // Create the scene.
                ovrScene_Create(&appState.Scene, appState.UseMultiview);
            }

            // This is the only place the frame index is incremented, right before
            // calling vrapi_GetPredictedDisplayTime().
            appState.FrameIndex++;

            // Get the HMD pose, predicted for the middle of the time period during which
            // the new eye images will be displayed. The number of frames predicted ahead
            // depends on the pipeline depth of the engine and the synthesis rate.
            // The better the prediction, the less black will be pulled in at the edges.
            const double predictedDisplayTime = vrapi_GetPredictedDisplayTime(appState.Ovr, appState.FrameIndex);
            const ovrTracking2 tracking = vrapi_GetPredictedTracking2(appState.Ovr, predictedDisplayTime);

            appState.DisplayTime = predictedDisplayTime;

            // Advance the simulation based on the elapsed time since start of loop till predicted
            // display time.
            ovrSimulation_Advance(&appState.Simulation, predictedDisplayTime - startTime);

            // Render eye images and setup the primary layer using ovrTracking2.
            const ovrLayerProjection2 worldLayer = ovrRenderer_RenderFrame(
                &appState.Renderer,
                &appState.Java,
                &appState.Scene,
                &appState.Simulation,
                &tracking,
                appState.Ovr);

            const ovrLayerHeader2 * layers[] = { &worldLayer.Header };

            ovrSubmitFrameDescription2 frameDesc = { };
            frameDesc.Flags = 0;
            frameDesc.SwapInterval = appState.SwapInterval;
            frameDesc.FrameIndex = appState.FrameIndex;
            frameDesc.DisplayTime = appState.DisplayTime;
            frameDesc.LayerCount = 1;
            frameDesc.Layers = layers;

            // Hand over the eye images to the time warp.
            vrapi_SubmitFrame2(appState.Ovr, &frameDesc);
        }

        ovrRenderer_Destroy(&appState.Renderer);

        ovrScene_Destroy(&appState.Scene);
        ovrEgl_DestroyContext(&appState.Egl);

        vrapi_Shutdown();

        java.Vm->DetachCurrentThread();
    }
}
