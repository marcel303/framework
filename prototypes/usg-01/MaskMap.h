#pragma once

#include "types.h"

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

		m_Values = new Int8[m_Sx * m_Sy];
	}

	BOOL Load(char* fileName);

	inline const Int8* GetLine(int y) const
	{
		return m_Values + y * m_Sx;
	}

	int m_Sx;
	int m_Sy;
	Int8* m_Values;
};
