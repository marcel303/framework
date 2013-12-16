#include "brush.h"

namespace Paint
{
	Brush::Brush()
	{
	}

	void Brush::MakeCircle(int s, Fix hardness)
	{
		if (s % 2 == 0)
			s++;
		if (hardness == 0)
			hardness = FIX_SMALLEST_NON_ZERO;

		m_Sx = s;
		m_Sy = s;
		m_MidX = INT_TO_FIX(m_Sx) / 2;
		m_MidY = INT_TO_FIX(m_Sy) / 2;
	//	m_MidX = INT_TO_FIX(m_Sx - 1) / 2;
	//	m_MidY = INT_TO_FIX(m_Sy - 1) / 2;
		m_OffsetX = (m_Sx - 1) / 2;
		m_OffsetY = (m_Sy - 1) / 2;
		m_Hardness = hardness;
		m_Type = BrushType_Circle;

		Calc();
	}

	void Brush::Calc()
	{
		const Fix scaleX = INT_TO_FIX(2) / m_Sx;
		const Fix scaleY = INT_TO_FIX(2) / m_Sy;

#define CALC(func)                                        \
	Fix pY = FIX_MUL(- m_MidY + FIX_ONE / 2, scaleX);     \
														  \
	for (int y = 0; y < m_Sy; ++y)                        \
	{                                                     \
		Fix pX = FIX_MUL(- m_MidY + FIX_ONE / 2, scaleX); \
														  \
		Fix* pix = GetLine(y);                            \
														  \
		for (int x = 0; x < m_Sx; ++x)                    \
		{                                                 \
			Fix v = func(pX, pY);                         \
														  \
			if (v < 0)                                    \
				v = 0;                                    \
			if (v > FIX_ONE)                              \
				v = FIX_ONE;                              \
														  \
			*pix = v;                                     \
														  \
			pX += scaleX;                                 \
			pix++;                                        \
		}                                                 \
														  \
		pY += scaleY;                                     \
	}

		switch (m_Type)
		{
		case BrushType_Circle:
			CALC(CalcCircle);
			break;
		}
	}

	Fix Brush::CalcCircle(Fix x, Fix y) const
	{
#if 1
		const Fix d2 = FIX_MUL(x, x) + FIX_MUL(y, y);
		const Fix d = FIX_SQRT(d2);

		Fix v;

		if (d < m_Hardness)
			v = FIX_ONE;
		else
		{
			// v = 1 - (d - hardness) / (1 - hardness + eps)

			Fix v1 = d - m_Hardness;
			Fix v2 = FIX_INV(m_Hardness) + FIX_SMALLEST_NON_ZERO;

			v = FIX_INV(FIX_DIV(v1, v2));
		}
#else
		float xf = FIX_TO_REAL(x);
		float yf = FIX_TO_REAL(y);
		float hf = FIX_TO_REAL(m_Hardness);

		float d2 = xf * xf + yf * yf;
		float d = sqrtf(d2);

		float vf;

		if (d < hf)
			vf = 1.0f;
		else
		{
			float v1 = d - hf;
			float v2 = 1.0f - hf + 0.0001f;

			vf = 1.0f - v1 / v2;
		}

		vf *= (1.0f + sinf(xf * 2.0f * 2.0f * M_PI)) * 0.5f;
		vf *= (1.0f + sinf(yf * 2.0f * 2.0f * M_PI)) * 0.5f;

		Fix v = REAL_TO_FIX(vf);
#endif
		
		return v;
	}
};
