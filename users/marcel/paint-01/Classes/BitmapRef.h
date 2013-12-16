#pragma once

#include "IBitmap.h"

class RefBitmap : GG_INTERFACE_IMP(IBitmap)
{
public:
	RefBitmap(UInt8* bytes, int sx, int sy, int stride)
	{
		m_Bytes = bytes;
		m_Sx = sx;
		m_Sy = sy;
		m_Stride = stride;
	}
	
	virtual UInt8* GetPixel(int x, int y)
	{
		int index = GetPixelIndex(x, y);
		
		return &m_Bytes[index];
	}
	
	virtual IBitmap* SubBitmap(int x, int y, int sx, int sy)
	{
		return 0;
	}
	
	virtual void DrawLine(int x1, int x2, int y, const Paint::Col* c)
	{
	}
	
	int Stride_get() const
	{
		return m_Stride;
	}
	
	UInt8* Bytes_get()
	{
		return m_Bytes;
	}
	
private:
	UInt8* m_Bytes;
	int m_Sx;
	int m_Sy;
	int m_Stride;
	
	virtual int GetPixelIndex(int x, int y)
	{
		return x + y * m_Stride;
	}
};
