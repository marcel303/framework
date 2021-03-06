#import "bufferPool.h"
#import "metal.h"
#import "metalView.h"
#import "shader.h"
#import "window_data.h"
#import <Cocoa/Cocoa.h>
#import <map>
#import <Metal/Metal.h>
#import <SDL2/SDL_syswm.h>
#import <SDL2/SDL.h>
#import <vector>

#include <assert.h>
#define fassert assert
#define Assert fassert

#define INDEX_TYPE uint32_t

#define FETCH_PIPELINESTATE_REFLECTION 0

#if 1
	#define VS_POSITION      0
	#define VS_NORMAL        1
	#define VS_COLOR         2
	#define VS_TEXCOORD0     3
	#define VS_TEXCOORD      VS_TEXCOORD0
	#define VS_TEXCOORD1     4
	#define VS_BLEND_INDICES 5
	#define VS_BLEND_WEIGHTS 6
#endif

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs, const int vsStride);

static id <MTLDevice> device = nullptr;

static id <MTLCommandQueue> queue = nullptr;

struct
{
	bool gxShaderIsDirty = true;
	Shader * shader = nullptr;
} globals;

static std::map<SDL_Window*, WindowData*> windowDatas;

WindowData * activeWindowData = nullptr;

#if FETCH_PIPELINESTATE_REFLECTION
static MTLRenderPipelineReflection * activeRenderPipelineReflection = nullptr;
#endif

static std::vector<id <MTLResource>> s_resourcesToFree;

struct RenderPassData
{
	id <MTLCommandBuffer> cmdbuf;
	
	MTLRenderPassDescriptor * renderdesc = nullptr;
	
	id <MTLRenderCommandEncoder> encoder;
	
	RenderPipelineState::RenderPass renderPass;
};

static std::vector<RenderPassData> s_renderPasses;

static RenderPassData * s_activeRenderPass = nullptr;

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
	
	queue = [device newCommandQueue];
}

void metal_attach(SDL_Window * window)
{
	@autoreleasepool
	{
		SDL_SysWMinfo info;
		SDL_VERSION(&info.version);
		SDL_GetWindowWMInfo(window, &info);
		
		NSView * sdl_view = info.info.cocoa.window.contentView;

		WindowData * windowData = new WindowData();
		windowData->metalview = [[MetalView alloc] initWithFrame:sdl_view.frame device:device wantsDepthBuffer:YES];
		[sdl_view addSubview:windowData->metalview];
		
		windowDatas[window] = windowData;
	}
}

