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

#import "framework.h"

#if ENABLE_METAL

#import "bufferPool.h"
#import "data/engine/ShaderCommon.txt" // VS_ constants
#import "internal.h"
#import "metal.h"
#import "metalView.h"
#import "StringEx.h" // strcpy_s
#import "texture.h"
#import "window_data.h"
#import <Cocoa/Cocoa.h>
#import <map>
#import <Metal/Metal.h>
#import <SDL2/SDL_syswm.h>
#import <vector>

#define INDEX_TYPE uint32_t

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride);

static void gxEndDraw();

//

static id <MTLDevice> device = nullptr;

static id <MTLCommandQueue> queue = nullptr;

static std::map<SDL_Window*, MetalWindowData*> windowDatas;

MetalWindowData * activeWindowData = nullptr;

static id<MTLSamplerState> samplerStates[3 * 2]; // [filter(nearest, linear, mipmapped)][clamp(false, true)]

//

#include <stack>

struct RenderPassData
{
	id <MTLCommandBuffer> cmdbuf;
	
	MTLRenderPassDescriptor * renderdesc = nullptr;
	
	id <MTLRenderCommandEncoder> encoder;
	
	RenderPipelineState::RenderPass renderPass;
	
	int viewportSx = 0;
	int viewportSy = 0;
	int backingScale = 0;
};

struct RenderPassDataForPushPop
{
	ColorTarget * target[kMaxColorTargets];
	int numTargets = 0;
	DepthTarget * depthTarget = nullptr;
	bool isBackbufferPass = false;
	int backingScale = 0;
	char passName[32] = { };
};

static std::stack<RenderPassDataForPushPop> s_renderPasses;

static RenderPassData s_renderPassData;
static RenderPassData * s_activeRenderPass = nullptr;

//

static id <MTLFence> waitForBlit = nullptr;

//

void metal_init()
{
	device = MTLCreateSystemDefaultDevice();
	
	queue = [device newCommandQueue];
	
	// pre-create all possible sampler states
	
	@autoreleasepool
	{
		for (int filter = 0; filter < 3; ++filter)
		{
			for (int clamp = 0; clamp < 2; ++clamp)
			{
				auto * descriptor = [[MTLSamplerDescriptor new] autorelease];
				
				descriptor.minFilter =
					(filter == 0)
					? MTLSamplerMinMagFilterNearest
					: MTLSamplerMinMagFilterLinear;
				descriptor.magFilter =
					(filter == 0)
					? MTLSamplerMinMagFilterNearest
					: MTLSamplerMinMagFilterLinear;
				
				descriptor.mipFilter =
					(filter == 0 || filter == 1)
					? MTLSamplerMipFilterNotMipmapped
					: MTLSamplerMipFilterLinear;
				
				descriptor.sAddressMode =
					(clamp == 0)
					? MTLSamplerAddressModeRepeat
					: MTLSamplerAddressModeClampToEdge;
				descriptor.tAddressMode = descriptor.sAddressMode;
				
				const int index = (filter << 1) | clamp;
				
				samplerStates[index] = [device newSamplerStateWithDescriptor:descriptor];
			}
		}
	}
}

void metal_attach(SDL_Window * window)
{
	@autoreleasepool
	{
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		SDL_GetWindowWMInfo(window, &info);
		
		NSView * sdl_view = info.info.cocoa.window.contentView;

		MetalWindowData * windowData = new MetalWindowData();
		windowData->metalview = [[MetalView alloc] initWithFrame:sdl_view.frame device:device wantsDepthBuffer:framework.enableDepthBuffer wantsVsync:framework.enableVsync];
		[sdl_view addSubview:windowData->metalview];

		windowDatas[window] = windowData;
	}
}

void metal_detach(SDL_Window * window)
{
	@autoreleasepool
	{
		auto i = windowDatas.find(window);
		
		if (i != windowDatas.end())
		{
			auto * windowData = i->second;
			
			SDL_SysWMinfo info;
			SDL_VERSION(&info.version);
			SDL_GetWindowWMInfo(window, &info);
			
			NSView * sdl_view = info.info.cocoa.window.contentView;
			
			[sdl_view willRemoveSubview:windowData->metalview];

			[windowData->metalview release];
			windowData->metalview = nullptr;
			
			delete windowData;
			windowData = nullptr;
			
			windowDatas.erase(i);
		}
	}
}

void metal_make_active(SDL_Window * window)
{
	auto i = windowDatas.find(window);
	
	Assert(i != windowDatas.end());
	if (i == windowDatas.end())
		activeWindowData = nullptr;
	else
		activeWindowData = i->second;
}

#include "renderTarget.h"

void metal_draw_begin(const float r, const float g, const float b, const float a, const float depth)
{
	[queue insertDebugCaptureBoundary]; // todo : should be done @ framework::process()
	
	activeWindowData->current_drawable = [activeWindowData->metalview.metalLayer nextDrawable];
	[activeWindowData->current_drawable retain];
	
	pushBackbufferRenderPass(true, Color(r, g, b, a), true, depth, "Framebuffer pass");
}

void metal_draw_end()
{
	Assert(s_activeRenderPass != nullptr);
	
	gxEndDraw();
	
	auto & pd = *s_activeRenderPass;
	
// todo : endRenderPass ?

	[pd.encoder endEncoding];
	
#if 0
	[pd.cmdbuf addCompletedHandler:
		^(id<MTLCommandBuffer> _Nonnull)
		{
			NSLog(@"hello done! %@", activeWindowData);
		}];
#endif

	[pd.cmdbuf presentDrawable:activeWindowData->current_drawable];
	[pd.cmdbuf commit];
	//[activeWindowData->current_drawable present]; // todo : research the appropriate drawable presentation method
	
	//
	
	[pd.encoder release];
	[pd.renderdesc release];
	[pd.cmdbuf release];
	
	pd.encoder = nullptr;
	pd.renderdesc = nullptr;
	pd.cmdbuf = nullptr;
	
	[activeWindowData->current_drawable release];
	activeWindowData->current_drawable = nullptr;
	
	s_renderPasses.pop();
	
	Assert(s_renderPasses.empty());
	
	s_activeRenderPass = nullptr;
	
	renderState.renderPass = RenderPipelineState::RenderPass();
	
#if 1 // todo : remove once we call popRenderPass here
	// restore state
	
	gxMatrixMode(GX_PROJECTION);
	gxPopMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPopMatrix();
#endif
}

void metal_set_viewport(const int sx, const int sy)
{
	if (!s_activeRenderPass)
		return; // todo : hack. remove

	[s_activeRenderPass->encoder setViewport:(MTLViewport){ 0, 0, (double)sx, (double)sy, 0.0, 1.0 }];
}

void metal_set_scissor(const int x, const int y, const int sx, const int sy)
{
	const MTLScissorRect rect = { (NSUInteger)x, (NSUInteger)y, (NSUInteger)sx, (NSUInteger)sy };
	
// todo : assert x/y >= 0 and x+sx/y+sy <= width/height
// todo : clip coords to be within render target size

	[s_activeRenderPass->encoder setScissorRect:rect];
}

void metal_clear_scissor()
{
	const NSUInteger sx = s_activeRenderPass->renderdesc.colorAttachments[0].texture.width;
	const NSUInteger sy = s_activeRenderPass->renderdesc.colorAttachments[0].texture.height;
	
	const MTLScissorRect rect = { 0, 0, sx, sy };
	
	[s_activeRenderPass->encoder setScissorRect:rect];
}

id <MTLDevice> metal_get_device()
{
	return device;
}

id <MTLCommandQueue> metal_get_command_queue()
{
	return queue;
}

bool metal_is_encoding_draw_commands()
{
	return s_activeRenderPass != nullptr;
}

void metal_upload_texture_area(
	const void * src,
	const int srcPitch,
	const int srcSx, const int srcSy,
	id <MTLTexture> dst,
	const int dstX, const int dstY,
	const MTLPixelFormat pixelFormat)
{
	@autoreleasepool
	{
		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat width:srcSx height:srcSy mipmapped:NO];
		
		id <MTLTexture> src_texture = [device newTextureWithDescriptor:descriptor];
		
		const MTLOrigin src_origin = { 0, 0, 0 };
		const MTLSize src_size = { (NSUInteger)srcSx, (NSUInteger)srcSy, 1 };
		const MTLOrigin dst_origin = { (NSUInteger)dstX, (NSUInteger)dstY, 0 };
		
		auto blit_cmdbuf = [queue commandBuffer];
		auto blit_encoder = [blit_cmdbuf blitCommandEncoder];
		
		if (s_activeRenderPass != nullptr)
		{
			id <MTLFence> waitForDraw = [device newFence];
			id <MTLFence> waitForBlit = [device newFence];
			
			[s_activeRenderPass->encoder updateFence:waitForDraw afterStages:MTLRenderStageFragment];
			{
				[blit_encoder waitForFence:waitForDraw];
				[blit_encoder
					copyFromTexture:src_texture
					sourceSlice:0 sourceLevel:0 sourceOrigin:src_origin sourceSize:src_size
					toTexture:dst
					destinationSlice:0 destinationLevel:0 destinationOrigin:dst_origin];
				[blit_encoder updateFence:waitForBlit];
			}
			[s_activeRenderPass->encoder waitForFence:waitForBlit beforeStages:MTLRenderStageVertex];
			
			[waitForDraw release];
			[waitForBlit release];
		}
		else
		{
			[blit_encoder
				copyFromTexture:src_texture
				sourceSlice:0 sourceLevel:0 sourceOrigin:src_origin sourceSize:src_size
				toTexture:dst
				destinationSlice:0 destinationLevel:0 destinationOrigin:dst_origin];
			
			metal_make_render_wait_for_blit(blit_encoder);
		}
		
		[blit_encoder endEncoding];
		[blit_cmdbuf commit];
		
		[src_texture release];
		src_texture = nullptr;
	}
}

void metal_copy_texture_to_texture(
	id <MTLTexture> src,
	const int srcX, const int srcY, const int srcZ,
	const int srcSx, const int srcSy, const int srcSz,
	id <MTLTexture> dst,
	const int dstX, const int dstY, const int dstZ,
	const MTLPixelFormat pixelFormat)
{
	@autoreleasepool
	{
		const MTLOrigin src_origin = { (NSUInteger)srcX, (NSUInteger)srcY, (NSUInteger)srcZ };
		const MTLSize src_size = { (NSUInteger)srcSx, (NSUInteger)srcSy, (NSUInteger)srcSz };
		const MTLOrigin dst_origin = { (NSUInteger)dstX, (NSUInteger)dstY, (NSUInteger)dstZ };
		
		auto blit_cmdbuf = [queue commandBuffer];
		auto blit_encoder = [blit_cmdbuf blitCommandEncoder];
		
		if (s_activeRenderPass != nullptr)
		{
			id <MTLFence> waitForDraw = [device newFence];
			id <MTLFence> waitForBlit = [device newFence];
			
			[s_activeRenderPass->encoder updateFence:waitForDraw afterStages:MTLRenderStageFragment];
			{
				[blit_encoder waitForFence:waitForDraw];
				[blit_encoder copyFromTexture:src sourceSlice:0 sourceLevel:0 sourceOrigin:src_origin sourceSize:src_size toTexture:dst destinationSlice:0 destinationLevel:0 destinationOrigin:dst_origin];
				[blit_encoder updateFence:waitForBlit];
			}
			[s_activeRenderPass->encoder waitForFence:waitForBlit beforeStages:MTLRenderStageVertex];
			
			[waitForDraw release];
			[waitForBlit release];
		}
		else
		{
			[blit_encoder copyFromTexture:src sourceSlice:0 sourceLevel:0 sourceOrigin:src_origin sourceSize:src_size toTexture:dst destinationSlice:0 destinationLevel:0 destinationOrigin:dst_origin];
				
			metal_make_render_wait_for_blit(blit_encoder);
		}
		
		[blit_encoder endEncoding];
		[blit_cmdbuf commit];
	}
}

