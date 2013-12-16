#pragma once

#include "libgui_forward.h"
#include "Widget.h"

namespace Gui
{
	class WidgetRoot : public Widget
	{
	public:
		WidgetRoot();

		void SetContext(Context* context);

		GuiResult DoRender(Graphics::ICanvas* canvas);              ///< Render GUI.
		GuiResult DoUpdate(float deltaTime);                        ///< Update GUI.
		GuiResult DoSerialize(IArchive& registry, int usageMask);   ///< Serialize GUI to registry.
		GuiResult DoDeSerialize(IArchive& registry, int usageMask, IWidgetFactory* widgetFactory); ///< Deserialize GUI from registry.
		GuiResult DoInitialize();                                   ///< Initialize all widgets in the GUI. Issued automatically after DeSerialize.

		void Clear();                                               ///< Remove all child widgets.

	private:
		virtual GuiResult DeSerialize(IArchive& registry, int usageMask)
		{
			return true;
		}
	};
};
