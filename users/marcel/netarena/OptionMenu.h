#pragma once

#include "MultiLevelMenu.h"

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
	virtual ~OptionMenu();

	void Draw(int x, int y, int sx, int sy);
};
