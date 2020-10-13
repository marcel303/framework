#include "Debugging.h"
#include "Log.h"
#include "masking.h"
#include <algorithm>

const int Mask::kMaxSegments;

void Mask::addSegment(const float in_begin, const float in_end)
{
	if (numSegments < kMaxSegments)
	{
		begin[numSegments] = in_begin;
		end[numSegments] = in_end;
		numSegments++;
	}
	else
	{
		LOG_DBG("the number of segments exceeds the maximum number of segments!");
	}
}

void Mask::finalize()
{
	// sort begin and end positions of segments
	
	std::sort(begin, begin + numSegments);
	std::sort(end, end + numSegments);
	
	// iterate over begin and end points, keeping track of the number of overlapping segments. whenever there's
	// a transition from zero to one, it means a new segment begins. likewise, if there's and transition from one
	// to zero, it means a segment ends
	
	int overlap = 0;
	
	int begin_index = 0;
	int end_index = 0;
	
	float segment_begin;
	
	while (end_index != numSegments)
	{
		while (begin_index != numSegments && begin[begin_index] <= end[end_index])
		{
			overlap++;
			
			if (overlap == 1)
			{
				// this is an actual begin!
				
				segment_begin = begin[begin_index];
			}
			
			begin_index++;
		}
		
		while (end_index != numSegments && (begin_index == numSegments || (end[end_index] < begin[begin_index])))
		{
			overlap--;
			
			if (overlap == 0)
			{
				// this is an actual end!
				
				const float segment_end = end[end_index];
				
				Assert(numMergedSegments < kMaxSegments);
				if (numMergedSegments < kMaxSegments)
				{
					mergedSegments[numMergedSegments].begin = segment_begin;
					mergedSegments[numMergedSegments].end = segment_end;
					numMergedSegments++;
				}
				else
				{
					LOG_DBG("the number of merged segments exceeds the maximum number of segments!");
				}
			}
			
			end_index++;
			
			Assert(end_index <= begin_index);
		}
	}
	
	Assert(begin_index == numSegments);
	Assert(end_index == numSegments);
}
