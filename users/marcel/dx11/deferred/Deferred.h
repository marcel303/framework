#pragma once

#include <algorithm>
#include "Types.h"

//

#include <windows.h>
static uint64_t GetTicks()
{
	LARGE_INTEGER count;
	QueryPerformanceCounter(&count);
	return count.QuadPart;
}
static float TicksToMS(uint64_t ticks)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return static_cast<float>(ticks * 1000.0 / freq.QuadPart);
}

//

static const uint32_t RENDER_ELEM_COUNT  = 1 << 16; // maximum number of elements
static const uint32_t RENDER_TYPE_COUNT  = 64;      // maximum number of element types
static const uint32_t RENDER_PASS_COUNT  = 8;       // maximum number of render passes
static const uint32_t RENDER_LAYER_COUNT = 8;       // maximum number of layers
static const uint32_t RENDER_LAYER_MASK  = 1 << 31; // bit mask used to identify the active layer
static const uint32_t RENDER_PASS_REGISTRATION_COUNT = RENDER_PASS_COUNT + RENDER_LAYER_COUNT;

//

typedef uint64_t RenderSortKey;
typedef uint32_t RenderPassMask;
typedef uint16_t RenderElemIndex;
typedef uint8_t RenderElemType;

//

class RenderCallbackList;
class RenderElem;
class RenderElemSort;
class RenderPass;
class RenderPassList;

//

typedef void (*RenderCallback)(RenderElem * elem);

class RenderCallbackList
{
public:
	RenderCallback m_callback[RENDER_TYPE_COUNT];
};

class RenderElem
{
public:
	RenderSortKey m_sortKey;
	RenderElemType m_type;
};

class RenderElemSort_ARR
{
public:
	RenderElemSort_ARR()
	{
	}

	inline bool operator()(RenderElem const * elem1, RenderElem const * elem2) const
	{
		return elem1->m_sortKey < elem2->m_sortKey;
	}
};

class RenderElemSort_IDX
{
public:
	RenderElemSort_IDX(RenderElem const * const * elemList)
		: m_elemList(elemList)
	{
	}

	inline bool operator()(uint16_t index1, uint16_t index2) const
	{
		RenderElem const * elem1 = m_elemList[index1];
		RenderElem const * elem2 = m_elemList[index2];

		return elem1->m_sortKey < elem2->m_sortKey;
	}

private:
	RenderElem const * const * m_elemList;
};

enum RenderPassType
{
	RenderPassType_Multipass,
	RenderPassType_Layer,
	RenderPassType_Undefined
};

enum RenderPassSort
{
	RenderPassSort_None,
	RenderPassSort_SortKey,
	RenderPassSort_Undefined
};

typedef void (*RenderPassBeginCallback)(RenderPass * pRenderPass);
typedef void (*RenderPassEndCallback)(RenderPass * pRenderPass);

#if 1

class RenderPass
{
public:
	RenderPass()
		: m_elemListSize(0)
	{
	}

	void Reset()
	{
		m_elemListSize = 0;
	}

	void Add(RenderElem * elem, RenderElemIndex index)
	{
		m_elemList[m_elemListSize++] = elem;
	}

	void Sort(RenderElem ** elemList)
	{
		//DWORD t1 = timeGetTime();

		RenderElemSort_ARR sort;

		std::sort(m_elemList, m_elemList + m_elemListSize, sort);

		//DWORD t2 = timeGetTime();

		//printf("sort time: %g\n", (t2-t1) / 1.f);
	}

