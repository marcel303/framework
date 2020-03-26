#pragma once

#include "lightDrawer.h"
#include "renderOptions.h"
#include <functional>

#define BLOOM_METHOD_DOWNSAMPLE_CHAIN   0
#define BLOOM_METHOD_ONE_LARGE_GAUSSIAN 1

//#define BLOOM_METHOD BLOOM_METHOD_DOWNSAMPLE_CHAIN
#define BLOOM_METHOD BLOOM_METHOD_ONE_LARGE_GAUSSIAN

class Color;
class ColorTarget;
class DepthTarget;
class Surface;

struct TypeDB;

typedef std::function<void()> RenderFunction;

struct RenderFunctions
{
	RenderFunction drawOpaque;
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
	} bloomChain;
#endif

	~RenderBuffers();
	
	void alloc(const int sx, const int sy, const bool linearColorSpace);
	void free();
};

struct Renderer
{
	RenderBuffers buffers;
	RenderBuffers buffers2;
	
	static void registerShaderOutputs();
	
	void render(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		const int viewportSx,
		const int viewportsy,
		const float timeStep);
	
	void render(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		ColorTarget * colorTarget,
		DepthTarget * depthTarget,
		const float timeStep);
	
	void render(
		const RenderFunctions & renderFunctions,
		const RenderOptions & renderOptions,
		const float timeStep);
};
