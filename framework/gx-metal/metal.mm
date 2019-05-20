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

#import "framework.h"

#if ENABLE_METAL

#import "bufferPool.h"
#import "data/engine/ShaderCommon.txt" // VS_ constants
#import "internal.h"
#import "metal.h"
#import "metalView.h"
#import "shader.h"
#import "shaders.h" // registerBuiltinShaders
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

static id <MTLDevice> device;

static std::map<SDL_Window*, MetalWindowData*> windowDatas;

MetalWindowData * activeWindowData = nullptr;

static std::vector<id <MTLResource>> s_resourcesToFree;

extern std::map<std::string, std::string> s_shaderSources; // todo : can this be exposed/determined more nicely?

static void freeResourcesToFree()
{
	for (id <MTLResource> & resource : s_resourcesToFree)
	{
		//NSLog(@"resource release, retain count: %lu", [resource retainCount]);
		
		[resource release];
		resource = nullptr;
	}
	
	s_resourcesToFree.clear();
}

void metal_init()
{
	device = MTLCreateSystemDefaultDevice();
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
		windowData->metalview = [[MetalView alloc] initWithFrame:sdl_view.frame device:device wantsDepthBuffer:YES];
		[sdl_view addSubview:windowData->metalview];

		windowData->renderdesc = [MTLRenderPassDescriptor renderPassDescriptor];
		[windowData->renderdesc retain];
		
		windowData->queue = [windowData->metalview.metalLayer.device newCommandQueue];
		
		windowDatas[window] = windowData;
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

void metal_draw_begin(const float r, const float g, const float b, const float a)
{
	@autoreleasepool
	{
		activeWindowData->cmdbuf = [[activeWindowData->queue commandBuffer] retain];
		
		activeWindowData->current_drawable = [activeWindowData->metalview.metalLayer nextDrawable];
		[activeWindowData->current_drawable retain];
		
		MTLRenderPassColorAttachmentDescriptor * colorattachment = activeWindowData->renderdesc.colorAttachments[0];
		colorattachment.texture = activeWindowData->current_drawable.texture;
		
		/* Clear to a red-orange color when beginning the render pass. */
		colorattachment.clearColor  = MTLClearColorMake(r, g, b, a);
		colorattachment.loadAction  = MTLLoadActionClear;
		colorattachment.storeAction = MTLStoreActionStore;
		
		MTLRenderPassDepthAttachmentDescriptor * depthattachment = activeWindowData->renderdesc.depthAttachment;
		depthattachment.texture = activeWindowData->metalview.depthTexture;
		depthattachment.clearDepth = 1.0;
		depthattachment.loadAction = MTLLoadActionClear;
		depthattachment.storeAction = MTLStoreActionDontCare;

		/* The drawable's texture is cleared to the specified color here. */
		activeWindowData->encoder = [[activeWindowData->cmdbuf renderCommandEncoderWithDescriptor:activeWindowData->renderdesc] retain];
		activeWindowData->encoder.label = @"hello encoder";
		
		const CGSize size = activeWindowData->metalview.frame.size;
		metal_set_viewport(size.width, size.height);
	}
}

void metal_draw_end()
{
	gxEndDraw();
	
	[activeWindowData->encoder endEncoding];
	
	[activeWindowData->cmdbuf addCompletedHandler:
		^(id<MTLCommandBuffer> _Nonnull)
		{
			//NSLog(@"hello done! %@", activeWindowData);
		}];

	[activeWindowData->cmdbuf presentDrawable:activeWindowData->current_drawable];
	[activeWindowData->cmdbuf commit];
	
// todo : remove and use addCompletedHandler instead
	//[activeWindowData->cmdbuf waitUntilCompleted];
	freeResourcesToFree(); // todo : call in response to completion handler
	
	//
	
	[activeWindowData->encoder release];
	[activeWindowData->current_drawable release];
	[activeWindowData->cmdbuf release];
	
	activeWindowData->encoder = nullptr;
	activeWindowData->current_drawable = nullptr;
	activeWindowData->cmdbuf = nullptr;
}

void metal_set_viewport(const int sx, const int sy)
{
	[activeWindowData->encoder setViewport:(MTLViewport){ 0, 0, (double)sx, (double)sy, 0.0, 1.0 }];
}

void metal_set_scissor(const int x, const int y, const int sx, const int sy)
{
	const MTLScissorRect rect = { (NSUInteger)x, (NSUInteger)y, (NSUInteger)sx, (NSUInteger)sy };
	
	[activeWindowData->encoder setScissorRect:rect];
}

void metal_clear_scissor()
{
	const NSUInteger sx = activeWindowData->renderdesc.colorAttachments[0].texture.width;
	const NSUInteger sy = activeWindowData->renderdesc.colorAttachments[0].texture.height;
	
	const MTLScissorRect rect = { 0, 0, sx, sy };
	
	[activeWindowData->encoder setScissorRect:rect];
}

id <MTLDevice> metal_get_device()
{
	return device;
}

void metal_upload_texture_area(const void * src, const int srcPitch, const int srcSx, const int srcSy, id <MTLTexture> dst, const int dstX, const int dstY, const MTLPixelFormat pixelFormat)
{
	@autoreleasepool
	{
		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:pixelFormat width:srcSx height:srcSy mipmapped:NO];
		
		id <MTLTexture> src_texture = [device newTextureWithDescriptor:descriptor];
		
		const MTLOrigin src_origin = { 0, 0, 0 };
		const MTLSize src_size = { (NSUInteger)srcSx, (NSUInteger)srcSy, 1 };
		const MTLOrigin dst_origin = { (NSUInteger)dstX, (NSUInteger)dstY, 0 };
		
		auto blit_cmdbuf = [activeWindowData->queue commandBuffer];
		auto blit_encoder = [blit_cmdbuf blitCommandEncoder];
		{
		// todo : reuse fences
			id <MTLFence> waitForDraw = [device newFence];
			id <MTLFence> waitForBlit = [device newFence];
			
			[activeWindowData->encoder updateFence:waitForDraw afterStages:MTLRenderStageFragment];
			{
				[blit_encoder waitForFence:waitForDraw];
				[blit_encoder copyFromTexture:src_texture sourceSlice:0 sourceLevel:0 sourceOrigin:src_origin sourceSize:src_size toTexture:dst destinationSlice:0 destinationLevel:0 destinationOrigin:dst_origin];
				[blit_encoder updateFence:waitForBlit];
			}
			[activeWindowData->encoder waitForFence:waitForBlit beforeStages:MTLRenderStageVertex];
			
			[waitForDraw release];
			[waitForBlit release];
		}
		[blit_encoder endEncoding];
		[blit_cmdbuf commit];
		
		s_resourcesToFree.push_back(src_texture);
	}
}

