#import "metal.h"
#import "metalView.h"
#import "shader.h"
#import "window_data.h"
#import <Cocoa/Cocoa.h>
#import <map>
#import <Metal/Metal.h>
#import <SDL2/SDL_syswm.h>
#import <vector>

#include <assert.h> // todo : remove once integrated w/ framework
#define fassert assert
#define Assert fassert

#define INDEX_TYPE uint32_t

#if 1 // todo : remove
	#define VS_POSITION      0
	#define VS_NORMAL        1
	#define VS_COLOR         2
	#define VS_TEXCOORD0     3
	#define VS_TEXCOORD      VS_TEXCOORD0
	#define VS_TEXCOORD1     4
	#define VS_BLEND_INDICES 5
	#define VS_BLEND_WEIGHTS 6
#endif

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs);

static id <MTLDevice> device;

struct
{
	bool gxShaderIsDirty = true;
	Shader * shader = nullptr;
} globals; // todo : remove

static std::map<SDL_Window*, WindowData*> windowDatas;

WindowData * activeWindowData = nullptr;

static MTLRenderPipelineReflection * activeRenderPipelineReflection = nullptr;

static Shader s_shader;

static std::vector<id <MTLResource>> s_resourcesToFree;

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

#if 1 // todo : move elsewhere

static const char * s_shaderVs = R"SHADER(

	#include <metal_stdlib>

	using namespace metal;

	struct ShaderInputs
	{
		float4 position [[attribute(0)]];
		float3 normal [[attribute(1)]];
		float4 color [[attribute(2)]];
		float2 texcoord [[attribute(3)]];
	};

	struct ShaderOutputs
	{
		float4 position [[position]];
		float4 color;
		float2 texcoord;
	};

	struct ShaderUniforms
	{
		float4x4 ProjectionMatrix;
		float4x4 ModelViewMatrix;
		float4x4 ModelViewProjectionMatrix;
	};

	#define unpackPosition() inputs.position

	vertex ShaderOutputs shader_main(
		ShaderInputs inputs [[stage_in]],
		constant ShaderUniforms & uniforms [[buffer(1)]])
	{
		ShaderOutputs outputs;
		
		outputs.position = uniforms.ModelViewProjectionMatrix * unpackPosition();
		//outputs.position = uniforms.ModelViewMatrix * inputs.position;
		//outputs.position = uniforms.ProjectionMatrix * float4(inputs.position.xy, 2, 1);
		outputs.color = inputs.color;
		outputs.texcoord = inputs.texcoord;
		
		return outputs;
	}

)SHADER";

static const char * s_shaderPs = R"SHADER(

	#include <metal_stdlib>

	using namespace metal;

	struct ShaderInputs
	{
		float4 position [[position]];
		float4 color;
		float2 texcoord;
	};

	struct ShaderUniforms
	{
		float4 params;
	};

	fragment float4 shader_main(
		ShaderInputs inputs [[stage_in]],
		constant ShaderUniforms & uniforms [[buffer(0)]],
		texture2d<float> textureResource [[texture(0)]])
	{
		float4 color = inputs.color;
		
		if (uniforms.params.x != 0.0)
		{
			constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
			
			color *= textureResource.sample(textureSampler, inputs.texcoord);
		}
		
		return color;
	}

)SHADER";

#endif

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

		WindowData * windowData = new WindowData();
		windowData->metalview = [[MetalView alloc] initWithFrame:sdl_view.frame device:device];
		[sdl_view addSubview:windowData->metalview];

		windowData->renderdesc = [MTLRenderPassDescriptor renderPassDescriptor];
		[windowData->renderdesc retain];
		
		windowData->queue = [windowData->metalview.metalLayer.device newCommandQueue];
		
		// create depth texture
		windowData->create_depth_texture_matching_metal_view();
		
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
	
	if (activeWindowData != nullptr)
	{
		if (activeWindowData->depth_texture != nullptr)
		{
			if (activeWindowData->depth_texture.width != activeWindowData->metalview.metalLayer.drawableSize.width ||
				activeWindowData->depth_texture.height != activeWindowData->metalview.metalLayer.drawableSize.height)
			{
				activeWindowData->create_depth_texture_matching_metal_view();
			}
		}
	}
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
		depthattachment.texture = activeWindowData->depth_texture;
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
	//NSLog(@"activeRenderPipelineReflection release, retain count: %lu", [activeRenderPipelineReflection retainCount]);
	[activeRenderPipelineReflection release];
	activeRenderPipelineReflection = nullptr;
	
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
	
	//NSLog(@"encoder release, retain count: %lu", [activeWindowData->encoder retainCount]);
	[activeWindowData->encoder release];
	//NSLog(@"current_drawable release, retain count: %lu", [activeWindowData->current_drawable retainCount]);
	[activeWindowData->current_drawable release];
	//NSLog(@"cmdbuf release, retain count: %lu", [activeWindowData->cmdbuf retainCount]);
	[activeWindowData->cmdbuf release];
	
	activeWindowData->encoder = nullptr;
	activeWindowData->current_drawable = nullptr;
	activeWindowData->cmdbuf = nullptr;
}

