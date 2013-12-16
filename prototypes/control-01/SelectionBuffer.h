#pragma once

#include "types.h"

#define CD_SX 480
#define CD_SY 320
typedef unsigned char CD_TYPE;

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

	void Clear();
	void Scan_Init();

	inline CD_TYPE Get(int x, int y) const;

	void Scan_Line(Vec2 p1, Vec2 p2);
	void Scan_Triangle(const Vec2* points, int c);
	void Scan_Commit(int c);

	int yMin;
	int yMax;

	int scan[CD_SY][2];

	CD_TYPE buffer[CD_SX * CD_SY];
};

inline CD_TYPE SelectionBuffer::Get(int x, int y) const
{
	if (x < 0 || y < 0 || x >= CD_SX || y >= CD_SY)
		return 0;

	return buffer[x + y * CD_SX];
}
