#pragma once

#include "libgg_forward.h"
#include "Types.h"

class Mat4x4;
class Res;
class SpriteGfx;

class MD3Vert
{
public:
	float p[3];
	float n[3];
	float uv[2];
};

class MD3Surf
{
public:
	MD3Surf()
		: VertList(0)
		, VertCount(0)
		, IndexList(0)
		, IndexCount(0)
	{
	}
	
	~MD3Surf()
	{
		delete[] VertList;
		VertList = 0;
		VertCount = 0;
		delete[] IndexList;
		IndexList = 0;
	}
	
	MD3Vert* VertList;
	uint16_t VertCount;
	uint16_t* IndexList;
	uint32_t IndexCount;
};

class MD3Model
{
public:
	MD3Model()
		: SurfList(0)
		, SurfCount(0)
		, Texture(0)
	{
	}
	
	~MD3Model()
	{
		delete[] SurfList;
		SurfList = 0;
		SurfCount = 0;
	}
	
	MD3Surf* SurfList;
	uint16_t SurfCount;
	Res* Texture;
};

class MD3Loader
{
public:
	static MD3Model* Load(Stream* stream);
};

void RenderMD3(MD3Model* model);
