/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#if FRAMEWORK_USE_OVR_MOBILE

#include "framework-android-app.h"
#include "framework-ovr.h"
#include "internal.h"
#include "ovr-glext.h"
#include <VrApi_Helpers.h>
#include <VrApi_Input.h>
#include <VrApi_SystemUtils.h>
#include <android_native_app_glue.h>
#include <android/window.h> // for AWINDOW_FLAG_KEEP_SCREEN_ON
#include <unistd.h>

FrameworkOvr frameworkOvr;

bool FrameworkOvr::init()
{
	android_app * app = get_android_app();

	ANativeActivity_setWindowFlags(app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

	Java.Vm = app->activity->vm;
	Java.Vm->AttachCurrentThread(&Java.Env, nullptr);
	Java.ActivityObject = app->activity->clazz;

	ovrOpenGLExtensions.init();

	ovrInitParms initParms = vrapi_DefaultInitParms(&Java);
	initParms.GraphicsAPI = VRAPI_GRAPHICS_API_OPENGL_ES_3;

	const int32_t initResult = vrapi_Initialize(&initParms);

	if (initResult != VRAPI_INITIALIZE_SUCCESS)
	{
		// If intialization failed, vrapi_* function calls will not be available.
		logError("failed to initialize VrApi: %d", initResult);
		return false;
	}

	MainThreadTid = gettid();

    UseMultiview = false;
	NumBuffers = UseMultiview ? 1 : VRAPI_FRAME_LAYER_EYE_MAX;

    // Create the frame buffers.
	for (int eyeIndex = 0; eyeIndex < NumBuffers; ++eyeIndex)
    {
        FrameBuffer[eyeIndex].init(
            UseMultiview,
            GL_RGBA8,
            vrapi_GetSystemPropertyInt(&Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH),
            vrapi_GetSystemPropertyInt(&Java, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT),
            NUM_MULTI_SAMPLES);
    }

	StartTime = getTimeInSeconds();
	
	// initialize vr hands and pointers
	
	for (int i = 0; i < VrSide_COUNT; ++i)
		vrHand[i].init((VrSide)i);

	for (int i = 0; i < VrSide_COUNT; ++i)
		vrPointer[i].init((VrSide)i);

	return true;
}

void FrameworkOvr::shutdown()
{
	// shut down vr hands and pointers
	
	for (int i = 0; i < VrSide_COUNT; ++i)
		vrHand[i].shut();
	
	for (int i = 0; i < VrSide_COUNT; ++i)
		vrPointer[i].shut();
	
	// free frame buffers
	
    for (int eyeIndex = 0; eyeIndex < NumBuffers; ++eyeIndex)
        FrameBuffer[eyeIndex].shut();

	// shut down vr api
	
	vrapi_Shutdown();

	Java.Vm->DetachCurrentThread();
}

void FrameworkOvr::process()
{
	// update haptics (from the previous frame)
	
	for (auto & pointer : vrPointer)
	{
		pointer.updateHaptics();
	}
	
	// process app events

	processEvents();

	// update head tracking info and prepare for drawing the next frame

	nextFrame();

	// calculate the time step, based on the current and previous predicted display time

    TimeStep = PredictedDisplayTime - StartTime;

    StartTime = PredictedDisplayTime;
	
    // update input state
	
	for (auto & hand : vrHand)
		hand.updateInputState();

	for (auto & pointer : vrPointer)
		pointer.updateInputState();
	
	// determine the active pointer

	for (int i = 0; i < 2; ++i)
		vrPointer[i].isPrimary = false;
	
	int activeInputDeviceId;
	if (!vrapi_GetPropertyInt(&frameworkOvr.Java, VRAPI_ACTIVE_INPUT_DEVICE_ID, &activeInputDeviceId))
		activeInputDeviceId = -1;
	
	if (activeInputDeviceId == -1)
	{
		logWarning("failed to get active input device id from vrapi");
		vrPointer[0].isPrimary = true;
	}
	else
	{
		for (int i = 0; i < 2; ++i)
		{
			if (vrPointer[i].DeviceID == activeInputDeviceId)
				vrPointer[i].isPrimary = true;
		}
	}
}

void FrameworkOvr::nextFrame()
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

	// Get the head transform.
	const ovrMatrix4f headTransform = vrapi_GetTransformFromPose(&Tracking.HeadPose.Pose);
	const ovrMatrix4f headTransform_transposed = ovrMatrix4f_Transpose(&headTransform);
	memcpy(&HeadTransform, &headTransform_transposed, sizeof(HeadTransform));

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

void FrameworkOvr::submitFrameAndPresent()
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

void FrameworkOvr::beginEye(const int eyeIndex, const Color & clearColor)
{
	Assert(currentEyeIndex == -1);
	currentEyeIndex = eyeIndex;

	// NOTE: In the non-mv case, latency can be further reduced by updating the sensor
	// prediction for each eye (updates orientation, not position)
	ovrFramebuffer * frameBuffer = &FrameBuffer[eyeIndex];
	ovrFramebuffer_SetCurrent(frameBuffer);

	const Mat4x4 projectionMatrix =
		ProjectionMatrices[eyeIndex]
		.Scale(1, 1, -1); // convert z-axis direction back into what the porjection matrix expects
		
	const Mat4x4 viewMatrix =
		Mat4x4(true)
		.Scale(1, 1, -1 ) // convert from right to left handed
		.Mul(ViewMatrices[eyeIndex])
		.Scale(1, 1, -1); // convert from right to left handed
		
	gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
	gxSetMatrixf(GX_MODELVIEW, viewMatrix.m_v);
	updateCullFlip();

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

void FrameworkOvr::endEye()
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

void FrameworkOvr::pushBlackFinal()
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

void FrameworkOvr::showLoadingScreen()
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
    frameDesc.FrameIndex = FrameIndex;
    frameDesc.DisplayTime = PredictedDisplayTime;
    frameDesc.LayerCount = 2;
    frameDesc.Layers = layers;

    vrapi_SubmitFrame2(Ovr, &frameDesc);
}

void FrameworkOvr::processEvents()
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

		#if false
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
                            // note : here we would perform actions due to the window being destroyed
							
                            NativeWindow = nullptr;
                        }

                        if (app->window != nullptr)
                        {
                            // note : here we would perform actions due to the window being created
							
                            NativeWindow = app->window;
                        }
                    }
                    break;

				default:
                    break;
				}

                android_app_post_exec_cmd(app, cmd);

                Assert(NativeWindow == app->window);
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
            else
            {
                if (source != nullptr)
                {
                    source->process(app, source);
                }
            }
		#else
            if (source != nullptr)
            {
                source->process(app, source);
            }

            NativeWindow = app->window;
            Resumed = app->activityState == APP_CMD_RESUME;

            if ((uint64_t)NativeWindow == 0x1)
            {
                auto cmd = android_app_read_cmd(app);
            }
		#endif

            // Update vr mode based on the current app state.
	        handleVrModeChanges();
        }

        // We must read from the event queue with regular frequency.
        handleVrApiEvents();

        // Handle device input, to see if we should present the system ui.
        handleDeviceInput();

        // When we're not in vr mode.. rather than letting the app simulate
        // and draw the next frame, we will wait for events until we are back
        // in vr mode again.
        if (Ovr == nullptr)
            continue;

        // Done!
        break;
    }

	Assert(Resumed && Ovr != nullptr && NativeWindow != nullptr);

    framework.quitRequested |= app->destroyRequested;
}

