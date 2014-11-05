#ifndef RESIB_H
#define RESIB_H
#pragma once

#include "Array.h"
#include "Mem.h"
#include "MemAllocators2.h"
#include "Res.h"
#include "Types.h"

class ResIB : public Res
{
public:
	ResIB();
	~ResIB();

	void Initialize(IMemAllocator * allocator, uint32_t iCnt);

	uint32_t GetIndexCnt() const;

	uint16_t* index;

private:
	COPY_PROTECT(ResIB);

	IMemAllocator * m_allocator;
	uint32_t m_indexCnt;
};


#endif
