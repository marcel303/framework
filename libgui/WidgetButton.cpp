#include "Precompiled.h"
#include "WidgetButton.h"
#include "WidgetImage.h"

// TODO: Make image a parameter.

#define BORDER_SIZE 4

namespace Gui
{
	WidgetButton::WidgetButton() : Widget()
	{
		SetClassName("Button");
		SetCaptureFlag(CAPTURE_MOUSE | CAPTURE_KEYBOARD);

		AddProperty(Property("State", m_state, Property::USAGE_DATA    ));
		AddProperty(Property("Mode",  m_mode,  Property::USAGE_BEHAVIOR));
		AddProperty(Property("Text",  m_text,  Property::USAGE_LAYOUT  ));
		AddProperty(Property("Font",  m_font,  Property::USAGE_LAYOUT  ));

		ADD_EVENT(OnMouseDown, m_onMouseDown, (this, HandleMouseDown));
		ADD_EVENT(OnMouseUp,   m_onMouseUp,   (this, HandleMouseUp  ));

		SetSize(32, 32);

		m_state = ButtonState_Up;
		m_mode = ButtonMode_PushButton;

		m_image = new WidgetImage;
		m_image->SetName("Image");
		m_image->SetSerializationFlag(false);
		m_image->SetPosition(4, 4);
		m_image->SetSize(32 - BORDER_SIZE * 2, 32 - BORDER_SIZE * 2);
		m_image->SetAnchorMask(Anchor_All);
		AddChild(m_image);

		AddProperty(Property("Image", const_cast<Graphics::Image&>(m_image->GetImage()), Property::USAGE_LAYOUT));
	}

	WidgetButton::~WidgetButton()
	{
	}

	ButtonState WidgetButton::GetState() const
	{
		return static_cast<ButtonState>(m_state);
	}

	WidgetButton::ButtonMode WidgetButton::GetMode() const
	{
		return static_cast<ButtonMode>(m_mode);
	}

	const std::string& WidgetButton::GetText() const
	{
		return m_text;
	}

	Graphics::Font& WidgetButton::GetFont()
	{
		return m_font;
	}

	const Graphics::Image& WidgetButton::GetImage() const
	{
		return m_image->GetImage();
	}

	void WidgetButton::SetState(ButtonState state)
	{
		if (state == m_state)
			return;

		switch (state)
		{
			case ButtonState_Down:
			{
				m_state = state;

				switch (m_mode)
				{
					case ButtonMode_ToggleButton:
					{
						DO_EVENT(OnToggle, (this));
					}
					break;

					default:
					break;
				}

				DO_EVENT(OnDown, (this));
			}
			break;

			case ButtonState_Up:
			{
				m_state = state;

				switch (m_mode)
				{
					case ButtonMode_PushButton:
					{
						if (IsMouseInside())
						{
							DO_EVENT(OnClick, (this));
						}
					}
					break;

					case ButtonMode_ToggleButton:
					{
						DO_EVENT(OnToggle, (this));
					}
					break;
				}

				DO_EVENT(OnUp, (this));
			}
			break;
		}

		Repaint();
	}

	void WidgetButton::SetMode(ButtonMode mode)
	{
		m_mode = mode;

		Repaint();
	}

	void WidgetButton::SetText(const std::string& text)
	{
		m_text = text;

		Repaint();
	}

	void WidgetButton::SetFont(const Graphics::Font& font)
	{
		m_font = font;

		Repaint();
	}

	void WidgetButton::SetImage(const Graphics::Image& image)
	{
		m_image->SetImage(image);

		Repaint();
	}

	void WidgetButton::HandleMouseDown(Object* me, MouseButton button, ButtonState state, MouseState* mouseState)
	{
		if (button == MouseButton_Left)
		{
			WidgetButton* button = static_cast<WidgetButton*>(me);
			button->HandleMouseDown();
		}
	}

	void WidgetButton::HandleMouseUp(Object* me, MouseButton button, ButtonState state, MouseState* mouseState)
	{
		if (button == MouseButton_Left)
		{
			WidgetButton* button = static_cast<WidgetButton*>(me);
			button->HandleMouseUp();
		}
	}

	void WidgetButton::HandleMouseDown()
	{
		// Regular button behavior.
		if (m_mode == ButtonMode_PushButton)
			SetState(ButtonState_Down);

		// Toggle button behavior.
		if (m_mode == ButtonMode_ToggleButton)
		{
			if (m_state == ButtonState_Up)
				SetState(ButtonState_Down);
			else
				SetState(ButtonState_Up);
		}
	}

	void WidgetButton::HandleMouseUp()
	{
		// Regular button behavior.
		if (m_mode == WidgetButton::ButtonMode_PushButton)
			SetState(ButtonState_Up);
	}
}
