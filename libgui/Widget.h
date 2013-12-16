#pragma once

#include <deque>
#include <string>
#include "GuiArea.h"
#include "GuiContext.h"
#include "GuiEHKey.h"
#include "GuiEHMessage.h"
#include "GuiEHMouse.h"
#include "GuiEHNotify.h"
#include "GuiEHUpdate.h"
#include "GuiGraphicsCursor.h"
#include "GuiObject.h"
#include "GuiRect.h"
//#include "GuiSkinObject.h"

// TODO: How to call OnCreate?

namespace Gui
{
	class Widget : public Object
	{
	public:
		///< Style bitmask. Specifies the behavior and appearence of a widget.
		enum STYLE
		{
			STYLE_ALWAYS_ON_TOP = 0x01, ///< The widget will always be placed on top of other widgets.
			STYLE_RAISABLE      = 0x02  ///< The widget is raisable (will be moved to front when clicked upon).
		};
		///< Capture bitmask. The capture bitmask specifies which types of input a widget consumes. Eg, a text edit will use keyboard and mouse input, whereas an image will use none (and will seem to be transparent to input).
		enum CAPTURE
		{
			CAPTURE_MOUSE    = 0x01, ///< The widget captures mouse input.
			CAPTURE_KEYBOARD = 0x02, ///< The widget captures keyboard input.
			CAPTURE_ALL      = 0xFF  ///< The widget captures all input.
		};

		typedef std::deque<Widget*> ChildList;

		Widget();
		virtual ~Widget();

		// --------
		// Getters.
		// --------
		const std::string& GetGlobalName() const;  ///< Get global name. The global name is the name used to reference the widget using a GUI context.
		int GetStyle() const;                      ///< Get style. The style determines the appearence and behavior of the widget.
		Point GetPosition() const;                 ///< Get position. The position is relative to the parent widget.
		Point GetSize() const;                     ///< Get size. The width and height are stored in .x ad .y respectively.
		int GetWidth() const;                      ///< Get width.
		int GetHeight() const;                     ///< Get height.
		const Area& GetArea() const;               ///< Get area. The area specifies both the position and size of the widget.
		const Area& GetClientArea() const;         ///< Get area of the client area.
		bool IsHidden() const;                     ///< Return true if the widget is hidden.
		Widget* GetParent() const;                 ///< Return the parent widget. The parent widget cannot be set directly.
		Context* GetContext() const;               ///< Return the context the widget belongs to.
		const ChildList& GetChildList() const;     ///< Return a list of child widgets.
		bool IsMouseInside() const;                ///< Return true if the mouse cursor is currently hovering over the widget's area.
		int GetCaptureFlag() const;                ///< Return the capture flag. The capture flag specifies which types of input are captured y the widget.
		virtual Widget* GetClient();               ///< Get client widget. The client area is used by eg AddChild to add children to the client area. Children are actually added to the client area of a widget by default. Eg a window could implement this to reserve a special widget where other widgets are placed instead of its main client area, where probably stuff such as title bars, borders, etc are placed.
		Alignment GetAlignment() const;            ///< Get alignment. The alignment specifies whether the widget tries to stick to the left, right, top, bottom or client area of its parent.
		int GetAlignmentIndex() const;             ///< Get alignment index. The alignment index determines the order of alignment when multiple widgets have to compete. Eg, if two widgets are aligned to the left, one widget will be positioned left-most. The widget with the lowest index is moved  to the more extreme location.
		int GetAnchorMask() const;                 ///< Get anchor mask. The anchor mask specifies to which sides of the parent widget the widget will anchor itself. Widgets anchored to all sides stretch with their parent's area.
		const Rect& GetAnchors() const;            ///< Get anchor position. Left, right, top and bottom offsets are returned in min.x, max.x, min.y and max.y respectively.
		Point GetMinSizeConstraint() const;        ///< Get the minimal size contraint. The widget cannot be sized smaller than the .x and .y specified in the minimal size constraint. The minimal size contraint takes precedence of the maximal size constraint.
		Point GetMaxSizeConstraint() const;        ///< Get the maximal size contraint. The widget cannot be sized larger than the .x and .y specified in the maximal size constraint.
		bool GetSerializationFlag() const;         ///< Get the deserialization flag. The deserialization flag specifies whether the widget and its children will be serialized or not.
		//SkinObject* GetSkinObject() const;         ///< Return the skin object associated with the widget. The skin object is used to render the widget.
		//Graphics::ShCursor GetMouseCursor() const; ///< Get mouse cursor. The mouse cursor, if set, is used when the mouse points over the widget.

