#pragma once

#include <string>
#include "GuiTypes.h"
#include "libgui_forward.h"
#include "Widget.h"

//#include "GuiGraphicsFont.h"
//#include "WidgetImage.h"

namespace Gui
{
	class WidgetButton : public Widget
	{
	public:
		enum ButtonMode
		{
			ButtonMode_PushButton = 0,
			ButtonMode_ToggleButton = 1
		};

		WidgetButton();
		virtual ~WidgetButton();

		ButtonState GetState() const;
		ButtonMode GetMode() const;
		const std::string& GetText() const;
		Graphics::Font& GetFont();
		const Graphics::Image& GetImage() const;

		void SetState(ButtonState state);
		void SetMode(ButtonMode mode);
		void SetText(const std::string& text);
		void SetFont(const Graphics::Font& font);
		void SetImage(const Graphics::Image& image);

		DECLARE_EVENT(EHNotify, OnClick);
		DECLARE_EVENT(EHNotify, OnToggle);
		DECLARE_EVENT(EHNotify, OnUp);
		DECLARE_EVENT(EHNotify, OnDown);

	private:
		static void HandleMouseDown(Object* me, MouseButton button, ButtonState state, MouseState* mouseState);
		static void HandleMouseUp(Object* me, MouseButton button, ButtonState state, MouseState* mouseState);

		void HandleMouseDown();
		void HandleMouseUp();

		int m_state;
		int m_mode;
		std::string m_text;
		Graphics::Font m_font;
		WidgetImage* m_image;

		EHMouseButton m_onMouseDown;
		EHMouseButton m_onMouseUp;
	};
}
