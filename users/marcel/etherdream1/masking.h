#pragma once

// masking support. merges many masking regions with different begin/end points
// into a sorted array of non-overlapping regions, ready to be applied to a
// laser scanning line

struct MaskSegment
{
	float begin;
	float end;
};

struct Mask
{
	static const int kMaxSegments = 200;
	
	float begin[kMaxSegments];
	float end[kMaxSegments];
	int numSegments = 0;
	
	MaskSegment mergedSegments[kMaxSegments];
	int numMergedSegments = 0;
	
	void addSegment(const float in_begin, const float in_end);
	void finalize();
};