		// --------
		// Setters.
		// --------
		void SetGlobalName(const std::string& name);
		void SetStyle(int style);
		void SetPosition(int x, int y);
		void SetSize(int width, int height);
		void SetIsHidden(bool hidden);
		void SetIsMouseInside(bool inside);
		void SetCaptureFlag(int captureFlag);
		void SetAlignment(Alignment alignment);
		void SetAlignmentIndex(int index);
		void SetAnchorMask(int mask);
		void SetAnchors(const Rect& anchors);
		void SetMinSizeConstraint(Point size);
		void SetMaxSizeConstraint(Point size);
		void SetSerializationFlag(bool serialize);
		//void SetSkinObject(SkinObject* object);
		//void SetMouseCursor(Graphics::ShCursor cursor);

		// ----------------------
		// Child list operations.
		// ----------------------
		//
		// NOTE: A widget has a child area and its own area.
		//       For regular widgets, the child area equals itself. Eg, widgets added
		//       to the child area are added to the widget's own child list.
		//       HOWEVER, composed widgets, such as windows, may have a client
		//       area that does not equal the widget itself. Eg, the WidgetWindow
		//       class has a client area different from itself.
		//
		//       There are two classes of child list operations:
		//       - Those that (implicitly) operate on the client area.
		//       - Those that (explicitly) operate on the widget itself.
		//
		//       Usually, the implicit client area functions suffice.
		//       However, sometimes it's necessary to add widgets directly to a
		//       widget. In those cases, use the methods postfixed with 'Pure'.
		//       These methods work on the widgets child list directly.
		void AddChildPure(Widget* child);    /// Add child widget. Does not add the widget to client, but immediately to the current widget's child list.
		void RemoveChildPure(Widget* child); /// Add child widget. Does not add the widget to client, but immediately to the current widget's child list.

		void AddChild(Widget* child);        ///< Adds the specified widget to the child list of the client.
		void RemoveChild(Widget* child);     ///< Removes the specified widget from the child list of the client.

		GuiResult RaiseChildPure(Widget* child);          ///< Moves the specified wdget to the front.
		GuiResult LowerChildPure(Widget* child);          ///< Moves the specified child widget below any other widgets.
		Widget* FindChildPure(const std::string& name); ///< Finds the child widget with the specified name.

		GuiResult RaiseChild(Widget* child);              ///< Moves the specified widget, contained in the client area, to the front.
		GuiResult LowerChild(Widget* child);              ///< Moves the specified child widget, contained in the client area, below any other widgets.
		Widget* FindChild(const std::string& name);     ///< Finds the child widget, contained in the client area, with the specified name.

		// ----------------
		// Utility methods.
		// ----------------
		Point GetGlobalPosition() const; ///< Returns the global position of the widget.

		// ---------------
		// Active methods.
		// ---------------
		void Destroy(); ///< Destroy widget. This sets the destroy flag. The widget is not immediately destroyed. The widget is destroyed on the next update of the parent widget.
		void Repaint(); ///< Repaints the widget. This basically calls the Repaint method of the skin object associated with the widget.

