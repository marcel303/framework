#pragma once

#include "renderPasses.h"

struct SceneRenderParams
{
	bool lightingIsEnabled = false; ///< When false, shaders to avoid lighting calculations, and output the albedo color for a material.
	
	bool shadowsAreEnabled = false; ///< When true, a shadowing solution may be used. Otherwise, shadowing should be disabled.
	
	bool hasDeferredLightingPass = false; ///< When true, scene renderers should prefer kRenderPass_Opaque to draw opaque geometry. Otherwise, kRenderPass_Opaque_ForwardShaded should be used for drawing *all* opaque geometry. This allows shaders to avoid spending time calculating lighting or each fragment when deferred shading is used. kRenderPass_Opaque_ForwardShaded should only be used when the lighting calculations performed by the shader diverge from the shading model used by the deferred renderer.
	
	bool outputToLinearColorSpace = false; ///< When true, shaders should output colors using the linear color space. If false, shaders should output to the sRGB color space.
};

// note : these will iterate over all scene render registrations and call the corresponding functions

void sceneRender_setup();
void sceneRender_beginDraw(const SceneRenderParams & params);
void sceneRender_endDraw();
void sceneRender_draw(const RenderPass pass);
void sceneRender_drawShadow(const ShadowPass pass);
void sceneRender_drawDeferredLights();

