#include "libgui_precompiled.h"
#include "EventManager.h"
#include "GuiContext.h"
#include "GuiDebug.h"
#include "GuiGraphics.h"
#include "IWidgetFactory.h"
#include "Widget.h"
#include "WidgetRoot.h"

#undef MB_RIGHT // Windows compatibility fix.

namespace Gui
{
	Context::Context(EventManager* inputMgr, IWidgetFactory* widgetFactory)
	{
		m_onDestroy.Initialize(this, HandleOnDestroy);
		m_onNameChangeBegin.Initialize(this, HandleOnNameChangeBegin);
		m_onNameChange.Initialize(this, HandleOnNameChange);
		m_focusKeyboard = 0;
		m_focusMouse = 0;
		m_hoverMouse = 0;

		m_mouseState.Initialize(
			ButtonState_Up,
			ButtonState_Up,
			ButtonState_Up,
			0,
			0,
			0);

		m_rootWidget = new WidgetRoot;
		m_rootWidget->SetName("root");
		m_rootWidget->SetContext(this);

		//inputMgr->CL_AddInputHandler(this);
		inputMgr->AddEventHandler(this, EVENT_PRIO_INTERFACE);

		m_viewWidth = 0;
		m_viewHeight = 0;

		m_widgetFactory = widgetFactory;

		//m_clientToServerChannel = 0;
	}

	Context::~Context()
	{
		// TODO: Remove input handler.
		//InputManager::I().UnRegisterInputListener(this);

		m_rootWidget->SetContext(0);
		delete m_rootWidget;

		// Meant for debugging purposes. If there are registered widgets left, report them.
		for (std::map<Widget*, bool>::iterator i = m_registeredWidgets.begin(); i != m_registeredWidgets.end(); ++i)
			Debug::Print("Class = %s, Name = %s.", i->first->GetClassName().c_str(), i->first->GetName().c_str());

		// Upon destruction, all widgets must have been destroyed first.
		Assert(m_registeredWidgets.size() == 0);
	}

	void Context::AddWidget(Widget* widget)
	{
		Assert(widget != 0);

		if (widget->GetGlobalName() != "")
		{
			// Named widget. Must be unique. Check it isn't added yet.
			Assert(m_namedWidgets.find(widget->GetGlobalName()) == m_namedWidgets.end());

			m_namedWidgets[widget->GetGlobalName()] = widget;

			Debug::Print("Added named widget %s.", widget->GetGlobalName().c_str());
		}

		m_registeredWidgets[widget] = true;

		// NOTE: Because OnDestroy cannot be removed during RemoveWidget, it needs to be removed here.
		widget->OnDestroy         -= &m_onDestroy;
		widget->OnDestroy         += &m_onDestroy;

		// We want to know about name change events, so we can update our maps.
		widget->OnGlobalNameChangeBegin += &m_onNameChangeBegin;
		widget->OnGlobalNameChange      += &m_onNameChange;
	}

	void Context::RemoveWidget(Widget* widget)
	{
		Assert(widget != 0);
		// The widget must be registered.
		Assert(m_registeredWidgets.find(widget) != m_registeredWidgets.end());

		m_registeredWidgets.erase(widget);

		if (widget->GetGlobalName() != "")
		{
			Debug::Print("Removing %s from globally named widgets.", widget->GetGlobalName().c_str());

			// The widget must exist in the list of named widgets.
			Assert(m_namedWidgets.find(widget->GetGlobalName()) != m_namedWidgets.end());
			m_namedWidgets.erase(widget->GetGlobalName());
		}

		// NOTE: RemoveWidget called from OnDestroy handler. Cannot remove handler.
		//widget->OnDestroy -= &m_onDestructHandler;

		widget->OnNameChangeBegin -= &m_onNameChangeBegin;
		widget->OnNameChange      -= &m_onNameChange;
	}

	Widget* Context::FindWidgetByGlobalName(const std::string& name)
	{
		Widget* result = 0;
		
		std::map<std::string, Widget*>::iterator iterator;

		iterator = m_namedWidgets.find(name);

		if (iterator != m_namedWidgets.end())
			result = iterator->second;

		return result;
	}

	Widget* Context::GetWidgetAtPosition(Point position)
	{
		Widget* widget = 0;
		Widget* next = m_rootWidget;
		
		do
		{
			widget = next;
			next = 0;
			
			const Area& area = widget->GetArea();

			position.x -= area.position.x;
			position.y -= area.position.y;

			const Widget::ChildList& children = widget->GetChildList();

			for (size_t i = 0; i < children.size() && next == 0; ++i)
			{
				if (children[i]->IsHidden())
					continue;

				if (!(children[i]->GetCaptureFlag() & Widget::CAPTURE_MOUSE))
					continue;
				
				// Check for collision between child and (x, y).
				const Area& area = children[i]->GetArea();

				if (position.x >= area.position.x &&
					position.y >= area.position.y &&
					position.x <= area.position.x + area.size.x - 1 &&
					position.y <= area.position.y + area.size.y - 1)
					next = children[i];
			}	
		} while (next);
			
		return widget;
	}

