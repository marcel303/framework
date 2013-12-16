#pragma once

class SplitInfo
{
public:
	SplitInfo()
	{
		FrontCount = 0;
		BackCount = 0;
		SpanCount = 0;
	}

	int FrontCount;
	int BackCount;
	int SpanCount;
};

class BspNode
{
public:
	void Split()
	{
	}

	bool FindPlane(Plane* o_Plane)
	{
		float bestHueristic = 1000000.0f;

		for (int i = 0; i < Polygons.size(); ++i)
		{
			SplitInfo si = CalcSplitInfo(
		}
	}

	SplitInfo CalcSplitInfo(const Plane& plane)
	{
		SplitInfo si;

		for (int i = 0; i < Polygons.size(); ++i)
		{
			Compare compare = Polygons[i].Compare(plane);

			if (compare == Compare_Front)
				si.FrontCount++;
			if (compare == Compare_Back)
				si.BackCount++;
			if (compare == Compare_Span)
			{
				si.FrontCount++;
				si.BackCount++;
				si.SpanCount++;
			}
		}

		return si;
	}

	float CalcHueristic(const SplitInfo& si)
	{
		return abs(si.FrontCount - si.BackCount) + si.SpanCount;
	}

	Poly Poly;
};

class BspTree
{
public:
};

class Convexinator
{
public:
	std::vector<Poly> Convexinate()
	{
		std::vector<Poly> result;

		return result;
	}
};
