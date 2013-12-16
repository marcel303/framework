#include "libgui_precompiled.h"
#include "GuiContext.h"
#include "WidgetRoot.h"

namespace Gui
{
	WidgetRoot::WidgetRoot() : Widget()
	{
		SetClassName("Root");
		SetCaptureFlag(0);
	}

	void WidgetRoot::SetContext(Context* context)
	{
		Widget::SetContext(context);
	}

	GuiResult WidgetRoot::DoRender(Graphics::ICanvas* canvas)
	{
		return DelegateRender(canvas);
	}

	GuiResult WidgetRoot::DoUpdate(float deltaTime)
	{
		return DelegateUpdate(deltaTime);
	}

	GuiResult WidgetRoot::DoSerialize(IArchive& registry, int usageMask)
	{
		return Widget::DelegateSerialize(registry, usageMask);
	}

	GuiResult WidgetRoot::DoDeSerialize(IArchive& registry, int usageMask, IWidgetFactory* widgetFactory)
	{
		GuiResult result = true;

		Clear();

		if (DelegateDeSerialize(registry, usageMask, widgetFactory) != true)
			result = false;

		if (DelegateInitialize() != true)
			result = false;

		return result;
	}

	GuiResult WidgetRoot::DoInitialize()
	{
		return DelegateInitialize();
	}

	void WidgetRoot::Clear()
	{
		ClearChildList();
	}
};