	void Context::RaiseWidget(Widget* widget)
	{
		Widget* temp = widget;
		
		while (temp && temp->GetParent())
		{
			if (temp->GetStyle() & Widget::STYLE_RAISABLE)
				temp->GetParent()->RaiseChildPure(temp);
		
  			temp = temp->GetParent();
		}
	}

	WidgetRoot* Context::GetRootWidget()
	{
		return m_rootWidget;
	}

	int Context::GetViewWidth() const
	{
		return m_viewWidth;
	}

	int Context::GetViewHeight() const
	{
		return m_viewHeight;
	}

	/*Graphics::ShCursor Context::GetMouseCursor()
	{
		return m_mouseCursor;
	}*/

	Point Context::GetMousePosition()
	{
		return Point(m_mouseState.x, m_mouseState.y);
	}

	/*Channel* Context::GetClientToServerChannel()
	{
		return m_clientToServerChannel;
	}*/

	void Context::SetViewSize(int width, int height)
	{
		m_viewWidth = width;
		m_viewHeight = height;

		m_rootWidget->SetSize(width, height);

		/* FIXME GUI TODO
		if (InputManager::I().IsHardwareMouseAvailable())
		{
			int x;
			int y;

			InputManager::I().GetHardwareMousePosition(x, y);

			m_mouseState.x = x;
			m_mouseState.y = y;
		}
		else*/
		{
			m_mouseState.x = width / 2;
			m_mouseState.y = height / 2;
		}
	}

	/*void Context::SetMouseCursor(Graphics::ShCursor cursor)
	{
		m_mouseCursor = cursor;
	}*/

	/*void Context::SetClientToServerChannel(Channel* channel)
	{
		m_clientToServerChannel = channel;
	}*/

	GuiResult Context::Render(Graphics::ICanvas* canvas)
	{
		GuiResult result = true;

		//Graphics::Canvas::I().MakeCurrent();

		if (m_rootWidget->DoRender(canvas) != true)
			result = false;

		// Render mouse cursor.
		//FIXME GUI TODO
		//if (InputManager::I().IsHardwareMouseAvailable() == false || InputManager::I().IsHardwareMouseVisible() == true)
		{
			/*
			Graphics::ShCursor cursor;

			if (m_hoverMouse)
				cursor = m_hoverMouse->GetMouseCursor();

			if (!cursor)
				cursor = m_mouseCursor;

			if (cursor.get())
				cursor->Render(m_mouseState.x, m_mouseState.y);
			else
				Graphics::Canvas::I().FilledRect(m_mouseState.x, m_mouseState.y, m_mouseState.x + 10, m_mouseState.y + 10, Graphics::Color(1.0f, 0.0f, 0.0f));
			*/
		}

		//Graphics::Canvas::I().UndoMakeCurrent();

		return result;
	}

	GuiResult Context::Update(float deltaTime)
	{
		GuiResult result = true;

		if (m_rootWidget->DoUpdate(deltaTime) != true)
			result = false;

		SetMouseHover(GetWidgetAtPosition(Point(m_mouseState.x, m_mouseState.y)));

		/*
		if (m_mouseCursor.get() != 0)
			m_mouseCursor->Update(deltaTime);
		*/

		return result;
	}

	GuiResult Context::Serialize(IArchive& registry, int usageMask)
	{
		return m_rootWidget->DoSerialize(registry, usageMask);
	}

	GuiResult Context::DeSerialize(IArchive& registry, int usageMask)
	{
		return m_rootWidget->DoDeSerialize(registry, usageMask, m_widgetFactory);
	}

	GuiResult Context::Load(IArchive& registry)
	{
		return DeSerialize(registry, Property::USAGE_ANY);
	}

	GuiResult Context::Save(IArchive& registry)
	{
		if (Serialize(registry, Property::USAGE_ANY) != true)
			return false;

		return true;
	}

	void Context::SetKeyboardFocus(Widget* widget)
	{
		if (m_focusKeyboard == widget)
			return;

		//Debug::Print("Keyboard focus changed (%s -> %s).", m_focusKeyboard ? m_focusKeyboard->GetName().c_str() : "0", widget ? widget->GetName().c_str() : "0");

		if (m_focusKeyboard != 0)
			DO_EVENT(m_focusKeyboard->OnFocusLost, (this));

		m_focusKeyboard = widget;

		if (m_focusKeyboard != 0)
			DO_EVENT(m_focusKeyboard->OnFocus, (this));
	}