		virtual GuiResult Render(Graphics::ICanvas* canvas); ///< Render widget. This basically calls the Render method of the skin object associated with the widget.
		GuiResult Update(float deltaTime);                   ///< Update widget. Generates the OnUpdate event and purges child widgets with the destroy flag set.

		// --------------
		// Serialization.
		// --------------
		virtual GuiResult Serialize(IArchive& registry, int usageMask);
		virtual GuiResult DeSerialize(IArchive& registry, int usageMask, IWidgetFactory* widgetFactory);

		// -----------------------------------------------
		// Delegation (hierarhical propagation) functions.
		// -----------------------------------------------
		GuiResult DelegateRender(Graphics::ICanvas* canvas);              ///< Render self and child widgets.
		GuiResult DelegateUpdate(float deltaTime);                        ///< Update self and child widgets.
		GuiResult DelegateSerialize(IArchive& registry, int usageMask);   ///< Serialize self and child widgets.
		GuiResult DelegateDeSerialize(IArchive& registry, int usageMask, IWidgetFactory* widgetFactory); ///< Deserialize self and child widgets.
		GuiResult DelegateInitialize();                                   ///< Initialize self and child widgets. The initialize method is called after deserialization.

		DECLARE_EVENT(EHNotify,        OnGlobalNameChangeBegin); ///< Generated before the widget changes its global name.
		DECLARE_EVENT(EHNotify,        OnGlobalNameChange);      ///< Generated after the widget changes its global name.
		DECLARE_EVENT(EHNotify,        OnCreate);                ///< Generated when a widget is created. Only available to widgets created with the widget factory. Must be called manually on orphaned widgets.
		DECLARE_EVENT(EHNotify,        OnDestroy);               ///< Generated when a widget is destroy. Only available to widgets owned by a parent widget. Must be called manually on orphaned widgets.
		DECLARE_EVENT(EHUpdate,        OnUpdate);                ///< Generated periodically so widgets can do some work, or animate.
		DECLARE_EVENT(EHNotify,        OnFocus);                 ///< Generated when the widget gains keyboard focus.
		DECLARE_EVENT(EHNotify,        OnFocusLost);             ///< Generated when the widget loses keyboard focus.
		DECLARE_EVENT(EHNotify,        OnMouseFocus);            ///< Generated when the widget gains mouse focus.
		DECLARE_EVENT(EHNotify,        OnMouseFocusLost);        ///< Generated when the widget loses mouse focus.
		DECLARE_EVENT(EHMouseButton,   OnMouseDown);             ///< Generated when a mouse button is pressed while the mouse cursor is over the widget.
		DECLARE_EVENT(EHMouseButton,   OnMouseUp);               ///< Generated when a mouse button is released and the widget is the mouse focus widget.
		DECLARE_EVENT(EHNotify,        OnMouseClick);            ///< Generated when the mouse is clicked on the widget.
		DECLARE_EVENT(EHMouseMovement, OnMouseOver);             ///< Generated when the mouse moves over the widget.
		DECLARE_EVENT(EHMouseScroll,   OnMouseScroll);           ///< Generated when the mouse scroll wheel is moved.
		DECLARE_EVENT(EHKeyDown,       OnKeyDown);               ///< Generated when a key is pressed.
		DECLARE_EVENT(EHKeyUp,         OnKeyUp);                 ///< Generated when a key is released.
		//DECLARE_EVENT(EHKeyRepeat,     OnKeyRepeat);             ///< Generated when a key is pressed and held down.
		DECLARE_EVENT(EHNotify,        OnMouseEnter);            ///< Generates when the mouse enters the widget area.
		DECLARE_EVENT(EHNotify,        OnMouseLeave);            ///< Generated when the mouse leaves the widget area.
		DECLARE_EVENT(EHNotify,        OnChildAdd);              ///< Generated after a child widget is added. NOTE: Sends pointer to child as sender.
		DECLARE_EVENT(EHNotify,        OnChildRemove);           ///< Generated after a child widget is removed. NOTE: Sends pointer to child as sender.
		DECLARE_EVENT(EHNotify,        OnChildRemoveBegin);      ///< Generated before a child widget is removed. NOTE: Sends pointer to child as sender.
		DECLARE_EVENT(EHNotify,        OnSize);                  ///< Generated when the size of the widget changes.
		DECLARE_EVENT(EHNotify,        OnPlacement);             ///< Generated when a widget is placed on another widget. Eg, parent->AddChild(widget). widget initiates to OnPlacement event.
		DECLARE_EVENT(EHNotify,        OnChildMetricChange);     ///< Generated when the size or position of a child widget changes.
		DECLARE_EVENT(EHNotify,        OnDeSerialize);           ///< Generated after the widget is deserialized.
		DECLARE_EVENT(EHNotify,        OnInitialize);            ///< Generated after an entire GUI has been deserialized. During this stage, widgets can retrieve other widgets by (global) name and resolve references.
		DECLARE_EVENT(EHMessage,       OnMessage);               ///< Generated when the widget receives a message. Either from another widget, or from the net.

