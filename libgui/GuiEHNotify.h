#pragma once

#include "Debugging.h"
#include "GuiEH.h"

namespace Gui
{
	class Object;

	typedef void (*NotifyCallback)(Object* me, Object* sender);

	class EHNotify : public EH
	{
	public:
		inline EHNotify() : EH()
		{
		}

		inline void Initialize(Object* me, NotifyCallback callback)
		{
			SetMe(me);
			m_callback = callback;
		}

		inline void Do(Object* sender)
		{
			// Callback must be set.
			Assert(m_callback != 0);

			m_callback(GetMe(), sender);
		}

	private:
		NotifyCallback m_callback;
	};
};
