/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

struct TrackedDot
{
	// (x, y) must be passed in as input
	float x;
	float y;
	
	// id and speed are outputted after running identify on the input set of dots
	int id;
	float speedX;
	float speedY;
	
	// stuff used internally by the dot tracker, or if you choose to use DGrid
	TrackedDot * next;
	int gridIndex;
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

	void identify(TrackedDot * dots, const int numDots, const float dt, const float maxDistance,
		const bool useExtrapolation = true,
		int * addedIds = nullptr, int * numAdded = nullptr,
		int * removedIds = nullptr, int * numRemoved = nullptr);
	
	// older, slower versions of the algorithm. these only exist for reference and verification of the newer version
	void identify_hgrid(TrackedDot * dots, const int numDots, const float dt, const float maxDistance);
	void identify_reference(TrackedDot * dots, const int numDots, const float dt, const float maxDistance);
};
