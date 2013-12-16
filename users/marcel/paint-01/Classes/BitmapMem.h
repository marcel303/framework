#pragma once

#include "IBitmap.h"

#include "BitmapRef.h" // todo, cpp
#include "Sample.h"

class MemBitmap : GG_INTERFACE_IMP(IBitmap) // todo: derive from BitmapRef?
{
public:
	MemBitmap(int sx, int sy)
	{
		m_Sx = sx;
		m_Sy = sy;
		
		Setup();
	}
	
	virtual UInt8* GetPixel(int x, int y)
	{
		int index = GetPixelIndex(x, y);
		
		return &m_Bytes[index];
	}
	
	virtual IBitmap* SubBitmap(int x, int y, int sx, int sy)
	{
		RefBitmap* result = new RefBitmap(GetPixel(x, y), sx, sy, m_Sx * 4);
		
		return result;
	}
	
	virtual void DrawLine(int x1, int x2, int y, const Paint::Col* c)
	{
#if 1
		UInt8* pixel = GetPixel(x1, y);
		
		for (int x = x1; x <= x2; ++x, ++c, pixel += 4)
		{
			pixel[0] = FIX_TO_INT(c->v[0]);
			pixel[1] = FIX_TO_INT(c->v[1]);
			pixel[2] = FIX_TO_INT(c->v[2]);
			
			pixel[3] = 255;
		}
#endif
	}
	
	UInt8* Bytes_get()
	{
		return m_Bytes;
	}
	
	void Clear(UInt8* bytes, int r, int g, int b)
	{
		int pixelCount = m_Sx * m_Sy;
		
		for (int i = 0; i < pixelCount; ++i)
		{
			UInt8* pixel = m_Bytes + i * 4;
			
			pixel[0] = r;
			pixel[1] = g;
			pixel[2] = b;
			pixel[3] = 255;
		}
	}
	
private:
	int m_Sx;
	int m_Sy;
	
	UInt8* m_Bytes;
	
	void Setup()
	{
		int pixelCount = m_Sx * m_Sy;

		m_Bytes = new UInt8[pixelCount * 4];
		
		Clear(m_Bytes, 0, 0, 0);
	}
	
	inline int GetPixelIndex(int x, int y) const
	{
		return (x + y * m_Sx) * 4;
	}
};