static id<MTLCommandBuffer> s_sync_blit_to_render_cmdbuf = nullptr;
static id<MTLRenderCommandEncoder> s_sync_blit_to_render_encoder = nullptr;

void metal_make_render_wait_for_blit(id<MTLBlitCommandEncoder> blit_encoder)
{
	[waitForBlit release];
	
	waitForBlit = [device newFence];
}

void metal_generate_mipmaps(id <MTLTexture> texture)
{
	@autoreleasepool
	{
		auto blit_cmdbuf = [queue commandBuffer];
		auto blit_encoder = [blit_cmdbuf blitCommandEncoder];
		
		if (s_activeRenderPass != nullptr)
		{
			id <MTLFence> waitForDraw = [device newFence];
			id <MTLFence> waitForBlit = [device newFence];
			
			[s_activeRenderPass->encoder updateFence:waitForDraw afterStages:MTLRenderStageFragment];
			{
				[blit_encoder waitForFence:waitForDraw];
				[blit_encoder generateMipmapsForTexture:texture];
				[blit_encoder updateFence:waitForBlit];
			}
			[s_activeRenderPass->encoder waitForFence:waitForBlit beforeStages:MTLRenderStageVertex];
			
			[waitForDraw release];
			[waitForBlit release];
		}
		else
		{
			[blit_encoder generateMipmapsForTexture:texture];
			
			metal_make_render_wait_for_blit(blit_encoder);
		}
		
		[blit_encoder endEncoding];
		[blit_cmdbuf commit];
	}
}

// -- render passes --

void beginRenderPass(
	ColorTarget * target,
	const bool clearColor,
	DepthTarget * depthTarget,
	const bool clearDepth,
	const char * passName,
	const int backingScale)
{
	beginRenderPass(&target, target == nullptr ? 0 : 1, clearColor, depthTarget, clearDepth, passName, backingScale);
}

void beginRenderPass(
	ColorTarget ** targets,
	const int numTargets,
	const bool in_clearColor,
	DepthTarget * depthTarget,
	const bool in_clearDepth,
	const char * passName,
	const int backingScale)
{
	Assert(numTargets >= 0 && numTargets <= kMaxColorTargets);
	
	@autoreleasepool
	{
		RenderPassData pd;
		
		pd.cmdbuf = [[queue commandBuffer] retain];

	 	pd.renderdesc = [[MTLRenderPassDescriptor renderPassDescriptor] retain];
		
		int viewportSx = 0;
		int viewportSy = 0;
		
		// specify the color and depth attachment(s)
		
		for (int i = 0; i < numTargets && i < kMaxColorTargets; ++i)
		{
			MTLRenderPassColorAttachmentDescriptor * colorattachment = pd.renderdesc.colorAttachments[i];
			colorattachment.texture = (id <MTLTexture>)targets[i]->getMetalTexture();
			
			const Color & clearColor = targets[i]->getClearColor();
			
			colorattachment.clearColor  = MTLClearColorMake(
				clearColor.r,
				clearColor.g,
				clearColor.b,
				clearColor.a);
			colorattachment.loadAction  = in_clearColor ? MTLLoadActionClear : MTLLoadActionLoad;
			colorattachment.storeAction = MTLStoreActionStore;
			
			pd.renderPass.colorFormat[i] = colorattachment.texture.pixelFormat;
			
			if (colorattachment.texture.width > viewportSx)
				viewportSx = colorattachment.texture.width;
			if (colorattachment.texture.height > viewportSy)
				viewportSy = colorattachment.texture.height;
		}
		
		if (depthTarget != nullptr)
		{
			MTLRenderPassDepthAttachmentDescriptor * depthattachment = pd.renderdesc.depthAttachment;
			depthattachment.texture = (id <MTLTexture>)depthTarget->getMetalTexture();
			depthattachment.clearDepth = depthTarget->getClearDepth();
			depthattachment.loadAction = in_clearDepth ? MTLLoadActionClear : MTLLoadActionLoad;
			depthattachment.storeAction = depthTarget->isTextureEnabled() ? MTLStoreActionStore : MTLStoreActionDontCare;
			
			pd.renderPass.depthFormat = depthattachment.texture.pixelFormat;
			
			if (depthattachment.texture.width > viewportSx)
				viewportSx = depthattachment.texture.width;
			if (depthattachment.texture.height > viewportSy)
				viewportSy = depthattachment.texture.height;
			
			if (depthattachment.texture.pixelFormat == MTLPixelFormatDepth24Unorm_Stencil8 ||
				depthattachment.texture.pixelFormat == MTLPixelFormatDepth32Float_Stencil8)
			{
				MTLRenderPassStencilAttachmentDescriptor * stencilattachment = pd.renderdesc.stencilAttachment;
				stencilattachment.texture = (id <MTLTexture>)depthTarget->getMetalTexture();
				stencilattachment.clearStencil = 0x00;
				stencilattachment.loadAction = in_clearDepth ? MTLLoadActionClear : MTLLoadActionLoad;
				stencilattachment.storeAction = depthTarget->isTextureEnabled() ? MTLStoreActionStore : MTLStoreActionDontCare;
				
				pd.renderPass.stencilFormat = stencilattachment.texture.pixelFormat;
			}
		}
		
		pd.viewportSx = viewportSx;
		pd.viewportSy = viewportSy;
		pd.backingScale = backingScale;
		
		// begin encoding
		
		pd.encoder = [[pd.cmdbuf renderCommandEncoderWithDescriptor:pd.renderdesc] retain];
		pd.encoder.label = [NSString stringWithCString:passName encoding:NSASCIIStringEncoding];
		
		// wait for blit operations (if any)
		
		if (waitForBlit != nullptr)
		{
			[pd.encoder waitForFence:waitForBlit beforeStages:MTLRenderStageVertex];
			
			[waitForBlit release];
			waitForBlit = nullptr;
		}
		
		renderState.renderPass = pd.renderPass;
		
		s_renderPassData = pd;
		s_activeRenderPass = &s_renderPassData;
		
		// set viewport and apply transform
		
		metal_set_viewport(viewportSx, viewportSy);
		
		applyTransform();
	}
}

void beginBackbufferRenderPass(const bool clearColor, const Color & color, const bool clearDepth, const float depth, const char * passName, const int backingScale)
{
	ColorTarget colorTarget(activeWindowData->current_drawable.texture);
	colorTarget.setClearColor(color.r, color.g, color.b, color.a);
	
	DepthTarget depthTarget(activeWindowData->metalview.depthTexture);
	depthTarget.setClearDepth(depth);
	
	beginRenderPass(
		&colorTarget,
		clearColor,
		activeWindowData->metalview.depthTexture ? &depthTarget : nullptr,
		clearDepth,
		passName,
		backingScale);
}

void endRenderPass()
{
	Assert(s_activeRenderPass != nullptr);
	
	gxEndDraw();
	
	auto & pd = *s_activeRenderPass;
	
	[pd.encoder endEncoding];
	
	[pd.cmdbuf commit];
	
	//
	
	[pd.encoder release];
	[pd.renderdesc release];
	[pd.cmdbuf release];
	
	s_activeRenderPass = nullptr;
	
	renderState.renderPass = RenderPipelineState::RenderPass();
}

// --- render passes stack ---

void pushRenderPass(ColorTarget * target, const bool clearColor, DepthTarget * depthTarget, const bool clearDepth, const char * passName, const int backingScale)
{
	pushRenderPass(&target, target == nullptr ? 0 : 1, clearColor, depthTarget, clearDepth, passName, backingScale);
}

