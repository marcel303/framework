#import "metal.h"
#import "metalView.h"
#import <Cocoa/Cocoa.h>
#import <map>
#import <Metal/Metal.h>
#import <SDL2/SDL_syswm.h>

static id <MTLDevice> device;

struct WindowData
{
	MetalView * metalview = nullptr;
	
	MTLRenderPassDescriptor * renderdesc = nullptr;

	id <MTLCommandQueue> queue;
	
	id <MTLCommandBuffer> cmdbuf;
	
	id <CAMetalDrawable> current_drawable;
	
	id <MTLRenderCommandEncoder> encoder;
};

static std::map<SDL_Window*, WindowData*> windowDatas;

static WindowData * activeWindowData = nullptr;

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
	};

	struct ShaderUniforms
	{
		float4x4 projectionMatrix;
		float4x4 modelViewMatrix;
	};

	vertex ShaderOutputs shader_main(
		ShaderInputs inputs [[stage_in]],
		constant ShaderUniforms & uniforms [[buffer(1)]])
	{
		ShaderOutputs outputs;
		
		//outputs.position = uniforms.projectionMatrix * uniforms.modelViewMatrix * float4(inputs.position, 1);
		
		outputs.position.xyz = inputs.position.xyz * .5f;
		outputs.position.w = 1.f;
		outputs.color = inputs.color;
		
		return outputs;
	}

)SHADER";

static const char * s_shaderPs = R"SHADER(

	#include <metal_stdlib>

	struct ShaderInputs
	{
		float4 position [[position]];
		float4 color;
	};

	fragment float4 shader_main(ShaderInputs inputs [[stage_in]])
	{
		return inputs.color;
	}

)SHADER";

#endif

void metal_init()
{
	device = MTLCreateSystemDefaultDevice();
}

void metal_attach(SDL_Window * window)
{
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(window, &info);
	
	NSView * sdl_view = info.info.cocoa.window.contentView;

	WindowData * windowData = new WindowData();
	windowData->metalview = [[MetalView alloc] initWithFrame:sdl_view.frame device:device];
	[sdl_view addSubview:windowData->metalview];

	windowData->renderdesc = [MTLRenderPassDescriptor renderPassDescriptor];
	
	windowData->queue = [windowData->metalview.metalLayer.device newCommandQueue];
	
	windowDatas[window] = windowData;
}

void metal_make_active(SDL_Window * window)
{
	auto i = windowDatas.find(window);
	
	// todo : assert
	
	if (i == windowDatas.end())
		activeWindowData = nullptr;
	else
		activeWindowData = i->second;
}

void metal_draw_begin(const float r, const float g, const float b, const float a)
{
	activeWindowData->cmdbuf = [[activeWindowData->queue commandBuffer] retain];

	activeWindowData->current_drawable = [activeWindowData->metalview.metalLayer nextDrawable];

	MTLRenderPassColorAttachmentDescriptor * colorattachment = activeWindowData->renderdesc.colorAttachments[0];
	colorattachment.texture = activeWindowData->current_drawable.texture;
	
	/* Clear to a red-orange color when beginning the render pass. */
	colorattachment.clearColor  = MTLClearColorMake(r, g, b, a);
	colorattachment.loadAction  = MTLLoadActionClear;
	colorattachment.storeAction = MTLStoreActionStore;

	/* The drawable's texture is cleared to the specified color here. */
	activeWindowData->encoder = [activeWindowData->cmdbuf renderCommandEncoderWithDescriptor:activeWindowData->renderdesc];
	activeWindowData->encoder.label = @"hello encoder";
}

void metal_draw_end()
{
	[activeWindowData->encoder endEncoding];

	[activeWindowData->cmdbuf presentDrawable:activeWindowData->current_drawable];
	[activeWindowData->cmdbuf commit];
	
	[activeWindowData->cmdbuf release];
	activeWindowData->cmdbuf = nullptr;
}


struct __attribute__((packed)) RenderState
{
	int blendMode = 0;
	bool depthTestEnabled = false;
	int depthTestFunction = 0;
};

struct ShaderCache
{

};

