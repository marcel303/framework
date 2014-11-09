#ifndef RESVB_H
#define RESVB_H
#pragma once

#include "Mem.h"
#include "MemAllocators2.h"
#include "Res.h"
#include "Types.h"
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

#define FVF_TEX(count) (count << 16)
#define FVF_TEX_EXTRACT(fvf) (fvf >> 16) & 0xF;

enum FVF
{
	FVF_XYZ = 0x01,
	FVF_NORMAL = 0x02,
	//FVF_TANGENT = 0x04,
	//FVF_BINORMAL = 0x08,
	FVF_COLOR = 0x10,
	FVF_TEX1 = FVF_TEX(1),
	FVF_TEX2 = FVF_TEX(2),
	FVF_TEX3 = FVF_TEX(3),
	FVF_TEX4 = FVF_TEX(4)
};

class ResVB : public Res
{
public:
	ResVB();
	~ResVB();

	void Initialize(IMemAllocator * allocator, uint32_t vCnt, uint32_t fvf);
	void Clear();

	uint32_t GetVertexCnt() const;
	uint32_t GetFVF() const;
	uint32_t GetTexCnt() const;

	void SetPosition(uint32_t index, float x, float y, float z);
	void SetNormal(uint32_t index, float x, float y, float z);
	void SetColor(uint32_t index, float r, float g, float b, float a = 1.0f);
	void SetTex(uint32_t index, uint32_t sampler, float u, float v);

	Vec3* position;
	Vec3* normal;
	//Vec3* tangent;
	//Vec3* binormal;
	Vec4* color;
	Vec4* tex[4];

private:
	COPY_PROTECT(ResVB);

	IMemAllocator* m_allocator;

//public: // FIXME
	uint32_t m_vCnt;
	uint32_t m_fvf;
	uint32_t m_texCnt; // Derived from m_fvf.
};

#endif