void metal_copy_texture_to_texture(id <MTLTexture> src, const int srcPitch, const int srcX, const int srcY, const int srcSx, const int srcSy, id <MTLTexture> dst, const int dstX, const int dstY, const MTLPixelFormat pixelFormat)
{
	@autoreleasepool
	{
		const MTLOrigin src_origin = { (NSUInteger)srcX, (NSUInteger)srcY, 0 };
		const MTLSize src_size = { (NSUInteger)srcSx, (NSUInteger)srcSy, 1 };
		const MTLOrigin dst_origin = { (NSUInteger)dstX, (NSUInteger)dstY, 0 };
		
		auto blit_cmdbuf = [activeWindowData->queue commandBuffer];
		auto blit_encoder = [blit_cmdbuf blitCommandEncoder];
		{
		// todo : reuse fences
			id <MTLFence> waitForDraw = [device newFence];
			id <MTLFence> waitForBlit = [device newFence];
			
			[activeWindowData->encoder updateFence:waitForDraw afterStages:MTLRenderStageFragment];
			{
				[blit_encoder waitForFence:waitForDraw];
				[blit_encoder copyFromTexture:src sourceSlice:0 sourceLevel:0 sourceOrigin:src_origin sourceSize:src_size toTexture:dst destinationSlice:0 destinationLevel:0 destinationOrigin:dst_origin];
				[blit_encoder updateFence:waitForBlit];
			}
			[activeWindowData->encoder waitForFence:waitForBlit beforeStages:MTLRenderStageVertex];
			
			[waitForDraw release];
			[waitForBlit release];
		}
		[blit_encoder endEncoding];
		[blit_cmdbuf commit];
	}
}

void metal_generate_mipmaps(id <MTLTexture> texture)
{
	@autoreleasepool
	{
		auto blit_cmdbuf = [activeWindowData->queue commandBuffer];
		auto blit_encoder = [blit_cmdbuf blitCommandEncoder];
		{
		// todo : reuse fences
			id <MTLFence> waitForDraw = [device newFence];
			id <MTLFence> waitForBlit = [device newFence];
			
			[activeWindowData->encoder updateFence:waitForDraw afterStages:MTLRenderStageFragment];
			{
				[blit_encoder waitForFence:waitForDraw];
				[blit_encoder generateMipmapsForTexture:texture];
				[blit_encoder updateFence:waitForBlit];
			}
			[activeWindowData->encoder waitForFence:waitForBlit beforeStages:MTLRenderStageVertex];
			
			[waitForDraw release];
			[waitForBlit release];
		}
		[blit_encoder endEncoding];
		[blit_cmdbuf commit];
	}
}

// -- render states --

static Stack<BLEND_MODE, 32> blendModeStack(BLEND_ALPHA);
static Stack<bool, 32> lineSmoothStack(false);
static Stack<bool, 32> wireframeStack(false);
static Stack<DepthTestInfo, 32> depthTestStack(DepthTestInfo { false, DEPTH_LESS, true });
static Stack<CullModeInfo, 32> cullModeStack(CullModeInfo { CULL_NONE, CULL_CCW });

