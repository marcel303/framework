#include <assert.h>
#include "RectSet.h"

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static RectList g_pool;

void RectSet::Add(const Rect& rect)
{
	RectNode* node = g_pool.Acquire();

	node->rect = rect;

	m_rects.Release(node);
}

void RectSet::Remove(RectNode* node)
{
	m_rects.UnLink(node);

	g_pool.Release(node);
}

void RectSet::Clear()
{
	while (m_rects.m_head)
	{
		Remove(m_rects.m_head);
	}
}

bool RectSet::AddClip(const Rect& rect)
{
	//printf("RectSet: Add: (%d, %d) - (%d, %d).\n", rect.x1, rect.y1, rect.x2, rect.y2);

	RectSet temp;

	temp.Add(rect);

	for (RectNode* node = m_rects.m_head; node && temp.m_rects.m_head; node = node->next)
		temp.Sub(node->rect);

	if (temp.m_rects.m_head == 0)
		return false;

	for (RectNode* node = temp.m_rects.m_head; node; node = node->next)
		Add(node->rect);

	return true;
}

#define ADD(x1, y1, x2, y2) \
	{ \
		Add(Rect(x1, y1, x2, y2)); \
	}

bool RectSet::Sub(const Rect& subRect)
{
	int clipL, clipR, clipT, clipB;

	for (RectNode* node = m_rects.m_head; node; )
	{
		const Rect& rect = node->rect;

		// Check if the two rectangles overlap - if not, there cannot be any substraction.

		if (rect.x1 > subRect.x2 || rect.y1 > subRect.y2 || rect.x2 < subRect.x1 || rect.y2 < subRect.y1)
		{
			node = node->next;
		}
		else
		{
			// Check if sides are clipped.

			if (subRect.x1 <= rect.x1) clipL = 1; else clipL = 0;
			if (subRect.x2 >= rect.x2) clipR = 1; else clipR = 0;
			if (subRect.y1 <= rect.y1) clipT = 1; else clipT = 0;
			if (subRect.y2 >= rect.y2) clipB = 1; else clipB = 0;

			// Add new rects.

			if (!clipL) ADD(rect.x1,      MAX(rect.y1, subRect.y1), subRect.x1-1, MIN(rect.y2, subRect.y2));
			if (!clipR) ADD(subRect.x2+1, MAX(rect.y1, subRect.y1), rect.x2,      MIN(rect.y2, subRect.y2));
			if (!clipT) ADD(rect.x1,      rect.y1,                  rect.x2,      subRect.y1-1            );
			if (!clipB) ADD(rect.x1,      subRect.y2+1,             rect.x2,      rect.y2                 );

			RectNode* next = node->next;

			Remove(node);

			node = next;
		}
	}

	return true;
}
