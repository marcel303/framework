#pragma once

#include "config.h"
#include "MultiLevelMenu.h"

#if ENABLE_OPTIONS

class OptionBase;

class OptionMenu : public MultiLevelMenuBase
{
	// MultiLevelMenuBase
	virtual std::vector<void*> GetMenuItems();
	virtual const char * GetPath(void * menuItem);
	virtual void Select(void * menuItem);
	virtual void Increment(void * menuItem);
	virtual void Decrement(void * menuItem);

public:
	OptionMenu();

	void Draw(int x, int y, int sx, int sy);
};

#endif