void pushRenderPass(ColorTarget ** targets, const int numTargets, const bool in_clearColor, DepthTarget * depthTarget, const bool in_clearDepth, const char * passName, const int backingScale)
{
	Assert(numTargets >= 0 && numTargets <= kMaxColorTargets);
	
	// save state
	
	gxMatrixMode(GX_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPushMatrix();
	
	// end the current pass and begin a new one
	
	if (s_renderPasses.empty() == false)
	{
		endRenderPass();
	}
	
	beginRenderPass(targets, numTargets, in_clearColor, depthTarget, in_clearDepth, passName, backingScale);
	
	// record the current render pass information in the render passes stack
	
	RenderPassDataForPushPop pd;
	for (int i = 0; i < numTargets && i < kMaxColorTargets; ++i)
		pd.target[pd.numTargets++] = targets[i];
	pd.depthTarget = depthTarget;
	pd.backingScale = backingScale;
	strcpy_s(pd.passName, sizeof(pd.passName), passName);

	s_renderPasses.push(pd);
}

void pushBackbufferRenderPass(const bool clearColor, const Color & color, const bool clearDepth, const float depth, const char * passName, const int backingScale)
{
	// save state
	
	gxMatrixMode(GX_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPushMatrix();
	
	// end the current pass and begin a new one
	
	if (s_renderPasses.empty() == false)
	{
		endRenderPass();
	}
	
	beginBackbufferRenderPass(clearColor, color, clearDepth, depth, passName, backingScale);
	
	// record the current render pass information in the render passes stack
	
	RenderPassDataForPushPop pd;
	pd.isBackbufferPass = true;
	pd.backingScale = backingScale;
	strcpy_s(pd.passName, sizeof(pd.passName), passName);
	
	s_renderPasses.push(pd);
}

void popRenderPass()
{
	// end the current pass
	
	endRenderPass();
	
	s_renderPasses.pop();
	
	// check if there was a previous pass. if so, begin a continuation of it
	
	if (s_renderPasses.empty())
	{
		//
	}
	else
	{
		auto & new_pd = s_renderPasses.top();
		
		if (new_pd.isBackbufferPass)
		{
			beginBackbufferRenderPass(false, colorBlackTranslucent, false, 0.f, new_pd.passName, new_pd.backingScale);
		}
		else
		{
			beginRenderPass(new_pd.target, new_pd.numTargets, false, new_pd.depthTarget, false, new_pd.passName, new_pd.backingScale);
		}
	}
	
	// restore state
	
	gxMatrixMode(GX_PROJECTION);
	gxPopMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPopMatrix();
}

bool getCurrentRenderTargetSize(int & sx, int & sy, int & backingScale)
{
	if (s_activeRenderPass == nullptr)
		return false;
	else
	{
		Assert(s_activeRenderPass->viewportSx != 0);
		Assert(s_activeRenderPass->viewportSy != 0);
		Assert(s_activeRenderPass->backingScale != 0);
		
		sx = s_activeRenderPass->viewportSx;
		sy = s_activeRenderPass->viewportSy;
		backingScale = s_activeRenderPass->backingScale;
		return true;
	}
}

// -- render states --

static Stack<int, 32> colorWriteStack(0xf);

RenderPipelineState renderState;

// render states affecting render pipeline state

void setColorWriteMask(int r, int g, int b, int a)
{
	int mask = 0;
	
	if (r) mask |= 1;
	if (g) mask |= 2;
	if (b) mask |= 4;
	if (a) mask |= 8;
	
	renderState.colorWriteMask = mask;
}

void setColorWriteMaskAll()
{
	setColorWriteMask(1, 1, 1, 1);
}

void pushColorWriteMask(int r, int g, int b, int a)
{
	colorWriteStack.push(renderState.colorWriteMask);
	
	setColorWriteMask(r, g, b, a);
}

void popColorWriteMask()
{
	const int colorWriteMask = colorWriteStack.popValue();
	
	renderState.colorWriteMask = colorWriteMask;
}

void setBlend(BLEND_MODE blendMode)
{
	globals.blendMode = blendMode;
	
	renderState.blendMode = blendMode;
}

// render states independent from render pipeline state

void setLineSmooth(bool enabled)
{
	//fassert(false);
}

void setWireframe(bool enabled)
{
	[s_activeRenderPass->encoder setTriangleFillMode:enabled ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
}

static MTLStencilOperation translateStencilOp(const GX_STENCIL_OP op)
{
	switch (op)
	{
	case GX_STENCIL_OP_KEEP:
		return MTLStencilOperationKeep;
	case GX_STENCIL_OP_REPLACE:
		return MTLStencilOperationReplace;
	case GX_STENCIL_OP_ZERO:
		return MTLStencilOperationZero;
	case GX_STENCIL_OP_INC:
		return MTLStencilOperationIncrementClamp;
	case GX_STENCIL_OP_DEC:
		return MTLStencilOperationDecrementClamp;
	case GX_STENCIL_OP_INC_WRAP:
		return MTLStencilOperationIncrementWrap;
	case GX_STENCIL_OP_DEC_WRAP:
		return MTLStencilOperationDecrementWrap;
	}
	
	AssertMsg(false, "unknown GX_STENCIL_OP", 0);
	return MTLStencilOperationKeep;
}

static MTLCompareFunction translateCompareFunction(const GX_STENCIL_FUNC func)
{
	switch (func)
	{
	case GX_STENCIL_FUNC_NEVER:
		return MTLCompareFunctionNever;
	case GX_STENCIL_FUNC_LESS:
		return MTLCompareFunctionLess;
	case GX_STENCIL_FUNC_LEQUAL:
		return MTLCompareFunctionLessEqual;
	case GX_STENCIL_FUNC_GREATER:
		return MTLCompareFunctionGreater;
	case GX_STENCIL_FUNC_GEQUAL:
		return MTLCompareFunctionGreaterEqual;
	case GX_STENCIL_FUNC_EQUAL:
		return MTLCompareFunctionEqual;
	case GX_STENCIL_FUNC_NOTEQUAL:
		return MTLCompareFunctionNotEqual;
	case GX_STENCIL_FUNC_ALWAYS:
		return MTLCompareFunctionAlways;
	}
	
	AssertMsg(false, "unknown GX_STENCIL_FUNC", 0);
	return MTLCompareFunctionAlways;
}

static void fillStencilDescriptor(
	MTLStencilDescriptor * descriptor,
	const StencilState & stencilState)
{
	descriptor.readMask = stencilState.compareMask;
	descriptor.stencilCompareFunction = translateCompareFunction(stencilState.compareFunc);
	
	descriptor.stencilFailureOperation = translateStencilOp(stencilState.onStencilFail);
	descriptor.depthFailureOperation = translateStencilOp(stencilState.onDepthFail);
	descriptor.depthStencilPassOperation = translateStencilOp(stencilState.onDepthStencilPass);
	
	descriptor.writeMask = stencilState.writeMask;
}

static void fillDepthStencilDescriptor(MTLDepthStencilDescriptor * descriptor)
{
	// fill depth state
	
	if (globals.depthTestEnabled)
	{
		switch (globals.depthTest)
		{
		case DEPTH_EQUAL:
			descriptor.depthCompareFunction = MTLCompareFunctionEqual;
			break;
		case DEPTH_LESS:
			descriptor.depthCompareFunction = MTLCompareFunctionLess;
			break;
		case DEPTH_LEQUAL:
			descriptor.depthCompareFunction = MTLCompareFunctionLessEqual;
			break;
		case DEPTH_GREATER:
			descriptor.depthCompareFunction = MTLCompareFunctionGreater;
			break;
		case DEPTH_GEQUAL:
			descriptor.depthCompareFunction = MTLCompareFunctionGreaterEqual;
			break;
		case DEPTH_ALWAYS:
			descriptor.depthCompareFunction = MTLCompareFunctionAlways;
			break;
		}
		
		descriptor.depthWriteEnabled = globals.depthTestWriteEnabled;
	}
	else
	{
		descriptor.depthCompareFunction = MTLCompareFunctionAlways;
		descriptor.depthWriteEnabled = false;
	}
	
	// fill stencil state
	
	if (globals.stencilEnabled)
	{
		fillStencilDescriptor(descriptor.frontFaceStencil, globals.frontStencilState);
		fillStencilDescriptor(descriptor.backFaceStencil, globals.backStencilState);
	}
	else
	{
		descriptor.frontFaceStencil = nil;
		descriptor.backFaceStencil = nil;
	}
}

void setDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled)
{
	globals.depthTestEnabled = enabled;
	globals.depthTest = test;
	globals.depthTestWriteEnabled = writeEnabled;
	
	// update depth-stencil state
	
	MTLDepthStencilDescriptor * descriptor = [[MTLDepthStencilDescriptor alloc] init];
	fillDepthStencilDescriptor(descriptor);
	
	id <MTLDepthStencilState> state = [device newDepthStencilStateWithDescriptor:descriptor];
	[s_activeRenderPass->encoder setDepthStencilState:state];
	
	[state release];
	state = nullptr;
	
	[descriptor release];
	descriptor = nullptr;
}

void setDepthBias(float depthBias, float slopeScale)
{
	[s_activeRenderPass->encoder setDepthBias:depthBias slopeScale:slopeScale clamp:0.f];
}

void setAlphaToCoverage(bool enabled)
{
	globals.alphaToCoverageEnabled = enabled;
	
	renderState.alphaToCoverageEnabled = enabled;
}

void clearStencil(uint8_t value, uint32_t writeMask)
{
	// capture state we need to restore later
	
	const auto restore_stencilTestEnabled = globals.stencilEnabled;
	const auto restore_frontStencilState = globals.frontStencilState;
	const auto restore_backStencilState = globals.backStencilState;
	const auto restore_matrixMode = gxGetMatrixMode();
	
	// set up the stencil state so as to overwrite the stencil buffer with the desired value
	
	StencilState clearStencilState;
	clearStencilState.compareFunc = GX_STENCIL_FUNC_ALWAYS;
	clearStencilState.compareRef = value;
	clearStencilState.compareMask = 0xff;
	clearStencilState.onStencilFail = GX_STENCIL_OP_REPLACE;
	clearStencilState.onDepthFail = GX_STENCIL_OP_REPLACE;
	clearStencilState.onDepthStencilPass = GX_STENCIL_OP_REPLACE;
	clearStencilState.writeMask = writeMask;
	
	setStencilTest(clearStencilState, clearStencilState);
	
	// prepare to draw a rect covering the entire buffer
	
	gxMatrixMode(GX_PROJECTION);
	gxPushMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPushMatrix();
	{
		projectScreen2d();
		
		// we only want to touch the stencil buffer,
		// so make sure to disable depth and color writes
		
		pushDepthTest(false, DEPTH_ALWAYS, false);
		pushColorWriteMask(0, 0, 0, 0);
		{
			int sx;
			int sy;
			framework.getCurrentViewportSize(sx, sy);
			
			drawRect(0, 0, sx, sy);
		}
		popColorWriteMask();
		popDepthTest();
	}
	gxMatrixMode(GX_PROJECTION);
	gxPopMatrix();
	gxMatrixMode(GX_MODELVIEW);
	gxPopMatrix();
	
	// restore previous states
	
	if (restore_stencilTestEnabled)
		setStencilTest(restore_frontStencilState, restore_backStencilState);
	else
	{
		globals.frontStencilState = restore_frontStencilState;
		globals.backStencilState = restore_backStencilState;
		clearStencilTest();
	}
	
	gxMatrixMode(restore_matrixMode);
}

void setStencilTest(const StencilState & front, const StencilState & back)
{
	globals.stencilEnabled = true;
	globals.frontStencilState = front;
	globals.backStencilState = back;
	
	// update depth-stencil state
	
	MTLDepthStencilDescriptor * descriptor = [[MTLDepthStencilDescriptor alloc] init];
	fillDepthStencilDescriptor(descriptor);
	
	id <MTLDepthStencilState> state = [device newDepthStencilStateWithDescriptor:descriptor];
	[s_activeRenderPass->encoder setDepthStencilState:state];
	
	[s_activeRenderPass->encoder setStencilFrontReferenceValue:globals.frontStencilState.compareRef backReferenceValue:globals.backStencilState.compareRef];
	
	[state release];
	state = nullptr;
	
	[descriptor release];
	descriptor = nullptr;
}

void clearStencilTest()
{
	globals.stencilEnabled = false;
	
	// update depth-stencil state
	
	MTLDepthStencilDescriptor * descriptor = [[MTLDepthStencilDescriptor alloc] init];
	fillDepthStencilDescriptor(descriptor);
	
	id <MTLDepthStencilState> state = [device newDepthStencilStateWithDescriptor:descriptor];
	[s_activeRenderPass->encoder setDepthStencilState:state];
	
	[state release];
	state = nullptr;
	
	[descriptor release];
	descriptor = nullptr;
}

void setCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding)
{
	const MTLCullMode metalCullMode =
		mode == CULL_NONE ? MTLCullModeNone :
		mode == CULL_FRONT ? MTLCullModeFront :
		MTLCullModeBack;
	
	[s_activeRenderPass->encoder setCullMode:metalCullMode];
	
	const MTLWinding metalFrontFaceWinding =
		frontFaceWinding == CULL_CCW ? MTLWindingCounterClockwise :
		MTLWindingClockwise;
	
	[s_activeRenderPass->encoder setFrontFacingWinding:metalFrontFaceWinding];
}

// -- gpu resources --

static GxTextureId createTexture(
	const void * source,
	const int sx, const int sy,
	const int bytesPerPixel,
	const int sourcePitch,
	const bool filter,
	const bool clamp,
	const MTLPixelFormat pixelFormat)
{
	@autoreleasepool
	{
		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat width:sx height:sy mipmapped:NO];
		
		id <MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
		
		if (texture == nullptr)
			return 0;
		else
		{
			const int pitch = (sourcePitch == 0 ? sx : sourcePitch) * bytesPerPixel;
			
			const MTLRegion region =
			{
				{ 0, 0, 0 },
				{ (NSUInteger)sx, (NSUInteger)sy, 1 }
			};
			
			[texture replaceRegion:region mipmapLevel:0 withBytes:source bytesPerRow:pitch];
			
			const GxTextureId textureId = s_nextTextureId++;
			
			s_textures[textureId] = texture;
			
			return textureId;
		}
	}
}

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 4, 0, filter, clamp, MTLPixelFormatRGBA8Unorm);
}

