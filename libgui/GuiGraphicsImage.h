#pragma once

#include <string>
//#include "ResBaseTex.h"

namespace Gui
{
	namespace Graphics
	{
		class Image
		{
		public:
			Image();

			void Setup(const char* name, bool isTransparent);

			std::string GetName() const;
			size_t GetWidth() const;
			size_t GetHeight() const;
			bool IsTransparent() const;

		private:
			std::string m_name;
			size_t m_width;
			size_t m_height;
			bool m_isTransparent;
		};
	};
};