RenderPipelineState renderState;

// render states independent from render pipeline state

static bool s_depthTestEnabled = false;
static DEPTH_TEST s_depthTest = DEPTH_ALWAYS;
static bool s_depthWriteEnabled = false;

//#include "renderTarget.h"

struct RenderPassData
{
	id <MTLCommandBuffer> cmdbuf;
	
	MTLRenderPassDescriptor * renderdesc = nullptr;
	
	id <MTLRenderCommandEncoder> encoder;
};

std::vector<RenderPassData> s_renderPasses;

#if TODO // todo : implement render passes

void pushRenderPass(ColorTarget * target, DepthTarget * depthTarget, const bool clearDepth)
{
	pushRenderPass(&target, target == nullptr ? 0 : 1, depthTarget, clearDepth);
}

void pushRenderPass(ColorTarget ** targets, const int numTargets, DepthTarget * depthTarget, const bool clearDepth)
{
	RenderPassData pd;
	
	@autoreleasepool
	{
		pd.cmdbuf = [[activeWindowData->queue commandBuffer] retain];

	 	pd.renderdesc = [[MTLRenderPassDescriptor renderPassDescriptor] retain];
		
		for (int i = 0; i < numTargets; ++i)
		{
			MTLRenderPassColorAttachmentDescriptor * colorattachment = activeWindowData->renderdesc.colorAttachments[i];
			colorattachment.texture = (id <MTLTexture>)targets[i]->getMetalTexture();
			
			colorattachment.clearColor  = MTLClearColorMake(
				targets[i]->clearColor.r,
				targets[i]->clearColor.g,
				targets[i]->clearColor.b,
				targets[i]->clearColor.a);
			colorattachment.loadAction  = MTLLoadActionClear;
			colorattachment.storeAction = MTLStoreActionStore;
		}
	
		if (depthTarget != nullptr)
		{
			MTLRenderPassDepthAttachmentDescriptor * depthattachment = pd.renderdesc.depthAttachment;
			depthattachment.texture = (id <MTLTexture>)depthTarget->m_depthTexture;
			depthattachment.clearDepth = depthTarget->clearDepth;
			depthattachment.loadAction = MTLLoadActionClear;
			depthattachment.storeAction = MTLStoreActionDontCare;
		}

		pd.encoder = [[pd.cmdbuf renderCommandEncoderWithDescriptor:pd.renderdesc] retain];
		pd.encoder.label = @"hello encoder";
		
		// todo : set viewport
		// todo : set blend mode
		
		s_renderPasses.push_back(pd);
	}
}

void popRenderPass()
{
	auto & pd = s_renderPasses.back();
	
	[pd.encoder endEncoding];
	
	[pd.cmdbuf commit];
	
	//
	
	[pd.encoder release];
	[pd.cmdbuf release];
	
	s_renderPasses.pop_back();
}

#endif

void setBlend(BLEND_MODE blendMode)
{
	renderState.blendMode = blendMode;
}

void pushBlend(BLEND_MODE blendMode)
{
	blendModeStack.push(globals.blendMode);
	
	setBlend(blendMode);
}

void popBlend()
{
	const BLEND_MODE blendMode = blendModeStack.popValue();
	
	setBlend(blendMode);
}

void setLineSmooth(bool enabled)
{
	//Assert(false);
}

void pushLineSmooth(bool enabled)
{
	lineSmoothStack.push(globals.lineSmoothEnabled);
	
	setLineSmooth(enabled);
}

void popLineSmooth()
{
	const bool value = lineSmoothStack.popValue();
	
	setLineSmooth(value);
}

void setWireframe(bool enabled)
{
	[activeWindowData->encoder setTriangleFillMode:enabled ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
}

void pushWireframe(bool enabled)
{
	wireframeStack.push(globals.wireframeEnabled);
	
	setWireframe(enabled);
}

void popWireframe()
{
	const bool value = wireframeStack.popValue();
	
	setWireframe(value);
}

void setDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled)
{
	s_depthTestEnabled = enabled;
	s_depthTest = test;
	s_depthWriteEnabled = writeEnabled;
	
	// depth state
	
	MTLDepthStencilDescriptor * descriptor = [[MTLDepthStencilDescriptor alloc] init];
	
	if (enabled)
	{
		switch (test)
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
		
		descriptor.depthWriteEnabled = writeEnabled;
	}
	else
	{
		descriptor.depthCompareFunction = MTLCompareFunctionAlways;
		descriptor.depthWriteEnabled = false;
	}
	
	id <MTLDepthStencilState> state = [device newDepthStencilStateWithDescriptor:descriptor];
	
	[activeWindowData->encoder setDepthStencilState:state];
	
	[state release];
	state = nullptr;
	
	[descriptor release];
	descriptor = nullptr;
}

