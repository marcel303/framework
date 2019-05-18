#pragma once

#include "framework.h"

#if ENABLE_METAL

#include "mesh.h"

class ColorTarget;
class DepthTarget;

void metal_init();
void metal_attach(SDL_Window * window);
void metal_make_active(SDL_Window * window);
void metal_draw_begin(const float r, const float g, const float b, const float a);
void metal_draw_end();
void metal_set_viewport(const int sx, const int sy);

void pushRenderPass(ColorTarget * target, DepthTarget * depthTarget, const bool clearDepth);
void pushRenderPass(ColorTarget ** targets, const int numTargets, DepthTarget * depthTarget, const bool clearDepth);
void popRenderPass();

void gxSetVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride);

// -- render states --

struct __attribute__((packed)) RenderPipelineState
{
	BLEND_MODE blendMode = BLEND_ALPHA;
	
	GxVertexInput vertexInputs[8] = { };
	int vertexInputCount = 0;
	int vertexStride = 0;
};

extern RenderPipelineState renderState;

#endif
