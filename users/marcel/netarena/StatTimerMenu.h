#pragma once

#include "config.h"
#include "MultiLevelMenu.h"

#if ENABLE_OPTIONS

class StatTimerMenu : public MultiLevelMenuBase
{
	virtual std::vector<void*> GetMenuItems() override;
	virtual const char * GetPath(void * menuItem) override;

public:
	StatTimerMenu();

	void Draw(int x, int y, int sx, int sy);
};

#endif
