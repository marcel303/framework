#pragma once

#include "GuiEH.h"
#include "GuiMessage.h"
#include "GuiObject.h"

namespace Gui
{
	typedef void (*MessageCallback)(Object* me, const Event& message);

	class EHMessage : public EH
	{
	public:
		inline EHMessage() : EH()
		{
		}

		inline void Do(const Event& message)
		{
			m_callback(GetMe(), message);
		}

		inline void Initialize(Object* me, MessageCallback callback)
		{
			SetMe(me);
			m_callback = callback;
		}

	private:
		MessageCallback m_callback;
	};
};