void pushDepthTest(bool enabled, DEPTH_TEST test, bool writeEnabled)
{
	const DepthTestInfo info =
	{
		globals.depthTestEnabled,
		globals.depthTest,
		globals.depthTestWriteEnabled
	};
	
	depthTestStack.push(info);
	
	setDepthTest(enabled, test, writeEnabled);
}

void popDepthTest()
{
	const DepthTestInfo depthTestInfo = depthTestStack.popValue();
	
	setDepthTest(depthTestInfo.testEnabled, depthTestInfo.test, depthTestInfo.writeEnabled);
}

void pushDepthWrite(bool enabled)
{
	pushDepthTest(globals.depthTestEnabled, globals.depthTest, enabled);
}

void popDepthWrite()
{
	popDepthTest();
}

void setCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding)
{
	const MTLCullMode metalCullMode =
		mode == CULL_NONE ? MTLCullModeNone :
		mode == CULL_FRONT ? MTLCullModeFront :
		MTLCullModeBack;
	
	[activeWindowData->encoder setCullMode:metalCullMode];
	
	const MTLWinding metalFrontFaceWinding =
		frontFaceWinding == CULL_CCW ? MTLWindingCounterClockwise :
		MTLWindingClockwise;
	
	[activeWindowData->encoder setFrontFacingWinding:metalFrontFaceWinding];
}

void pushCullMode(CULL_MODE mode, CULL_WINDING frontFaceWinding)
{
	const CullModeInfo info =
	{
		globals.cullMode,
		globals.cullWinding
	};
	
	cullModeStack.push(info);
	
	setCullMode(mode, frontFaceWinding);
}

void popCullMode()
{
	const CullModeInfo cullMode = cullModeStack.popValue();
	
	setCullMode(cullMode.mode, cullMode.winding);
}

// -- gpu resources --

static GxTextureId createTexture(
	const void * source, const int sx, const int sy, const int bytesPerPixel, const int sourcePitch,
	const bool filter, const bool clamp,
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

GxTextureId createTextureFromR8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 1, 0, filter, clamp, MTLPixelFormatR8Unorm);
}

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 4, 0, filter, clamp, MTLPixelFormatRGBA8Unorm);
}

// --- internal texture creation functions ---
GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, int sourcePitch, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 4, sourcePitch, filter, clamp, MTLPixelFormatRGBA8Unorm);
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
			
			s_resourcesToFree.push_back(texture);
			
			s_textures.erase(i);
		}
		
		textureId = 0;
	}
}

// -- gx api implementation --

#include "Mat4x4.h"
#include "Quat.h"

#define TODO 0

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
	if (s_gxMatrixStack == &s_gxProjection)
		return GX_PROJECTION;
	else
	{
		Assert(false);
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
	Mat4x4 m;
	m.MakeTranslation(x, y, z);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxRotatef(float angle, float x, float y, float z)
{
	Quat q;
	q.fromAxisAngle(Vec3(x, y, z), angle * M_PI / 180.f);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * q.toMatrix();
}

void gxScalef(float x, float y, float z)
{
	Mat4x4 m;
	m.MakeScaling(x, y, z);
	
	s_gxMatrixStack->getRw() = s_gxMatrixStack->get() * m;
}

void gxValidateMatrices()
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);

	//printf("validate1\n");
	
	if (globals.shader && globals.shader->getType() == SHADER_VSPS)
	{
		Shader * shader = static_cast<Shader*>(globals.shader);
		
		const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader->getCacheElem());
	
		if (shaderElem.vsInfo.uniformBufferIndex != -1)
		{
			uint8_t * data = (uint8_t*)shader->m_cacheElem->vsUniformData;
		
			// check if matrices are dirty
			
			if ((globals.gxShaderIsDirty || s_gxModelView.isDirty) && shaderElem.vsInfo.params[ShaderCacheElem::kSp_ModelViewMatrix].offset >= 0)
			{
				Mat4x4 * dst = (Mat4x4*)(data + shaderElem.vsInfo.params[ShaderCacheElem::kSp_ModelViewMatrix].offset);
				*dst = s_gxModelView.get();
				//printf("validate2\n");
			}
			if ((globals.gxShaderIsDirty || s_gxModelView.isDirty || s_gxProjection.isDirty) && shaderElem.vsInfo.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].offset >= 0)
			{
				Mat4x4 * dst = (Mat4x4*)(data + shaderElem.vsInfo.params[ShaderCacheElem::kSp_ModelViewProjectionMatrix].offset);
				*dst = s_gxProjection.get() * s_gxModelView.get();
				//printf("validate3\n");
			}
			if ((globals.gxShaderIsDirty || s_gxProjection.isDirty) && shaderElem.vsInfo.params[ShaderCacheElem::kSp_ProjectionMatrix].offset >= 0)
			{
				Mat4x4 * dst = (Mat4x4*)(data + shaderElem.vsInfo.params[ShaderCacheElem::kSp_ProjectionMatrix].offset);
				*dst = s_gxProjection.get();
				//printf("validate4\n");
			}
			
			[activeWindowData->encoder setVertexBytes:data length:shaderElem.vsInfo.uniformBufferSize atIndex:shaderElem.vsInfo.uniformBufferIndex];
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

