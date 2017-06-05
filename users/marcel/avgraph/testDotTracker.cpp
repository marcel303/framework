#include "framework.h"
#include "Timer.h"
#include <algorithm>

#define DO_VALIDATION 0

extern const int GFX_SX;
extern const int GFX_SY;

const int kNumPoints = 256;

// fixme : what happens when the glyph texture atlas grows during a text batch ?
//         -> maybe only grow but not optimize texture atlas when in a batch ?

struct TrackedDot
{
	float x;
	float y;
	
	int id;
	
	TrackedDot()
		: x(0.f)
		, y(0.f)
		, id(-1)
		, next(nullptr)
	{
	}
	
	TrackedDot * next;
	int gridIndex;
	bool identified;
};

struct DotTracker
{
	static const int kHistorySize = kNumPoints * 2;
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
		
		void init(HistoryElem * history, const int historySize, const int cellSize)
		{
			memset(elems, 0, sizeof(elems));
			
			for (int i = 0; i < historySize; ++i)
			{
				HistoryElem & h = history[i];
				
				const int cellX = (int(std::round(h.projectedX / cellSize)) + (kGridSize << 10)) % kGridSize;
				const int cellY = (int(std::round(h.projectedY / cellSize)) + (kGridSize << 10)) % kGridSize;
				Assert(cellX >= 0);
				Assert(cellY >= 0);
				
				const int cellIndex = cellX + cellY * kGridSize;
				
				h.next = elems[cellIndex];
				h.gridIndex = cellIndex;
				
				elems[cellIndex] = &h;
			}
		}
		