void FrameworkOvr::handleVrModeChanges()
{
    if (Resumed != false && NativeWindow != nullptr)
    {
        if (Ovr == nullptr)
        {
            ovrModeParms parms = vrapi_DefaultModeParms(&Java);

            // Must reset the FLAG_FULLSCREEN window flag when using a SurfaceView
            //parms.Flags |= VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

            // No need to reset the FLAG_FULLSCREEN window flag when using a View
            parms.Flags &= ~VRAPI_MODE_FLAG_RESET_WINDOW_FULLSCREEN;

            parms.Flags |= VRAPI_MODE_FLAG_NATIVE_WINDOW;
            parms.Display = (size_t)globals.egl.Display;
            parms.WindowSurface = (size_t)NativeWindow;
            parms.ShareContext = (size_t)globals.egl.Context;

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

	            // Set the tracking transform to use, by default this is eye level.
	            vrapi_SetTrackingSpace(Ovr, VRAPI_TRACKING_SPACE_LOCAL_FLOOR);
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

void FrameworkOvr::handleDeviceInput()
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

int FrameworkOvr::handleKeyEvent(const int keyCode, const int action)
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

void FrameworkOvr::handleVrApiEvents()
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

double FrameworkOvr::getTimeInSeconds() const
{
    // note : this time must match the time used internally by the VrApi. this is due to us having
    //        to simulate the time step since the last update until the predicted (by the vr system) display time

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (now.tv_sec * 1e9 + now.tv_nsec) * 0.000000001;
}

#endif