static Shader s_gxShader;

static GxVertex s_gxVertexBuffer[1024*64];

static GX_PRIMITIVE_TYPE s_gxPrimitiveType = GX_INVALID_PRIM;
static GxVertex * s_gxVertices = nullptr;
static int s_gxVertexCount = 0;
static int s_gxMaxVertexCount = 0;
static int s_gxPrimitiveSize = 0;
static GxVertex s_gxVertex = { };
static GxTextureId s_gxTexture = 0;
static bool s_gxTextureEnabled = false;

static GX_PRIMITIVE_TYPE s_gxLastPrimitiveType = GX_INVALID_PRIM;
static int s_gxLastVertexCount = -1;

static DynamicBufferPool s_gxVertexBufferPool;
static DynamicBufferPool::PoolElem * s_gxVertexBufferElem = nullptr;
static int s_gxVertexBufferElemOffset = 0;
static GxIndexBuffer s_gxIndexBuffer;

static const GxVertexInput s_gxVsInputs[] =
{
	{ VS_POSITION,  4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, px), 0 },
	{ VS_NORMAL,    3, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, nx), 0 },
	{ VS_COLOR,     4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, cx), 0 },
	{ VS_TEXCOORD0, 2, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, tx), 0 },
	{ VS_TEXCOORD1, 2, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, tx), 0 }, // fixme : remove ? needed to make shader compiler happy, even though not referenced, only declared
	{ VS_BLEND_INDICES, 4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, px), 0 }, // fixme : remove ? needed to make shader compiler happy, even though not referenced, only declared
	{ VS_BLEND_WEIGHTS, 4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, px), 0 } // fixme : remove ? needed to make shader compiler happy, even though not referenced, only declared
};

static float scale255(const float v)
{
	static const float m = 1.f / 255.f;
	return v * m;
}

void gxValidateShaderResources();

void gxInitialize()
{
	fassert(s_shaderSources.empty());
	
	bindVsInputs(s_gxVsInputs, sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]), sizeof(GxVertex));
	
	registerBuiltinShaders();

	s_gxShader.load("engine/Generic", "engine/Generic.vs", "engine/Generic.ps");

	memset(&s_gxVertex, 0, sizeof(s_gxVertex));
	s_gxVertex.cx = 1.f;
	s_gxVertex.cy = 1.f;
	s_gxVertex.cz = 1.f;
	s_gxVertex.cw = 1.f;

	s_gxVertexBufferPool.init(sizeof(s_gxVertexBuffer));

	const int maxVertexCount = sizeof(s_gxVertexBuffer) / sizeof(s_gxVertexBuffer[0]);
	const int maxQuads = maxVertexCount / 4;
	const int maxIndicesForQuads = maxQuads * 6;
	
	s_gxIndexBuffer.alloc(maxIndicesForQuads, sizeof(INDEX_TYPE) == 2 ? GX_INDEX_16 : GX_INDEX_32);

	bindVsInputs(s_gxVsInputs, sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]), sizeof(GxVertex));
	
#if TODO
	// enable seamless cube map sampling along the edges
	
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif
}

void gxShutdown()
{
	s_gxVertexBufferPool.free();
	
	s_gxIndexBuffer.free();

#if TODO
	glDisable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
#endif

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
		fassert(false);
		return MTLPrimitiveTypeLine; // fixme !
	case GX_LINE_STRIP:
		return MTLPrimitiveTypeLineStrip;
	case GX_TRIANGLES:
		return MTLPrimitiveTypeTriangle;
	case GX_TRIANGLE_FAN:
		return MTLPrimitiveTypeTriangle;
	case GX_TRIANGLE_STRIP:
		return MTLPrimitiveTypeTriangleStrip;
	case GX_QUADS:
		fassert(false);
		return MTLPrimitiveTypeTriangle; // fixme !
	default:
		Assert(false);
		return (MTLPrimitiveType)-1;
	}
}

#define FNV_Offset32 2166136261
#define FNV_Prime32 16777619

static uint32_t computeHash(const void* bytes, int byteCount)
{
	uint32_t hash = FNV_Offset32;
	
#if 0 // todo : profile and use optimized hash function
	const int wordCount = byteCount / 4;
	
	for (int i = 0; i < wordCount; ++i)
	{
		hash = hash ^ ((uint32_t*)bytes)[i];
		hash = hash * FNV_Prime32;
	}
	
	for (int i = wordCount * 4; i < byteCount; ++i)
	{
		hash = hash ^ ((uint8_t*)bytes)[i];
		hash = hash * FNV_Prime32;
	}
#else
	for (int i = 0; i < byteCount; ++i)
	{
		hash = hash ^ ((uint8_t*)bytes)[i];
		hash = hash * FNV_Prime32;
	}
#endif
	
	return hash;
}

