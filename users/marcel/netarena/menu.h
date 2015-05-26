#pragma once

class Menu
{
public:
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
};
