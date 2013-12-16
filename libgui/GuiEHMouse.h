#pragma once

#include "GuiEH.h"
#include "GuiMouseState.h"
#include "GuiObject.h"

namespace Gui
{
	typedef void (*MouseMovementCallback)(Object* me, int x, int y, MouseState* mouseState);
	typedef void (*MouseScrollCallback)(Object* me, int deltaZ, MouseState* mouseState);
	typedef void (*MouseButtonCallback)(Object* me, MouseButton button, ButtonState state, MouseState* mouseState);

	class EHMouse : public EH
	{
	public:
		inline EHMouse() : EH()
		{
		}

		inline void Do(int x, int y, MouseState* mouseState)
		{
			// Callback must be set.
			Assert(m_movementCallback != 0);

			m_movementCallback(GetMe(), x, y, mouseState);
		}

		inline void Do(int deltaZ, MouseState* mouseState)
		{
			// Callback must be set.
			Assert(m_scrollCallback != 0);

			m_scrollCallback(GetMe(), deltaZ, mouseState);
		}

		inline void Do(MouseButton button, ButtonState state, MouseState* mouseState)
		{
			// Callback must be set.
			Assert(m_buttonCallback != 0);

			m_buttonCallback(GetMe(), button, state, mouseState);
		}

		inline void Initialize(Object* me, MouseMovementCallback callback)
		{
			Zero();
			SetMe(me);
			m_movementCallback = callback;
		}

		inline void Initialize(Object* me, MouseScrollCallback callback)
		{
			Zero();
			SetMe(me);
			m_scrollCallback = callback;
		}

		inline void Initialize(Object* me, MouseButtonCallback callback)
		{
			Zero();
			SetMe(me);
			m_buttonCallback = callback;
		}

	private:
		inline void Zero()
		{
			m_movementCallback = 0;
			m_scrollCallback = 0;
			m_buttonCallback = 0;
		}

		MouseMovementCallback m_movementCallback;
		MouseScrollCallback m_scrollCallback;
		MouseButtonCallback m_buttonCallback;
	};

	class EHMouseMovement : public EHMouse
	{
	public:
		inline EHMouseMovement() : EHMouse()
		{
		}

		inline void Initialize(Object* me, MouseMovementCallback callback)
		{
			EHMouse::Initialize(me, callback);
		}
	};

	class EHMouseScroll : public EHMouse
	{
	public:
		inline EHMouseScroll() : EHMouse()
		{
		}

		inline void Initialize(Object* me, MouseScrollCallback callback)
		{
			EHMouse::Initialize(me, callback);
		}
	};

	class EHMouseButton : public EHMouse
	{
	public:
		inline EHMouseButton() : EHMouse()
		{
		}

		inline void Initialize(Object* me, MouseButtonCallback callback)
		{
			EHMouse::Initialize(me, callback);
		}
	};
};
