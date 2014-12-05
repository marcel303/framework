#include "framework.h"
#include "StatTimerMenu.h"
#include "StatTimers.h"

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

	const int fontSize = 30;
	const int lineSize = 40;

	int numNodes = 0;

	for (Node * node = m_currentNode->m_firstChild; node != 0; node = node->m_nextSibling)
		numNodes++;

	setColor(0, 0, 0, 191);
	drawRect(x, y, x + sx, y + numNodes * lineSize);

	Font font("calibri.ttf");
	setFont(font);

	int index = 0;
	for (Node * node = m_currentNode->m_firstChild; node != 0; node = node->m_nextSibling, index++)
	{
		StatTimer * timer = static_cast<StatTimer*>(node->m_menuItem);

		if (node == m_currentNode->m_currentSelection)
		{
			setColor(127, 227, 255, 191);
			drawRect(x, y + index * lineSize, x + sx, y + (index + 1) * lineSize);
			setColor(0, 0, 0);
		}
		else
		{
			setColor(127, 227, 255);
		}
		drawText(x + 2, y + index * lineSize, fontSize, +1.f, +1.f, timer ? "%s" : "[ %s ]", node->m_name.c_str());
		if (timer)
		{
			const int bufferSize = 64;
			char buffer[bufferSize];
			sprintf_s(buffer, bufferSize, "%.2f ms - %u",
				timer->GetAverageTime() / 1000.f,
				timer->GetAverageCount());
			drawText(x + sx - 1 - 2, y + index * lineSize, fontSize, -1.f, +1.f, buffer);
		}
	}

	setColor(colorWhite);
}