GxTextureId createTextureFromRGB8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	uint8_t * data = new uint8_t[sx * sy * 4];
	
	const uint8_t * src = (const uint8_t*)source;
	      uint8_t * dst = data;
	
	for (int i = sx * sy; i > 0; --i)
	{
		dst[0] = src[0];
		dst[1] = src[1];
		dst[2] = src[2];
		dst[3] = 0xff;
		
		src += 3;
		dst += 4;
	}
	
	const GxTextureId result = createTextureFromRGBA8(data, sx, sy, filter, clamp);
	
	delete [] data;
	data = nullptr;
	
	return result;
}

GxTextureId createTextureFromR8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 1, 0, filter, clamp, MTLPixelFormatR8Unorm);
}

GxTextureId createTextureFromR16(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 2, 0, filter, clamp, MTLPixelFormatR16Unorm);
}

GxTextureId createTextureFromR32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 4, 0, filter, clamp, MTLPixelFormatR32Float);
}

GxTextureId createTextureFromRG32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 8, 0, filter, clamp, MTLPixelFormatRG32Float);
}

GxTextureId createTextureFromRGBA32F(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 8, 0, filter, clamp, MTLPixelFormatRGBA32Float);
}

// --- internal texture creation functions ---

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, int sourcePitch, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 4, sourcePitch, filter, clamp, MTLPixelFormatRGBA8Unorm);
}

GxTextureId copyTexture(const GxTextureId source)
{
	if (source != 0)
	{
		@autoreleasepool
		{
			auto & src = s_textures[source];
			
			const GxTextureId textureId = s_nextTextureId++;
			
			MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:src.pixelFormat width:src.width height:src.height mipmapped:NO];
			
			id <MTLTexture> dst = [device newTextureWithDescriptor:descriptor];
			
			metal_copy_texture_to_texture(
				src,
				0, 0, 0,
				src.width, src.height, src.depth,
				dst,
				0, 0, 0,
				dst.pixelFormat);
			
			s_textures[s_nextTextureId] = dst;
			
			return textureId;
		}
	}
	else
	{
		return 0;
	}
}

void freeTexture(GxTextureId & textureId)
{
	if (textureId != 0)
	{
		auto i = s_textures.find(textureId);
		
		Assert(i != s_textures.end());
		if (i != s_textures.end())
		{
			auto & texture = i->second;
			
			[texture release];
			texture = nullptr;
			
			s_textures.erase(i);
		}
		
		textureId = 0;
	}
}

// -- gx api implementation --

#include "Mat4x4.h"
#include "Quat.h"

class GxMatrixStack
{
public:
	static const int kSize = 32;
	Mat4x4 stack[kSize];
	int stackDepth;
	bool isDirty;
	
	GxMatrixStack()
	{
		stackDepth = 0;
		stack[0].MakeIdentity();
		
		isDirty = true;
	}
	
	void push()
	{
		fassert(stackDepth + 1 < kSize);
		stackDepth++;
		stack[stackDepth] = stack[stackDepth - 1];
	}
	
	void pop()
	{
		fassert(stackDepth > 0);
		stackDepth--;
		
		isDirty = true;
	}
	
	const Mat4x4 & get() const
	{
		return stack[stackDepth];
	}
	
	Mat4x4 & getRw()
	{
		isDirty = true;
		
		return stack[stackDepth];
	}
	
	void makeDirty()
	{
		isDirty = true;
	}
};

static GxMatrixStack s_gxModelView;
static GxMatrixStack s_gxProjection;
static GxMatrixStack * s_gxMatrixStack = &s_gxModelView;

void gxMatrixMode(GX_MATRIX mode)
{
	switch (mode)
	{
		case GX_MODELVIEW:
			s_gxMatrixStack = &s_gxModelView;
			break;
		case GX_PROJECTION:
			s_gxMatrixStack = &s_gxProjection;
			break;
		default:
			fassert(false);
			break;
	}
}

GX_MATRIX gxGetMatrixMode()
{
	if (s_gxMatrixStack == &s_gxModelView)
		return GX_MODELVIEW;
	else if (s_gxMatrixStack == &s_gxProjection)
		return GX_PROJECTION;
	else
	{
		fassert(false);
		return GX_MODELVIEW;
	}
}

void gxPopMatrix()
{
	s_gxMatrixStack->pop();
}

void gxPushMatrix()
{
	s_gxMatrixStack->push();
}

void gxLoadIdentity()
{
	s_gxMatrixStack->getRw().MakeIdentity();
}

void gxLoadMatrixf(const float * m)
{
	memcpy(s_gxMatrixStack->getRw().m_v, m, sizeof(float) * 16);
}

void gxGetMatrixf(GX_MATRIX mode, float * m)
{
	switch (mode)
	{
		case GX_PROJECTION:
			memcpy(m, s_gxProjection.get().m_v, sizeof(float) * 16);
			break;
		case GX_MODELVIEW:
			memcpy(m, s_gxModelView.get().m_v, sizeof(float) * 16);
			break;
		default:
			fassert(false);
			break;
	}
}

void gxSetMatrixf(GX_MATRIX mode, const float * m)
{
	switch (mode)
	{
		case GX_PROJECTION:
			memcpy(s_gxProjection.getRw().m_v, m, sizeof(float) * 16);
			break;
		case GX_MODELVIEW:
			memcpy(s_gxModelView.getRw().m_v, m, sizeof(float) * 16);
			break;
		default:
			fassert(false);
			break;
	}
}

void gxMultMatrixf(const float * _m)
{
	Mat4x4 m;
	memcpy(m.m_v, _m, sizeof(m.m_v));

	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxTranslatef(float x, float y, float z)
{
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get().Translate(x, y, z);
}

void gxRotatef(float angle, float x, float y, float z)
{
	Quat q;
	q.fromAxisAngle(Vec3(x, y, z), angle * M_PI / 180.f);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * q.toMatrix();
}

void gxScalef(float x, float y, float z)
{
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get().Scale(x, y, z);
}

void gxValidateMatrices()
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);

	//printf("validate1\n");
	
	if (globals.shader && globals.shader->getType() == SHADER_VSPS)
	{
		Shader * shader = static_cast<Shader*>(globals.shader);
		
		const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader->getCacheElem());
	
		// check if matrices are dirty
		
		if ((globals.gxShaderIsDirty || s_gxModelView.isDirty) &&
			shaderElem.params[ShaderCacheElem::kSp_ModelViewMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(
				shaderElem.params[ShaderCacheElem::kSp_ModelViewMatrix].index,
				s_gxModelView.get().m_v);
			//printf("validate2\n");
		}
		
		if ((globals.gxShaderIsDirty || s_gxModelView.isDirty || s_gxProjection.isDirty) &&
			shaderElem.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(
				shaderElem.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].index,
				(s_gxProjection.get() * s_gxModelView.get()).m_v);
			//printf("validate3\n");
		}
		
		if ((globals.gxShaderIsDirty || s_gxProjection.isDirty) &&
			shaderElem.params[ShaderCacheElem::kSp_ProjectionMatrix].index >= 0)
		{
			shader->setImmediateMatrix4x4(
				shaderElem.params[ShaderCacheElem::kSp_ProjectionMatrix].index,
				s_gxProjection.get().m_v);
			//printf("validate4\n");
		}
		
		// set vertex stage uniform buffers
		
		for (int i = 0; i < ShaderCacheElem_Metal::kMaxBuffers; ++i)
		{
			if (shaderElem.vsInfo.uniformBufferSize[i] == 0)
				continue;
			
			[s_activeRenderPass->encoder
				setVertexBytes:shaderElem.vsUniformData[i]
				length:shaderElem.vsInfo.uniformBufferSize[i]
				atIndex:i];
		}
	}

	s_gxModelView.isDirty = false;
	s_gxProjection.isDirty = false;
}

struct GxVertex
{
	float px, py, pz, pw;
	float nx, ny, nz;
	float cx, cy, cz, cw;
	float tx, ty;
};

static GxVertex s_gxVertexBuffer[1024*64];

static GX_PRIMITIVE_TYPE s_gxPrimitiveType = GX_INVALID_PRIM;
static GxVertex * s_gxVertices = nullptr;
static int s_gxVertexCount = 0;
static int s_gxMaxVertexCount = 0;
static int s_gxPrimitiveSize = 0;
static GxVertex s_gxVertex = { };
static GxTextureId s_gxTexture = 0;
static bool s_gxTextureEnabled = false;
static int s_gxTextureSampler = 0;

static GX_PRIMITIVE_TYPE s_gxLastPrimitiveType = GX_INVALID_PRIM;
static int s_gxLastVertexCount = -1;
static int s_gxLastIndexOffset = -1;

static GxVertex s_gxFirstVertex;
static bool s_gxHasFirstVertex = false;

static DynamicBufferPool s_gxVertexBufferPool;
static DynamicBufferPool::PoolElem * s_gxVertexBufferElem = nullptr;
static int s_gxVertexBufferElemOffset = 0;

static DynamicBufferPool s_gxIndexBufferPool;
static DynamicBufferPool::PoolElem * s_gxIndexBufferElem = nullptr;
static int s_gxIndexBufferElemOffset = 0;

static GxCaptureCallback s_gxCaptureCallback = nullptr;

static const GxVertexInput s_gxVsInputs[] =
{
	{ VS_POSITION,  4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, px) },
	{ VS_NORMAL,    3, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, nx) },
	{ VS_COLOR,     4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, cx) },
	{ VS_TEXCOORD0, 2, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, tx) },
	{ VS_TEXCOORD1, 2, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, tx) }
};

static float scale255(const float v)
{
	static const float m = 1.f / 255.f;
	return v * m;
}

void gxValidateShaderResources(const bool useGenericShader);

void gxInitialize()
{
	memset(&renderState, 0, sizeof(renderState));
	renderState.blendMode = BLEND_ALPHA;
	renderState.colorWriteMask = 0xf;
	
	bindVsInputs(s_gxVsInputs, sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]), sizeof(GxVertex));
	
	memset(&s_gxVertex, 0, sizeof(s_gxVertex));
	s_gxVertex.cx = 1.f;
	s_gxVertex.cy = 1.f;
	s_gxVertex.cz = 1.f;
	s_gxVertex.cw = 1.f;

	s_gxVertexBufferPool.init(sizeof(s_gxVertexBuffer));

	const int maxVertexCount = sizeof(s_gxVertexBuffer) / sizeof(s_gxVertexBuffer[0]);
	const int maxQuads = maxVertexCount / 4;
	const int maxIndicesForQuads = maxQuads * 6;
	
	s_gxIndexBufferPool.init(maxIndicesForQuads * sizeof(INDEX_TYPE));
}

