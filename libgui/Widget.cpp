#include "libgui_precompiled.h"
#include <algorithm>
#include "GuiConstraintSolver.h"
#include "GuiDebug.h"
//#include "GuiGraphics.h"
#include "GuiGraphicsCanvas.h"
//#include "GuiSkinFactory.h"
#include "IWidgetFactory.h"
#include "Widget.h"

namespace Gui
{
	Widget::Widget() : Object()
	{
		SetClassName("Widget");

		AddProperty(Property("GlobalName",     m_globalName,          Property::USAGE_BEHAVIOR | Property::USAGE_DATA));
		AddProperty(Property("PosX",           m_area.position.x,     Property::USAGE_LAYOUT));
		AddProperty(Property("PosY",           m_area.position.y,     Property::USAGE_LAYOUT));
		AddProperty(Property("SizeX",          m_area.size.x,         Property::USAGE_LAYOUT));
		AddProperty(Property("SizeY",          m_area.size.y,         Property::USAGE_LAYOUT));
		//AddProperty(Property("Style",          m_style,               Property::USAGE_LAYOUT));
		AddProperty(Property("Alignment",      m_alignment,           Property::USAGE_LAYOUT));
		AddProperty(Property("AlignmentIndex", m_alignmentIndex,      Property::USAGE_LAYOUT));
		AddProperty(Property("AnchorMask",     m_anchorMask,          Property::USAGE_LAYOUT));
		AddProperty(Property("MinX",           m_minSizeConstraint.x, Property::USAGE_LAYOUT));
		AddProperty(Property("MinY",           m_minSizeConstraint.y, Property::USAGE_LAYOUT));
		AddProperty(Property("MaxX",           m_maxSizeConstraint.x, Property::USAGE_LAYOUT));
		AddProperty(Property("MaxY",           m_maxSizeConstraint.y, Property::USAGE_LAYOUT));
		AddProperty(Property("Hidden",         m_isHidden,            Property::USAGE_LAYOUT));

		ADD_EVENT(OnCreate,            m_onCreate,              (this, HandleOnCreate           ));
		ADD_EVENT(OnChildAdd,          m_onChildAdd,            (this, HandleOnChildAdd         ));
		ADD_EVENT(OnChildRemove,       m_onChildRemove,         (this, HandleOnChildRemove      ));
		ADD_EVENT(OnMouseUp,           m_onMouseUp,             (this, HandleOnMouseUp          ));
		ADD_EVENT(OnMouseEnter,        m_onMouseEnter,          (this, HandleOnMouseEnter       ));
		ADD_EVENT(OnMouseLeave,        m_onMouseLeave,          (this, HandleOnMouseLeave       ));
		ADD_EVENT(OnSize,              m_onSize,                (this, HandleOnSize             ));
		ADD_EVENT(OnPlacement,         m_onPlacement,           (this, HandleOnPlacement        ));
		ADD_EVENT(OnChildMetricChange, m_onChildMetricChange,   (this, HandleOnChildMetricChange));

		m_context     = 0;
		m_parent      = 0;
		m_style       = 0;
		m_isHidden    = false;

		m_isMouseInside = false;
		m_captureFlag   = CAPTURE_ALL;
		m_destroyed     = false;

		m_alignment      = Alignment_None;
		m_alignmentIndex = 0;
		m_anchorMask     = 0;

		m_serializationFlag = true;

		//m_skinObject = 0;

		m_mtxAlignChildren = false;
		m_mtxOnChildMetricChange = false;
	}

	Widget::~Widget()
	{
		// Widget must be removed from context & linked list before destruction.
		Assert(m_context == 0);
		Assert(m_parent == 0);

		// Remove children.
		while (m_children.size() > 0)
			RemoveChild(m_children.front());

		//SetSkinObject(0);
	}

	const std::string& Widget::GetGlobalName() const
	{
		return m_globalName;
	}

	int Widget::GetStyle() const
	{
		return m_style;
	}

	Point Widget::GetPosition() const
	{
		return m_area.position;
	}

	Point Widget::GetSize() const
	{
		return m_area.size;
	}

	int Widget::GetWidth() const
	{
		return m_area.size.x;
	}

