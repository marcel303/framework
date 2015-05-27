#include <string.h>
#include "Debugging.h"
#include "menu.h"

MenuMgr::MenuMgr()
	: m_menuStackSize(0)
{
	memset(m_menuStack, 0, sizeof(m_menuStack));
}

MenuMgr::~MenuMgr()
{
	reset(0);
}

Menu * MenuMgr::getActiveMenu()
{
	if (m_menuStackSize > 0)
		return m_menuStack[m_menuStackSize - 1];
	else
		return 0;
}

void MenuMgr::push(Menu * menu)
{
	Assert(menu);
	Assert(m_menuStackSize < kMaxMenuStack);
	Assert(m_menuStack[m_menuStackSize] == 0);
	if (m_menuStackSize >= 1)
		m_menuStack[m_menuStackSize - 1]->onExit();
	m_menuStack[m_menuStackSize] = menu;
	m_menuStack[m_menuStackSize]->onEnter();
	m_menuStackSize++;
}

void MenuMgr::pop()
{
	Assert(m_menuStackSize > 0);
	m_menuStackSize--;
	m_menuStack[m_menuStackSize]->onExit();
	delete m_menuStack[m_menuStackSize];
	m_menuStack[m_menuStackSize] = 0;
	if (m_menuStackSize >= 1)
		m_menuStack[m_menuStackSize - 1]->onEnter();
}

void MenuMgr::reset(Menu * menu)
{
	while (m_menuStackSize > 0)
		pop();

	if (menu)
		push(menu);
}

void MenuMgr::tick(float dt)
{
	Menu * menu = getActiveMenu();

	if (menu)
	{
		if (menu->tick(dt))
			pop();
	}
}

void MenuMgr::draw()
{
	Menu * menu = getActiveMenu();

	if (menu)
		menu->draw();
}