	void Render(RenderElem ** elemList)
	{
		uint32_t todo = m_elemListSize;
		RenderElem ** ptr = m_elemList;

		/*
		while (todo >= 4)
		{
			RenderElem * elem0 = ptr[0];
			RenderElem * elem1 = ptr[1];
			RenderElem * elem2 = ptr[2];
			RenderElem * elem3 = ptr[3];

			RenderElemType type0 = elem0->m_type;
			RenderElemType type1 = elem1->m_type;
			RenderElemType type2 = elem2->m_type;
			RenderElemType type3 = elem3->m_type;

			RenderCallback callback0 = m_callbackList.m_callback[type0];
			RenderCallback callback1 = m_callbackList.m_callback[type1];
			RenderCallback callback2 = m_callbackList.m_callback[type2];
			RenderCallback callback3 = m_callbackList.m_callback[type3];

			callback0(elem0);
			callback1(elem1);
			callback2(elem2);
			callback3(elem3);

			todo -= 4;
			ptr += 4;
		}
		*/

		while (todo != 0)
		{
			RenderElem * elem = ptr[0];

			RenderElemType type = elem->m_type;

			RenderCallback callback = m_callbackList.m_callback[type];

			callback(elem);

			todo--;
			ptr++;
		}
	}

	RenderCallbackList m_callbackList;
	uint32_t m_elemListSize;
	RenderElem * m_elemList[RENDER_ELEM_COUNT];
};

#else

class RenderPass
{
public:
	RenderPass()
		: m_elemIndexCount(0)
	{
	}

	void Reset()
	{
		m_elemIndexCount = 0;
	}

	void Add(RenderElem * elem, RenderElemIndex index)
	{
		m_elemIndexList[m_elemIndexCount++] = index;
	}

	void Sort(RenderElem ** elemList)
	{
		uint64_t t1 = GetTicks();

		RenderElemSort_IDX sort(elemList);

		std::sort(m_elemIndexList, m_elemIndexList + m_elemIndexCount, sort);

		uint64_t t2 = GetTicks();

		printf("sort time: %g\n", TicksToMS(t2 - t1));
	}

	void Render(RenderElem ** elemList)
	{
		uint32_t todo = m_elemIndexCount;
		RenderElemIndex * ptr = m_elemIndexList;

		/*
		while (todo >= 4)
		{
			RenderElemIndex index0 = ptr[0];
			RenderElemIndex index1 = ptr[1];
			RenderElemIndex index2 = ptr[2];
			RenderElemIndex index3 = ptr[3];

			RenderElem * elem0 = elemList[index0];
			RenderElem * elem1 = elemList[index1];
			RenderElem * elem2 = elemList[index2];
			RenderElem * elem3 = elemList[index3];

			RenderElemType type0 = elem0->m_type;
			RenderElemType type1 = elem1->m_type;
			RenderElemType type2 = elem2->m_type;
			RenderElemType type3 = elem3->m_type;

			RenderCallback callback0 = m_callbackList.m_callback[type0];
			RenderCallback callback1 = m_callbackList.m_callback[type1];
			RenderCallback callback2 = m_callbackList.m_callback[type2];
			RenderCallback callback3 = m_callbackList.m_callback[type3];

			callback0(elem0);
			callback1(elem1);
			callback2(elem2);
			callback3(elem3);

			todo -= 4;
			ptr += 4;
		}
		*/

		while (todo != 0)
		{
			RenderElemIndex index = ptr[0];

			RenderElem * elem = elemList[index];

			RenderElemType type = elem->m_type;

			RenderCallback callback = m_callbackList.m_callback[type];

			callback(elem);

			todo--;
			ptr++;
		}
	}

	RenderCallbackList m_callbackList;
	uint32_t m_elemIndexCount;
	RenderElemIndex m_elemIndexList[RENDER_ELEM_COUNT];
};

#endif

class RenderPassRegistration
{
public:
	RenderPassRegistration()
		: m_index(0)
		, m_isActive(false)
		, m_priority(0)
		, m_sort(RenderPassSort_Undefined)
		, m_beginCallback(0)
		, m_endCallback(0)
	{
	}