	int Widget::GetHeight() const
	{
		return m_area.size.y;
	}

	const Area& Widget::GetArea() const
	{
		return m_area;
	}

	const Area& Widget::GetClientArea() const
	{
		return const_cast<Widget*>(this)->GetClient()->GetArea();
	}

	bool Widget::IsHidden() const
	{
		return m_isHidden;
	}

	Widget* Widget::GetParent() const
	{
		return m_parent;
	}

	Context* Widget::GetContext() const
	{
		return m_context;
	}

	const Widget::ChildList& Widget::GetChildList() const
	{
		return m_children;
	}

	bool Widget::IsMouseInside() const
	{
		return m_isMouseInside;
	}

	int Widget::GetCaptureFlag() const
	{
		return m_captureFlag;
	}

	Widget* Widget::GetClient()
	{
		return this;
	}

	Alignment Widget::GetAlignment() const
	{
		return static_cast<Alignment>(m_alignment);
	}

	int Widget::GetAlignmentIndex() const
	{
		return m_alignmentIndex;
	}

	int Widget::GetAnchorMask() const
	{
		return m_anchorMask;
	}

	const Rect& Widget::GetAnchors() const
	{
		return m_anchors;
	}

	Point Widget::GetMinSizeConstraint() const
	{
		return Widget::m_minSizeConstraint;
	}

	Point Widget::GetMaxSizeConstraint() const
	{
		return Widget::m_maxSizeConstraint;
	}

	bool Widget::GetSerializationFlag() const
	{
		return m_serializationFlag;
	}

	/*SkinObject* Widget::GetSkinObject() const
	{
		return m_skinObject;
	}*/

	/*Graphics::ShCursor Widget::GetMouseCursor() const
	{
		return m_mouseCursor;
	}*/

	void Widget::SetGlobalName(const std::string& name)
	{
		DO_EVENT(OnGlobalNameChangeBegin, (this));

		m_globalName = name;

		DO_EVENT(OnGlobalNameChange, (this));
	}

	void Widget::SetStyle(int style)
	{
		//Assert(m_parent == 0);

		m_style = style;

		if (m_parent)
			m_parent->SortChildren();
	}

	void Widget::SetPosition(int x, int y)
	{
		if (m_area.position.x == x && m_area.position.y == y)
			return;

		m_area.position = Point(x, y);

		if (m_parent)
			DO_EVENT(m_parent->OnChildMetricChange, (this));
	}

	void Widget::SetSize(int width, int height)
	{
		Point size = Point(width, height);

		size = ConstraintSolver::I().GetConstrainedSize(this, size);

		if (size.x == m_area.size.x && size.y == m_area.size.y)
			return;

		m_area.size = size;

		DO_EVENT(OnSize, (this));

		if (m_parent)
			DO_EVENT(m_parent->OnChildMetricChange, (this));
	}

	void Widget::SetIsHidden(bool hidden)
	{
		m_isHidden = hidden;
	}

	void Widget::SetIsMouseInside(bool inside)
	{
		m_isMouseInside = inside;

		Repaint();
	}

	void Widget::SetCaptureFlag(int captureFlag)
	{
		m_captureFlag = captureFlag;
	}

	void Widget::SetAlignment(Alignment alignment)
	{
		if (alignment == m_alignment)
			return;

		m_alignment = alignment;

		if (m_parent)
			DO_EVENT(m_parent->OnChildMetricChange, (this));
	}

	void Widget::SetAnchorMask(int mask)
	{
		if (mask == m_anchorMask)
			return;

		m_anchorMask = mask;

		FixateAnchors();

		if (m_parent)
			DO_EVENT(m_parent->OnChildMetricChange, (this));
	}

	void Widget::SetAnchors(const Rect& anchors)
	{
		m_anchors = anchors;
	}

	void Widget::SetMinSizeConstraint(Point size)
	{
		m_minSizeConstraint = size;
	}

	void Widget::SetMaxSizeConstraint(Point size)
	{
		m_maxSizeConstraint = size;
	}

	void Widget::SetSerializationFlag(bool serialize)
	{
		m_serializationFlag = serialize;
	}

