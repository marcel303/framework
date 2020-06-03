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

#pragma once

#if FRAMEWORK_USE_OVR_MOBILE

#include "ovr-framebuffer.h"
#include "framework.h"
#include <VrApi.h>

static const int CPU_LEVEL = 2;
static const int GPU_LEVEL = 3;
static const int NUM_MULTI_SAMPLES = 4;

class ANativeWindow;

struct FrameworkVr
{
	// Java context.
    ovrJava Java;

    // Window state.
    ANativeWindow * NativeWindow = nullptr;
    bool Resumed = false;

    // Oculus Vr state.
    ovrMobile * Ovr = nullptr;

    // Frame timing.
    long long FrameIndex = 1;
    double PredictedDisplayTime = 0.0;
    double StartTime = 0.0;
    double TimeStep = 0.0;

    // Scheduling.
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

	// Frame state.
	ovrTracking2 Tracking;
	Mat4x4 HeadTransform;
	Mat4x4 ProjectionMatrices[2];
	Mat4x4 ViewMatrices[2];
	ovrLayerProjection2 WorldLayer;

	// Draw state.
	int currentEyeIndex = -1;

    bool init();
    void shutdown();

	void process();

    void nextFrame();
    void submitFrameAndPresent();

	int getEyeCount() const
    {
        return NumBuffers;
    }

	void beginEye(const int eyeIndex, const Color & clearColor);
	void endEye();

	void pushBlackFinal();
	void showLoadingScreen();

	void processEvents();
	void handleVrModeChanges();
	void handleDeviceInput();
	int handleKeyEvent(const int keyCode, const int action);
	void handleVrApiEvents();

	double getTimeInSeconds() const;
};

extern FrameworkVr frameworkVr;

#endif
