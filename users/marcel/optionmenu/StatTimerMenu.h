#pragma once

#include "MultiLevelMenu.h"

class StatTimerMenu : public MultiLevelMenuBase
{
	virtual std::vector<void*> GetMenuItems();
	virtual const char * GetPath(void * menuItem);

public:
	StatTimerMenu();

	void Draw(int x, int y, int sx, int sy);
};