	/*void Widget::SetSkinObject(SkinObject* object)
	{
		if (object == m_skinObject)
			return;

		if (m_skinObject)
			delete m_skinObject;

		m_skinObject = object;

		Repaint();
	}*/

	/*void Widget::SetMouseCursor(Graphics::ShCursor cursor)
	{
		m_mouseCursor = cursor;
	}*/

	void Widget::AddChildPure(Widget* child)
	{
		Assert(child != 0);

		LinkChild(child);

		DO_EVENT(OnChildAdd, (child));
	}

	void Widget::RemoveChildPure(Widget* child)
	{
		Assert(child != 0);

		DO_EVENT(OnChildRemoveBegin, (child));

		UnLinkChild(child);

		DO_EVENT(child->OnDestroy, (child));
		delete child;

		DO_EVENT(OnChildRemove, (child));
	}

	void Widget::AddChild(Widget* child)
	{
		GetClient()->AddChildPure(child);
	}

	void Widget::RemoveChild(Widget* child)
	{
		GetClient()->RemoveChildPure(child);
	}

	GuiResult Widget::RaiseChildPure(Widget* child)
	{
		Assert(child != 0);

		std::deque<Widget*>::iterator iterator;

		iterator = std::find(m_children.begin(), m_children.end(), child);

		// Child must exist in the child list.
		Assert(iterator != m_children.end());

		m_children.erase(iterator);

		m_children.push_front(child);

		SortChildren();

		return true;
	}

	GuiResult Widget::LowerChildPure(Widget* child)
	{
		Assert(child != 0);

		std::deque<Widget*>::iterator iterator;

		iterator = std::find(m_children.begin(), m_children.end(), child);

		// Child must exist in the child list.
		Assert(iterator != m_children.end());

		m_children.erase(iterator);

		m_children.push_back(child);

		SortChildren();

		return true;
	}

	Widget* Widget::FindChildPure(const std::string& name)
	{
		Widget* result = 0;

		for (size_t i = 0; i < m_children.size(); ++i)
			if (m_children[i]->GetName() == name)
				result = m_children[i];

		return result;
	}

	GuiResult Widget::RaiseChild(Widget* child)
	{
		return GetClient()->RaiseChildPure(child);
	}

	GuiResult Widget::LowerChild(Widget* child)
	{
		return GetClient()->LowerChildPure(child);
	}

	Widget* Widget::FindChild(const std::string& name)
	{
		return GetClient()->FindChildPure(name);
	}

	Point Widget::GetGlobalPosition() const
	{
		Point position = m_area.position;

		Widget* parent = m_parent;

		while (parent)
		{
			position.x += parent->GetPosition().x;
			position.y += parent->GetPosition().y;

			parent = parent->GetParent();
		}

		return position;
	}

	void Widget::Destroy()
	{
		m_destroyed = true;
	}

	void Widget::Repaint()
	{
		//if (m_skinObject)
			//m_skinObject->Repaint(this);
	}

	GuiResult Widget::Render(Graphics::ICanvas* canvas)
	{
		GuiResult result = true;

		//if (m_skinObject)
			//result = m_skinObject->Render(this);

		return result;
	}

	GuiResult Widget::Update(float deltaTime)
	{
		if (m_children.size() > 0)
		{
			static std::vector<Widget*> childrenToDestroy;

			for (size_t i = 0; i < m_children.size(); ++i)
				if (m_children[i]->m_destroyed)
					childrenToDestroy.push_back(m_children[i]);

			if (childrenToDestroy.size() > 0)
			{
				for (size_t i = 0; i < childrenToDestroy.size(); ++i)
					RemoveChild(childrenToDestroy[i]);
				childrenToDestroy.clear();
			}
		}

		DO_EVENT(OnUpdate, (deltaTime));

		return true;
	}

	GuiResult Widget::Serialize(IArchive& registry, int usageMask)
	{
		Object::Serialize(registry, usageMask);

		return true;
	}

	GuiResult Widget::DeSerialize(IArchive& registry, int usageMask, IWidgetFactory* widgetFactory)
	{
		Object::DeSerialize(registry, usageMask);

		// NOTE: Could do this in an event handler.
		FixateAnchors();

		DO_EVENT(OnSize, (this));

		return true;
	}

