#pragma once

#include "CallBack.h"
#include "ColBuf.h"
#include "Rect.h"

namespace Paint
{
	typedef struct
	{
		bool dirty;
		int x1;
		int x2;
	} DirtyLine;

	typedef struct
	{
		int x1;
		int x2;
		int y;
	} DirtyEvent;

	class Dirty
	{
	public:
		Dirty();

		void Validate(int y);
		void Validate();

		void MakeDirty();
		void MakeDirty(const Rect* rect);
		void MakeDirty(int x1, int x2, int y);

		DirtyLine m_Lines[COLBUF_SY];
		CallBack m_DirtyCB;
	};
};
