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
		float3 position [[attribute(0)]];
	};

	struct ShaderOutputs
	{
		float4 position [[position]];
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
		
		outputs.position.xyz = inputs.position * .5f;
		outputs.position.w = 1.f;
		
		return outputs;
	}

)SHADER";

static const char * s_shaderPs = R"SHADER(

	#include <metal_stdlib>

	struct ShaderInputs
	{
		float4 position [[position]];
	};

	fragment float4 shader_main(ShaderInputs inputs [[stage_in]])
	{
		return float4(1, 0, 0, 1);
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

void metal_drawtest()
{
	@autoreleasepool
	{
	#if 1
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
		vertexDescriptor.attributes[0].format = MTLVertexFormatFloat3;
		vertexDescriptor.attributes[0].offset = 0;
		vertexDescriptor.attributes[0].bufferIndex = 0;
		
		vertexDescriptor.layouts[0].stride = 4 * 3;
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