void metal_insert_capture_boundary()
{
	[queue insertDebugCaptureBoundary];
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

void metal_draw_begin(const float r, const float g, const float b, const float a, const float depth)
{
	@autoreleasepool
	{
		RenderPassData pd;
		
		pd.cmdbuf = [[queue commandBuffer] retain];

int t1 = SDL_GetTicks();
		activeWindowData->current_drawable = [activeWindowData->metalview.metalLayer nextDrawable];
		[activeWindowData->current_drawable retain];
int t2 = SDL_GetTicks();
	printf("wait: %dms\n", t2 - t1);
		
		pd.renderdesc = [MTLRenderPassDescriptor renderPassDescriptor];
		[pd.renderdesc retain];
		
		// specify the color and depth attachment(s)
		
		MTLRenderPassColorAttachmentDescriptor * colorattachment = pd.renderdesc.colorAttachments[0];
		colorattachment.texture = activeWindowData->current_drawable.texture;
		
		colorattachment.clearColor  = MTLClearColorMake(r, g, b, a);
		colorattachment.loadAction  = MTLLoadActionClear;
		colorattachment.storeAction = MTLStoreActionStore;
		
		pd.renderPass.colorFormat[0] = colorattachment.texture.pixelFormat;
		
		if (activeWindowData->metalview.depthTexture != nullptr)
		{
			MTLRenderPassDepthAttachmentDescriptor * depthattachment = pd.renderdesc.depthAttachment;
			depthattachment.texture = activeWindowData->metalview.depthTexture;
			depthattachment.clearDepth = depth;
			depthattachment.loadAction = MTLLoadActionClear;
			depthattachment.storeAction = MTLStoreActionDontCare;
			
			pd.renderPass.depthFormat = depthattachment.texture.pixelFormat;
		}
		
		// begin encoding
		
		pd.encoder = [[pd.cmdbuf renderCommandEncoderWithDescriptor:pd.renderdesc] retain];
		pd.encoder.label = @"hello encoder";
		
		s_renderPasses.push_back(pd);
		
		s_activeRenderPass = &s_renderPasses.back();
		
		renderState.renderPass = s_activeRenderPass->renderPass;
		
		// set viewport
		
		const CGSize size = activeWindowData->metalview.frame.size;
		metal_set_viewport(size.width, size.height);
	}
}

void metal_draw_end()
{
#if FETCH_PIPELINESTATE_REFLECTION
	//NSLog(@"activeRenderPipelineReflection release, retain count: %lu", [activeRenderPipelineReflection retainCount]);
	[activeRenderPipelineReflection release];
	activeRenderPipelineReflection = nullptr;
#endif

	auto & pd = s_renderPasses.back();
	
	[pd.encoder endEncoding];
	
	[pd.cmdbuf addCompletedHandler:
		^(id<MTLCommandBuffer> _Nonnull)
		{
			//NSLog(@"hello done! %@", activeWindowData);
		}];

	//[pd.cmdbuf presentDrawable:activeWindowData->current_drawable];
	[pd.cmdbuf commit];
	[activeWindowData->current_drawable present];
	
	
	//[activeWindowData->cmdbuf waitUntilCompleted];
	freeResourcesToFree(); // todo # call in response to completion handler
	
	//
	
	//NSLog(@"encoder release, retain count: %lu", [s_activeRenderPass->encoder retainCount]);
	[pd.encoder release];
	//NSLog(@"current_drawable release, retain count: %lu", [activeWindowData->current_drawable retainCount]);
	[activeWindowData->current_drawable release];
	//NSLog(@"cmdbuf release, retain count: %lu", [s_activeRenderPass->cmdbuf retainCount]);
	[pd.cmdbuf release];
	
	pd.encoder = nullptr;
	activeWindowData->current_drawable = nullptr;
	pd.cmdbuf = nullptr;
	
	s_renderPasses.pop_back();
	
	if (s_renderPasses.empty())
	{
		s_activeRenderPass = nullptr;
		
		renderState.renderPass = RenderPipelineState::RenderPass();
	}
	else
	{
		s_activeRenderPass = &s_renderPasses.back();
		
		renderState.renderPass = s_activeRenderPass->renderPass;
	}
}

void metal_set_viewport(const int sx, const int sy)
{
	[s_activeRenderPass->encoder setViewport:(MTLViewport){ 0, 0, (double)sx, (double)sy, 0.0, 1.0 }];
}

id <MTLDevice> metal_get_device()
{
	return device;
}

// -- render states --

RenderPipelineState renderState;

// render states independent from render pipeline state

static bool s_depthTestEnabled = false;
static DEPTH_TEST s_depthTest = DEPTH_ALWAYS;
static bool s_depthWriteEnabled = false;

#include "renderTarget.h"

void pushRenderPass(ColorTarget * target, DepthTarget * depthTarget, const bool clearDepth, const char * passName)
{
	pushRenderPass(&target, target == nullptr ? 0 : 1, depthTarget, clearDepth, passName);
}

void pushRenderPass(ColorTarget ** targets, const int numTargets, DepthTarget * depthTarget, const bool clearDepth, const char * passName)
{
	Assert(numTargets >= 0 && numTargets <= 4);
	
	RenderPassData pd;
	
	@autoreleasepool
	{
		pd.cmdbuf = [[queue commandBuffer] retain];

	 	pd.renderdesc = [[MTLRenderPassDescriptor renderPassDescriptor] retain];
		
		int viewportSx = 0;
		int viewportSy = 0;
		
		// specify the color and depth attachment(s)
		
		for (int i = 0; i < numTargets; ++i)
		{
			MTLRenderPassColorAttachmentDescriptor * colorattachment = pd.renderdesc.colorAttachments[i];
			colorattachment.texture = (id <MTLTexture>)targets[i]->getMetalTexture();
			
			const Color & clearColor = targets[i]->getClearColor();
			
			colorattachment.clearColor  = MTLClearColorMake(
				clearColor.r,
				clearColor.g,
				clearColor.b,
				clearColor.a);
			colorattachment.loadAction  = MTLLoadActionClear;
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
			depthattachment.loadAction = MTLLoadActionClear;
			depthattachment.storeAction = MTLStoreActionDontCare;
			
			pd.renderPass.depthFormat = depthattachment.texture.pixelFormat;
			
			if (depthattachment.texture.width > viewportSx)
				viewportSx = depthattachment.texture.width;
			if (depthattachment.texture.height > viewportSy)
				viewportSy = depthattachment.texture.height;
		}
		
		// begin encoding
		
		pd.encoder = [[pd.cmdbuf renderCommandEncoderWithDescriptor:pd.renderdesc] retain];
		pd.encoder.label = [NSString stringWithCString:passName encoding:NSASCIIStringEncoding];
		
		s_renderPasses.push_back(pd);
		
		renderState.renderPass = pd.renderPass;
		
		s_activeRenderPass = &s_renderPasses.back();
		
		// set viewport
		
		metal_set_viewport(viewportSx, viewportSy);
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
	
	if (s_renderPasses.empty())
	{
		s_activeRenderPass = nullptr;
		
		renderState.renderPass = RenderPipelineState::RenderPass();
	}
	else
	{
		s_activeRenderPass = &s_renderPasses.back();
		
		renderState.renderPass = s_activeRenderPass->renderPass;
	}
}

void setBlend(BLEND_MODE blendMode)
{
	renderState.blendMode = blendMode;
}

void setLineSmooth(bool enabled)
{
	//Assert(false);
}

void setWireframe(bool enabled)
{
	[s_activeRenderPass->encoder setTriangleFillMode:enabled ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
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

void setShader(Shader & shader)
{
	globals.shader = &shader;
}

void clearShader()
{
	globals.shader = nullptr;
}

// -- gpu resources --

static std::map<int, id <MTLTexture>> s_textures;
static int s_nextTextureId = 1;

static GxTextureId createTexture(
	const void * source, const int sx, const int sy, const int bytesPerPixel,
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
			const int pitch = sx * bytesPerPixel;
			
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
	return createTexture(source, sx, sy, 1, filter, clamp, MTLPixelFormatR8Unorm);
}

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	return createTexture(source, sx, sy, 4, filter, clamp, MTLPixelFormatRGBA8Unorm);
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
		
		const ShaderCacheElem & shaderElem = shader->getCacheElem();
	
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
			
			[s_activeRenderPass->encoder setVertexBytes:data length:shaderElem.vsInfo.uniformBufferSize atIndex:shaderElem.vsInfo.uniformBufferIndex];
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

static GX_PRIMITIVE_TYPE s_gxLastPrimitiveType = GX_INVALID_PRIM;
static int s_gxLastVertexCount = -1;

static DynamicBufferPool s_gxVertexBufferPool;
static GxIndexBuffer s_gxIndexBuffer;

static const GxVertexInput s_gxVsInputs[] =
{
	{ VS_POSITION, 4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, px), 0 },
	{ VS_NORMAL,   3, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, nx), 0 },
	{ VS_COLOR,    4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, cx), 0 },
	{ VS_TEXCOORD, 2, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, tx), 0 }
};

