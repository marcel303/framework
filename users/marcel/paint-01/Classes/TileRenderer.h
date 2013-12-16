#pragma once

#include "BitmapMem.h"

#include <CoreGraphics/CoreGraphics.h> // todo, cpp?

class TileRenderer
{
public:
	TileRenderer(int sx, int sy, int tileSx, int tileSy)
	{
		m_Sx = sx;
		m_Sy = sy;
		m_TileSx = tileSx;
		m_TileSy = tileSy;
		
		SetupTiles();
	}
	
	inline int Sx_get() const
	{
		return m_Sx;
	}
	
	inline int Sy_get() const
	{
		return m_Sy;
	}
	
	int TileCountX_calc() const
	{
		int result = m_Sx / m_TileSx;
		
		if (m_Sx - result * m_TileSx > 0)
			result++;
		
		return result;
	}
	
	int TileCountY_calc() const
	{
		int result = m_Sy / m_TileSy;
		
		if (m_Sy - result * m_TileSy > 0)
			result++;
		
		return result;
	}
	
	inline int TileCountX_get() const
	{
		return m_TileCountX;
	}
	
	inline int TileCountY_get() const
	{
		return m_TileCountY;
	}
	
	inline int TileSx_get() const
	{
		return m_TileSx;
	}
	
	inline int TileSy_get() const
	{
		return m_TileSy;
	}
	
	MemBitmap* Bitmap_get()
	{
		return m_Bitmap;
	}
	
	UInt8* Bytes_get()
	{
		return m_Bitmap->Bytes_get();
	}
	
	void MarkAsbDirty(int x1, int y1, int x2, int y2)
	{
		int tileX1 = x1 / m_TileSx;
		int tileY1 = y1 / m_TileSy;
		int tileX2 = x2 / m_TileSx;
		int tileY2 = y2 / m_TileSy;
		
		for (int y = tileY1; y <= tileY2; ++y)
		{
			for (int x = tileX1; x <= tileX2; ++x)
			{
				int index = GetTileIndex(x, y);
				
				m_Tiles[index].IsDirty = true;
			}
		}
	}

private:
	int m_Sx;
	int m_Sy;
	int m_TileSx;
	int m_TileSy;
	int m_TileCountX;
	int m_TileCountY;
	
	MemBitmap* m_Bitmap;
	
public:
	class Tile
	{
	public:
		Tile()
		{
			Context = 0;
			IsDirty = false;
		}
		
		IBitmap* Bitmap;
		CGContextRef Context;
		CGImageRef Image;
		CGRect Rect;
		bool IsDirty;
	};
	
	Tile* m_Tiles;
	CGImageRef m_Image;
	
private:
	void SetupTiles()
	{
		m_Bitmap = new MemBitmap(m_Sx, m_Sy);
		
		m_TileCountX = TileCountX_calc();
		m_TileCountY = TileCountY_calc();
		
		int tileCount = m_TileCountX * m_TileCountY;
		
		m_Tiles = new Tile[tileCount];
		
//		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceGray();
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		
		m_Image = CGImageCreate(
			m_Sx,
			m_Sy, 8, 32, m_Sx * 4, colorSpace, 0, CGDataProviderCreateWithData(0, m_Bitmap->GetPixel(0, 0), m_Sx * m_Sy * 4, 0), 0, false, kCGRenderingIntentDefault);
		
		for (int y = 0; y < m_TileCountY; ++y)
		{
			for (int x = 0; x < m_TileCountX; ++x)
			{
				int index = GetTileIndex(x, y);
					
				RefBitmap* subBitmap = (RefBitmap*)m_Bitmap->SubBitmap(x * m_TileSx, y * m_TileSy, m_TileSx, m_TileSy);
				
				m_Tiles[index].Bitmap = subBitmap;
				
				UInt8* bytes = subBitmap->Bytes_get();
				
				m_Tiles[index].Context = CGBitmapContextCreate(bytes, m_TileSx, m_TileSy, 8, subBitmap->Stride_get(), colorSpace, kCGImageAlphaPremultipliedLast);
//				m_Tiles[index].Image = CGBitmapContextCreateImage(m_Tiles[index].Context);
				
				m_Tiles[index].Rect = CGRectMake(
					x * m_TileSx,
					y * m_TileSy,
					m_TileSx,
					m_TileSy);
			}
		}
		
		CGColorSpaceRelease(colorSpace);
	}
	
public:
	inline int GetTileIndex(int tileX, int tileY) const
	{
		return tileX + tileY * TileCountX_get();
	}
};
