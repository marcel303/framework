#include "libgui_precompiled.h"
#include "GuiGraphicsFont.h"
#include "GuiSystem.h"

namespace Gui
{
	namespace Graphics
	{
		Font::Font()
		{
		}

		void Font::Setup(const char* name)
		{
			m_name = name;
			m_height = ISystem::Current()->FontHeight(name);
		}

		std::string Font::GetName() const
		{
			return m_name;
		}

		int Font::GetWidth(const char* text) const
		{
			return ISystem::Current()->FontWidth(m_name.c_str(), text);
		}

		int Font::GetHeight() const
		{
			return m_height;
		}
	};
};
