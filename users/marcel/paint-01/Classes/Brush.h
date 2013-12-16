#pragma once

#include "Calc.h" // todo, cpp
#include "Sample.h"

#define BRUSH_MAXSX 128
#define BRUSH_MAXSY 128

// TODO: Store inverse brush value alongside brush value..

namespace Paint
{
	enum BrushType
	{
		BrushType_Circle
	};

	class Brush
	{
	public:
		Brush();

		void MakeCircle(int s, Fix hardness);
		void Calc();
		Fix CalcCircle(Fix x, Fix y) const;

		inline Fix* GetLine(int y)
		{
			return m_Pixs[y];
		}

		inline Fix* GetPix(int x, int y)
		{
			return GetLine(y) + x;
		}

		Fix m_Pixs[BRUSH_MAXSY][BRUSH_MAXSX];
		int m_Sx;
		int m_Sy;
		Fix m_MidX;
		Fix m_MidY;
		int m_OffsetX;
		int m_OffsetY;
		Fix m_Hardness;
		BrushType m_Type;
	};
};
