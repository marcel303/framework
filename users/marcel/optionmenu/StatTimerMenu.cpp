#include "framework.h"
#include "StatTimerMenu.h"
#include "StatTimers.h"

#define HIGH_QUALITY 1

StatTimerMenu::StatTimerMenu()
	: MultiLevelMenuBase()
{
	Create();
}

std::vector<void*> StatTimerMenu::GetMenuItems()
{
	std::vector<void*> result;

	for (StatTimer * timer = g_statTimerManager.GetFirstTimer(); timer != 0; timer = timer->GetNext())
		result.push_back(timer);

	return result;
}

const char * StatTimerMenu::GetPath(void * menuItem)
{
	return static_cast<StatTimer*>(menuItem)->GetPath();
}

void StatTimerMenu::Draw(int x, int y, int sx, int sy)
{
	if (m_currentNode->m_currentSelection == 0)
		return; // empty tree

	const int fontSize = 18;
	const int lineSize = 24;
	const int captionSize = 100;

	int numNodes = 0;

	for (Node * node = m_currentNode->m_firstChild; node != 0; node = node->m_nextSibling)
		numNodes++;

	setColor(0, 0, 0, 120);
#if HIGH_QUALITY
	hqBegin(HQ_FILLED_ROUNDED_RECTS);
	hqFillRoundedRect(x, y, x + sx, y + (numNodes + 1) * lineSize, 8.f);
	hqEnd();
#else
	drawRect(x, y, x + sx, y + (numNodes + 1) * lineSize);
#endif

	int currentY = y;

	setColor(127, 227, 255);
	drawText(x + sx - 8 - 2 - captionSize, currentY + lineSize * .5f, fontSize, -1.f, 0.f, "time");
	drawText(x + sx - 8 - 2              , currentY + lineSize * .5f, fontSize, -1.f, 0.f, "count");

	currentY += lineSize;

	int index = 0;
	for (Node * node = m_currentNode->m_firstChild; node != 0; node = node->m_nextSibling, index++)
	{
		StatTimer * timer = static_cast<StatTimer*>(node->m_menuItem);

		if (node == m_currentNode->m_currentSelection)
		{
			setColor(127, 227, 255, 240);
		#if HIGH_QUALITY
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(x, currentY, x + sx, currentY + lineSize, 4.f);
			hqEnd();
		#else
			drawRect(x, currentY, x + sx, currentY + lineSize);
		#endif
			setColor(0, 0, 0);
		}
		else
		{
			setColor(127, 227, 255);
		}
		drawText(x + 2, currentY + lineSize * .5f, fontSize, +1.f, 0.f, timer ? "%s" : "[ %s ]", node->m_name.c_str());
		if (timer)
		{
			const uint32_t time = timer->GetAverageTime();
			const uint32_t count = timer->GetAverageCount();
			if (time != 0)
				drawText(x + sx - 2 - captionSize, currentY + lineSize * .5f, fontSize, -1.f, 0.f, "%.2f ms", time / 1000.f);
			drawText(x + sx - 1 - 2, currentY + lineSize * .5f, fontSize, -1.f, 0.f, "%u", count);
		}

		currentY += lineSize;
	}

	setColor(colorWhite);
}
