#pragma once

#include "CallBack.h"
#include "MaskMap.h"
#include "types.h"

//typedef unsigned char CD_TYPE;
typedef unsigned short CD_TYPE;
#define CD_COUNT 65536

#define SELECTION_X_UNSET -1337
#define SELECTION_Y_UNSET -1337

/**
 * SelectionBuffer
 *
 * The selection buffer is an implementation of a pixel space based object picker.
 * Each object is assigned an index, which gets drawn onto the selection buffer.
 * Queries on the selection buffer yield the indices corresponding to the objects found.
 */
class SelectionBuffer
{
public:
	SelectionBuffer();
	void Initialize();
	void SetSize(int sx, int sy);

	void Clear();
	void Clear(const RectI& rect);
	void Scan_Init();

	inline CD_TYPE Get(int x, int y) const;

	void Scan_Line(Vec2F p1, Vec2F p2);
	void Scan_Triangle(const Vec2F* points, int c);
	void Scan_Circle(const Vec2F& p, float r, int c);
	void Scan_Rect(const Vec2F& p1, const Vec2F& p2, int c);
	void Scan_MaskMap(const MaskMap* mask, const Vec2F& p, int c);
	void Scan_Commit(int c);

	CallBack OnDirtyRect;

	int yMin;
	int yMax;

	class ScanInfo
	{
	public:
		int x1;
		int x2;
	};

	ScanInfo* scan;

	int m_Sx;
	int m_Sy;
	CD_TYPE* buffer;
};

inline CD_TYPE SelectionBuffer::Get(int x, int y) const
{
	if (x < 0 || y < 0 || x >= m_Sx || y >= m_Sy)
		return 0;

	return buffer[x + y * m_Sx];
}
