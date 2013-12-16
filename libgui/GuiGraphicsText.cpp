#include "libgui_precompiled.h"
#include "GuiGraphicsFont.h"
#include "GuiUtil.h"
//#include "Renderer.h"

namespace Gui
{
	namespace Graphics
	{
		Text::Text()
		{
			//m_mesh = 0;
		}

		Text::~Text()
		{
			/*if (m_mesh)
				Destroy();*/
		}

		int Text::GetTextLength() const
		{
			return m_textLength;
		}

		const IFont& Text::GetFont() const
		{
			return m_font;
		}

		void Text::MakeText(const IFont& font, const std::string& text, Color color, bool useFontColor)
		{
			/*

			Destroy();

			m_textLength = 0;

			if (text.length() == 0)
				return;

			if (font.GetColor().color != COLOR_DEFAULT)
				color = font.GetColor();

			color = Util::TranslateColor(color);

			Create(PT_TRIANGLE_LIST, 6 * text.length(), FVF_XYZ | FVF_COLOR | FVF_TEX1, color.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

			const int height = font.GetFont()->GetHeight();
			const int spacing = font.GetFont()->GetSpacing();

			int position = 0;

			for (size_t i = 0; i < text.length(); ++i)
			{
				const ResFont::Glyph* glyph = font.GetFont()->GetGlyph(text[i]);

				if (glyph != 0)
				{
					const int width = glyph->width;
					
					const float x1 = static_cast<float>(position);
					const float y1 = static_cast<float>(0);
					const float x2 = static_cast<float>(position + width);
					const float y2 = static_cast<float>(height);

					vb.SetPosition(i * 6 + 0, x1, y1, 0.0f);
					vb.SetPosition(i * 6 + 1, x2, y1, 0.0f);
					vb.SetPosition(i * 6 + 2, x2, y2, 0.0f);

					vb.SetPosition(i * 6 + 3, x1, y1, 0.0f);
					vb.SetPosition(i * 6 + 4, x2, y2, 0.0f);
					vb.SetPosition(i * 6 + 5, x1, y2, 0.0f);

					vb.SetTex(i * 6 + 0, 0, glyph->u1, glyph->v1);
					vb.SetTex(i * 6 + 1, 0, glyph->u2, glyph->v1);
					vb.SetTex(i * 6 + 2, 0, glyph->u2, glyph->v2);

					vb.SetTex(i * 6 + 3, 0, glyph->u1, glyph->v1);
					vb.SetTex(i * 6 + 4, 0, glyph->u2, glyph->v2);
					vb.SetTex(i * 6 + 5, 0, glyph->u1, glyph->v2);

					position += width + spacing;
				}
				else
				{
					vb.SetPosition(i * 6 + 0, 0.0f, 0.0f, 0.0f);
					vb.SetPosition(i * 6 + 1, 0.0f, 0.0f, 0.0f);
					vb.SetPosition(i * 6 + 2, 0.0f, 0.0f, 0.0f);
					vb.SetPosition(i * 6 + 3, 0.0f, 0.0f, 0.0f);
					vb.SetPosition(i * 6 + 4, 0.0f, 0.0f, 0.0f);
					vb.SetPosition(i * 6 + 5, 0.0f, 0.0f, 0.0f);
				}
			}

			const size_t vertexCount = vb.GetVertexCnt();

			for (size_t i = 0; i < vertexCount; ++i)
				vb.SetColor(i, color.r, color.g, color.b, color.a);

			// Calculate width/height.
			m_textLength = position - spacing;
			m_font = font;

			*/
		}

		void Text::Render() const
		{
			/*

			if (m_mesh == 0)
				return;

			if (m_blendEnabled)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_INV_SRC);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			}

			Renderer::I().RenderMesh(*m_mesh);

			if (m_blendEnabled)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			}

			*/
		}

		/*void Text::Create(PRIMITIVE_TYPE primitiveType, size_t vertexCount, size_t fvf, bool blendEnabled)
		{
			if (m_mesh)
				Destroy();

			m_primitiveType = primitiveType;
			m_fvf = fvf;
			m_blendEnabled = blendEnabled;

			m_mesh = new Mesh;
			m_mesh->Initialize(primitiveType, false, vertexCount, m_fvf, 0);
		}

		void Text::Destroy()
		{
			SAFE_FREE(m_mesh);
		}*/
	};
};