void metal_set_viewport(const int sx, const int sy)
{
	[activeWindowData->encoder setViewport:(MTLViewport){ 0, 0, (double)sx, (double)sy, 0.0, 1.0 }];
}

id <MTLDevice> metal_get_device()
{
	return device;
}

// -- render states --

struct RenderPipelineState
{
	BLEND_MODE blendMode = BLEND_ALPHA;
};

static RenderPipelineState renderState;

// render states independent from render pipeline state

static bool s_depthTestEnabled = false;
static DEPTH_TEST s_depthTest = DEPTH_ALWAYS;
static bool s_depthWriteEnabled = false;

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
	[activeWindowData->encoder setTriangleFillMode:enabled ? MTLTriangleFillModeLines : MTLTriangleFillModeFill];
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

// -- gpu resources --

static std::map<int, id <MTLTexture>> s_textures;
static int s_nextTextureId = 1;

GxTextureId createTextureFromRGBA8(const void * source, int sx, int sy, bool filter, bool clamp)
{
	@autoreleasepool
	{
		MTLTextureDescriptor * descriptor = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:sx height:sy mipmapped:NO];
		
		id <MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
		
		if (texture == nullptr)
			return 0;
		else
		{
			const int pitch = sx * 4;
			
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

void freeTexture(GxTextureId & textureId)
{
	if (textureId != 0)
	{
		// todo : defer freeing the memory
		
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
#if TODO
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);
#endif

	//printf("validate1\n");
	
#if TODO
	if (globals.shader && globals.shader->getType() == SHADER_VSPS)
#endif
	{
		Shader * shader = static_cast<Shader*>(globals.shader);
		
		const ShaderCacheElem & shaderElem = shader->getCacheElem();
	
		if (shaderElem.vsInfo.uniformBufferIndex != -1)
		{
			uint8_t * data = (uint8_t*)shader->m_cacheElem.vsUniformData;
		
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

// todo : separate low level things from these type of things needed for GX vertex/matrix/generic shader API
static GxVertexInput s_gxVertexInputs[16];
static int s_gxVertexInputCount = 0;

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

static void gxValidatePipelineState()
{
	@autoreleasepool
	{
		//MTLCompileOptions * options = [[[MTLCompileOptions alloc] init] autorelease];
		//options.fastMathEnabled = false;
		//option.preprocessorMacros;
		MTLCompileOptions * options = nullptr;

		NSError * error = nullptr;

		id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:s_shaderVs encoding:NSASCIIStringEncoding] options:options error:&error];
		if (library_vs == nullptr && error != nullptr)
			NSLog(@"%@", error);
		
		id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:s_shaderPs encoding:NSASCIIStringEncoding] options:options error:&error];
		if (library_ps == nullptr && error != nullptr)
			NSLog(@"%@", error);
		
		id <MTLFunction> vs = [library_vs newFunctionWithName:@"shader_main"];
		id <MTLFunction> ps = [library_ps newFunctionWithName:@"shader_main"];
		[vs retain]; // this fixes the occasional pipeline building crash I was seeing. not sure why this is needed
		[ps retain]; // fixme : remove these retain calls

		MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
			// position
	
		for (int i = 0; i < s_gxVertexInputCount; ++i)
		{
			auto & e = s_gxVertexInputs[i];
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
		
	// todo : stride should come from the gxSetVertexBuffer call
		vertexDescriptor.layouts[0].stride = sizeof(GxVertex);
		vertexDescriptor.layouts[0].stepRate = 1;
		vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

		MTLRenderPipelineDescriptor * pipelineDescriptor = [[MTLRenderPipelineDescriptor new] autorelease];
		pipelineDescriptor.label = @"hello pipeline";
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

		[activeRenderPipelineReflection release];
		activeRenderPipelineReflection = nullptr;
		
		const MTLPipelineOption pipelineOptions = MTLPipelineOptionBufferTypeInfo | MTLPipelineOptionArgumentInfo;
		
		id <MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor options:pipelineOptions reflection:&activeRenderPipelineReflection error:&error];
		if (error != nullptr)
			NSLog(@"%@", error);
		
		[activeRenderPipelineReflection retain];
		
	// todo : cache shader elems and pipeline states
		s_shader = Shader();
		s_shader.m_cacheElem.init(activeRenderPipelineReflection);
		globals.shader = &s_shader;
		
	#if 0 // todo : remove. test if setting immediates works
		const int params_offset = s_shader.getImmediate("params");
		s_shader.setImmediate("params", 1, 2, 3, 4);
	#endif
		
		//NSLog(@"%@", pipelineState);

		//

		[activeWindowData->encoder setRenderPipelineState:pipelineState];

		//

		[pipelineState release];

		[vs release];
		[ps release];

		[library_vs release];
		[library_ps release];
	}
}

static void gxFlush(bool endOfBatch)
{
#if TODO
	fassert(!globals.shader || globals.shader->getType() == SHADER_VSPS);
#endif

	if (s_gxVertexCount)
	{
		const GX_PRIMITIVE_TYPE primitiveType = s_gxPrimitiveType;

	#if TODO
		Shader genericShader("engine/Generic");
		
		Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : genericShader;

		setShader(shader);
	#else
		globals.shader = &s_shader; // todo : remove. just to ensure a shader is set
		
		Shader & shader = *globals.shader;
	#endif
	
		const GxVertexInput vsInputs[] =
		{
			{ VS_POSITION, 4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, px), 0 },
			{ VS_NORMAL,   3, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, nx), 0 },
			{ VS_COLOR,    4, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, cx), 0 },
			{ VS_TEXCOORD, 2, GX_ELEMENT_FLOAT32, false, offsetof(GxVertex, tx), 0 }
		};
		
	// todo : refactor s_gxVertices to use a GxVertexBuffer object
	
		const int vertexDataSize = s_gxVertexCount * sizeof(GxVertex);
		
		if (vertexDataSize <= 4096)
		{
			// optimize using setVertexBytes when the draw call is small
			[activeWindowData->encoder setVertexBytes:s_gxVertices length:vertexDataSize atIndex:0];
		}
		else
		{
			auto * elem = s_gxVertexBufferPool.allocBuffer();
			[activeWindowData->cmdbuf addCompletedHandler:
				^(id<MTLCommandBuffer> _Nonnull)
				{
					s_gxVertexBufferPool.freeBuffer(elem);
				}];
			id <MTLBuffer> buffer = (id <MTLBuffer>)elem->m_buffer;
			memcpy(buffer.contents, s_gxVertices, vertexDataSize);
			[activeWindowData->encoder setVertexBuffer:buffer offset:0 atIndex:0];
		}
		
		bindVsInputs(vsInputs, sizeof(vsInputs) / sizeof(vsInputs[0]));
	
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
			
			// todo: use triangle strip + compute index buffer once at init time
			
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
			//globals.colorMode, // todo : re-add globals.colorMode etc
			//globals.colorPost,
			//globals.colorClamp);
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
	
		[activeWindowData->encoder setFragmentBytes:shader.m_cacheElem.psUniformData length:shaderElem.psInfo.uniformBufferSize atIndex:shaderElem.psInfo.uniformBufferIndex];
		
	#if TODO
		if (shader.isValid())
	#endif
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
	#if TODO
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
#if TODO
	Shader & shader = globals.shader ? *static_cast<Shader*>(globals.shader) : s_gxShader;

	setShader(shader);
#endif

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

static void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs)
{
	const int maxVsInputs = sizeof(s_gxVertexInputs) / sizeof(s_gxVertexInputs[0]);
	Assert(numVsInputs <= maxVsInputs);
	const int numVsInputsToCopy = numVsInputs < maxVsInputs ? numVsInputs : maxVsInputs;
	memcpy(s_gxVertexInputs, vsInputs, numVsInputsToCopy * sizeof(GxVertexInput));
	s_gxVertexInputCount = numVsInputsToCopy;
}

void gxSetVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vsInputs, const int numVsInputs)
{
	bindVsInputs(vsInputs, numVsInputs);
	
	id <MTLBuffer> metalBuffer = (id <MTLBuffer>)buffer->getMetalBuffer();
	[activeWindowData->encoder setVertexBuffer:metalBuffer offset:0 atIndex:0];
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
			[activeWindowData->encoder setFragmentTexture:texture atIndex:0];
		}
	}
	else
	{
		[activeWindowData->encoder setFragmentTexture:nullptr atIndex:0];
	}
}