void metal_drawtest()
{
	@autoreleasepool
	{
	#if 1
		printf("sizeof(RenderState): %d\n", sizeof(RenderState));
		
		//MTLCompileOptions * options = [[[MTLCompileOptions alloc] init] autorelease];
		//options.fastMathEnabled = false;
		//option.preprocessorMacros;
		MTLCompileOptions * options = nullptr;
		
		NSError * error = nullptr;
		
		id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:s_shaderVs encoding:NSASCIIStringEncoding] options:options error:&error];
		id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:s_shaderPs encoding:NSASCIIStringEncoding] options:options error:&error];
		
		id <MTLFunction> vs = [library_vs newFunctionWithName:@"shader_main"];
		id <MTLFunction> ps = [library_ps newFunctionWithName:@"shader_main"];
		
		MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
			// position
		vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
		vertexDescriptor.attributes[0].offset = 0;
		vertexDescriptor.attributes[0].bufferIndex = 0;
		
		vertexDescriptor.layouts[0].stride = 52;
		vertexDescriptor.layouts[0].stepRate = 1;
		vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
		
		MTLRenderPipelineDescriptor * pipelineDescriptor = [[[MTLRenderPipelineDescriptor alloc] init] autorelease];
		pipelineDescriptor.label = @"hello pipeline";
		pipelineDescriptor.sampleCount = 1;
		pipelineDescriptor.vertexFunction = vs;
		pipelineDescriptor.fragmentFunction = ps;
		pipelineDescriptor.vertexDescriptor = vertexDescriptor;
		pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
		//pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
		pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;
		
		id <MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
		
		//NSLog(@"%@", pipelineState);
		
		//
		
		struct Vertex
		{
			float x, y, z;
		};
		
		const Vertex vertices[3] =
		{
			{ -1.f, -1.f, .5f },
			{ +1.f, -1.f, .5f },
			{ -1.f, +1.f, .5f }
		};
		
		[activeWindowData->encoder setViewport:(MTLViewport){0.0, 0.0, 300, 600, 0.0, 1.0 }];

		[activeWindowData->encoder setRenderPipelineState:pipelineState];
		
		[activeWindowData->encoder setVertexBytes:vertices length:sizeof(vertices) atIndex:0];
		
		[activeWindowData->encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3];
		
		//
		
		[pipelineState release];
		
		[vs release];
		[ps release];
		
		[library_vs release];
		[library_ps release];
	#endif
	}
}

// -- gx api implementation --

#include <assert.h>
#define fassert assert
#define Assert fassert

struct GxVertex
{
	float px, py, pz, pw;
	float nx, ny, nz;
	float cx, cy, cz, cw;
	float tx, ty;
};

//static GxVertex s_gxVertexBuffer[1024*16];
static GxVertex s_gxVertexBuffer[64];

static GX_PRIMITIVE_TYPE s_gxPrimitiveType = GX_INVALID_PRIM;
static GxVertex * s_gxVertices = nullptr;
static int s_gxVertexCount = 0;
static int s_gxMaxVertexCount = 0;
static int s_gxPrimitiveSize = 0;
static GxVertex s_gxVertex = { };

static float scale255(const float v)
{
	static const float m = 1.f / 255.f;
	return v * m;
}

#define TODO 0