	void Set(
		uint32_t index,
		bool isActive,
		uint32_t priority,
		RenderPassSort sort,
		RenderPassBeginCallback beginCallback,
		RenderPassEndCallback endCallback)
	{
		m_index = index;
		m_isActive = isActive;
		m_priority = priority;
		m_sort = sort;
		m_beginCallback = beginCallback;
		m_endCallback = endCallback;
	}

	bool operator<(const RenderPassRegistration & other) const
	{
		if (m_isActive != other.m_isActive)
			return m_isActive > other.m_isActive;

		return m_priority < other.m_priority;
	}

	uint32_t m_index;
	bool m_isActive;
	uint32_t m_priority;
	RenderPassSort m_sort;
	RenderPassBeginCallback m_beginCallback;
	RenderPassEndCallback m_endCallback;
};

class RenderPassRegistrationList
{
public:
	RenderPassRegistrationList()
		: m_isInUpdate(false)
		, m_registrationCount(0)
	{
	}

	void UpdateBegin()
	{
		assert(!m_isInUpdate);

		m_isInUpdate = true;
	}

	void UpdateEnd()
	{
		assert(m_isInUpdate);

		Sort();

		m_isInUpdate = false;
	}

	void Register(uint32_t index, bool isActive, uint32_t priority, RenderPassSort sort, RenderPassBeginCallback beginCallback, RenderPassEndCallback endCallback)
	{
		assert(m_isInUpdate);
		assert(m_registrationCount + 1 <= RENDER_PASS_REGISTRATION_COUNT);

		m_registrationList[m_registrationCount].Set(index, isActive, priority, sort, beginCallback, endCallback);

		m_registrationCount++;
	}

	uint32_t RegistrationCount_get() const
	{
		return m_registrationCount;
	}

	const RenderPassRegistration & Registration_get(uint32_t index) const
	{
		return m_registrationList[index];
	}

private:
	bool m_isInUpdate;
	uint32_t m_registrationCount;
	RenderPassRegistration m_registrationList[RENDER_PASS_REGISTRATION_COUNT];

	void Sort()
	{
		assert(m_isInUpdate);

		std::sort(m_registrationList, m_registrationList + RENDER_PASS_REGISTRATION_COUNT);
	}
};

class RenderPassList
{
public:
	RenderPassList()
		: m_elemListSize(0)
		, m_activeLayer(0)
	{
	}

	void Reset()
	{
		for (uint32_t i = 0; i < RENDER_PASS_COUNT; ++i)
		{
			m_passList[i].Reset();
		}

		m_elemListSize = 0;
	}

	void Add(RenderPassMask passMask, RenderElem * elem)
	{
		RenderElemIndex index = m_elemListSize;

		m_elemList[index] = elem;

		for (uint32_t i = 0; i < RENDER_PASS_COUNT; ++i)
			if (passMask & (1 << i))
				m_passList[i].Add(elem, index);

		if ((passMask & RENDER_LAYER_MASK) != 0 && m_activeLayer != 0)
			m_activeLayer->Add(elem, index);

		m_elemListSize++;
	}

	void Render()
	{
		for (uint32_t i = 0; i < m_registrationList.RegistrationCount_get(); ++i)
		{
			const RenderPassRegistration & reg = m_registrationList.Registration_get(i);

			if (reg.m_isActive)
			{
				RenderPass & pass = m_passList[reg.m_index];

				switch (reg.m_sort)
				{
				case RenderPassSort_SortKey:
					pass.Sort(m_elemList);
					break;
				}

				if (reg.m_beginCallback)
					reg.m_beginCallback(&pass);

				pass.Render(m_elemList);

				if (reg.m_endCallback)
					reg.m_endCallback(&pass);
			}
		}
	}