void gxShutdown()
{
	s_gxVertexBufferPool.free();
	
	s_gxIndexBufferPool.free();

	s_gxPrimitiveType = GX_INVALID_PRIM;
	s_gxVertices = nullptr;
	s_gxVertexCount = 0;
	s_gxMaxVertexCount = 0;
	s_gxPrimitiveSize = 0;
	s_gxVertex = GxVertex();
	s_gxTextureEnabled = false;
	
	s_gxLastPrimitiveType = GX_INVALID_PRIM;
	s_gxLastVertexCount = -1;
}

static MTLPrimitiveType toMetalPrimitiveType(const GX_PRIMITIVE_TYPE primitiveType)
{
	switch (primitiveType)
	{
	case GX_POINTS:
		return MTLPrimitiveTypePoint;
	case GX_LINES:
		return MTLPrimitiveTypeLine;
	case GX_LINE_LOOP:
		return MTLPrimitiveTypeLineStrip;
	case GX_LINE_STRIP:
		return MTLPrimitiveTypeLineStrip;
	case GX_TRIANGLES:
		return MTLPrimitiveTypeTriangle;
	case GX_TRIANGLE_FAN:
		return MTLPrimitiveTypeTriangle;
	case GX_TRIANGLE_STRIP:
		return MTLPrimitiveTypeTriangleStrip;
	case GX_QUADS:
		return MTLPrimitiveTypeTriangle;
		
	case GX_INVALID_PRIM:
		break;
	}
	
	logError("unknown GX_PRIMITIVE_TYPE");
	return (MTLPrimitiveType)-1;
}

//

namespace xxHash
{
	static const uint32_t PRIME32_1 = 2654435761U;
	static const uint32_t PRIME32_2 = 2246822519U;
	static const uint32_t PRIME32_3 = 3266489917U;
	static const uint32_t PRIME32_4 = 668265263U;
	static const uint32_t PRIME32_5 = 374761393U;
	
	template <int count>
	static uint32_t rotateLeft(const uint32_t value)
	{
		return (value << count) | (value >> (32 - count));
	}
	
	static uint32_t readU32(const void * buf)
	{
		return *(uint32_t*)buf;
	}
	
	static uint32_t subHash(uint32_t value, const uint8_t * buf, const int index)
	{
		const uint32_t read_value = readU32(buf + index);
		value += read_value * PRIME32_2;
		value = rotateLeft<13>(value);
		value *= PRIME32_1;
		return value;
	}
	
	static uint32_t hash32(const void * in_buf, const int numBytes, const int seed = 0)
	{
		const uint8_t * buf = (uint8_t*)in_buf;
		
		uint32_t h32;
		
		int index = 0;
		
		if (numBytes >= 16)
		{
			const int limit = numBytes - 16;
			
			uint32_t v1 = seed + PRIME32_1 + PRIME32_2;
			uint32_t v2 = seed + PRIME32_2;
			uint32_t v3 = seed + 0;
			uint32_t v4 = seed - PRIME32_1;

			do
			{
				v1 = subHash(v1, buf, index);
				v2 = subHash(v2, buf, index + 4);
				v3 = subHash(v3, buf, index + 8);
				v4 = subHash(v4, buf, index + 12);
				index += 16;
			} while (index <= limit);

			h32 = rotateLeft<1>(v1) + rotateLeft<7>(v2) + rotateLeft<12>(v3) + rotateLeft<18>(v4);
		}
		else
		{
			h32 = seed + PRIME32_5;
		}

		h32 += (uint32_t)numBytes;

		while (index + 4 <= numBytes)
		{
			h32 += readU32(buf + index) * PRIME32_3;
			h32 = rotateLeft<17>(h32) * PRIME32_4;
			index += 4;
		}

		while (index < numBytes)
		{
			h32 += buf[index] * PRIME32_5;
			h32 = rotateLeft<11>(h32) * PRIME32_1;
			index++;
		}

		h32 ^= h32 >> 15;
		h32 *= PRIME32_2;
		h32 ^= h32 >> 13;
		h32 *= PRIME32_3;
		h32 ^= h32 >> 16;

		return h32;
	}
}

static id <MTLRenderPipelineState> s_currentRenderPipelineState = nullptr;

static void gxValidatePipelineState()
{
	if (globals.shader == nullptr || globals.shader->getType() != SHADER_VSPS || !globals.shader->isValid())
	{
		return;
	}
	
	Shader * shader = static_cast<Shader*>(globals.shader);
	
	auto & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader->getCacheElem());
	
	/*
	pipeline state dependencies:
	- render target format(s)
	- vs bindings
	- blend mode
	- color write mask
	- alpha to coverage
	- vs/ps function (shader)
	*/
	
	const uint32_t hash = xxHash::hash32(&renderState, sizeof(renderState));
	
	id <MTLRenderPipelineState> pipelineState = shaderElem.findPipelineState(hash);

	if (pipelineState == nullptr)
	{
		@autoreleasepool
		{
			id <MTLFunction> vsFunction = (id <MTLFunction>)shaderElem.vsFunction;
			id <MTLFunction> psFunction = (id <MTLFunction>)shaderElem.psFunction;
		
			MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
		
			const bool useMultipleVertexBuffers = (renderState.vertexStride == 0);
			
			for (int i = 0; i < renderState.vertexInputCount; ++i)
			{
				auto & e = renderState.vertexInputs[i];
				auto * a = vertexDescriptor.attributes[e.id];
				
				MTLVertexFormat metalFormat = MTLVertexFormatInvalid;
				
				if (e.type == GX_ELEMENT_FLOAT32)
				{
					if (e.numComponents == 1)
						metalFormat = MTLVertexFormatFloat;
					else if (e.numComponents == 2)
						metalFormat = MTLVertexFormatFloat2;
					else if (e.numComponents == 3)
						metalFormat = MTLVertexFormatFloat3;
					else if (e.numComponents == 4)
						metalFormat = MTLVertexFormatFloat4;
				}
				else if (e.type == GX_ELEMENT_UINT8)
				{
					if (e.numComponents == 1)
						metalFormat = e.normalize ? MTLVertexFormatUCharNormalized : MTLVertexFormatUChar;
					else if (e.numComponents == 2)
						metalFormat = e.normalize ? MTLVertexFormatUChar2Normalized : MTLVertexFormatUChar2;
					else if (e.numComponents == 3)
						metalFormat = e.normalize ? MTLVertexFormatUChar3Normalized : MTLVertexFormatUChar3;
					else if (e.numComponents == 4)
						metalFormat = e.normalize ? MTLVertexFormatUChar4Normalized : MTLVertexFormatUChar4;
				}
				else if (e.type == GX_ELEMENT_UINT16)
				{
					if (e.numComponents == 1)
						metalFormat = e.normalize ? MTLVertexFormatUShortNormalized : MTLVertexFormatUShort;
					else if (e.numComponents == 2)
						metalFormat = e.normalize ? MTLVertexFormatUShort2Normalized : MTLVertexFormatUShort2;
					else if (e.numComponents == 3)
						metalFormat = e.normalize ? MTLVertexFormatUShort3Normalized : MTLVertexFormatUShort3;
					else if (e.numComponents == 4)
						metalFormat = e.normalize ? MTLVertexFormatUShort4Normalized : MTLVertexFormatUShort4;
				}
				
				Assert(metalFormat != MTLVertexFormatInvalid);
				if (metalFormat != MTLVertexFormatInvalid)
				{
					a.format = metalFormat;
					a.offset = useMultipleVertexBuffers ? 0 : e.offset;
					a.bufferIndex = useMultipleVertexBuffers ? i : 0;
					
					if (useMultipleVertexBuffers)
					{
						// assign vertex buffers
						
						if (e.stride == 0)
						{
							const int componentSize =
								e.type == GX_ELEMENT_UINT8 ? 1 :
								e.type == GX_ELEMENT_UINT16 ? 2 :
								e.type == GX_ELEMENT_FLOAT32 ? 4 :
								-1;
							
							const int stride = componentSize * e.numComponents;
							
							vertexDescriptor.layouts[i].stride = stride;
							vertexDescriptor.layouts[i].stepRate = 1;
							vertexDescriptor.layouts[i].stepFunction = MTLVertexStepFunctionPerVertex;
						}
						else
						{
							vertexDescriptor.layouts[i].stride = e.stride;
							vertexDescriptor.layouts[i].stepRate = 1;
							vertexDescriptor.layouts[i].stepFunction = MTLVertexStepFunctionPerVertex;
						}
					}
				}
			}
			
			if (useMultipleVertexBuffers == false)
			{
				vertexDescriptor.layouts[0].stride = renderState.vertexStride;
				vertexDescriptor.layouts[0].stepRate = 1;
				vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
			}
		
			MTLRenderPipelineDescriptor * pipelineDescriptor = [[MTLRenderPipelineDescriptor new] autorelease];
			pipelineDescriptor.label = [NSString stringWithCString:shaderElem.name.c_str() encoding:NSASCIIStringEncoding];
			pipelineDescriptor.sampleCount = 1;
			pipelineDescriptor.vertexFunction = vsFunction;
			pipelineDescriptor.fragmentFunction = psFunction;
			pipelineDescriptor.vertexDescriptor = vertexDescriptor;
			pipelineDescriptor.alphaToCoverageEnabled = renderState.alphaToCoverageEnabled;
			
			for (int i = 0; i < kMaxColorTargets; ++i)
			{
				if (renderState.renderPass.colorFormat[i] == 0)
					continue;
				
				auto * att = pipelineDescriptor.colorAttachments[i];

				att.pixelFormat = (MTLPixelFormat)renderState.renderPass.colorFormat[i];

				int writeMask = 0;
				if ((renderState.colorWriteMask & 1) != 0) writeMask |= MTLColorWriteMaskRed;
				if ((renderState.colorWriteMask & 2) != 0) writeMask |= MTLColorWriteMaskGreen;
				if ((renderState.colorWriteMask & 4) != 0) writeMask |= MTLColorWriteMaskBlue;
				if ((renderState.colorWriteMask & 8) != 0) writeMask |= MTLColorWriteMaskAlpha;
				att.writeMask = writeMask;
				
				// blend state
				
				switch (renderState.blendMode)
				{
				case BLEND_OPAQUE:
					att.blendingEnabled = false;
					break;
				case BLEND_ALPHA:
					// note : source alpha is set to ZERO!
					// assuming the destination surface starts at 100% alpha, sussively multiplication by 1-srcA will yield an inverse opacity value stored inside the destination alpha. the destination may then be blended using an inverted premultiplied-alpha blend mode for correctly composing the surface on top of something else
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
					att.sourceAlphaBlendFactor = MTLBlendFactorZero;
					att.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					att.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					break;
				case BLEND_PREMULTIPLIED_ALPHA:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorOne;
					att.sourceAlphaBlendFactor = MTLBlendFactorOne;
					att.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					att.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					break;
				case BLEND_PREMULTIPLIED_ALPHA_DRAW:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
					att.sourceAlphaBlendFactor = MTLBlendFactorOne;
					att.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					att.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					break;
				case BLEND_ABSORBTION_MASK:
					// this is a special blending mode, where the user may generate an 'absorbtion mask'. initially, the user should clear the mask to white. translucent objects may then be drawn onto the mask. the color written to the mask MUST be premultiplied with alpha. the mask will accumulate opacity, where more opaque areas will become dark. alpha will gradually become zero, meaning fully masked. the color should be interpreted as the inverse of the color still allowed to 'bleed through' the mask
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorZero;
					att.sourceAlphaBlendFactor = MTLBlendFactorZero;
					att.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceColor;
					att.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					break;
				case BLEND_ADD:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
					att.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
					att.destinationRGBBlendFactor = MTLBlendFactorOne;
					att.destinationAlphaBlendFactor = MTLBlendFactorOne;
					break;
				case BLEND_ADD_OPAQUE:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorOne;
					att.sourceAlphaBlendFactor = MTLBlendFactorOne;
					att.destinationRGBBlendFactor = MTLBlendFactorOne;
					att.destinationAlphaBlendFactor = MTLBlendFactorOne;
					break;
				case BLEND_SUBTRACT:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationReverseSubtract;
					att.alphaBlendOperation = MTLBlendOperationReverseSubtract;
					att.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
					att.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
					att.destinationRGBBlendFactor = MTLBlendFactorOne;
					att.destinationAlphaBlendFactor = MTLBlendFactorOne;
					break;
				case BLEND_INVERT:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorOneMinusDestinationColor;
					att.sourceAlphaBlendFactor = MTLBlendFactorOneMinusDestinationAlpha;
					att.destinationRGBBlendFactor = MTLBlendFactorZero;
					att.destinationAlphaBlendFactor = MTLBlendFactorZero;
					break;
				case BLEND_MUL:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationAdd;
					att.alphaBlendOperation = MTLBlendOperationAdd;
					att.sourceRGBBlendFactor = MTLBlendFactorZero;
					att.sourceAlphaBlendFactor = MTLBlendFactorZero;
					att.destinationRGBBlendFactor = MTLBlendFactorSourceColor;
					att.destinationAlphaBlendFactor = MTLBlendFactorSourceAlpha;
					break;
				case BLEND_MIN:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationMin;
					att.alphaBlendOperation = MTLBlendOperationMin;
					att.sourceRGBBlendFactor = MTLBlendFactorOne;
					att.sourceAlphaBlendFactor = MTLBlendFactorOne;
					att.destinationRGBBlendFactor = MTLBlendFactorOne;
					att.destinationAlphaBlendFactor = MTLBlendFactorOne;
					break;
				case BLEND_MAX:
					att.blendingEnabled = true;
					att.rgbBlendOperation = MTLBlendOperationMax;
					att.alphaBlendOperation = MTLBlendOperationMax;
					att.sourceRGBBlendFactor = MTLBlendFactorOne;
					att.sourceAlphaBlendFactor = MTLBlendFactorOne;
					att.destinationRGBBlendFactor = MTLBlendFactorOne;
					att.destinationAlphaBlendFactor = MTLBlendFactorOne;
					break;
				#if defined(DEBUG) // DEBUG only, so we get a warning about a missing case statement when compiling in release
				default:
					fassert(false);
					break;
				#endif
				}
			}
			
			if (renderState.renderPass.depthFormat != 0)
			{
				pipelineDescriptor.depthAttachmentPixelFormat = (MTLPixelFormat)renderState.renderPass.depthFormat;
			}
			
			if (renderState.renderPass.stencilFormat != 0)
			{
				pipelineDescriptor.stencilAttachmentPixelFormat = (MTLPixelFormat)renderState.renderPass.stencilFormat;
			}

			NSError * error;
			pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
			if (error != nullptr)
				NSLog(@"%@", error);
			
			//NSLog(@"%@", pipelineState);
			
			shaderElem.addPipelineState(hash, pipelineState);
		}
	}
	
	if (pipelineState != s_currentRenderPipelineState)
	{
		s_currentRenderPipelineState = pipelineState;
		
		[s_activeRenderPass->encoder setRenderPipelineState:pipelineState];
	}
}