static id <MTLRenderPipelineState> s_currentRenderPipelineState = nullptr;

static void gxValidatePipelineState()
{
	if (globals.shader == nullptr || globals.shader->getType() != SHADER_VSPS)
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
	- vs/ps function (shader)
	*/
	
	const uint32_t hash = computeHash(&renderState, sizeof(renderState));
	
	id <MTLRenderPipelineState> pipelineState = shaderElem.findPipelineState(hash);

	if (pipelineState == nullptr)
	{
		@autoreleasepool
		{
			id <MTLFunction> vs = (id <MTLFunction>)shaderElem.vs;
			id <MTLFunction> ps = (id <MTLFunction>)shaderElem.ps;
		
			MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
		
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
				
				Assert(metalFormat != MTLVertexFormatInvalid);
				if (metalFormat != MTLVertexFormatInvalid)
				{
					a.format = metalFormat;
					a.offset = e.offset;
					a.bufferIndex = 0;
				}
			}
			
			vertexDescriptor.layouts[0].stride = renderState.vertexStride;
			vertexDescriptor.layouts[0].stepRate = 1;
			vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

			MTLRenderPipelineDescriptor * pipelineDescriptor = [[MTLRenderPipelineDescriptor new] autorelease];
			pipelineDescriptor.label = [NSString stringWithCString:shaderElem.name.c_str() encoding:NSASCIIStringEncoding];
			pipelineDescriptor.sampleCount = 1;
			pipelineDescriptor.vertexFunction = vs;
			pipelineDescriptor.fragmentFunction = ps;
			pipelineDescriptor.vertexDescriptor = vertexDescriptor;
			
			pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
			
			auto * att = pipelineDescriptor.colorAttachments[0];
			
			// blend state
			
			switch (renderState.blendMode)
			{
			case BLEND_OPAQUE:
				att.blendingEnabled = false;
				break;
			case BLEND_ALPHA:
				att.blendingEnabled = true;
				att.rgbBlendOperation = MTLBlendOperationAdd;
				att.alphaBlendOperation = MTLBlendOperationAdd;
				att.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
				att.sourceAlphaBlendFactor = MTLBlendFactorSourceAlpha;
				att.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
				att.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
				break;
			case BLEND_PREMULTIPLIED_ALPHA:
				att.blendingEnabled = true;
				att.rgbBlendOperation = MTLBlendOperationAdd;
				att.alphaBlendOperation = MTLBlendOperationAdd;
				att.sourceRGBBlendFactor = MTLBlendFactorOne;
				att.sourceAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
				att.destinationRGBBlendFactor = MTLBlendFactorOne;
				att.destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
				break;
			case BLEND_PREMULTIPLIED_ALPHA_DRAW:
			// todo : remove ?
				att.blendingEnabled = true;
				att.rgbBlendOperation = MTLBlendOperationAdd;
				att.alphaBlendOperation = MTLBlendOperationAdd;
				att.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
				att.sourceAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
				att.destinationRGBBlendFactor = MTLBlendFactorOne;
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
			default:
				fassert(false);
				break;
			}
			
			pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
			//pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;

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
		
		[activeWindowData->encoder setRenderPipelineState:pipelineState];
	}
}

