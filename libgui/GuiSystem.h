#pragma once

#include <stdlib.h>

namespace Gui
{
	class ISystem
	{
	public:
		typedef const char* ImageHandle;
		typedef const char* FontHandle;

		static ISystem* Current();
		static void SetCurrent(ISystem* system);

		virtual size_t ImageWidth(ImageHandle image) = 0;
		virtual size_t ImageHeight(ImageHandle image) = 0;
		
		virtual size_t FontHeight(FontHandle font) = 0;
		virtual size_t FontWidth(FontHandle font, const char* text) = 0;

	private:
		static ISystem* mCurrent;
	};
}
