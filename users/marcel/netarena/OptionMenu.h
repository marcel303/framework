#pragma once

#include "config.h"
#include "MultiLevelMenu.h"

#if ENABLE_OPTIONS

class OptionBase;

class OptionMenu : public MultiLevelMenuBase
{
	// MultiLevelMenuBase
	virtual std::vector<void*> GetMenuItems() override;
	virtual const char * GetPath(void * menuItem) override;
	virtual void Select(void * menuItem) override;
	virtual void Increment(void * menuItem) override;
	virtual void Decrement(void * menuItem) override;

public:
	OptionMenu();

	void Draw(int x, int y, int sx, int sy);
};

#endif