static float scale255(const float v)
{
	static const float m = 1.f / 255.f;
	return v * m;
}

void gxEmitVertex();
void gxValidateShaderResources();

void gxInitialize()
{
#if TODO
	fassert(s_shaderSources.empty());
	
	registerBuiltinShaders();

	s_gxShader.load("engine/Generic", "engine/Generic.vs", "engine/Generic.ps");
#endif

	memset(&s_gxVertex, 0, sizeof(s_gxVertex));
	s_gxVertex.cx = 1.f;
	s_gxVertex.cy = 1.f;
	s_gxVertex.cz = 1.f;
	s_gxVertex.cw = 1.f;

	s_gxVertexBufferPool.init(sizeof(s_gxVertexBuffer));

	const int maxVertexCount = sizeof(s_gxVertexBuffer) / sizeof(s_gxVertexBuffer[0]);
	const int maxQuads = maxVertexCount / 4;
	const int maxIndicesForQuads = maxQuads * 6;
	
	s_gxIndexBuffer.init(maxIndicesForQuads, sizeof(INDEX_TYPE) == 2 ? GX_INDEX_16 : GX_INDEX_32);

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
		return MTLPrimitiveTypeTriangleStrip; // fixme !
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
	
#if 0 // todo # profile and use optimized hash function
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

static void gxValidatePipelineState()
{
	Shader shader("test");
	
	auto & shaderElem = shader.getCacheElem();
	
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
				}
			}
			
			vertexDescriptor.layouts[0].stride = renderState.vertexStride;
			vertexDescriptor.layouts[0].stepRate = 1;
			vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

			MTLRenderPipelineDescriptor * pipelineDescriptor = [[MTLRenderPipelineDescriptor new] autorelease];
			pipelineDescriptor.label = @"hello pipeline";
			pipelineDescriptor.sampleCount = 1;
			pipelineDescriptor.vertexFunction = vs;
			pipelineDescriptor.fragmentFunction = ps;
			pipelineDescriptor.vertexDescriptor = vertexDescriptor;
			
			for (int i = 0; i < 4; ++i)
			{
				if (renderState.renderPass.colorFormat[i] == 0)
					continue;
				
				auto * att = pipelineDescriptor.colorAttachments[i];
				
				att.pixelFormat = (MTLPixelFormat)renderState.renderPass.colorFormat[i];
				
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
			}
			
			if (renderState.renderPass.depthFormat != 0)
			{
				pipelineDescriptor.depthAttachmentPixelFormat = (MTLPixelFormat)renderState.renderPass.depthFormat;
			}

		#if FETCH_PIPELINESTATE_REFLECTION
			[activeRenderPipelineReflection release];
			activeRenderPipelineReflection = nullptr;
		#endif
			
		#if FETCH_PIPELINESTATE_REFLECTION
			const MTLPipelineOption pipelineOptions = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
			
			NSError * error;
			pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor options:pipelineOptions reflection:&activeRenderPipelineReflection error:&error];
			if (error != nullptr)
				NSLog(@"%@", error);
			
			[activeRenderPipelineReflection retain];
		#else
			NSError * error;
			pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
			if (error != nullptr)
				NSLog(@"%@", error);
		#endif
			
			//NSLog(@"%@", pipelineState);
			
			shaderElem.addPipelineState(hash, pipelineState);
		}
	}
	
	[s_activeRenderPass->encoder setRenderPipelineState:pipelineState];
}

