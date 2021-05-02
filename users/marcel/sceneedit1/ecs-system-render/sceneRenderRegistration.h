#pragma once

#include "renderPasses.h"
#include <functional>

// scene render registration list

struct SceneRenderParams
{
	bool hasDeferredShadingPass = false; ///< When true, scene renderers should prefer kRenderPass_Opaque to draw opaque geometry. Otherwise, kRenderPass_Opaque_ForwardShaded should be used for drawing *all* opaque geometry. This allows shaders to avoid spending time calculating lighting or each fragment when deferred shading is used. kRenderPass_Opaque_ForwardShaded should only be used when the lighting calculations performed by the shader diverge from the shading model used by the deferred renderer.
};

struct SceneRenderRegistrationBase
{
	SceneRenderRegistrationBase * next = nullptr;
	
	std::function<void(const RenderPass pass)> draw;
	std::function<void(const ShadowPass pass)> drawShadow;
	
	SceneRenderRegistrationBase();
	
	virtual void setup() = 0;
	
	virtual void beginDraw(const SceneRenderParams & params) { };
	virtual void endDraw() { };
};

// convenience functions. these will iterate over all scene renderers and call the corresponding functions

void sceneRender_setup();
void sceneRender_beginDraw(const SceneRenderParams & params);
void sceneRender_endDraw();
void sceneRender_draw(const RenderPass pass);
void sceneRender_drawShadow(const ShadowPass pass);

extern SceneRenderRegistrationBase * g_sceneRenderRegistrationList;