static void ensureIndexBufferCapacity(const int numIndices)
{
	const int remaining = (s_gxIndexBufferElem == nullptr) ? 0 : (s_gxIndexBufferPool.m_numBytesPerBuffer - s_gxIndexBufferElemOffset);
	
	if (numIndices * sizeof(INDEX_TYPE) > remaining)
	{
		if (s_gxIndexBufferElem != nullptr)
		{
			auto * elem = s_gxIndexBufferElem;
			
			if (@available(macOS 10.13, *)) [s_activeRenderPass->cmdbuf pushDebugGroup:@"GxBufferPool Release (gxFlush)"];
			[s_activeRenderPass->cmdbuf addCompletedHandler:
				^(id<MTLCommandBuffer> _Nonnull)
				{
					s_gxIndexBufferPool.freeBuffer(elem);
				}];
			if (@available(macOS 10.13, *)) [s_activeRenderPass->cmdbuf popDebugGroup];
		}
		
		s_gxIndexBufferElem = s_gxIndexBufferPool.allocBuffer();
		s_gxIndexBufferElemOffset = 0;
	}
}

static void doCapture(const bool endOfBatch)
{
	Mat4x4 modelView;
	gxGetMatrixf(GX_MODELVIEW, modelView.m_v);
	
	for (int i = 0; i < s_gxVertexCount; ++i)
	{
		const Vec3 p = modelView.Mul4(Vec3(s_gxVertices[i].px, s_gxVertices[i].py, s_gxVertices[i].pz));
		
		s_gxVertices[i].px = p[0];
		s_gxVertices[i].py = p[1];
		s_gxVertices[i].pz = p[2];
	}
	
	s_gxCaptureCallback(
		s_gxVertices,
		s_gxVertexCount * sizeof(GxVertex),
		s_gxVsInputs,
		sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]),
		sizeof(GxVertex),
		s_gxPrimitiveType,
		s_gxVertexCount,
		endOfBatch);

	if (endOfBatch)
	{
		s_gxVertices = nullptr;
		s_gxVertexCount = 0;
	}
	else
	{
		switch (s_gxPrimitiveType)
		{
			case GX_LINE_LOOP:
				s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 1;
				break;
			case GX_LINE_STRIP:
				s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 1;
				break;
			case GX_TRIANGLE_FAN:
				s_gxVertices[0] = s_gxVertices[0];
				s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 2;
				break;
			case GX_TRIANGLE_STRIP:
				s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 2];
				s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 2;
				break;
			default:
				s_gxVertexCount = 0;
		}
	}
}

static void gxFlush(bool endOfBatch)
{
	if (s_gxCaptureCallback != nullptr)
	{
		doCapture(endOfBatch);
		return;
	}
	
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);
	
	Shader genericShader;
	
	const bool useGenericShader = (globals.shader == nullptr);
	
	if (useGenericShader)
		genericShader = Shader("engine/Generic");
	
	Shader & shader =
		useGenericShader
		? genericShader
		:  *static_cast<Shader*>(globals.shader);
	
	const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader.getCacheElem());
	
	const GX_PRIMITIVE_TYPE primitiveType = s_gxPrimitiveType;
	
	if (shader.isValid() == false)
	{
		logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
	}
	else if (s_gxVertexCount != 0)
	{
		// Metal doesn't support line loops. so we emulate support for it here by duplicating
		// the first point at the end of the vertex buffer if this is a line loop
		if (primitiveType == GX_LINE_LOOP)
		{
			if (s_gxHasFirstVertex == false)
			{
				s_gxFirstVertex = s_gxVertices[0];
				s_gxHasFirstVertex = true;
			}
			
			if (endOfBatch)
			{
				s_gxVertices[s_gxVertexCount++] = s_gxFirstVertex;
			}
		}

		const int vertexDataSize = s_gxVertexCount * sizeof(GxVertex);
		
		if (vertexDataSize <= 4096)
		{
			// optimize using setVertexBytes when the draw call is small
			[s_activeRenderPass->encoder setVertexBytes:s_gxVertices length:vertexDataSize atIndex:0];
		}
		else
		{
		// note : keep a reference to the current buffer and allocate vertices from the same buffer
		//        when possible. once the buffer is depleted, or when the command buffer is scheduled,
		//        add the completion handler
			const int remaining = (s_gxVertexBufferElem == nullptr) ? 0 : (s_gxVertexBufferPool.m_numBytesPerBuffer - s_gxVertexBufferElemOffset);
			
			if (vertexDataSize > remaining)
			{
				if (s_gxVertexBufferElem != nullptr)
				{
					auto * elem = s_gxVertexBufferElem;
					
					if (@available(macOS 10.13, *)) [s_activeRenderPass->cmdbuf pushDebugGroup:@"GxBufferPool Release (gxFlush)"];
					{
						[s_activeRenderPass->cmdbuf addCompletedHandler:
							^(id<MTLCommandBuffer> _Nonnull)
							{
								s_gxVertexBufferPool.freeBuffer(elem);
							}];
					}
					if (@available(macOS 10.13, *)) [s_activeRenderPass->cmdbuf popDebugGroup];
				}
				
				s_gxVertexBufferElem = s_gxVertexBufferPool.allocBuffer();
				s_gxVertexBufferElemOffset = 0;
			}
			
			memcpy((uint8_t*)s_gxVertexBufferElem->m_buffer.contents + s_gxVertexBufferElemOffset, s_gxVertices, vertexDataSize);
			[s_activeRenderPass->encoder setVertexBuffer:s_gxVertexBufferElem->m_buffer offset:s_gxVertexBufferElemOffset atIndex:0];
			
			s_gxVertexBufferElemOffset += vertexDataSize;
		}
	
		bindVsInputs(s_gxVsInputs, sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]), sizeof(GxVertex));
	
		setShader(shader);
		
		// set shader parameters for the generic shader
		
		if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
		{
			shader.setImmediate(
				shaderElem.params[ShaderCacheElem::kSp_Params].index,
				s_gxTextureEnabled ? 1.f : 0.f,
				globals.colorMode,
				globals.colorPost,
				globals.colorClamp);
		}
	
		gxValidatePipelineState();
	
		gxValidateMatrices();
	
		gxValidateShaderResources(useGenericShader);
	
		bool indexed = false;
		int numElements = s_gxVertexCount;

		bool needToRegenerateIndexBuffer = false;
	
		if (s_gxPrimitiveType != s_gxLastPrimitiveType || s_gxVertexCount != s_gxLastVertexCount)
		{
			s_gxLastPrimitiveType = s_gxPrimitiveType;
			s_gxLastVertexCount = s_gxVertexCount;

			needToRegenerateIndexBuffer = true;
		}
	
		// convert quads to triangles
	
		if (s_gxPrimitiveType == GX_QUADS)
		{
			fassert(s_gxVertexCount <= 65536);
			
			const int numQuads = s_gxVertexCount / 4;
			const int numIndices = numQuads * 6;

			if (needToRegenerateIndexBuffer)
			{
				ensureIndexBufferCapacity(numIndices);
			
				s_gxLastIndexOffset = s_gxIndexBufferElemOffset;
				
				INDEX_TYPE * indices = (INDEX_TYPE*)((uint8_t*)s_gxIndexBufferElem->m_buffer.contents + s_gxIndexBufferElemOffset);

				INDEX_TYPE * __restrict indexPtr = indices;
				INDEX_TYPE baseIndex = 0;
			
				for (int i = 0; i < numQuads; ++i)
				{
					*indexPtr++ = baseIndex + 0;
					*indexPtr++ = baseIndex + 1;
					*indexPtr++ = baseIndex + 2;
				
					*indexPtr++ = baseIndex + 0;
					*indexPtr++ = baseIndex + 2;
					*indexPtr++ = baseIndex + 3;
				
					baseIndex += 4;
				}
				
				s_gxIndexBufferElemOffset += numIndices * sizeof(INDEX_TYPE);
			}
			
			s_gxPrimitiveType = GX_TRIANGLES;
			numElements = numIndices;
			
			indexed = true;
		}
	
		// convert triangle fan to triangles
	
		if (s_gxPrimitiveType == GX_TRIANGLE_FAN)
		{
			fassert(s_gxVertexCount <= 65536);
			
			const int numTriangles = s_gxVertexCount - 2;
			const int numIndices = numTriangles * 3;

			if (needToRegenerateIndexBuffer)
			{
				ensureIndexBufferCapacity(numIndices);
				
				s_gxLastIndexOffset = s_gxIndexBufferElemOffset;
				
				INDEX_TYPE * indices = (INDEX_TYPE*)((uint8_t*)s_gxIndexBufferElem->m_buffer.contents + s_gxIndexBufferElemOffset);

				INDEX_TYPE * __restrict indexPtr = indices;
				INDEX_TYPE baseIndex = 0;
			
				for (int i = 0; i < numTriangles; ++i)
				{
					*indexPtr++ = 0;
					*indexPtr++ = baseIndex + 1;
					*indexPtr++ = baseIndex + 2;
				
					baseIndex += 1;
				}
				
				s_gxIndexBufferElemOffset += numIndices * sizeof(INDEX_TYPE);
			}
			
			s_gxPrimitiveType = GX_TRIANGLES;
			numElements = numIndices;
			
			indexed = true;
		}
	
		const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(s_gxPrimitiveType);

		if (indexed)
		{
 			[s_activeRenderPass->encoder drawIndexedPrimitives:metalPrimitiveType indexCount:numElements indexType:MTLIndexTypeUInt32 indexBuffer:s_gxIndexBufferElem->m_buffer indexBufferOffset:s_gxLastIndexOffset];
		}
		else
		{
			[s_activeRenderPass->encoder drawPrimitives:metalPrimitiveType vertexStart:0 vertexCount:numElements];
		}
	}
	
	if (endOfBatch)
	{
		s_gxVertices = nullptr;
		s_gxVertexCount = 0;
	}
	else
	{
		switch (s_gxPrimitiveType)
		{
			case GX_LINE_LOOP:
				s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 1;
				break;
			case GX_LINE_STRIP:
				s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 1;
				break;
			case GX_TRIANGLE_FAN:
				s_gxVertices[0] = s_gxVertices[0];
				s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 2;
				break;
			case GX_TRIANGLE_STRIP:
				s_gxVertices[0] = s_gxVertices[s_gxVertexCount - 2];
				s_gxVertices[1] = s_gxVertices[s_gxVertexCount - 1];
				s_gxVertexCount = 2;
				break;
			default:
				s_gxVertexCount = 0;
		}
	}
	
	globals.gxShaderIsDirty = false;

	s_gxPrimitiveType = primitiveType;
}