static void gxFlush(bool endOfBatch)
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);

	if (s_gxVertexCount)
	{
		const GX_PRIMITIVE_TYPE primitiveType = s_gxPrimitiveType;

	// todo : refactor s_gxVertices to use a GxVertexBuffer object
	
		const int vertexDataSize = s_gxVertexCount * sizeof(GxVertex);
		
		if (vertexDataSize <= 4096)
		{
			// optimize using setVertexBytes when the draw call is small
			[activeWindowData->encoder setVertexBytes:s_gxVertices length:vertexDataSize atIndex:0];
		}
		else
		{
		// todo : keep a reference to the current buffer and allocate vertices from the same buffer
		//        when possible. once the buffer is depleted, or when the command buffer is scheduled,
		//        add the completion handler
			const int remaining = (s_gxVertexBufferElem == nullptr) ? 0 : (s_gxVertexBufferPool.m_numBytesPerBuffer - s_gxVertexBufferElemOffset);
			
			if (vertexDataSize > remaining)
			{
				if (s_gxVertexBufferElem != nullptr)
				{
					auto * elem = s_gxVertexBufferElem;
					[activeWindowData->cmdbuf setLabel:@"GxBufferPool Release (gxFlush)"];
					[activeWindowData->cmdbuf addCompletedHandler:
						^(id<MTLCommandBuffer> _Nonnull)
						{
							s_gxVertexBufferPool.freeBuffer(elem);
						}];
				}
				
				s_gxVertexBufferElem = s_gxVertexBufferPool.allocBuffer();
				s_gxVertexBufferElemOffset = 0;
			}
			
			memcpy((uint8_t*)s_gxVertexBufferElem->m_buffer.contents + s_gxVertexBufferElemOffset, s_gxVertices, vertexDataSize);
			[activeWindowData->encoder setVertexBuffer:s_gxVertexBufferElem->m_buffer offset:s_gxVertexBufferElemOffset atIndex:0];
			
			s_gxVertexBufferElemOffset += vertexDataSize;
		}
		
		bindVsInputs(s_gxVsInputs, sizeof(s_gxVsInputs) / sizeof(s_gxVsInputs[0]), sizeof(GxVertex));
		
		Shader genericShader("engine/Generic");
		
		Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : genericShader;
	
		setShader(shader);
	
		gxValidatePipelineState();
		
		gxValidateMatrices();
		
		gxValidateShaderResources();
		
		bool indexed = false;
		uint32_t * indices = 0;
		int numElements = s_gxVertexCount;
		int numIndices = 0;

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
			fassert(s_gxVertexCount < 65536);
			
			const int numQuads = s_gxVertexCount / 4;
			numIndices = numQuads * 6;

			if (needToRegenerateIndexBuffer)
			{
				indices = (INDEX_TYPE*)s_gxIndexBuffer.updateBegin();

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
				
				s_gxIndexBuffer.updateEnd(0, indexPtr - indices);
			}
			
			s_gxPrimitiveType = GX_TRIANGLES;
			numElements = numIndices;
			
			indexed = true;
		}
		
		// convert triangle fan to triangles
		
		if (s_gxPrimitiveType == GX_TRIANGLE_FAN)
		{
			fassert(s_gxVertexCount < 65536);
			
			const int numTriangles = s_gxVertexCount - 2;
			numIndices = numTriangles * 3;

			if (needToRegenerateIndexBuffer)
			{
				indices = (INDEX_TYPE*)s_gxIndexBuffer.updateBegin();

				INDEX_TYPE * __restrict indexPtr = indices;
				INDEX_TYPE baseIndex = 0;
			
				for (int i = 0; i < numTriangles; ++i)
				{
					*indexPtr++ = 0;
					*indexPtr++ = baseIndex + 1;
					*indexPtr++ = baseIndex + 2;
				
					baseIndex += 1;
				}
				
				s_gxIndexBuffer.updateEnd(0, indexPtr - indices);
			}
			
			s_gxPrimitiveType = GX_TRIANGLES;
			numElements = numIndices;
			
			indexed = true;
		}
		
		const ShaderCacheElem_Metal & shaderElem = static_cast<const ShaderCacheElem_Metal&>(shader.getCacheElem());
		
		uint8_t * data = (uint8_t*)shaderElem.psUniformData;
		
		if (shaderElem.psInfo.params[ShaderCacheElem::kSp_Params].offset != -1)
		{
			float * values = (float*)(data + shaderElem.psInfo.params[ShaderCacheElem::kSp_Params].offset);
			
			values[0] = s_gxTextureEnabled ? 1.f : 0.f,
			values[1] = globals.colorMode;
			values[2] = globals.colorPost;
			values[3] = globals.colorClamp;
		}

		if (globals.gxShaderIsDirty)
		{
			if (shaderElem.psInfo.params[ShaderCacheElem::kSp_Texture].offset != -1)
				shader.setTextureUnit(shaderElem.psInfo.params[ShaderCacheElem::kSp_Texture].offset, 0);
		}
	
		[activeWindowData->encoder setFragmentBytes:shader.m_cacheElem->psUniformData length:shaderElem.psInfo.uniformBufferSize atIndex:shaderElem.psInfo.uniformBufferIndex];
		
		if (shader.isValid())
		{
			const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(s_gxPrimitiveType);

			if (indexed)
			{
				id <MTLBuffer> buffer = (id <MTLBuffer>)s_gxIndexBuffer.getMetalBuffer();
				
				[activeWindowData->encoder drawIndexedPrimitives:metalPrimitiveType indexCount:numElements indexType:MTLIndexTypeUInt32 indexBuffer:buffer indexBufferOffset:0];
			}
			else
			{
				[activeWindowData->encoder drawPrimitives:metalPrimitiveType vertexStart:0 vertexCount:numElements];
			}
		}
		else
		{
			logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
		}
	
		if (endOfBatch)
		{
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
		
		if (&shader == &genericShader)
		{
			clearShader(); // todo : remove. here since Shader dtor doesn't clear globals.shader yet when it's the current shader
		}
		
		globals.gxShaderIsDirty = false;

		s_gxPrimitiveType = primitiveType;
	}
	
	if (endOfBatch)
		s_gxVertices = 0;
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
			s_gxPrimitiveSize = 1;
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
		default:
			fassert(false);
	}
	
	fassert(s_gxVertexCount == 0);
}

