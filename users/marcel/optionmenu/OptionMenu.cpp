#include "framework.h"
#include "OptionMenu.h"
#include "Options.h"
#include "StringEx.h"

OptionMenu::OptionMenu()
	: MultiLevelMenuBase()
{
	Create();
}

//

std::vector<void*> OptionMenu::GetMenuItems()
{
	std::vector<void*> result;

	for (OptionBase * option = g_optionManager.m_head; option != 0; option = option->GetNext())
		if (!option->HasFlags(OPTION_FLAG_HIDDEN))
			result.push_back(option);

	return result;
}

const char * OptionMenu::GetPath(void * menuItem)
{
	return static_cast<OptionBase*>(menuItem)->GetPath();
}

void OptionMenu::Select(void * menuItem)
{
	static_cast<OptionBase*>(menuItem)->Select();
}

void OptionMenu::Increment(void * menuItem)
{
	static_cast<OptionBase*>(menuItem)->Increment();
}

void OptionMenu::Decrement(void * menuItem)
{
	static_cast<OptionBase*>(menuItem)->Decrement();
}

//

void OptionMenu::Draw(int x, int y, int sx, int sy)
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

	int index = 0;
	for (Node * node = m_currentNode->m_firstChild; node != 0; node = node->m_nextSibling, index++)
	{
		OptionBase * option = static_cast<OptionBase*>(node->m_menuItem);

		if (node == m_currentNode->m_currentSelection)
		{
			if (option && option->GetType() == OptionBase::kType_Command)
				setColor(255, 227, 127, 191);
			else
				setColor(127, 227, 255, 191);
			drawRect(x, y + index * lineSize, x + sx, y + (index + 1) * lineSize);
			setColor(0, 0, 0);
		}
		else
		{
			if (option && option->GetType() == OptionBase::kType_Command)
				setColor(255, 227, 127);
			else
				setColor(127, 227, 255);
		}
		drawText(x + 2, y + index * lineSize, fontSize, +1.f, +1.f, option ? "%s" : "[ %s ]", node->m_name.c_str());
		if (option)
		{
			const int bufferSize = 256;
			char buffer[bufferSize];
			option->ToString(buffer, bufferSize);

			const OptionValueAlias * alias;
			for (alias = option->GetValueAliasList(); alias != 0; alias = alias->GetNext())
				if (!strcmp(alias->GetValue(), buffer))
					break;
			if (alias)
				strcpy_s(buffer, sizeof(buffer), alias->GetAlias());

			drawText(x + sx - 1 - 2, y + index * lineSize, fontSize, -1.f, +1.f, "%s", buffer);
		}
	}

	setColor(colorWhite);
}
