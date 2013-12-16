#pragma once

#include <vector>
#include "Types.h"

class RectSetI
{
public:
	void Add(const RectI& rect);
	void Clear();

	inline int Count_get() const
	{
		return m_Rects.size();
	}

	inline const RectI& operator[](int index) const
	{
		return m_Rects[index];
	}

private:
	std::vector<RectI> m_Rects;
};