	protected:
		void SetContext(Context* context); ///< Sets the context that owns the widget. This is a delegated function.
		void ClearChildList();             ///< Removes all child widgets.

	private:
		void SetParent(Widget* parent);  ///< Sets the parent of the widget. This is a delegated function.

		void LinkChild(Widget* child);   ///< Adds a child to the linked list of children.
		void UnLinkChild(Widget* child); ///< Removes a child from the linked list of children.

		void SortChildren();  ///< Sort children. Children with the ALWAYS_ON_TOP style are moved to top.
		void AlignChildren(); ///< Align children. This makes sure the alignment properties of the child widgets are applied.
		void FixateAnchors(); ///< Fixates the anchors of the widget. Called when the widget is first added to another widget.

	private:
		// ------------------------------
		// Event handler implementations.
		// ------------------------------
		static void HandleOnCreate(Object* me, Object* sender);
		static void HandleOnChildAdd(Object* me, Object* sender);
		static void HandleOnChildRemove(Object* me, Object* sender);
		static void HandleOnMouseUp(Object* me, MouseButton button, ButtonState state, MouseState* mouseState);
		static void HandleOnMouseEnter(Object* me, Object* sender);
		static void HandleOnMouseLeave(Object* me, Object* sender);
		static void HandleOnSize(Object* me, Object* sender);
		static void HandleOnPlacement(Object* me, Object* sender);
		static void HandleOnChildMetricChange(Object* me, Object* sender);

		// Identifiers.
		Context*    m_context;
		Widget*     m_parent;
		std::string m_globalName;

		// Children.
		ChildList m_children;

		// Layout.
		int  m_style;
		Area m_area;
		bool m_isHidden;

		// Behavior.
		bool m_isMouseInside;
		int  m_captureFlag;
		bool m_destroyed;

		// Alignment & constraints.
		int   m_alignment;
		int   m_alignmentIndex;
		int   m_anchorMask;
		Rect  m_anchors;
		Point m_minSizeConstraint;
		Point m_maxSizeConstraint;

		// Serialization.
		bool m_serializationFlag;

		// Visuals.
		//SkinObject* m_skinObject;
		//Graphics::ShCursor m_mouseCursor;

		// Mutex.
		bool m_mtxAlignChildren;
		bool m_mtxOnChildMetricChange;

		// ---------------
		// Event handlers.
		// ---------------
		EHNotify      m_onCreate;
		EHNotify      m_onChildAdd;
		EHNotify      m_onChildRemove;
		EHMouseButton m_onMouseUp;
		EHNotify      m_onMouseEnter;
		EHNotify      m_onMouseLeave;
		EHNotify      m_onSize;
		EHNotify      m_onPlacement;
		EHNotify      m_onChildMetricChange;
	};
};
