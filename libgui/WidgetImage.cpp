#include "Precompiled.h"
#include "GuiGraphicsCanvas.h"
#include "WidgetImage.h"

namespace Gui
{
	WidgetImage::WidgetImage() : Widget()
	{
		SetClassName("Image");
		SetCaptureFlag(0);

		AddProperty(Property("ScaleMode", m_scaleMode, Property::USAGE_LAYOUT));
		AddProperty(Property("Image", m_image, Property::USAGE_LAYOUT));

		m_scaleMode = ScaleMode_None;
	}

	WidgetImage::~WidgetImage()
	{
	}

	const Graphics::Image& WidgetImage::GetImage() const
	{
		return m_image;
	}

	WidgetImage::ScaleMode WidgetImage::GetScaleMode() const
	{
		return static_cast<ScaleMode>(m_scaleMode);
	}

	void WidgetImage::SetImage(const Graphics::Image& image)
	{
		m_image = image;
	}

	void WidgetImage::SetScaleMode(WidgetImage::ScaleMode mode)
	{
		m_scaleMode = mode;
	}

	GuiResult WidgetImage::Render(Graphics::ICanvas* canvas)
	{
		Point position = GetPosition();

		RenderImage(canvas, 0, 0);

		return true;
	}

	void WidgetImage::RenderImage(Graphics::ICanvas* canvas, int x, int y)
	{
		if (m_image.GetWidth() == 0 || m_image.GetHeight() == 0)
			return;

		int width;
		int height;
		int offsetX = 0;
		int offsetY = 0;

		switch (m_scaleMode)
		{
		case ScaleMode_None:
			width = static_cast<int>(m_image.GetWidth());
			height = static_cast<int>(m_image.GetHeight());
			break;

		case ScaleMode_Proportional:
			{
				float wScale = GetWidth() / static_cast<float>(m_image.GetWidth());
				float hScale = GetHeight() / static_cast<float>(m_image.GetHeight());
				
				float scale = wScale < hScale ? wScale : hScale;
				
				float deltaX = GetWidth() - m_image.GetWidth() * scale;
				float deltaY = GetHeight() - m_image.GetHeight() * scale;	
			
				offsetX = static_cast<int>(deltaX) / 2;
				offsetY = static_cast<int>(deltaY) / 2;

				width = GetWidth() - offsetX * 2;
				height = GetHeight() - offsetY * 2;
			}
			break;

		case ScaleMode_Fit:
			width = GetWidth();
			height = GetHeight();
			break;

		default:
			width = 0;
			height = 0;
		}

		canvas->DrawImageStretch(x + offsetX, y + offsetY, width, height, m_image);
	}
}