void gxBegin(GX_PRIMITIVE_TYPE primitiveType)
{
	s_gxPrimitiveType = primitiveType;
	s_gxVertices = s_gxVertexBuffer;
	s_gxMaxVertexCount = sizeof(s_gxVertexBuffer) / sizeof(s_gxVertexBuffer[0]);
	
	switch (primitiveType)
	{
		case GX_TRIANGLES:
			s_gxPrimitiveSize = 3;
			break;
		case GX_QUADS:
			s_gxPrimitiveSize = 4;
			break;
		case GX_LINES:
			s_gxPrimitiveSize = 2;
			break;
		case GX_LINE_LOOP:
			s_gxPrimitiveSize = 2; // +1 to ensure we can append the first vertex to close the line loop
			break;
		case GX_LINE_STRIP:
			s_gxPrimitiveSize = 1;
			break;
		case GX_POINTS:
			s_gxPrimitiveSize = 1;
			break;
		case GX_TRIANGLE_FAN:
			s_gxPrimitiveSize = 1;
			break;
		case GX_TRIANGLE_STRIP:
			s_gxPrimitiveSize = 1;
			break;
		#if defined(DEBUG) // DEBUG only, so we get a warning about a missing case statement when compiling in release
		default:
			fassert(false);
		#endif
	}
	
	fassert(s_gxVertexCount == 0);
	
	s_gxHasFirstVertex = false;
}

void gxEnd()
{
	gxFlush(true);
}

static void gxEndDraw()
{
	// add completion handler if there's still a buffer pool element in use
	
	if (s_gxVertexBufferElem != nullptr || s_gxIndexBufferElem != nullptr)
	{
		auto * vb_elem = s_gxVertexBufferElem;
		auto * ib_elem = s_gxIndexBufferElem;
		
		if (@available(macOS 10.13, *)) [s_activeRenderPass->cmdbuf pushDebugGroup:@"GxBufferPool Release (gxEndDraw)"];
		{
			[s_activeRenderPass->cmdbuf addCompletedHandler:
				^(id<MTLCommandBuffer> _Nonnull)
				{
					if (vb_elem != nullptr)
						s_gxVertexBufferPool.freeBuffer(vb_elem);
					if (ib_elem != nullptr)
						s_gxIndexBufferPool.freeBuffer(ib_elem);
				}];
		}
		if (@available(macOS 10.13, *)) [s_activeRenderPass->cmdbuf popDebugGroup];
		
		s_gxVertexBufferElem = nullptr;
		s_gxVertexBufferElemOffset = 0;
		
		s_gxIndexBufferElem = nullptr;
		s_gxIndexBufferElemOffset = 0;
	}
	
	s_gxLastVertexCount = -1; // reset, to ensure the index buffer gets regenerated
	s_gxLastIndexOffset = -1; // reset, to ensure we don't reuse old index buffers
	
	// clear textures to avoid freed textures from being reused (prefer to crash instead)
	
	[s_activeRenderPass->encoder insertDebugSignpost:@"Clear textures (gxEndDraw)"];
	for (int i = 0; i < ShaderCacheElem_Metal::kMaxVsTextures; ++i)
		[s_activeRenderPass->encoder setVertexTexture:nullptr atIndex:i];
	for (int i = 0; i < ShaderCacheElem_Metal::kMaxPsTextures; ++i)
		[s_activeRenderPass->encoder setFragmentTexture:nullptr atIndex:i];
	
	// reset the current pipeline state, to ensure we set it again when recording the next command buffer
	
	s_currentRenderPipelineState = nullptr;
}

void gxEmitVertices(GX_PRIMITIVE_TYPE primitiveType, int numVertices)
{
	fassert(primitiveType == GX_POINTS || primitiveType == GX_LINES || primitiveType == GX_TRIANGLES || primitiveType == GX_TRIANGLE_STRIP);
	
	bindVsInputs(nullptr, 0, 0);
	
	Shader genericShader;
	
	const bool useGenericShader = (globals.shader == nullptr);
	
	if (useGenericShader)
		genericShader = Shader("engine/Generic");
	
	Shader & shader =
		useGenericShader
		? genericShader
		: *static_cast<Shader*>(globals.shader);

	setShader(shader);

	const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader.getCacheElem());
	
	// set shader parameters for the generic shader
	
	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1 : 0,
			globals.colorMode,
			globals.colorPost,
			globals.colorClamp);
	}
	
	gxValidatePipelineState();
	
	gxValidateMatrices();
	
	gxValidateShaderResources(useGenericShader);

	//
	
	const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(primitiveType);

	[s_activeRenderPass->encoder drawPrimitives:metalPrimitiveType vertexStart:0 vertexCount:numVertices];

	globals.gxShaderIsDirty = false;
	
// todo : bind VS inputs on gxBegin call ?
	bindVsInputs(s_gxVsInputs, sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]), sizeof(GxVertex));
}

void gxEmitVertex()
{
	s_gxVertices[s_gxVertexCount++] = s_gxVertex;
	
	if (s_gxVertexCount + s_gxPrimitiveSize > s_gxMaxVertexCount)
	{
		if (s_gxVertexCount % s_gxPrimitiveSize == 0)
		{
			gxFlush(false);
		}
	}
}

void gxColor3f(float r, float g, float b)
{
	s_gxVertex.cx = r;
	s_gxVertex.cy = g;
	s_gxVertex.cz = b;
	s_gxVertex.cw = 1.f;
}

void gxColor3fv(const float * v)
{
	s_gxVertex.cx = v[0];
	s_gxVertex.cy = v[1];
	s_gxVertex.cz = v[2];
	s_gxVertex.cw = 1.f;
}

void gxColor4f(float r, float g, float b, float a)
{
	s_gxVertex.cx = r;
	s_gxVertex.cy = g;
	s_gxVertex.cz = b;
	s_gxVertex.cw = a;
}

void gxColor4fv(const float * rgba)
{
	s_gxVertex.cx = rgba[0];
	s_gxVertex.cy = rgba[1];
	s_gxVertex.cz = rgba[2];
	s_gxVertex.cw = rgba[3];
}

void gxColor3ub(int r, int g, int b)
{
	gxColor4f(
		scale255(r),
		scale255(g),
		scale255(b),
		1.f);
}

void gxColor4ub(int r, int g, int b, int a)
{
	gxColor4f(
		scale255(r),
		scale255(g),
		scale255(b),
		scale255(a));
}

void gxTexCoord2f(float u, float v)
{
	s_gxVertex.tx = u;
	s_gxVertex.ty = v;
}

void gxNormal3f(float x, float y, float z)
{
	s_gxVertex.nx = x;
	s_gxVertex.ny = y;
	s_gxVertex.nz = z;
}

void gxNormal3fv(const float * v)
{
	gxNormal3f(v[0], v[1], v[2]);
}

void gxVertex2f(float x, float y)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = 0.f;
	s_gxVertex.pw = 1.f;

	gxEmitVertex();
}

void gxVertex3f(float x, float y, float z)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = z;
	s_gxVertex.pw = 1.f;
	
	gxEmitVertex();
}

void gxVertex4f(float x, float y, float z, float w)
{
	s_gxVertex.px = x;
	s_gxVertex.py = y;
	s_gxVertex.pz = z;
	s_gxVertex.pw = w;

	gxEmitVertex();
}

void gxVertex3fv(const float * v)
{
	gxVertex3f(v[0], v[1], v[2]);
}

void gxVertex4fv(const float * v)
{
	gxVertex4f(v[0], v[1], v[2], v[3]);
}

