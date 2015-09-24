#include <string.h>
#include "Debugging.h"
#include "framework.h"
#include "menu.h"

MenuInputMode g_currentMenuInputMode = kMenuInputMode_Gamepad;

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
	static Gamepad s_oldGamepad;

	if (memcmp(&s_oldGamepad, &gamepad[0], sizeof(Gamepad)))
	{
		g_currentMenuInputMode = kMenuInputMode_Gamepad;
		s_oldGamepad = gamepad[0];
	}

	if (!keyboard.isIdle() || mouse.dx || mouse.dy)
		g_currentMenuInputMode = kMenuInputMode_Keyboard;
	
	//

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

MenuId MenuMgr::getActiveMenuId()
{
	Menu * activeMenu = getActiveMenu();

	if (activeMenu)
		return activeMenu->m_menuId;
	else
		return kMenuId_Unknown;
}
