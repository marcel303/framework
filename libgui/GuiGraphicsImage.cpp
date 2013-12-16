#include "libgui_precompiled.h"
#include "GuiGraphicsImage.h"
#include "GuiSystem.h"

namespace Gui
{
	namespace Graphics
	{
		Image::Image()
		{
			m_width = 0;
			m_height = 0;

			m_isTransparent = false;
		}

		void Image::Setup(const char* name, bool isTransparent)
		{
			m_name = name;
			m_isTransparent = isTransparent;
			m_width = ISystem::Current()->ImageWidth(name);
			m_height = ISystem::Current()->ImageHeight(name);
		}

		std::string Image::GetName() const
		{
			return m_name;
		}

		size_t Image::GetWidth() const
		{
			return m_width;
		}

		size_t Image::GetHeight() const
		{
			return m_height;
		}

		bool Image::IsTransparent() const
		{
			return m_isTransparent;
		}
	};
};