		void erase(HistoryElem * e)
		{
			Assert(e->gridIndex != -1);
			
		#if DO_VALIDATION
			bool removed = false;
		#endif
			
			for (HistoryElem ** gridElem = &elems[e->gridIndex]; *gridElem != nullptr; gridElem = &(*gridElem)->next)
			{
				if (*gridElem == e)
				{
					*gridElem = (*gridElem)->next;
					
				#if DO_VALIDATION
					removed = true;
				#endif
					
					break;
				}
			}
			
		#if DO_VALIDATION
			Assert(removed);
		#endif
			
			e->gridIndex = -1;
		}
	};
	
	struct DGrid
	{
		TrackedDot * elems[kGridSizeSq];
		
		void init(TrackedDot * dots, const int numDots, const int cellSize)
		{
			memset(elems, 0, sizeof(elems));
			
			for (int i = 0; i < numDots; ++i)
			{
				TrackedDot & d = dots[i];
				
				const int cellX = (int(std::round(d.x / cellSize)) + (kGridSize << 10)) % kGridSize;
				const int cellY = (int(std::round(d.y / cellSize)) + (kGridSize << 10)) % kGridSize;
				Assert(cellX >= 0);
				Assert(cellY >= 0);
				
				const int cellIndex = cellX + cellY * kGridSize;
				
				d.next = elems[cellIndex];
				d.gridIndex = cellIndex;
				
				elems[cellIndex] = &d;
			}
		}
		
		void erase(TrackedDot * e)
		{
			Assert(e->gridIndex != -1);
			
		#if DO_VALIDATION
			bool removed = false;
		#endif
			
			for (TrackedDot ** gridElem = &elems[e->gridIndex]; *gridElem != nullptr; gridElem = &(*gridElem)->next)
			{
				if (*gridElem == e)
				{
					*gridElem = (*gridElem)->next;
					
				#if DO_VALIDATION
					removed = true;
				#endif
					
					break;
				}
			}
			
		#if DO_VALIDATION
			Assert(removed);
		#endif
		
			e->gridIndex = -1;
		}
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

	void identify_slow(TrackedDot * dots, const int numDots, const float dt, const float maxDistance)
	{
		for (int i = 0; i < historySize; ++i)
		{
			HistoryElem & h = history[i];
			
			h.projectedX = h.x + h.speedX * dt;
			h.projectedY = h.y + h.speedY * dt;
		}
		
		// setup grid data structure
		
		const float maxDistanceSq = maxDistance * maxDistance;
		
		const int cellSize = (int(std::ceil(maxDistance)) + 1) / 2;
		
		HGrid hgrid;
		
		hgrid.init(history, historySize, cellSize);
		
		//
		
		bool identified[kNumPoints];
		memset(identified, 0, sizeof(identified));
		
	#if DO_VALIDATION
		int numEvaluated = 0;
	#endif
		
		for (int n = 0; n < numDots; ++n)
		{
			float bestDistance = std::numeric_limits<float>::max();
			int bestValueIndex = -1;
			int bestHistoryIndex = -1;
			
			for (int i = 0; i < numDots; ++i)
			{
				if (identified[i])
					continue;
				
				const TrackedDot & dot = dots[i];
				
				const int baseCellX = int(std::round(dot.x / cellSize));
				const int baseCellY = int(std::round(dot.y / cellSize));
				
				const int cellX1 = baseCellX - 1 + kGridSize;
				const int cellY1 = baseCellY - 1 + kGridSize;
				
				const int cellX2 = cellX1 + 2;
				const int cellY2 = cellY1 + 2;
				
				for (int _cellX = cellX1; _cellX <= cellX2; ++_cellX)
				{
					for (int _cellY = cellY1; _cellY <= cellY2; ++_cellY)
					{
						const int cellX = _cellX % kGridSize;
						const int cellY = _cellY % kGridSize;
						
						const int cellIndex = cellX + cellY * kGridSize;
						
						for (HistoryElem * h = hgrid.elems[cellIndex]; h != nullptr; h = h->next)
						{
						#if DO_VALIDATION
							Assert(h->gridIndex != -1);
						#endif
							
							const float dx = h->projectedX - dot.x;
							const float dy = h->projectedY - dot.y;
							const float dsSq = dx * dx + dy * dy;
							
							if (dsSq < bestDistance && dsSq < maxDistanceSq)
							{
								bestDistance = dsSq;
								bestValueIndex = i;
								bestHistoryIndex = h - history;
							}
							
						#if DO_VALIDATION
							numEvaluated++;
						#endif
						}
					}
				}
			}
			
			if (bestValueIndex == -1)
			{
				for (int i = 0; i < numDots; ++i)
				{
					if (identified[i])
						continue;
					
					auto & h = history[historySize++];
					
					h.x = dots[i].x;
					h.y = dots[i].y;
					h.speedX = 0.f;
					h.speedY = 0.f;
					
					h.id = allocId();
					
					h.gridIndex = -1;
					
					dots[i].id = h.id;
				}
				
				break;
			}
			else
			{
				auto & h = history[bestHistoryIndex];
				
				Assert(h.gridIndex != -1);
				
				dots[bestValueIndex].id = h.id;
				identified[bestValueIndex] = true;
				
				h.speedX = (dots[bestValueIndex].x - h.x) / dt;
				h.speedY = (dots[bestValueIndex].y - h.y) / dt;
				
				h.x = dots[bestValueIndex].x;
				h.y = dots[bestValueIndex].y;
				
				hgrid.erase(&h);
			}
		}
		
		for (int i = 0; i < historySize; ++i)
		{
			if (history[i].gridIndex != -1)
			{
				history[i] = history[historySize - 1];
				
				historySize--;
			}
		}
		
	#if DO_VALIDATION
		logDebug("numEvaluated: %d", numEvaluated);
	#endif
		
		Assert(historySize == numDots);
	}
	
	void identify(TrackedDot * dots, const int numDots, const float dt, const float maxDistance)
	{
		// note : even faster would is to iterate over each history item, compute the interesting region (item position in cell coordinates +/- 1), and within this region, calculate the minimum of any dot to any history item in the dot region (dot position in cell coordinates +/- 1). if there is a dot with a nearest history item being the current history item, the current history item can 'claim' this dot. otherwise, the history item is considered 'dead'
		
		const float maxDistanceSq = maxDistance * maxDistance;
		
		const int cellSize = (int(std::ceil(maxDistance)) + 1) / 2;
		
		for (int i = 0; i < historySize; ++i)
		{
			HistoryElem & h = history[i];
			
			h.projectedX = h.x + h.speedX * dt;
			h.projectedY = h.y + h.speedY * dt;
			
			h.identified = false;
		}
		
		// insert history elements into a grid
		
		HGrid hgrid;
		hgrid.init(history, historySize, cellSize);
		
		// insert dots into a grid
		
		DGrid dgrid;
		dgrid.init(dots, numDots, cellSize);
		
		//
		
	#if DO_VALIDATION
		int numEvaluated = 0;
	#endif
		
		for (int n = 0; n < historySize; ++n)
		{
			HistoryElem & h = history[n];
			
			float bestDistance = std::numeric_limits<float>::max();
			TrackedDot * bestDot = nullptr;
			
			const int hbaseCellX = int(std::round(h.projectedX / cellSize));
			const int hbaseCellY = int(std::round(h.projectedY / cellSize));
			
			const int hcellX1 = hbaseCellX - 1 + kGridSize;
			const int hcellY1 = hbaseCellY - 1 + kGridSize;
			
			const int hcellX2 = hcellX1 + 2;
			const int hcellY2 = hcellY1 + 2;
		
			for (int _hcellX = hcellX1; _hcellX <= hcellX2; ++_hcellX)
			{
				const int hcellX = _hcellX % kGridSize;
				
				for (int _hcellY = hcellY1; _hcellY <= hcellY2; ++_hcellY)
				{
					const int hcellY = _hcellY % kGridSize;
					
					const int hcellIndex = hcellX + hcellY * kGridSize;
					
					for (TrackedDot * dot = dgrid.elems[hcellIndex]; dot != nullptr; dot = dot->next)
					{
					#if DO_VALIDATION
						Assert(dot->gridIndex != -1);
					#endif
						
						const float hdx = h.projectedX - dot->x;
						const float hdy = h.projectedY - dot->y;
						const float hdsSq = hdx * hdx + hdy * hdy;
						
						if (hdsSq >= bestDistance || hdsSq >= maxDistanceSq)
						{
							// there's another dot that considers the current history element as its nearest history element. and it's distance is less than the current dot's. we can safely skip this dot for consideration
							
							continue;
						}
						
						//
						
						const int cellX1 = hcellX - 1 + kGridSize;
						const int cellY1 = hcellY - 1 + kGridSize;
						
						const int cellX2 = cellX1 + 2;
						const int cellY2 = cellY1 + 2;
						
						for (int _cellX = cellX1; _cellX <= cellX2; ++_cellX)
						{
							const int cellX = _cellX % kGridSize;
							
							for (int _cellY = cellY1; _cellY <= cellY2; ++_cellY)
							{
								const int cellY = _cellY % kGridSize;
								
								const int cellIndex = cellX + cellY * kGridSize;
								
								for (HistoryElem * h = hgrid.elems[cellIndex]; h != nullptr; h = h->next)
								{
								#if DO_VALIDATION
									Assert(h->gridIndex != -1);
								
									numEvaluated++;
								#endif
									
									const float dx = h->projectedX - dot->x;
									const float dy = h->projectedY - dot->y;
									const float dsSq = dx * dx + dy * dy;
									
									if (dsSq < hdsSq)
									{
										goto nohit;
									}
								}
							}
						}
						
						bestDistance = hdsSq;
						bestDot = dot;
						
					nohit:
						do { } while (false);
					}
				}
			}
			
			if (bestDot != nullptr)
			{
				bestDot->id = h.id;
				
				// remove dot from grid. it's been claimed by a history element
			
				dgrid.erase(bestDot);
				
				// update history element
				
				h.speedX = (bestDot->x - h.x) / dt;
				h.speedY = (bestDot->y - h.y) / dt;
				
				h.x = bestDot->x;
				h.y = bestDot->y;
				
				h.identified = true;
			}
			
			// remove history element from grid. it's been considered by all dots surrounding it
			
			hgrid.erase(&h);
		}
	
		// purge unidentified history elements from the list
		
		for (int i = 0; i < historySize; ++i)
		{
			if (history[i].identified == false)
			{
				history[i] = history[historySize - 1];
				
				historySize--;
			}
		}
		
		// allocate history elements for unidentified dots
		
		for (int i = 0; i < numDots; ++i)
		{
			if (dots[i].gridIndex != -1)
			{
				auto & h = history[historySize++];
				
				h.x = dots[i].x;
				h.y = dots[i].y;
				h.speedX = 0.f;
				h.speedY = 0.f;
				
				h.id = allocId();
				
				dots[i].id = h.id;
			}
		}
		
	#if DO_VALIDATION
		logDebug("numEvaluated: %d", numEvaluated);
	#endif
		
		Assert(historySize == numDots);
	}
	
	void identify_reference(TrackedDot * dots, const int numDots, const float dt, const float maxDistance)
	{
		const float maxDistanceSq = maxDistance * maxDistance;
		
		for (int i = 0; i < historySize; ++i)
		{
			history[i].identified = false;
			
			history[i].projectedX = history[i].x + history[i].speedX * dt;
			history[i].projectedY = history[i].y + history[i].speedY * dt;
		}
		
		bool identified[kNumPoints];
		memset(identified, 0, sizeof(identified));
		
	#if DO_VALIDATION
		int numEvaluated = 0;
	#endif
		
		for (int n = 0; n < numDots; ++n)
		{
			float bestDistance = std::numeric_limits<float>::max();
			int bestValueIndex = -1;
			int bestHistoryIndex = -1;
			
			for (int i = 0; i < numDots; ++i)
			{
				if (identified[i])
					continue;
				
				const TrackedDot & dot = dots[i];
				
				for (int j = 0; j < historySize; ++j)
				{
					if (history[j].identified)
						continue;
					
					const float dx = history[j].projectedX - dot.x;
					const float dy = history[j].projectedY - dot.y;
					const float dsSq = dx * dx + dy * dy;
					
					if (dsSq < bestDistance && dsSq < maxDistanceSq)
					{
						bestDistance = dsSq;
						bestValueIndex = i;
						bestHistoryIndex = j;
					}
					
				#if DO_VALIDATION
					numEvaluated++;
				#endif
				}
			}
			
			if (bestValueIndex == -1)
			{
				for (int i = 0; i < numDots; ++i)
				{
					if (identified[i])
						continue;
					
					auto & h = history[historySize++];
					
					h.x = dots[i].x;
					h.y = dots[i].y;
					h.speedX = 0.f;
					h.speedY = 0.f;
					
					h.id = allocId();
					
					h.identified = true;
					
					dots[i].id = h.id;
				}
				
				break;
			}
			else
			{
				auto & h = history[bestHistoryIndex];
				
				Assert(h.identified == false);
				
				dots[bestValueIndex].id = h.id;
				identified[bestValueIndex] = true;
				
				h.speedX = (dots[bestValueIndex].x - h.x) / dt;
				h.speedY = (dots[bestValueIndex].y - h.y) / dt;
				
				h.x = dots[bestValueIndex].x;
				h.y = dots[bestValueIndex].y;
				
				h.identified = true;
			}
		}
		
		for (int i = 0; i < historySize; ++i)
		{
			if (history[i].identified == false)
			{
				history[i] = history[historySize - 1];
				
				historySize--;
			}
		}
		
	#if DO_VALIDATION
		logDebug("numEvaluated: %d", numEvaluated);
	#endif
		
		Assert(historySize == numDots);
	}
};

