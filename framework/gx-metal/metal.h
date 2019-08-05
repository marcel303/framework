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

// --- functions for framework interop ---

void metal_init();
void metal_attach(SDL_Window * window);
void metal_detach(SDL_Window * window);
void metal_make_active(SDL_Window * window);
void metal_draw_begin(const float r, const float g, const float b, const float a, const float depth);
void metal_draw_end();
void metal_set_viewport(const int sx, const int sy);
void metal_set_scissor(const int x, const int y, const int sx, const int sy);
void metal_clear_scissor();

// --- experimental functions for merge into framework at some point ---

// todo : move to framework header

void pushRenderPass(ColorTarget * target, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName);
void pushRenderPass(ColorTarget ** targets, const int numTargets, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName);
void popRenderPass();

void setColorWriteMask(int r, int g, int b, int a);

#ifdef __OBJC__

// --- private data and helper functions ---

#import <Metal/Metal.h>
#include "gx_mesh.h"

struct RenderPipelineState
{
	BLEND_MODE blendMode = BLEND_ALPHA;
	uint8_t colorWriteMask = 0xf;
	
	GxVertexInput vertexInputs[8] = { };
	uint8_t vertexInputCount = 0;
	uint8_t vertexStride = 0;
	
	struct RenderPass
	{
		uint16_t colorFormat[4] = { };
		uint16_t depthFormat = 0;
	} renderPass;
};

extern RenderPipelineState renderState;

void metal_upload_texture_area(const void * src, const int srcPitch, const int srcSx, const int srcSy, id <MTLTexture> dst, const int dstX, const int dstY, const MTLPixelFormat pixelFormat);
void metal_copy_texture_to_texture(id <MTLTexture> src, const int srcPitch, const int srcX, const int srcY, const int srcSx, const int srcSy, id <MTLTexture> dst, const int dstX, const int dstY, const MTLPixelFormat pixelFormat);
void metal_generate_mipmaps(id <MTLTexture> texture);

#endif

#endif
