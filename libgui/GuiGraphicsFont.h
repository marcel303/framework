#pragma once

#include <string>

namespace Gui
{
	namespace Graphics
	{
		class Font
		{
		public:
			Font();
			
			void Setup(const char* name);

			std::string GetName() const;
			int GetHeight() const;
			int GetWidth(const char* text) const;

		private:
			std::string m_name;
			int m_height;
		};
	}
}

#if 0

#include <string>
#include "GuiGraphicsColor.h"
#include "ResFont.h"

namespace Gui
{
	namespace Graphics
	{
		class Font
		{
		public:
			Font();

			void Initialize(ShFont font);

			const ShFont GetFont() const;
			const Graphics::Color& GetColor() const;

			//void SetFont(ShFont font);
			void SetFont(const std::string& font);
			void SetColor(const Graphics::Color& color);

		private:
			ShFont m_font;
			Graphics::Color m_color;
		};
	};
};

#endif
