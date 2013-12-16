#pragma once

#include <string>
#include "GuiGraphicsColor.h"
#include "GuiGraphicsFont.h"
//#include "Mesh.h"

namespace Gui
{
	namespace Graphics
	{
		class Text
		{
		public:
			Text();
			~Text();

			int GetTextLength() const;
			const Font& GetFont() const;

			void MakeText(const Font& font, const std::string& text, Color color, bool useFontColor = false);

			void Render() const;

		private:
			//void Create(PRIMITIVE_TYPE primitiveType, size_t vertexCount, size_t fvf, bool blendEnabled);
			//void Destroy();

			/*
			Mesh* m_mesh;
			PRIMITIVE_TYPE m_primitiveType;
			size_t m_fvf;
			bool m_blendEnabled;
			*/

			int m_textLength;
			Font m_font;
		};
	};
};
