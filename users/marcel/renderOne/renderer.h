#pragma once

#include "lightDrawer.h"
#include "renderOptions.h"
#include <functional>

#define BLOOM_METHOD_DOWNSAMPLE_CHAIN   0
#define BLOOM_METHOD_ONE_LARGE_GAUSSIAN 1

#define BLOOM_METHOD BLOOM_METHOD_DOWNSAMPLE_CHAIN
//#define BLOOM_METHOD BLOOM_METHOD_ONE_LARGE_GAUSSIAN

class Color;
class ColorTarget;
class DepthTarget;
class Surface;

struct TypeDB;

namespace rOne
{
	typedef std::function<void()> RenderFunction;

	struct RenderFunctions
	{
		RenderFunction drawOpaque;
		RenderFunction drawBackground;
		RenderFunction drawTranslucent;
		RenderFunction drawLights;
	};

	struct RenderBuffers
	{
		int sx = 0;
		int sy = 0;
		bool linearColorSpace = false;
		
		ColorTarget * colors = nullptr;
		ColorTarget * normals = nullptr;
		ColorTarget * specularColor = nullptr;
		ColorTarget * specularExponent = nullptr;
		ColorTarget * emissive = nullptr;
		DepthTarget * depth = nullptr;
		ColorTarget * light = nullptr;
		ColorTarget * composite1 = nullptr;
		ColorTarget * composite2 = nullptr;
		ColorTarget * velocity = nullptr;
		
	#if BLOOM_METHOD == BLOOM_METHOD_ONE_LARGE_GAUSSIAN
		ColorTarget * bloomBuffer = nullptr;
	#endif

	#if BLOOM_METHOD == BLOOM_METHOD_DOWNSAMPLE_CHAIN
		struct BloomChain
		{
			ColorTarget * buffers = nullptr;
			int numBuffers = 0;
			
			void alloc(const int sx, const int sy);
			void free();
		};
		
		BloomChain bloomDownsampleChain;
		BloomChain bloomBlurChain;
	#endif

		~RenderBuffers();
		
		void alloc(const int sx, const int sy, const bool linearColorSpace);
		void free();
	};

	struct RenderEyeData
	{
		bool isValid = false;
		float timeStep = 0.f;
		
		Mat4x4 projectionToWorld_curr = Mat4x4(true);
		Mat4x4 projectionToWorld_prev = Mat4x4(true);
		
	#if 0
		static const int kHistorySize = 100;
		int projectionToWorld_prev_hist_idx = -1;
		Mat4x4 projectionToWorld_prev_hist[kHistorySize];
	#endif
	};
	
	struct Renderer
	{
		RenderBuffers buffers;
		RenderBuffers buffers2;
		
		RenderEyeData eyeData[2]; // left, right
		
		static void registerShaderOutputs();
		
		void free();
		
		void render(
			const RenderFunctions & renderFunctions,
			const RenderOptions & renderOptions,
			const int viewportSx,
			const int viewportsy,
			const float timeStep,
			const bool updateHistory = true);
		
		void render(
			const RenderFunctions & renderFunctions,
			const RenderOptions & renderOptions,
			ColorTarget * colorTarget,
			DepthTarget * depthTarget,
			const float timeStep,
			const bool updateHistory = true);
		
		void render(
			const RenderFunctions & renderFunctions,
			const RenderOptions & renderOptions,
			const float timeStep,
			const bool updateHistory = true);
	};
}