void gxEmitVertex();

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
		id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:s_shaderPs encoding:NSASCIIStringEncoding] options:options error:&error];

		id <MTLFunction> vs = [library_vs newFunctionWithName:@"shader_main"];
		id <MTLFunction> ps = [library_ps newFunctionWithName:@"shader_main"];

		MTLVertexDescriptor * vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
			// position
		vertexDescriptor.attributes[0].format = MTLVertexFormatFloat4;
		vertexDescriptor.attributes[0].offset = 0;
		vertexDescriptor.attributes[0].bufferIndex = 0;
		vertexDescriptor.attributes[1].format = MTLVertexFormatFloat3;
		vertexDescriptor.attributes[1].offset = 16;
		vertexDescriptor.attributes[1].bufferIndex = 0;
		vertexDescriptor.attributes[2].format = MTLVertexFormatFloat4;
		vertexDescriptor.attributes[2].offset = 28;
		vertexDescriptor.attributes[2].bufferIndex = 0;
		vertexDescriptor.attributes[3].format = MTLVertexFormatFloat2;
		vertexDescriptor.attributes[3].offset = 44;
		vertexDescriptor.attributes[3].bufferIndex = 0;

		vertexDescriptor.layouts[0].stride = sizeof(GxVertex);
		vertexDescriptor.layouts[0].stepRate = 1;
		vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;

		MTLRenderPipelineDescriptor * pipelineDescriptor = [[[MTLRenderPipelineDescriptor alloc] init] autorelease];
		pipelineDescriptor.label = @"hello pipeline";
		pipelineDescriptor.sampleCount = 1;
		pipelineDescriptor.vertexFunction = vs;
		pipelineDescriptor.fragmentFunction = ps;
		pipelineDescriptor.vertexDescriptor = vertexDescriptor;
		pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
		//pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float_Stencil8;
		pipelineDescriptor.depthAttachmentPixelFormat = MTLPixelFormatInvalid;

		id <MTLRenderPipelineState> pipelineState = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];

		//NSLog(@"%@", pipelineState);

		//

		[activeWindowData->encoder setViewport:(MTLViewport){0.0, 0.0, 300, 600, 0.0, 1.0 }];

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
		
		gxValidateMatrices();
	#endif
	
		gxValidatePipelineState();
	
		[activeWindowData->encoder setVertexBytes:s_gxVertices length:sizeof(GxVertex) * s_gxVertexCount atIndex:0];
		
		bool indexed = false;
		uint32_t * indices = 0;
		int numElements = s_gxVertexCount;
		int numIndices = 0;

		bool needToRegenerateIndexBuffer = false;
		
	#if TODO
		if (s_gxPrimitiveType != s_gxLastPrimitiveType || s_gxVertexCount != s_gxLastVertexCount)
		{
			s_gxLastPrimitiveType = s_gxPrimitiveType;
			s_gxLastVertexCount = s_gxVertexCount;

			needToRegenerateIndexBuffer = true;
		}
	#endif
	
	#if TODO
		// convert quads to triangles
		
		if (s_gxPrimitiveType == GX_QUADS)
		{
			fassert(s_gxVertexCount < 65536);
			
			// todo: use triangle strip + compute index buffer once at init time
			
			const int numQuads = s_gxVertexCount / 4;
			numIndices = numQuads * 6;

			if (needToRegenerateIndexBuffer)
			{
				indices = (glindex_t*)alloca(sizeof(glindex_t) * numIndices);

				glindex_t * __restrict indexPtr = indices;
				glindex_t baseIndex = 0;
			
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
			
			#if GX_USE_ELEMENT_ARRAY_BUFFER
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_gxIndexBufferObject[vaoIndex]);
				#if GX_USE_BUFFER_RENAMING
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glindex_t) * numIndices, 0, GX_BUFFER_DRAW_MODE);
				#endif
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glindex_t) * numIndices, indices, GX_BUFFER_DRAW_MODE);
				checkErrorGL();
			#endif
			}
			
			s_gxPrimitiveType = GX_TRIANGLES;
			numElements = numIndices;
			
			indexed = true;
		}
	#endif
		
	#if TODO
		const ShaderCacheElem & shaderElem = shader.getCacheElem();
		
		if (shaderElem.params[ShaderCacheElem::kSp_Params].index != -1)
		{
			shader.setImmediate(
				shaderElem.params[ShaderCacheElem::kSp_Params].index,
				s_gxTextureEnabled ? 1 : 0,
				globals.colorMode,
				globals.colorPost,
				globals.colorClamp);
		}

		if (globals.gxShaderIsDirty)
		{
			if (shaderElem.params[ShaderCacheElem::kSp_Texture].index != -1)
				shader.setTextureUnit(shaderElem.params[ShaderCacheElem::kSp_Texture].index, 0);
		}
	#endif
		
	#if TODO
		if (shader.isValid())
	#endif
		{
			const MTLPrimitiveType metalPrimitiveType = toMetalPrimitiveType(s_gxPrimitiveType);

			if (indexed)
			{
				fassert(false); // todo
				//[activeWindowData->encoder drawIndexedPrimitives:metalPrimitiveType indexCount:numElements indexType:MTLIndexTypeUInt32 indexBuffer:indices indexBufferOffset:0];
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

	gxValidateMatrices();

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