void gxEnd()
{
	gxFlush(true);
}

static void gxEndDraw()
{
	// add completion handler if there's still a buffer pool element in use
	
	if (s_gxVertexBufferElem != nullptr)
	{
		auto * elem = s_gxVertexBufferElem;
		[activeWindowData->cmdbuf setLabel:@"GxBufferPool Release (gxEndDraw)"];
		[activeWindowData->cmdbuf addCompletedHandler:
			^(id<MTLCommandBuffer> _Nonnull)
			{
				s_gxVertexBufferPool.freeBuffer(elem);
			}];
		
		s_gxVertexBufferElem = nullptr;
		s_gxVertexBufferElemOffset = 0;
	}
	
	// clear textures to avoid freed textures from being reused (prefer to crash instead)
	
	[activeWindowData->cmdbuf setLabel:@"Clear textures (gxEndDraw)"];
	for (int i = 0; i < ShaderCacheElem_Metal::kMaxVsTextures; ++i)
		[activeWindowData->encoder setVertexTexture:nullptr atIndex:i];
	for (int i = 0; i < ShaderCacheElem_Metal::kMaxPsTextures; ++i)
		[activeWindowData->encoder setFragmentTexture:nullptr atIndex:i];
	
	// reset the current pipeline state, to ensure we set it again when recording the next command buffer
	
	s_currentRenderPipelineState = nullptr;
}

void gxEmitVertices(int primitiveType, int numVertices)
{
	Shader genericShader("engine/Generic");
	
	Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : genericShader;

	setShader(shader);

	gxValidatePipelineState();
	
	gxValidateMatrices();
	
	gxValidateShaderResources();

#if TODO
	//

	const int vaoIndex = 0;
	glBindVertexArray(s_gxVertexArrayObject[vaoIndex]);
	checkErrorGL();

	//

	const ShaderCacheElem & shaderElem = shader.getCacheElem();

	if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
	{
		shader.setImmediate(
			shaderElem.params[ShaderCacheElem::kSp_Params].index,
			s_gxTextureEnabled ? 1 : 0,
			globals.colorMode,
			globals.colorPost,
			0);
	}

	if (globals.gxShaderIsDirty)
	{
		if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
			shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
	}

	//

	glDrawArrays(primitiveType, 0, numVertices);
	checkErrorGL();

	globals.gxShaderIsDirty = false;
#endif
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
	Assert(false); // todo
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
	bindVsInputs(vsInputs, numVsInputs, vsStride);
	
	id <MTLBuffer> metalBuffer = (id <MTLBuffer>)buffer->getMetalBuffer();
	[activeWindowData->encoder setVertexBuffer:metalBuffer offset:0 atIndex:0];
}

//

void gxValidateShaderResources()
{
	if (s_gxTextureEnabled)
	{
	// todo : avoid setting textures when not needed
	//        needed when: texture changed
	//                  or shader changed
	
		auto i = s_textures.find(s_gxTexture);
		
		Assert(i != s_textures.end());
		if (i != s_textures.end())
		{
			auto & texture = i->second;
			[activeWindowData->encoder setFragmentTexture:texture atIndex:0];
			
		#if 0 // todo : add sampler state cache and implement gxSetTextureSampler
			@autoreleasepool
			{
				auto * descriptor = [[MTLSamplerDescriptor new] autorelease];
			#if 1
				descriptor.minFilter = MTLSamplerMinMagFilterNearest;
				descriptor.magFilter = MTLSamplerMinMagFilterNearest;
				descriptor.mipFilter = MTLSamplerMipFilterNotMipmapped;
			#else
				descriptor.minFilter = MTLSamplerMinMagFilterLinear;
				descriptor.magFilter = MTLSamplerMinMagFilterLinear;
				descriptor.mipFilter = MTLSamplerMipFilterLinear;
			#endif
				descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
				descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
				auto samplerState = [[device newSamplerStateWithDescriptor:descriptor] autorelease];
				[activeWindowData->encoder setFragmentSamplerState:samplerState atIndex:0];
			}
		#endif
		}
	}
	else
	{
		auto * shader = static_cast<Shader*>(globals.shader);
		auto & cacheElem = static_cast<const ShaderCacheElem_Metal&>(shader->getCacheElem());
		
		for (int i = 0; i < ShaderCacheElem_Metal::kMaxVsTextures; ++i)
			if (cacheElem.vsTextures[i] != nullptr)
				[activeWindowData->encoder setVertexTexture:cacheElem.vsTextures[i] atIndex:i];
		
		for (int i = 0; i < ShaderCacheElem_Metal::kMaxPsTextures; ++i)
			if (cacheElem.psTextures[i] != nullptr)
				[activeWindowData->encoder setFragmentTexture:cacheElem.psTextures[i] atIndex:i];
	}
}

#endif
