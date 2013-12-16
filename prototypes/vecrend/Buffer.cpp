#include "Precompiled.h"
#include <string.h>
#include "Buffer.h"
//#include "SIMD.h"
#include "Types.h"

Buffer::Buffer()
{
	m_Sx = 0;
	m_Sy = 0;
	m_Values = 0;
}

Buffer::~Buffer()
{
	SetSize(0, 0);
}

void Buffer::SetSize(int sx, int sy)
{
	m_Sx = sx;
	m_Sy = sy;
	delete[] m_Values;
	m_Values = 0;

	if (m_Sx * m_Sy == 0)
		return;

	m_Values = new float[sx * sy * 4];
}

void Buffer::Clear(float v)
{
	for (int n = m_Sx * m_Sy * 4, i = 0; n > 0; --n, ++i)
	{
		m_Values[i] = v;
	}
}

Buffer* Buffer::DownScale(int scale) const
{
	const int sx = m_Sx / scale;
	const int sy = m_Sy / scale;
	const int area = scale * scale;
	//const SimdVec areaRcp(1.0f / area);
	const float areaRcp = 1.0f / area;

	Buffer* result = new Buffer();
	result->SetSize(sx, sy);

	for (int y = 0; y < sy; ++y)
	{
		float* dline = result->GetLine(y);

		const int y1 = (y + 0) * scale;
		const int y2 = (y + 1) * scale;

		for (int x = 0; x < sx; ++x)
		{
			const int x1 = (x + 0) * scale;
			const int x2 = (x + 1) * scale;

			//SimdVec rgba(VZERO);
			float rgba[4] = { 0.0f };

			for (int sy = y1; sy < y2; ++sy)
			{
				const float* sline = GetLine(sy) + (x1 << 2);

				for (int sx = x1; sx < x2; ++sx)
				{
					//rgba = rgba.Add(SimdVec(sline[0], sline[1], sline[2], sline[3]));
					rgba[0] += sline[0];
					rgba[1] += sline[1];
					rgba[2] += sline[2];
					rgba[3] += sline[3];

					sline += 4;
				}
			}

			//rgba = rgba.Mul(areaRcp);

			//rgba.Store(dline);
			
			dline[0] = rgba[0] * areaRcp;
			dline[1] = rgba[1] * areaRcp;
			dline[2] = rgba[2] * areaRcp;
			dline[3] = rgba[3] * areaRcp;

			dline += 4;
		}
	}

	return result;
}

Buffer* Buffer::Scale(int scale) const
{
	Buffer* result = new Buffer();
	result->SetSize(m_Sx * scale, m_Sy * scale);

	for (int y = 0; y < m_Sy; ++y)
	{
		const float* sline = GetLine(y);

		for (int x = 0; x < m_Sx; ++x)
		{
			const int x1 = (x + 0) * scale;
			const int y1 = (y + 0) * scale;
			const int x2 = (x + 1) * scale;
			const int y2 = (y + 1) * scale;

			for (int dy = y1; dy < y2; ++dy)
			{
				float* dline = result->GetLine(dy) + x1 * 4;

				for (int dx = x1; dx < x2; ++dx)
				{
					memcpy(dline, sline, sizeof(float) * 4);

					dline += 4;
				}
			}

			sline += 4;
		}
	}

	return result;
}

void Buffer::Modulate(const float* rgb)
{
	const int area = m_Sx * m_Sy;

	float* pixel = m_Values;

	for (int i = 0; i < area; ++i)
	{
		pixel[0] *= rgb[0];
		pixel[1] *= rgb[1];
		pixel[2] *= rgb[2];

		pixel += 4;
	}
}

void Buffer::ModulateAlpha(float a)
{
	const int area = m_Sx * m_Sy;

	float* pixel = m_Values;

	for (int i = 0; i < area; ++i)
	{
		pixel[3] *= a;

		pixel += 4;
	}
}

