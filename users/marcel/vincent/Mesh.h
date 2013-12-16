#pragma once

#include <assert.h>
#include "Types2.h"

typedef struct VB_Position3
{
	float x, y, z;
};

typedef struct VB_Color4
{
	union
	{
		struct
		{
			uint8_t r, g, b, a;
		};
		uint32_t c;
	};
};

typedef struct VB_Normal3
{
	float x, y, z;
};

typedef struct VB_Texcoord4
{
	float u, v, s, t;
};

class VertexBuffer
{
public:
	enum FVF
	{
		FVF_XYZ = 1 << 0,
		FVF_COLOR = 1 << 1,
		FVF_NORMAL = 1 << 2,
		FVF_TANGENT = 1 << 3,
		FVF_BINORMAL = 1 << 4,
	};
	
	static inline uint32_t FVF_TEXn(int n)
	{
		return n << 5;
	}
	
	static inline int FVF_GetTexN(uint32_t fvf)
	{
		return (fvf >> 5) & 3;
	}
	
	VertexBuffer();
	VertexBuffer(const VertexBuffer&);
	~VertexBuffer();
	void Initialize(uint32_t count, uint32_t fvf);
	
	uint32_t GetFVF();
	uint32_t GetVertexCount();
	uint32_t GetTexcoordCount();
	uint32_t GetBlendWeightCount();
	
	void SetPosition3(size_t index, float x, float y, float z);
	void SetColor(size_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
	void SetColor(size_t index, uint8_t r, uint8_t g, uint8_t b);
	void SetNormal(size_t index, float x, float y, float z);
	void SetTangent(size_t index, float x, float y, float z);
	void SetBiNormal(size_t index, float x, float y, float z);
	void SetTexcoord(size_t index, size_t texIndex, float u, float v, float s, float t);
	void SetTexcoord(size_t index, size_t texIndex, float u, float v);
	
	VB_Position3* position3;
	VB_Color4* color;
	VB_Normal3* normal;
	VB_Normal3* tangent;
	VB_Normal3* biNormal;
	VB_Texcoord4* tex[1];
	float* blendn[4];
	
private:
	uint32_t m_FVF;
	uint32_t m_VertexCount;
	
	void ZeroArrays();
};

class IndexBuffer
{
public:
	enum IF
	{
		IF_INDEX16/*,
		IF_INDEX32*/
	};
	
	IndexBuffer();
	IndexBuffer(const IndexBuffer&);
	~IndexBuffer();
	
	void Initialize(uint32_t count, IF format);
	
	IF GetIndexFormat();
	uint32_t GetIndexCount();
	
	uint16_t* index;
	
private:
	IF m_IndexFormat;
	uint32_t m_IndexCount;
	
	void ZeroArrays();
};

class Mesh
{
public:
	enum PT
	{
		PT_NONE,
		PT_TRIANGLE_FAN,
		PT_TRIANGLE_LIST,
		PT_TRIANGLE_STRIP
	};
	
	Mesh();
	~Mesh();

	void Initialize(PT pt, uint32_t vertexCount, uint32_t vertexFVF, uint32_t indexCount, IndexBuffer::IF indexFormat);
	void Initialize(PT pt, uint32_t vertexCount, uint32_t vertexFVF, uint32_t indexCount);
	void Initialize(PT pt, uint32_t vertexCount, uint32_t vertexFVF);
	
	PT GetPrimitiveType();
	uint32_t GetPrimitiveCount();
	
	bool IsIndexed();
	
	VertexBuffer& GetVertexBuffer();
	IndexBuffer& GetIndexBuffer();
	
private:
	PT m_PT;
	VertexBuffer m_VertexBuffer;
	IndexBuffer m_IndexBuffer;
};
