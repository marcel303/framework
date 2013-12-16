#pragma once

#include "GuiEH.h"

namespace Gui
{
	class Object;

	typedef void (*UpdateCallback)(Object* me, float deltaTime);

	class EHUpdate : public EH
	{
	public:
		inline EHUpdate() : EH()
		{
		}

		inline void Initialize(Object* me, UpdateCallback callback)
		{
			SetMe(me);
			m_callback = callback;
		}

		inline void Do(float deltaTime)
		{
			// Callback must be set.
			Assert(m_callback != 0);

			m_callback(GetMe(), deltaTime);
		}

	private:
		UpdateCallback m_callback;
	};
};