	GuiResult Widget::DelegateRender(Graphics::ICanvas* canvas)
	{
		bool result = true;

		if (m_isHidden == false)
		{
			canvas->GetMatrixStack().PushTranslation(static_cast<float>(m_area.position.x), static_cast<float>(m_area.position.y), 0.0f);
			{
				Point globalPosition = GetGlobalPosition();

				canvas->GetVisibleRectStack().PushClip(Rect(
					globalPosition.x,
					globalPosition.y,
					globalPosition.x + m_area.size.x - 1,
					globalPosition.y + m_area.size.y - 1)); // FIXME: This OK?
				{
					result |= Render(canvas);

					for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i)
						result |= m_children[i]->DelegateRender(canvas);
				}
				canvas->GetVisibleRectStack().Pop();
			}
			canvas->GetMatrixStack().Pop();
		}

		return result;
	}

	GuiResult Widget::DelegateUpdate(float deltaTime)
	{
		bool result = true;

		result |= Update(deltaTime);

		for (int i = static_cast<int>(m_children.size()) - 1; i >= 0; --i)
			result |= m_children[i]->DelegateUpdate(deltaTime);

		return result;
	}

	GuiResult Widget::DelegateSerialize(IArchive& registry, int usageMask)
	{
		if (m_serializationFlag == false)
			return true;

		bool result = true;

		result |= Serialize(registry, usageMask);

		const ChildList& children = GetClient()->GetChildList();

		size_t tag = 0;

		for (int i = static_cast<int>(children.size()) - 1; i >= 0; --i)
		{
			if (children[i]->GetSerializationFlag() == true)
			{
				registry.Begin("Widget", tag);
				{
					result |= children[i]->DelegateSerialize(registry, usageMask);
				}
				registry.End();

				++tag;
			}
		}

		return result;
	}

	GuiResult Widget::DelegateDeSerialize(IArchive& registry, int usageMask, IWidgetFactory* widgetFactory)
	{
		if (m_serializationFlag == false)
			return true;

		bool result = true;

		result |= DeSerialize(registry, usageMask, widgetFactory);

		for (size_t i = 0; registry.Exists("Widget", i); ++i)
		{
			registry.Begin("Widget", i);
			{
				std::string className = registry("ClassName", "");
				std::string name = registry("Name", "");
				std::string globalName = registry("GlobalName", "");

				Widget* child = 0;

				if (name != "")
					//child = FindChildPure(name);
					child = FindChild(name);

				if (child && child->GetClassName() != className)
				{
					Debug::Print("Classname mismatch: %s, Instance = %s.", className.c_str(), child->GetClassName().c_str());
					Debug::Print("\n Name: %s, Instance = %s.", name.c_str(), child->GetName().c_str());
					// Class types differ, when they may not.
					Assert(0);
					child = 0;
				}

				bool childIsNew = false;

				if (child == 0)
				{
					widgetFactory->Create(className.c_str());
					childIsNew = true;
				}

				if (child)
				{
					if (childIsNew == true)
						child->SetGlobalName(globalName);

					result |= child->DelegateDeSerialize(registry, usageMask, widgetFactory);

					if (childIsNew)
						//AddChildPure(child);
						AddChild(child);
				}
				else
				{
					result |= false;
					// Unable to create child.
					Assert(0);
				}
			}
			registry.End();
		}

		DO_EVENT(OnDeSerialize, (this));

		return result;
	}

	GuiResult Widget::DelegateInitialize()
	{
		GuiResult result = true;

		DO_EVENT(OnInitialize, (this));

		for (size_t i = 0; i < m_children.size(); ++i)
			if (m_children[i]->DelegateInitialize() != true)
				result = false;

		return result;
	}

	void Widget::SetContext(Context* context)
	{
		if (context == m_context)
			return;

		if (m_context)
			m_context->RemoveWidget(this);

		m_context = context;

		if (m_context)
			m_context->AddWidget(this);

		// Propagate context to children.
		for (size_t i = 0; i < m_children.size(); ++i)
			m_children[i]->SetContext(context);
	}

	void Widget::SetParent(Widget* parent)
	{
		if (parent == m_parent)
			return;

		m_parent = parent;

		if (m_parent)
			DO_EVENT(OnPlacement, (this));
	}

	void Widget::ClearChildList()
	{
		while (m_children.size() > 0)
			RemoveChildPure(m_children[0]);
	}

	void Widget::LinkChild(Widget* child)
	{
		Assert(child != 0);
		// The child may not exist in the child list.
		Assert(std::find(m_children.begin(), m_children.end(), child) == m_children.end());

		m_children.push_front(child);

		child->SetContext(m_context);
		child->SetParent(this);
	}

	void Widget::UnLinkChild(Widget* child)
	{
		Assert(child != 0);

		std::deque<Widget*>::iterator iterator;
		iterator = std::find(m_children.begin(), m_children.end(), child);

		// The child must exist in the child list.
		Assert(iterator != m_children.end());

		child->SetParent(0);
		child->SetContext(0);

		m_children.erase(iterator);
	}

	// Sort children/render index. ALWAYS_ON_TOP style moved to top.
	void Widget::SortChildren()
	{
		Debug::Print("%s: Sorting child list.", GetName().c_str());

		bool stop = false;
		while (!stop)
		{
			bool foundEmpty = false;
			int to = -1;
			int from = -1;

			for (int i = 0; i < static_cast<int>(m_children.size()) && (to == -1 || from == -1); ++i)
			{
				if ((m_children[i]->GetStyle() & STYLE_ALWAYS_ON_TOP) == 0)
				{
					foundEmpty = true;
					to = i;
				}
				if ((m_children[i]->GetStyle() & STYLE_ALWAYS_ON_TOP) == 1 && foundEmpty)
					from = i;
			}

			if (to == -1 || from == -1)
				stop = true;
			else
				std::swap(m_children[from], m_children[to]);
		}
	}

	void Widget::AlignChildren()
	{
		if (m_mtxAlignChildren == true)
			return;

		m_mtxAlignChildren = true;

		//Debug::Print("%s: Aligning children.", GetName().c_str());

		ConstraintSolver::I().Align(this);

		m_mtxAlignChildren = false;
	}

	void Widget::FixateAnchors()
	{
		if (m_parent == 0 || m_anchorMask == 0)
			return;

		//Debug::Print("%s: Fixating anchors.", GetName().c_str());

		const Area parentArea = m_parent->GetClientArea();
		//const Area parentArea = m_parent->GetArea();
		const Area area = GetArea();

		m_anchors.min.x = area.position.x;
		m_anchors.min.y = area.position.y;
		m_anchors.max.x = parentArea.size.x - m_anchors.min.x - area.size.x;
		m_anchors.max.y = parentArea.size.y - m_anchors.min.y - area.size.y;
	}

	void Widget::HandleOnCreate(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		//widget->SetSkinObject(SkinFactory::I().CreateSkinObject(widget->GetClassNameList()));
	}

	void Widget::HandleOnChildAdd(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		widget->AlignChildren();
	}

	void Widget::HandleOnChildRemove(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		widget->AlignChildren();
	}

	void Widget::HandleOnMouseUp(Object* me, MouseButton button, ButtonState state, MouseState* mouseState)
	{
		Widget* widget = static_cast<Widget*>(me);

		if (widget->IsMouseInside())
			DO_EVENT(widget->OnMouseClick, (widget));
	}

	void Widget::HandleOnMouseEnter(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		widget->SetIsMouseInside(true);
	}

	void Widget::HandleOnMouseLeave(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		widget->SetIsMouseInside(false);
	}

	void Widget::HandleOnSize(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		widget->AlignChildren();

		widget->Repaint();
	}

	void Widget::HandleOnPlacement(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		//if (widget->m_skinObject == 0)
			//widget->SetSkinObject(SkinFactory::I().CreateSkinObject(widget->GetClassNameList()));

		widget->FixateAnchors();
	}

	void Widget::HandleOnChildMetricChange(Object* me, Object* sender)
	{
		Widget* widget = static_cast<Widget*>(me);

		if (widget->m_mtxOnChildMetricChange == true)
			return;

		widget->m_mtxOnChildMetricChange = true;

		widget->AlignChildren();

		widget->m_mtxOnChildMetricChange = false;
	}
};