	void RegisterPass(RenderPassType type, uint32_t index, bool isActive, uint32_t priority, RenderPassSort sort, RenderPassBeginCallback beginCallback, RenderPassEndCallback endCallback)
	{
		m_registrationList.UpdateBegin();

		switch (type)
		{
		case RenderPassType_Multipass:
			assert(index >= 0 && index < RENDER_PASS_COUNT);
			m_registrationList.Register(index, isActive, priority, sort, beginCallback, endCallback);
			break;
		case RenderPassType_Layer:
			assert(index >= 0 && index < RENDER_LAYER_COUNT);
			m_registrationList.Register(index + RENDER_PASS_COUNT, isActive, priority, sort, beginCallback, endCallback);
			break;
		}

		m_registrationList.UpdateEnd();
	}

	void ResetActiveLayer()
	{
		m_activeLayer = 0;
	}

	void SetActiveLayer(uint32_t layerIdx)
	{
		assert(layerIdx >= 0 && layerIdx < RENDER_LAYER_COUNT);

		m_activeLayer = &m_passList[RENDER_PASS_COUNT + layerIdx];
	}

	RenderPassRegistrationList m_registrationList;
	RenderElem * m_elemList[RENDER_ELEM_COUNT];
	uint32_t m_elemListSize;
	RenderPass m_passList[RENDER_PASS_COUNT + RENDER_LAYER_COUNT];
	RenderPass * m_activeLayer;
};

static void Render(RenderElem * elem)
{
	//printf("elem: key=%d, type=%d\n", (int)elem->m_sortKey, (int)elem->m_type);
}

static void RenderPassBegin(RenderPass * pRenderPass)
{
	printf("render pass begin\n");
}

static void RenderPassEnd(RenderPass * pRenderPass)
{
	printf("render pass end\n");
}

static void TestDeferredStuff()
{
	static const uint32_t kTestSize = RENDER_ELEM_COUNT >> 3;

	Sleep(3000);

	RenderPassList * passList = new RenderPassList();

	for (uint32_t i = 0; i < RENDER_PASS_COUNT + RENDER_LAYER_COUNT; ++i)
		for (uint32_t j = 0; j < RENDER_TYPE_COUNT; ++j)
			passList->m_passList[i].m_callbackList.m_callback[j] = Render;

	for (uint32_t i = 0; i < RENDER_PASS_COUNT; ++i)
		passList->RegisterPass(RenderPassType_Multipass, i, true, rand(), RenderPassSort_SortKey, RenderPassBegin, RenderPassEnd);

	for (uint32_t i = 0; i < RENDER_LAYER_COUNT; ++i)
		passList->RegisterPass(RenderPassType_Layer, i, true, rand(), RenderPassSort_None, RenderPassBegin, RenderPassEnd);

	//

	RenderElem * elemList = new RenderElem[kTestSize];

	RenderPassMask * passMask = new RenderPassMask[kTestSize];

	for (uint32_t i = 0; i < kTestSize; ++i)
	{
		elemList[i].m_sortKey = rand() % (kTestSize >> 4);
		elemList[i].m_type = rand() % RENDER_TYPE_COUNT;

		passMask[i] = 0;
		for (uint32_t j = 0; j < 32; ++j)
			passMask[i] |= (rand() & 1) << j;
	}

	uint64_t t1 = GetTicks();

	for (uint32_t i = 0; i < 10; ++i)
	{
		passList->Reset();

		for (uint32_t i = 0; i < kTestSize; ++i)
		{
			passList->SetActiveLayer(i % RENDER_LAYER_COUNT);

			passList->Add(passMask[i], &elemList[i]);
		}
	}

	uint64_t t2 = GetTicks();

	printf("add time: %g ms\n", TicksToMS((t2-t1) / 10));

	uint64_t t3 = GetTicks();

	for (uint32_t i = 0; i < 5; ++i)
	{
		passList->Render();
	}

	uint64_t t4 = GetTicks();

	printf("render time: %g ms\n", TicksToMS((t4-t3) / 5));

	printf("done\n");

	delete[] passMask;
	passMask = 0;

	delete[] elemList;
	elemList = 0;

	delete passList;
	passList = 0;
}
