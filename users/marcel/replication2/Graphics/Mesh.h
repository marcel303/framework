#ifndef MESH_H
#define MESH_H
#pragma once

#include "AABBObject.h"
#include "GraphicsTypes.h"
#include "Mem.h"
#include "ResIB.h"
#include "ResVB.h"

class Mesh : public AABBObject
{
public:
	Mesh();
	virtual ~Mesh();

	void Initialize(IMemAllocator * allocator, PRIMITIVE_TYPE type, bool dynamic, int vCnt, int fvf, int iCnt);
	bool IsIndexed() const;

	PRIMITIVE_TYPE GetPT() const;
	ResIB* GetIB();
	ResVB* GetVB();
	uint32_t GetPrimitiveCount() const;

	virtual AABB CalcAABB() const;

private:
	COPY_PROTECT(Mesh);

	IMemAllocator* m_allocator;
	PRIMITIVE_TYPE m_type;
	ResIB* m_ib;
	ResVB* m_vb;
};

#endif