	void Context::SetMouseFocus(Widget* widget)
	{
		if (m_focusMouse == widget)
			return;

		//Debug::Print("Mouse focus changed (%s -> %s).", m_focusMouse ? m_focusMouse->GetName().c_str() : "0", widget ? widget->GetName().c_str() : "0");

		if (m_focusMouse != 0)
			DO_EVENT(m_focusMouse->OnMouseFocusLost, (this));

		m_focusMouse = widget;

		if (m_focusMouse != 0)
			DO_EVENT(m_focusMouse->OnMouseFocus, (this));
	}

	void Context::SetMouseHover(Widget* widget)
	{
		if (m_hoverMouse == widget)
			return;

		Debug::Print("Mouse hover changed (%s -> %s).", m_hoverMouse ? m_hoverMouse->GetName().c_str() : "0", widget ? widget->GetName().c_str() : "0");

		if (m_hoverMouse != 0)
			DO_EVENT(m_hoverMouse->OnMouseLeave, (this));

		m_hoverMouse = widget;

		if (m_hoverMouse != 0)
			DO_EVENT(m_hoverMouse->OnMouseEnter, (this));
	}

	void Context::HandleOnDestroy(Object* me, Object* object)
	{
		#if defined(DEBUG)

		Context* context = static_cast<Context*>(me);
		Widget* widget = static_cast<Widget*>(object);

		// Validate that when a widget is destroyed, it is already removed from the context.
		// This is a safety measure against abuse of widget destroys.
		Assert(context->m_registeredWidgets.find(widget) == context->m_registeredWidgets.end());

		#endif
	}

	void Context::HandleOnNameChangeBegin(Object* me, Object* sender)
	{
		Context* context = static_cast<Context*>(me);
		Widget* widget = static_cast<Widget*>(sender);

		context->RemoveWidget(widget);
	}

	void Context::HandleOnNameChange(Object* me, Object* sender)
	{
		Context* context = static_cast<Context*>(me);
		Widget* widget = static_cast<Widget*>(sender);

		context->AddWidget(widget);
	}

	/*

	TODO: At some point in time,
	      implement to remember the input's shift state &
		  translate key codes (eg, capitalize if shift is down, etc).

	enum SHIFT_STATE
	{
		SHIFT_LEFT    = 0x01,
		SHIFT_RIGHT   = 0x02,
		SHIFT_ALT     = 0x04,
		SHIFT_CONTROL = 0x08
	};

	static void TranslateKey(int shiftState, char key, char& out_key)
	{
		if (shiftState & SHIFT_LEFT || shiftState & SHIFT_RIGHT)
		{
			if (key >= 'a' && key <= 'z')
				out_key = 'A' + key - 'a';
		}
	}
	*/

	bool Context::OnEvent(Event& event)
	{
		if (event.type == EVT_KEY)
			return OnKeyEvent(event);
		if (event.type == EVT_MOUSEMOVE || event.type == EVT_MOUSEMOVE_ABS || event.type == EVT_MOUSEBUTTON)
			return OnMouseEvent(event);

		return false;
	}

	bool Context::OnKeyEvent(Event& event)
	{
		// TODO: Handled..?
		// FIXME GUI TODO: Gui requires key as ASCII..

		//Debug::Print("Context: OnKeyEvent");

		if (m_focusKeyboard)
		{
			// TODO: Add shift states.
			//       Translate key to uppercase / special char when shift.
			//TranslateKey(SHIFT_LEFT, event->key, event->key);

			if (event.key.state == BUTTON_DOWN)
				DO_EVENT(m_focusKeyboard->OnKeyDown, (event.key.key, event.key.keyCode));
			if (event.key.state == BUTTON_UP)
				DO_EVENT(m_focusKeyboard->OnKeyUp, (event.key.key, event.key.keyCode));
		}

		return false;
	}

