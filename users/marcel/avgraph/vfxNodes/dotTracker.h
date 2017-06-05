#pragma once

struct TrackedDot
{
	float x;
	float y;
	
	int id;
	
	TrackedDot * next;
	int gridIndex;
	bool identified;

	TrackedDot()
		: x(0.f)
		, y(0.f)
		, id(-1)
		, next(nullptr)
		, gridIndex(-1)
		, identified(false)
	{
	}
};

struct DotTracker
{
	static const int kMaxPoints = 1024;
	static const int kHistorySize = kMaxPoints * 2;
	static const int kGridSize = 128;
	static const int kGridSizeSq = kGridSize * kGridSize;
	
	struct HistoryElem
	{
		float x;
		float y;
		float speedX;
		float speedY;
		float projectedX;
		float projectedY;
		
		int id;
		
		bool identified;
		
		HistoryElem * next;
		int gridIndex;
	};
	
	struct HGrid
	{
		HistoryElem * elems[kGridSizeSq];
		
		void init(HistoryElem * history, const int historySize, const int cellSize);
		void erase(HistoryElem * e);
	};
	
	struct DGrid
	{
		TrackedDot * elems[kGridSizeSq];
		
		void init(TrackedDot * dots, const int numDots, const int cellSize);
		void erase(TrackedDot * e);
	};

	HistoryElem history[kHistorySize];
	int historySize;

	int nextAllocId;

	DotTracker()
		: history()
		, historySize(0)
		, nextAllocId(0)
	{
	}

	int allocId()
	{
		return nextAllocId++;
	}

	void identify(TrackedDot * dots, const int numDots, const float dt, const float maxDistance);
	void identify_hgrid(TrackedDot * dots, const int numDots, const float dt, const float maxDistance);
	void identify_reference(TrackedDot * dots, const int numDots, const float dt, const float maxDistance);
};
