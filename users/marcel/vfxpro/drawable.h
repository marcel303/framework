#pragma once

#include "framework.h"
#include <algorithm>

#ifdef WIN32
	#include <malloc.h>
#endif

struct Drawable
{
	float m_z;

	Drawable(float z)
		: m_z(z)
	{
	}

	virtual ~Drawable()
	{
		// nop
	}

	virtual void draw() = 0;

	bool operator<(const Drawable & other) const
	{
		return m_z > other.m_z;
	}
};

struct DrawableList
{
	static const int kMaxDrawables = 1024;

	int numDrawables;
	Drawable * drawables[kMaxDrawables];

	DrawableList()
		: numDrawables(0)
	{
	}

	~DrawableList()
	{
		// note : using a transient allocator instead of malloc/free would be faster. in practice this never became a problem so I haven't implemented it

		for (int i = 0; i < numDrawables; ++i)
		{
			delete drawables[i];
			drawables[i] = nullptr;
		}

		numDrawables = 0;
	}

	void add(Drawable * drawable)
	{
		fassert(numDrawables < kMaxDrawables);

		if (numDrawables < kMaxDrawables)
		{
			drawables[numDrawables++] = drawable;
		}
	}

	void sort()
	{
		std::stable_sort(drawables, drawables + numDrawables, [](const Drawable * d1, const Drawable * d2) { return (*d1) < (*d2); });
	}

	void draw()
	{
		for (int i = 0; i < numDrawables; ++i)
		{
			drawables[i]->draw();
		}
	}
};

inline void * operator new(size_t size, DrawableList & list)
{
	Drawable * drawable = (Drawable*)malloc(size);

	list.add(drawable);

	return drawable;
}

inline void operator delete(void * p, DrawableList & list)
{
	free(p);
}
