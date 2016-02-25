#pragma once

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

	bool operator<(const Drawable * other) const
	{
		return m_z > other->m_z;
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
		// todo : use a transient allocator instead of malloc/free

		for (int i = 0; i < numDrawables; ++i)
		{
			delete drawables[i];
			drawables[i] = nullptr;
		}

		numDrawables = 0;
	}

	void add(Drawable * drawable)
	{
		assert(numDrawables < kMaxDrawables);

		if (numDrawables < kMaxDrawables)
		{
			drawables[numDrawables++] = drawable;
		}
	}

	void sort()
	{
		std::sort(drawables, drawables + numDrawables);
	}

	void draw()
	{
		for (int i = 0; i < numDrawables; ++i)
		{
			drawables[i]->draw();
		}
	}
};

void * operator new(size_t size, DrawableList & list)
{
	Drawable * drawable = (Drawable*)malloc(size);

	list.add(drawable);

	return drawable;
}

void operator delete(void * p, DrawableList & list)
{
	free(p);
}
