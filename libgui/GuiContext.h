#ifndef __GUICONTEXT_H__
#define __GUICONTEXT_H__

#include <map>
#include <string>
//#include "Channel.h"
#include "EventHandler.h"
#include "GuiEHNotify.h"
//#include "GuiGraphicsCursor.h"
#include "GuiMouseState.h"
#include "GuiObject.h"
#include "GuiPoint.h"
#include "libgui_forward.h"
#include "libiphone_forward.h"
#include "Types.h"

namespace Gui
{
	class Widget;
	class WidgetRoot;

	class Context : public Object, public EventHandler
	{
	public:
		Context(EventManager* eventMgr, IWidgetFactory* widgetFactory);
		~Context();

		// -------------------
		// Management methods.
		// -------------------
		void AddWidget(Widget* widget);    ///< Add a widget to the context. Only Widget ought to call this method.
		void RemoveWidget(Widget* widget); ///< Remove a widget from the context. Only Widget ought to call this method.

		// ----------------
		// Utility methods.
		// ----------------
		Widget* FindWidgetByGlobalName(const std::string& name); ///< Get a widget by referencing it using its global name.
		Widget* GetWidgetAtPosition(Point position);             ///< Find out which widget is located at the specified location.
		void RaiseWidget(Widget* widget);                        ///< Raise the specified widget to the top.

		// --------
		// Getters.
		// --------
		WidgetRoot* GetRootWidget();         ///< Get access the root widget.
		int GetViewWidth() const;            ///< Get the width of the view.
		int GetViewHeight() const;           ///< Get the height of the view.
		//Graphics::ShCursor GetMouseCursor(); ///< Get default mouse cursor.
		Point GetMousePosition();            ///< Get mouse position.
		//Channel* GetClientToServerChannel(); ///< Get channel used to communicate to the server.

		// --------
		// Setters.
		// --------
		void SetViewSize(int width, int height);         ///< Set size of the view, in pixel. This must be equal to the size of the display/window.
		//void SetMouseCursor(Graphics::ShCursor cursor);  ///< Set default mouse cursor. The default mouse cursor is used, unless the widget below the mouse cursor specifies a custom cursor.
		//void SetClientToServerChannel(Channel* channel); ///< Set channel to use to communicate with the server.

		// ---------------
		// Active methods.
		// ---------------
		GuiResult Render(Graphics::ICanvas* canvas); ///< Render GUI context.
		GuiResult Update(float deltaTime);           ///< Update GUI context.

		GuiResult Serialize(IArchive& registry, int usageMask);   ///< Serialize the GUI context.
		GuiResult DeSerialize(IArchive& registry, int usageMask); ///< Deserialize the GUI context.

		GuiResult Load(IArchive& registry);
		GuiResult Save(IArchive& registry);

	private:
		void SetKeyboardFocus(Widget* widget); ///< Set the keyboard focus widget.
		void SetMouseFocus(Widget* widget);    ///< Set the mouse focus widget.
		void SetMouseHover(Widget* widget);    ///< Set the mouse hover widget.

		// ------------------------------
		// Event handler implementations.
		// ------------------------------
		static void HandleOnDestroy(Object* me, Object* object);
		static void HandleOnNameChangeBegin(Object* me, Object* sender);
		static void HandleOnNameChange(Object* me, Object* sender);

		virtual bool OnEvent(Event& event);                ///< Implementation of InputHandler. Responds to keyboard & mouse input.
		virtual bool OnKeyEvent(Event& event);     ///< Responds to keyboard input.
		virtual bool OnMouseEvent(Event& event); ///< Responds to mouse input.

		WidgetRoot* m_rootWidget;    ///< Root widget.
		Widget*     m_focusKeyboard; ///< Widget that has the keyboard focus. Key events are sent to this widget.
		Widget*     m_focusMouse;    ///< Widget that has the mouse focus. Mouse events are sent to this widget.
		Widget*     m_hoverMouse;    ///< Widget that has the mouse hover focus. Mouse movement events are sent to this widget.

		std::map<std::string, Widget*> m_namedWidgets;      ///< List of widgets that can be referenced by a global name.
		std::map<Widget*, bool>        m_registeredWidgets; ///< List of all registered widgets.

		MouseState m_mouseState; ///< Current mouse state (position, buttons, etc).

		int m_viewWidth;                  ///< Width of the view.
		int m_viewHeight;                 ///< Height of the view.
		//Graphics::ShCursor m_mouseCursor; ///< Default mouse cursor.

		IWidgetFactory* m_widgetFactory;
		//Channel* m_clientToServerChannel; ///< Channel used to communicate between the client and the server.

		// ---------------
		// Event handlers.
		// ---------------
		EHNotify m_onDestroy;
		EHNotify m_onNameChangeBegin;
		EHNotify m_onNameChange;
	};
};

#endif
