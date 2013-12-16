#if 0

#pragma once

#include "UsgTypes.h"

class MaskMap
{
public:
	MaskMap()
	{
		Initialize();
	}

	~MaskMap()
	{
		SetSize(0, 0);
	}

	void Initialize()
	{
		m_Sx = 0;
		m_Sy = 0;
		m_Values = 0;
	}

	void SetSize(int sx, int sy)
	{
		m_Sx = sx;
		m_Sy = sy;
		delete[] m_Values;
		m_Values = 0;

		if (sx * sy == 0)
			return;

		m_Values = new int8_t[m_Sx * m_Sy];
	}

	XBOOL Load(char* fileName);

	inline const int8_t* GetLine(int y) const
	{
		return m_Values + y * m_Sx;
	}

	int m_Sx;
	int m_Sy;
	int8_t* m_Values;
};

#endif