void testDotTracker()
{
	TrackedDot dots[kNumPoints];
	DotTracker dotTracker;
	
	TrackedDot dots_reference[kNumPoints];
	DotTracker dotTracker_reference;

	int delay = 0;
	
	float time = 0.f;
	
	uint64_t timeAvg = 0;
	
	bool validateReference = false;
	
	bool fast = true;
	
	float maxRadius = 10.f;
	
	do
	{
		SDL_Delay(delay);
		
		//
		
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_a, true))
			delay += 20;
		if (keyboard.wentDown(SDLK_z, true))
			delay -= 5;
		
		if (keyboard.wentDown(SDLK_v))
			validateReference = !validateReference;
		
		if (keyboard.wentDown(SDLK_f))
			fast = !fast;
		
		//
		
		const float dt = std::max(1.f / 1000.f, std::min(framework.timeStep * 10.f, 1.f / 15.f));
		
		time += dt;
		
		//

		for (int i = 0; i < kNumPoints; ++i)
		{
			dots[i].x = std::sin(time / 10.f * (.123f + i / 100.f) + i / 11.f) * GFX_SX/3 + GFX_SX/2;
			dots[i].y = std::cos(time / 10.f * (.234f + i / 111.f) + i / 13.f) * GFX_SY/3 + GFX_SY/2;
		}
		
		std::random_shuffle(dots, dots + kNumPoints);
		
		//
		
		bool validationPassed = true;
		
		if (validateReference)
		{
			dotTracker_reference = dotTracker;
			
			memcpy(dots_reference, dots, sizeof(dots_reference));
		}
		
		auto t1 = g_TimerRT.TimeUS_get();
		
		if (validateReference)
		{
			dotTracker_reference.identify_reference(dots_reference, kNumPoints, dt, maxRadius);
		}
		
		if (fast)
			dotTracker.identify(dots, kNumPoints, dt, maxRadius);
		else
			dotTracker.identify_slow(dots, kNumPoints, dt, maxRadius);
		
		auto t2 = g_TimerRT.TimeUS_get();
		
		printf("identify took %.2fms. timeStep: %.2f\n", (t2 - t1) / 1000.0, dt);
		
		timeAvg = (timeAvg * 90 + (t2 - t1) * 10) / 100;
		
		if (validateReference)
		{
			for (int i = 0; i < kNumPoints; ++i)
			{
				Assert(dots[i].id == dots_reference[i].id);
				
				if (dots[i].id != dots_reference[i].id)
					validationPassed = false;
			}
		}
		
		//

		framework.beginDraw(255, 255, 255, 0);
		{
			setFont("calibri.ttf");
			
			setColor(100, 100, 100);
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < kNumPoints; ++i)
				{
					hqFillCircle(dots[i].x, dots[i].y, 15.f);
				}
			}
			hqEnd();

			setColor(255, 200, 200);
			for (int i = 0; i < kNumPoints; ++i)
			{
				drawText(dots[i].x, dots[i].y, 14, 0, 0, "%d", dots[i].id);
			}
			
			setColor(0, 0, 0);
			drawText(5, 5, 14, +1, +1, "nextAllocId: %d, delay: %dms, time: %.2fms", dotTracker.nextAllocId, delay, timeAvg / 1000.0);
			drawText(5, 25, 14, +1, +1, "validate: %d [V to toggle]. validationPassed: %d. fast: %d (F to toggle)", validateReference, validationPassed, fast);
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
}
