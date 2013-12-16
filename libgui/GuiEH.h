#pragma once

//#include "Debugging.h"
#include "GuiInterfaceList.h"

namespace Gui
{
	class Object;

	class EH
	{
	public:
		inline EH()
		{
			m_me = 0;
		}

	protected:
		inline Object* GetMe()
		{
			return m_me;
		}

		inline void SetMe(Object* me)
		{
			m_me = me;
		}

	private:
		Object* m_me;
	};
};

#define DECLARE_EVENT(type, name) Gui::InterfaceList<type> name
#define DO_EVENT(event, args) INTERFACE_LIST_CALL(event, Do, args)
#define ADD_EVENT(event, handler, initargs) { handler.Initialize initargs; event += &handler; }
