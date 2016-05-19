#pragma once

enum MenuInputMode
{
	kMenuInputMode_Gamepad,
	kMenuInputMode_Keyboard
};

extern MenuInputMode g_currentMenuInputMode;

enum MenuId
{
	kMenuId_IntroScreen,
	kMenuId_Unknown
};

class Menu
{
public:
	MenuId m_menuId;

	Menu()
		: m_menuId(kMenuId_Unknown)
	{
	}

	virtual ~Menu() { }

	virtual void onEnter() { }
	virtual void onExit() { }

	virtual bool tick(float dt) { return false; }
	virtual void draw() { }
};

class MenuMgr
{
	const static int kMaxMenuStack = 4;

	Menu * getActiveMenu();

	Menu * m_menuStack[kMaxMenuStack];
	int m_menuStackSize;

public:
	MenuMgr();
	~MenuMgr();

	void push(Menu * menu);
	void pop();
	void reset(Menu * menu);

	void tick(float dt);
	void draw();

	MenuId getActiveMenuId();
};