void gxSetTexture(GxTextureId texture)
{
	s_gxTexture = texture;
	s_gxTextureEnabled = texture != 0;
}

void gxSetTextureSampler(GX_SAMPLE_FILTER filter, bool clamp)
{
	const int filter_index =
		filter == GX_SAMPLE_NEAREST ? 0 :
		filter == GX_SAMPLE_LINEAR ? 1 :
		2;
		
	s_gxTextureSampler = (filter_index << 1) | clamp;
}

//

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride)
{
	const int maxVsInputs = sizeof(renderState.vertexInputs) / sizeof(renderState.vertexInputs[0]);
	Assert(numVsInputs <= maxVsInputs);
	const int numVsInputsToCopy = numVsInputs < maxVsInputs ? numVsInputs : maxVsInputs;
	memcpy(renderState.vertexInputs, vsInputs, numVsInputsToCopy * sizeof(GxVertexInput));
	if (numVsInputsToCopy < maxVsInputs)
		memset(renderState.vertexInputs + numVsInputsToCopy, 0, (maxVsInputs - numVsInputsToCopy) * sizeof(GxVertexInput)); // set the remainder of the inputs to zero, to ensure the contents of the render state struct stays deterministics, so we can hash it to look up pipeline states
	renderState.vertexInputCount = numVsInputsToCopy;
	renderState.vertexStride = vsStride;
}

void gxSetVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride)
{
	if (buffer == nullptr)
	{
		// restore buffer bindings to the default GX buffer bindings
		
		bindVsInputs(s_gxVsInputs, sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]), sizeof(GxVertex));
	}
	else
	{
		// bind the specified vertex buffer and vertex buffer bindings
		
		bindVsInputs(vsInputs, numVsInputs, vsStride);
	
		if (vsStride == 0)
		{
			id <MTLBuffer> metalBuffer = (id <MTLBuffer>)buffer->getMetalBuffer();
			for (int i = 0; i < numVsInputs; ++i)
				[s_activeRenderPass->encoder setVertexBuffer:metalBuffer offset:vsInputs[i].offset atIndex:i];
		}
		else
		{
			id <MTLBuffer> metalBuffer = (id <MTLBuffer>)buffer->getMetalBuffer();
			[s_activeRenderPass->encoder setVertexBuffer:metalBuffer offset:0 atIndex:0];
		}
	}
}

void gxDrawIndexedPrimitives(const GX_PRIMITIVE_TYPE type, const int firstIndex, const int numIndices, const GxIndexBuffer * indexBuffer)
{
	Assert(indexBuffer != nullptr);
	
	Shader genericShader;
	
	const bool useGenericShader = (globals.shader == nullptr);
	
	if (useGenericShader)
		genericShader = Shader("engine/Generic");
	
	Shader & shader =
		useGenericShader
		? genericShader
		: *static_cast<Shader*>(globals.shader);

	setShader(shader);

	const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader.getCacheElem());

	// set shader parameters for the generic shader
	
	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1.f : 0.f,
			globals.colorMode,
			globals.colorPost,
			globals.colorClamp);
	}
	
	gxValidatePipelineState();

	gxValidateMatrices();

	gxValidateShaderResources(useGenericShader);
	
	if (shader.isValid())
	{
		const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(type);

		id <MTLBuffer> buffer = (id <MTLBuffer>)indexBuffer->getMetalBuffer();
		
		const int indexSize =
			indexBuffer->getFormat() == GX_INDEX_16
				? 2
				: 4;
		const int indexOffset = firstIndex * indexSize;
		
		[s_activeRenderPass->encoder
			drawIndexedPrimitives:metalPrimitiveType
			indexCount:numIndices
			indexType:
				indexBuffer->getFormat() == GX_INDEX_16
				? MTLIndexTypeUInt16
				: MTLIndexTypeUInt32
			indexBuffer:buffer
			indexBufferOffset:indexOffset];
	}
	else
	{
		logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
	}

	globals.gxShaderIsDirty = false;
}

void gxDrawPrimitives(const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices)
{
	Shader genericShader;
	
	const bool useGenericShader = (globals.shader == nullptr);
	
	if (useGenericShader)
		genericShader = Shader("engine/Generic");
	
	Shader & shader =
		useGenericShader
		? genericShader
		: *static_cast<Shader*>(globals.shader);

	setShader(shader);

	const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader.getCacheElem());

	// set shader parameters for the generic shader
	
	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1.f : 0.f,
			globals.colorMode,
			globals.colorPost,
			globals.colorClamp);
	}
	
	gxValidatePipelineState();

	gxValidateMatrices();

	gxValidateShaderResources(useGenericShader);
	
	if (shader.isValid())
	{
		const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(type);

		[s_activeRenderPass->encoder drawPrimitives:metalPrimitiveType vertexStart:firstVertex vertexCount:numVertices];
	}
	else
	{
		logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
	}
	
	globals.gxShaderIsDirty = false;
}

void gxDrawInstancedIndexedPrimitives(const int numInstances, const GX_PRIMITIVE_TYPE type, const int firstIndex, const int numIndices, const GxIndexBuffer * indexBuffer)
{
	Assert(indexBuffer != nullptr);
	
	Shader genericShader;
	
	const bool useGenericShader = (globals.shader == nullptr);
	
	if (useGenericShader)
		genericShader = Shader("engine/Generic");
	
	Shader & shader =
		useGenericShader
		? genericShader
		: *static_cast<Shader*>(globals.shader);

	setShader(shader);
	
	const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader.getCacheElem());

	// set shader parameters for the generic shader
	
	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1.f : 0.f,
			globals.colorMode,
			globals.colorPost,
			globals.colorClamp);
	}

	gxValidatePipelineState();

	gxValidateMatrices();

	gxValidateShaderResources(useGenericShader);
	
	if (shader.isValid())
	{
		const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(type);

		id <MTLBuffer> buffer = (id <MTLBuffer>)indexBuffer->getMetalBuffer();
		
		const int indexSize =
			indexBuffer->getFormat() == GX_INDEX_16
				? 2
				: 4;
		const int indexOffset = firstIndex * indexSize;
		
		[s_activeRenderPass->encoder
			drawIndexedPrimitives:metalPrimitiveType
			indexCount:numIndices
			indexType:
				indexBuffer->getFormat() == GX_INDEX_16
				? MTLIndexTypeUInt16
				: MTLIndexTypeUInt32
			indexBuffer:buffer
			indexBufferOffset:indexOffset
			instanceCount:numInstances];
	}
	else
	{
		logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
	}

	globals.gxShaderIsDirty = false;
}

void gxDrawInstancedPrimitives(const int numInstances, const GX_PRIMITIVE_TYPE type, const int firstVertex, const int numVertices)
{
	Shader genericShader;
	
	const bool useGenericShader = (globals.shader == nullptr);
	
	if (useGenericShader)
		genericShader = Shader("engine/Generic");
	
	Shader & shader =
		useGenericShader
		? genericShader
		: *static_cast<Shader*>(globals.shader);

	setShader(shader);

	const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader.getCacheElem());

	// set shader parameters for the generic shader
	
	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1.f : 0.f,
			globals.colorMode,
			globals.colorPost,
			globals.colorClamp);
	}
	
	gxValidatePipelineState();

	gxValidateMatrices();

	gxValidateShaderResources(useGenericShader);
	
	if (shader.isValid())
	{
		const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(type);

		[s_activeRenderPass->encoder
			drawPrimitives:metalPrimitiveType
			vertexStart:firstVertex
			vertexCount:numVertices
			instanceCount:numInstances];
	}
	else
	{
		logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
	}
	
	globals.gxShaderIsDirty = false;
}

void gxSetCaptureCallback(GxCaptureCallback callback)
{
	Assert(s_gxCaptureCallback == nullptr);
	s_gxCaptureCallback = callback;
}

void gxClearCaptureCallback()
{
	s_gxCaptureCallback = nullptr;
}

//

void gxValidateShaderResources(const bool useGenericShader)
{
	auto * shader = static_cast<Shader*>(globals.shader);
	auto & cacheElem = static_cast<const ShaderCacheElem_Metal&>(shader->getCacheElem());
	
	if (useGenericShader && s_gxTextureEnabled)
	{
		// todo : avoid setting textures when not needed
		//        needed when: texture changed
		//                  or shader changed
		
		auto i = s_textures.find(s_gxTexture);
		
		Assert(i != s_textures.end());
		if (i != s_textures.end())
		{
			auto & texture = i->second;
			[s_activeRenderPass->encoder setFragmentTexture:texture atIndex:0];
			
			id<MTLSamplerState> samplerState = samplerStates[s_gxTextureSampler];
			[s_activeRenderPass->encoder setFragmentSamplerState:samplerState atIndex:0];
		}
	}
	else
	{
		for (int i = 0; i < ShaderCacheElem_Metal::kMaxVsTextures; ++i)
			if (cacheElem.vsTextures[i] != nullptr)
				[s_activeRenderPass->encoder setVertexTexture:cacheElem.vsTextures[i] atIndex:i];
		
	#if 0
		for (int i = 0; i < ShaderCacheElem_Metal::kMaxPsTextures; ++i)
			if (cacheElem.psTextures[i] != nullptr)
				[s_activeRenderPass->encoder setFragmentTexture:cacheElem.psTextures[i] atIndex:i];
	#else
		[s_activeRenderPass->encoder setFragmentTextures:cacheElem.psTextures withRange:NSMakeRange(0, ShaderCacheElem_Metal::kMaxPsTextures)];
	#endif
	
		for (auto & textureInfo : cacheElem.textureInfos)
		{
			// todo : set sampler states at the start of a render pass. or set an invalidation bit
			//        right now we just set it _always_ to pass validation..
			
			if (textureInfo.vsOffset != -1)
			{
				const int i = textureInfo.vsOffset;
				id<MTLSamplerState> samplerState = samplerStates[cacheElem.vsTextureSamplers[i]];
				[s_activeRenderPass->encoder setVertexSamplerState:samplerState atIndex:i];
			}
			
			if (textureInfo.psOffset != -1)
			{
				const int i = textureInfo.psOffset;
				id<MTLSamplerState> samplerState = samplerStates[cacheElem.psTextureSamplers[i]];
				[s_activeRenderPass->encoder setFragmentSamplerState:samplerState atIndex:i];
			}
		}
		
		NSUInteger offsets[ShaderCacheElem_Metal::kMaxBuffers] = { };
		[s_activeRenderPass->encoder setFragmentBuffers:cacheElem.psBuffers offsets:offsets withRange:NSMakeRange(0, ShaderCacheElem_Metal::kMaxBuffers)];
	}
	
	// set fragment stage uniform buffers

	for (int i = 0; i < ShaderCacheElem_Metal::kMaxBuffers; ++i)
	{
		if (cacheElem.psInfo.uniformBufferSize[i] == 0)
			continue;
		if (cacheElem.psBuffers[i] != nullptr)
			continue;
		
		[s_activeRenderPass->encoder
			setFragmentBytes:cacheElem.psUniformData[i]
			length:cacheElem.psInfo.uniformBufferSize[i]
			atIndex:i];
	}
}

#endif