	bool Context::OnMouseEvent(Event& event)
	{
		bool handled = false;

		if (event.type == EVT_MOUSEMOVE_ABS)
		{
			// Handle mouse movement.
			if (event.mouse_move.axis == INPUT_AXIS_X || event.mouse_move.axis == INPUT_AXIS_Y)
			{
				//printf("Axis: %d. Position: %d.\n", event.mouse_move.axis, event.mouse_move.position);

				// Mouse move.
				int deltaX = 0;
				int deltaY = 0;

#if 0
				if (event.mouse_move.axis == INPUT_AXIS_X) deltaX = event.mouse_move.position;
				if (event.mouse_move.axis == INPUT_AXIS_Y) deltaY = event.mouse_move.position;

				int newX = m_mouseState.x + deltaX;
				int newY = m_mouseState.y + deltaY;
#else
				int newX = event.mouse_move.axis == INPUT_AXIS_X ? event.mouse_move.position : m_mouseState.x;
				int newY = event.mouse_move.axis == INPUT_AXIS_Y ? event.mouse_move.position : m_mouseState.y;
#endif

				if (/*FIXME GUI TODO InputManager::I().IsHardwareMouseAvailable() == false*/1)
				{
					if (newX < 0)
						newX = 0;
					if (newY < 0)
						newY = 0;
					if (newX > m_viewWidth - 1)
						newX = m_viewWidth - 1;
					if (newY > m_viewHeight - 1)
						newY = m_viewHeight - 1;
				}

				deltaX = newX - m_mouseState.x;
				deltaY = newY - m_mouseState.y;

				m_mouseState.x = newX;
				m_mouseState.y = newY;

				//printf("MouseX: %d. MouseY: %d.\n", newX, newY);

				if (m_hoverMouse || m_focusMouse)
				{
					if (deltaX != 0 || deltaY != 0)
					{
						// Generate event.
						m_mouseState.deltaX = deltaX;
						m_mouseState.deltaY = deltaY;

						if (m_hoverMouse)
							DO_EVENT(m_hoverMouse->OnMouseOver, (m_mouseState.x, m_mouseState.y, &m_mouseState));

						if (m_focusMouse && m_focusMouse != m_hoverMouse)
							DO_EVENT(m_focusMouse->OnMouseOver, (m_mouseState.x, m_mouseState.y, &m_mouseState));

						/*
						FIXME GUI TODO
						// Deny widgets to alter the new mouse position if a hardware mouse is available.
						if (InputManager::I().IsHardwareMouseAvailable())
						{
							m_mouseState.x = newX;
							m_mouseState.y = newY;
						}*/

						//Debug::Print("OnMouseOver (%03d - %+02d %02d - %+02d %+02d).", event->position, m_mouseState.x, m_mouseState.y, deltaX, deltaY);
					}
				}

				SetMouseHover(GetWidgetAtPosition(Point(m_mouseState.x, m_mouseState.y)));

				if (m_hoverMouse)
					handled = true;

				/*
				if (m_mouseCursor.get())
					m_mouseCursor->OnMove(m_mouseState.x, m_mouseState.y, &m_mouseState);
				*/
			}

			// Handle mouse scrolling.
			if (event.mouse_move.axis == INPUT_AXIS_Z)
			{
				// Mouse scroll.
				int deltaZ = event.mouse_move.position;

				m_mouseState.z += event.mouse_move.position;

				if (m_focusMouse)
				{
					handled = true;

					// Generate event.
					m_mouseState.deltaZ = deltaZ;

					DO_EVENT(m_focusMouse->OnMouseScroll, (deltaZ, &m_mouseState));

					//Debug::Print("OnMouseScroll (%03d - %03d - %03d).", event->position, m_mouseState.z, deltaZ);
				}
			}
		}
		else if (event.type == EVT_MOUSEBUTTON)
		{
			// Handle buttons.
			INPUT_BUTTON iButton = (INPUT_BUTTON)event.mouse_button.button;
			MouseButton gButton;
			bool handleButton = true;

			switch (iButton)
			{
			case INPUT_BUTTON1: gButton = MouseButton_Left;   break;
			case INPUT_BUTTON2: gButton = MouseButton_Right;  break;
			case INPUT_BUTTON3: gButton = MouseButton_Middle; break;
			default:
				handleButton = false;
				break;
			}

			if (handleButton)
			{
				if (event.mouse_button.state == BUTTON_DOWN)
				{
					// Determine new mouse/keyboard focus widget if the user pressed the left mouse button.
					{
						Widget* clickedWidget = GetWidgetAtPosition(Point(m_mouseState.x, m_mouseState.y));
						SetMouseFocus(clickedWidget);
						SetKeyboardFocus(clickedWidget);
					}

					if (m_focusMouse)
					{
						handled = true;
						DO_EVENT(m_focusMouse->OnMouseDown, (gButton, ButtonState_Down, &m_mouseState));
						//Debug::Print("OnMouseDown.");

						RaiseWidget(m_focusMouse);
					}
				}
				if (m_focusMouse)
				{
					handled = true;

					if (event.mouse_button.state == BUTTON_UP)
					{
						DO_EVENT(m_focusMouse->OnMouseUp, (gButton, ButtonState_Up, &m_mouseState));
						//Debug::Print("OnMouseUp.");
					}
				}

				/*
				if (m_mouseCursor.get())
					m_mouseCursor->OnButton(gButton, event.mouse_button.state == BUTTON_DOWN ? ButtonState_Down : ButtonState_Up, &m_mouseState);
				*/
			}
		}

		// FIXME GUI REVERT
		handled = false;

		return handled;
	}
};
