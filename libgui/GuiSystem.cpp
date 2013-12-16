#include "GuiSystem.h"

namespace Gui
{
	ISystem* ISystem::mCurrent = 0;

	ISystem* ISystem::Current()
	{
		return mCurrent;
	}

	void ISystem::SetCurrent(ISystem* system)
	{
		mCurrent = system;
	}
}