static void gxFlush(bool endOfBatch)
{
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);

	if (s_gxVertexCount)
	{
		const GX_PRIMITIVE_TYPE primitiveType = s_gxPrimitiveType;

		const int vertexDataSize = s_gxVertexCount * sizeof(GxVertex);
		
		if (vertexDataSize <= 4096)
		{
			// optimize using setVertexBytes when the draw call is small
			[s_activeRenderPass->encoder setVertexBytes:s_gxVertices length:vertexDataSize atIndex:0];
		}
		else
		{
			auto * elem = s_gxVertexBufferPool.allocBuffer();
			[s_activeRenderPass->cmdbuf addCompletedHandler:
				^(id<MTLCommandBuffer> _Nonnull)
				{
					s_gxVertexBufferPool.freeBuffer(elem);
				}];
			memcpy(elem->m_buffer.contents, s_gxVertices, vertexDataSize);
			[s_activeRenderPass->encoder setVertexBuffer:elem->m_buffer offset:0 atIndex:0];
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
		
		const ShaderCacheElem & shaderElem = shader.getCacheElem();
		
		uint8_t * data = (uint8_t*)shaderElem.psUniformData;
		
		if (shaderElem.psInfo.params[ShaderCacheElem::kSp_Params].offset != -1)
		{
			float * values = (float*)(data + shaderElem.psInfo.params[ShaderCacheElem::kSp_Params].offset);
			
			values[0] = s_gxTextureEnabled ? 1.f : 0.f,
			values[1] = 0;
			values[2] = 0;
			values[3] = 0;
		}

	#if TODO
		if (globals.gxShaderIsDirty)
		{
			if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
				shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
		}
	#endif
	
		[s_activeRenderPass->encoder setFragmentBytes:shader.m_cacheElem->psUniformData length:shaderElem.psInfo.uniformBufferSize atIndex:shaderElem.psInfo.uniformBufferIndex];
		
		if (shader.isValid())
		{
			const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(s_gxPrimitiveType);

			if (indexed)
			{
				id <MTLBuffer> buffer = (id <MTLBuffer>)s_gxIndexBuffer.getMetalBuffer();
				
				[s_activeRenderPass->encoder drawIndexedPrimitives:metalPrimitiveType indexCount:numElements indexType:MTLIndexTypeUInt32 indexBuffer:buffer indexBufferOffset:0];
			}
			else
			{
				[s_activeRenderPass->encoder drawPrimitives:metalPrimitiveType vertexStart:0 vertexCount:numElements];
			}
		}
	#if TODO // requires logDebug
		else
		{
			logDebug("shader %s is invalid. omitting draw call", shaderElem.name.c_str());
		}
	#endif
	
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
			clearShader();
		}
		
	#if TODO
		globals.gxShaderIsDirty = false;
	#endif

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
	[s_activeRenderPass->encoder setVertexBuffer:metalBuffer offset:0 atIndex:0];
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

//

void gxValidateShaderResources()
{
	if (s_gxTextureEnabled)
	{
		auto i = s_textures.find(s_gxTexture);
		
		Assert(i != s_textures.end());
		if (i != s_textures.end())
		{
			auto & texture = i->second;
			[s_activeRenderPass->encoder setFragmentTexture:texture atIndex:0];
		}
	}
	else
	{
		[s_activeRenderPass->encoder setFragmentTexture:nullptr atIndex:0];
	}
}
