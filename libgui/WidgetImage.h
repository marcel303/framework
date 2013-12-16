#pragma once

#include <string>
#include "libgui_forward.h"
#include "Widget.h"

//#include "GuiGraphicsColor.h"
//#include "GuiGraphicsImage.h"
//#include "GuiTypes.h"

namespace Gui
{
	class WidgetImage : public Widget
	{
	public:
		enum ScaleMode
		{
			ScaleMode_None         = 0,
			ScaleMode_Proportional = 1,
			ScaleMode_Fit          = 2
		};

		WidgetImage();
		virtual ~WidgetImage();

		const Graphics::Image& GetImage() const;
		ScaleMode GetScaleMode() const;

		void SetImage(const Graphics::Image& image);
		void SetScaleMode(ScaleMode mode);

		virtual GuiResult Render(Graphics::ICanvas* canvas);

	private:
		void RenderImage(Graphics::ICanvas* canvas, int x, int y);
		
		Graphics::Image m_image;
		int m_scaleMode;
	};
}
