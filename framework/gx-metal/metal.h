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

#include "framework.h"

#if ENABLE_METAL

class ColorTarget;
class DepthTarget;

// --- functions for framework interop ---

void metal_init();
void metal_shut();
void metal_attach(SDL_Window * window);
void metal_detach(SDL_Window * window);
void metal_make_active(SDL_Window * window);
void metal_capture_boundary();
void metal_acquire_drawable();
void metal_present();
void metal_set_viewport(const int sx, const int sy);
void metal_set_scissor(const int x, const int y, const int sx, const int sy);
void metal_clear_scissor();

#ifdef __OBJC__

// --- private data and helper functions ---

#import <Metal/Metal.h>
#import "gx_mesh.h"

static const int kMaxVertexInputs = 8;
static const int kMaxColorTargets = 8;

struct RenderPipelineState
{
	BLEND_MODE blendMode = BLEND_ALPHA;
	uint8_t colorWriteMask = 0xf;
	bool alphaToCoverageEnabled = false;
	
	GxVertexInput vertexInputs[kMaxVertexInputs] = { };
	uint8_t vertexInputCount = 0;
	uint8_t vertexStride = 0;
	
	struct RenderPass
	{
		uint16_t colorFormat[kMaxColorTargets] = { };
		uint16_t depthFormat = 0;
		uint16_t stencilFormat = 0;
	} renderPass;
};

id <MTLDevice> metal_get_device();
id <MTLCommandQueue> metal_get_command_queue();
id <MTLCommandBuffer> metal_get_command_buffer();
bool metal_is_encoding_draw_commands();

void metal_make_render_wait_for_blit(id<MTLBlitCommandEncoder> blit_encoder);

void metal_upload_texture_area(
	const void * src,
	const int srcPitch,
	const int srcSx, const int srcSy,
	id <MTLTexture> dst,
	const int dstX, const int dstY,
	const MTLPixelFormat pixelFormat);

void metal_copy_texture_to_texture(
	id <MTLTexture> src,
	const int srcX, const int srcY, const int srcZ,
	const int srcSx, const int srcSy, const int srcSz,
	id <MTLTexture> dst,
	const int dstX, const int dstY, const int dstZ,
	const MTLPixelFormat pixelFormat);

void metal_generate_mipmaps(id <MTLTexture> texture);

#endif

#endif
