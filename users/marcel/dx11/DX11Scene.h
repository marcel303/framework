#pragma once

#include "Heap.h"

class Scene
{
public:
	Scene()
	{
		m_pFrameHeap = new MemAllocatorTransient(kFrameHeapSize);
		m_pScratchHeap = new MemAllocatorManualStack(kScratchHeapSize, kScratchHeapDepth);
	}

	~Scene()
	{
		delete m_pScratchHeap;
		m_pScratchHeap = 0;

		delete m_pFrameHeap;
		m_pFrameHeap = 0;
	}

	void Begin()
	{
		m_pScratchHeap->Push();
	}

	void End()
	{
		Commit();

		m_pScratchHeap->Pop();
	}

	void Commit()
	{
		m_pFrameHeap->Reset();
	}

	IMemAllocator * GetFrameHeap()
	{
		return m_pFrameHeap;
	}

	MemAllocatorManualStack * GetScratchHeap()
	{
		return m_pScratchHeap;
	}

private:
	const static uint32_t kFrameHeapSize = 1024 * 256;
	const static uint32_t kScratchHeapSize = 1024 * 256;
	const static uint32_t kScratchHeapDepth = 64;

	MemAllocatorTransient * m_pFrameHeap;
	MemAllocatorManualStack * m_pScratchHeap;
};
