#pragma once

#include "sceneRender.h"
#include <functional>

// scene render registration list

struct SceneRenderRegistrationBase
{
	SceneRenderRegistrationBase * next = nullptr;
	
	std::function<void(const RenderPass pass)> draw;
	std::function<void(const ShadowPass pass)> drawShadow;
	
	std::function<void()> drawDeferredLights;
	
	SceneRenderRegistrationBase();
	
	virtual void setup() = 0;
	
	virtual void beginDraw(const SceneRenderParams & params) { };
	virtual void endDraw() { };
};

extern SceneRenderRegistrationBase * g_sceneRenderRegistrationList;
