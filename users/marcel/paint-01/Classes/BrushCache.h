#pragma once

#include "Brush.h"
#include "Log.h"
#include "Types.h"

#define BRUSH_SIZE_LEVELS 25

namespace Paint
{
	class BrushCache
	{
	public:
		BrushCache()
		{
			m_Log = LogCtx("BrushCache");
		}

		void Prepare()
		{
			m_Log.WriteLine(LogLevel_Info, "Prepare");

			for (int i = 0; i < BRUSH_SIZE_LEVELS; ++i)
				m_CircleBrushes[i].MakeCircle(1 + i * 2, 0);
		}

#if 0
		void CreateCircle(int size, Fix hardness, Brush* brush)
		{
			Brush* ref = &m_CircleBrushes[(size - 1) / 2];

			// TODO: Rescale brush values, with regard to hardness.

			brush->MakeCircle(size, hardness);
		}
#endif
		
		Brush m_CircleBrushes[BRUSH_SIZE_LEVELS];
		LogCtx m_Log;
	};
};