/*
	Copyright (C) 2017 Marcel Smit
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

#include "framework.h"

#if ENABLE_METAL

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

// todo : move to framework header
class GxVertexBuffer;
struct GxVertexInput;

void gxSetVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride);

// -- render states --

#include "gx_mesh.h"

struct __attribute__((packed)) RenderPipelineState
{
	BLEND_MODE blendMode = BLEND_ALPHA;
	
	GxVertexInput vertexInputs[8] = { };
	int vertexInputCount = 0;
	int vertexStride = 0;
};

extern RenderPipelineState renderState;

#endif