void Buffer::Combine(Buffer* other)
{
	if (m_Sx != other->m_Sx || m_Sy != other->m_Sy)
		throw ExceptionVA("size mismatch");

	for (int y = 0; y < m_Sy; ++y)
	{
		const float* sline = other->GetLine(y);
		float* dline = GetLine(y);

		for (int x = 0; x < m_Sx; ++x)
		{
#if 0
			for (int i = 0; i < 4; ++i)
				if (sline[i] > dline[i])
					dline[i] = sline[i];
#else
			const float t1 = sline[3];
			const float t2 = 1.0f - t1;

			for (int i = 0; i < 3; ++i)
			{
				dline[i] = t1 * sline[i] + t2 * dline[i];
			}

			if (sline[3] > dline[3])
				dline[3] = sline[3];
#endif

			sline += 4;
			dline += 4;
		}
	}
}

void Buffer::Combine(Buffer* other, int x, int y)
{
	int dx1 = x;
	int dy1 = y;
	int dx2 = x + other->m_Sx - 1;
	int dy2 = y + other->m_Sy - 1;

	int sx1 = 0;
	int sy1 = 0;
	int sx2 = other->m_Sx - 1;
	int sy2 = other->m_Sy - 1;

	if (dx1 < 0)
	{
		sx1 -= dx1;
		dx1 -= dx1;
	}

	if (dy1 < 0)
	{
		sy1 -= dy1;
		dy1 -= dy1;
	}

	if (dx2 > m_Sx - 1)
	{
		sx2 -= dx2 - (m_Sx - 1);
		dx2 -= dx2 - (m_Sx - 1);
	}

	if (dy2 > m_Sy - 1)
	{
		sy2 -= dy2 - (m_Sy - 1);
		dy2 -= dy2 - (m_Sy - 1);
	}

	for (int dy = dy1, sy = sy1; dy <= dy2; ++dy, ++sy)
	{
		const float* sline = other->GetLine(sy) + sx1 * 4;
		float* dline = GetLine(dy) + dx1 * 4;

		for (int dx = dx1; dx <= dx2; ++dx)
		{
			const float t1 = sline[3];
			const float t2 = 1.0f - t1;

			for (int i = 0; i < 3; ++i)
			{
				dline[i] = t1 * sline[i] + t2 * dline[i];
			}

			if (sline[3] > dline[3])
				dline[3] = sline[3];

			sline += 4;
			dline += 4;
		}
	}
}

Buffer* Buffer::Blur(int strength)
{
	Buffer* buffer = new Buffer();
	
	buffer->SetSize(m_Sx, m_Sy);
	
	for (int y = 0; y < m_Sy; ++y)
	{
		float* dline = buffer->GetLine(y);
		
		for (int x = 0; x < m_Sx; ++x)
		{
			float rgba[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
			int n = 0;

			for (int oy = -strength; oy <= +strength; ++oy)
			{
				for (int ox = -strength; ox <= +strength; ++ox)
				{
					bool hasValue = false;
					float* value;
					
					GetValue_Ex(x + ox, y + oy, &value, hasValue);
					
					if (hasValue)
					{
						for (int j = 0; j < 4; ++j)
							rgba[j] += value[j];
					}
					
					n++;
				}
			}
			
			if (n > 0)
			{
				for (int j = 0; j < 4; ++j)
					dline[j] = rgba[j] / n;
			}
			
			dline += 4;
		}
	}
	
	return buffer;
}

void Buffer::DemultiplyAlpha()
{
	for (int y = 0; y < m_Sy; ++y)
	{
		float* dline = GetLine(y);

		for (int x = 0; x < m_Sx; ++x)
		{
			const float t = dline[3];

			if (t > 0.0f)
			{
				for (int i = 0; i < 3; ++i)
				{
					dline[i] = dline[i] / t;

					if (dline[i] > 1.0f)
						dline[i] = 1.0f;
				}
			}

			dline += 4;
		}
	}
}
