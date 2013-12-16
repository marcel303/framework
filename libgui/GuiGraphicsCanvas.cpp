#include "libgui_precompiled.h"
#include "GuiGraphicsCanvas.h"
#include "GuiGraphicsFont.h"
#include "GuiGraphicsImage.h"
#include "GuiUtil.h"
//#include "MxMath.h"
//#include "Renderer.h"

#define EXTENT 1

namespace Gui
{
	namespace Graphics
	{
		ICanvas* ICanvas::mCurrent = 0;

		/*ICanvas* ICanvas::Current()
		{
			return mCurrent;
		}

		void ICanvas::SetCurrent(ICanvas* canvas)
		{
			mCurrent = canvas;
		}*/

		void ICanvas::MakeCurrent()
		{
			mCurrent = this;
		}

		void ICanvas::UndoMakeCurrent()
		{
			mCurrent = 0;
		}

#if 0
		Canvas& Canvas::I()
		{
			static Canvas canvas;
			return canvas;
		}

		void Canvas::Initialize()
		{
			// TODO: Create mesh in Initialize method.
			m_meshRect = new Mesh;
			m_meshRectFill = new Mesh;
			m_meshRect3D = new Mesh;
			m_meshArc = new Mesh;

			m_imageRect = new Shape;

			m_meshRect->Initialize(PT_LINE_STRIP, true, 5, FVF_XYZ | FVF_COLOR | FVF_TEX1, 0);
			m_meshRectFill->Initialize(PT_TRIANGLE_FAN, true, 4, FVF_XYZ | FVF_COLOR | FVF_TEX1, 0);
			m_meshRect3D->Initialize(PT_LINE_STRIP, true, 6, FVF_XYZ | FVF_COLOR | FVF_TEX1, 0);
			m_meshArc->Initialize(PT_TRIANGLE_FAN, true, ARC_VERTEX_COUNT, FVF_XYZ | FVF_COLOR, 0);

			m_imageRect->MakeFilledRect(0, 0, 1, 1, Graphics::Color(1.0f, 1.0f, 1.0f));

			m_textureAnts4 = new ResTex;
			m_textureAnts4->SetSize(4, 1);
			m_textureAnts4->SetPixel(0, 0, ::Color(1.0f, 1.0f, 1.0f, 1.0f));
			m_textureAnts4->SetPixel(1, 0, ::Color(1.0f, 1.0f, 1.0f, 0.0f));
			m_textureAnts4->SetPixel(2, 0, ::Color(1.0f, 1.0f, 1.0f, 0.0f));
			m_textureAnts4->SetPixel(3, 0, ::Color(1.0f, 1.0f, 1.0f, 0.0f));
		}

		void Canvas::Shutdown()
		{
			delete m_meshRect;
			delete m_meshRectFill;
			delete m_meshRect3D;
			delete m_meshArc;

			delete m_imageRect;

			delete m_textureAnts4;
		}

		void Canvas::MakeCurrent()
		{
			GraphicsDevice* graphicsDevice = Renderer::I().GetGraphicsDevice();

			graphicsDevice->RS(RS_DEPTHTEST, 0);
			graphicsDevice->RS(RS_SCISSORTEST, 1);

			m_oldProjMatrix = graphicsDevice->GetMatrix(MAT_PROJ);
			m_oldViewMatrix = graphicsDevice->GetMatrix(MAT_VIEW);
			m_oldWrldMatrix = graphicsDevice->GetMatrix(MAT_WRLD);

			Mat4x4 projMatrix;
			Mat4x4 viewMatrix;
			Mat4x4 wrldMatrix;

			const float offset = 0.0f;

			projMatrix.MakeOrthoLH(
				0.0f + offset,
				static_cast<float>(graphicsDevice->GetRTW()) - offset,
				0.0f + offset,
				static_cast<float>(graphicsDevice->GetRTH()) - offset,
				0.1f,
				10.0f);
			viewMatrix.MakeTranslation(0.0f, 0.0f, 1.0f);
			wrldMatrix.MakeIdentity();

			graphicsDevice->SetMatrix(MAT_PROJ, projMatrix);
			graphicsDevice->SetMatrix(MAT_VIEW, viewMatrix);
			graphicsDevice->SetMatrix(MAT_WRLD, wrldMatrix);
		}

		void Canvas::UndoMakeCurrent()
		{
			GraphicsDevice* graphicsDevice = Renderer::I().GetGraphicsDevice();

			Graphics::Canvas::I().UpdateVisibleRect();

			graphicsDevice->SetMatrix(MAT_PROJ, m_oldProjMatrix);
			graphicsDevice->SetMatrix(MAT_VIEW, m_oldViewMatrix);
			graphicsDevice->SetMatrix(MAT_WRLD, m_oldWrldMatrix);

			graphicsDevice->RS(RS_DEPTHTEST, 1);
			graphicsDevice->RS(RS_SCISSORTEST, 0);
		}

		MatrixStack& Canvas::GetMatrixStack()
		{
			return m_matrixStack;
		}

		RectStack& Canvas::GetVisibleRectStack()
		{
			return m_visibleRectStack;
		}

		void Canvas::DrawShape(const Shape& shape)
		{
 			UpdateMatrix();
			UpdateVisibleRect();

			shape.Render();
		}

		void Canvas::DrawImage(int x, int y, const Image& image)
		{
			DrawImageStretch(x, y, static_cast<int>(image.GetWidth()), static_cast<int>(image.GetHeight()), image);
		}

		void Canvas::DrawImageScale(int x, int y, float scale, const Image& image)
		{
			const int width = static_cast<int>(floor(image.GetWidth() * scale));
			const int height = static_cast<int>(floor(image.GetHeight() * scale));

			DrawImageStretch(x, y, width, height, image);
		}

		void Canvas::DrawImageStretch(int x, int y, int width, int height, const Image& image)
		{
			Renderer::I().GetGraphicsDevice()->SetTex(0, image.GetTexture().get());
			if (image.IsTransparent())
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			}
			if (static_cast<int>(image.GetWidth()) == width && static_cast<int>(image.GetHeight()) == height)
			{
				Renderer::I().GetGraphicsDevice()->SS(0, SS_FILTER, FILTER_POINT);
			}

			GetMatrixStack().PushTranslation(static_cast<float>(x), static_cast<float>(y), 0.0f);
			GetMatrixStack().PushScaling(static_cast<float>(width), static_cast<float>(height), 1.0f);

			DrawShape(*m_imageRect);

			GetMatrixStack().Pop();
			GetMatrixStack().Pop();

			if (static_cast<int>(image.GetWidth()) == width && static_cast<int>(image.GetHeight()) == height)
			{
				Renderer::I().GetGraphicsDevice()->SS(0, SS_FILTER, FILTER_INTERPOLATE);
			}
			if (image.IsTransparent())
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			}
			Renderer::I().GetGraphicsDevice()->SetTex(0, 0);
		}

		void Canvas::Rect(int x1, int y1, int x2, int y2, Color color)
		{
			x2 -= EXTENT;
			y2 -= EXTENT;

			color = Util::TranslateColor(color);

			UpdateMatrix();
			UpdateVisibleRect();

			ResVB& vb = *m_meshRect->GetVB();

			vb.SetPosition(0, static_cast<float>(x1), static_cast<float>(y1), 0.0f);
			vb.SetPosition(1, static_cast<float>(x2), static_cast<float>(y1), 0.0f);
			vb.SetPosition(2, static_cast<float>(x2), static_cast<float>(y2), 0.0f);
			vb.SetPosition(3, static_cast<float>(x1), static_cast<float>(y2), 0.0f);
			vb.SetPosition(4, static_cast<float>(x1), static_cast<float>(y1), 0.0f);

			vb.SetTex(0, 0, 0.0f, 0.0f);
			vb.SetTex(1, 0, 1.0f, 0.0f);
			vb.SetTex(2, 0, 1.0f, 1.0f);
			vb.SetTex(3, 0, 0.0f, 1.0f);
			vb.SetTex(4, 0, 0.0f, 0.0f);

			for (int i = 0; i < 5; ++i)
				vb.SetColor(i, color.r, color.g, color.b, color.a);

			vb.Invalidate();

			if (color.a != 1.0f)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			}

			Renderer::I().RenderMesh(*m_meshRect);

			if (color.a != 1.0f)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			}
		}

		void Canvas::FilledRect(int x1, int y1, int x2, int y2, Color color)
		{
			color = Util::TranslateColor(color);

			UpdateMatrix();
			UpdateVisibleRect();

			ResVB& vb = *m_meshRectFill->GetVB();

			vb.SetPosition(0, static_cast<float>(x1), static_cast<float>(y1), 0.0f);
			vb.SetPosition(1, static_cast<float>(x2), static_cast<float>(y1), 0.0f);
			vb.SetPosition(2, static_cast<float>(x2), static_cast<float>(y2), 0.0f);
			vb.SetPosition(3, static_cast<float>(x1), static_cast<float>(y2), 0.0f);

			vb.SetTex(0, 0, 0.0f, 0.0f);
			vb.SetTex(1, 0, 1.0f, 0.0f);
			vb.SetTex(2, 0, 1.0f, 1.0f);
			vb.SetTex(3, 0, 0.0f, 1.0f);

			for (int i = 0; i < 4; ++i)
				vb.SetColor(i, color.r, color.g, color.b, color.a);

			vb.Invalidate();

			if (color.a != 1.0f)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			}

			Renderer::I().RenderMesh(*m_meshRectFill);

			if (color.a != 1.0f)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			}
		}

		void Canvas::BeveledRect(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int distance)
		{
			if (x1 + distance >= x2 - distance || y1 + distance >= y2 - distance)
				return;

			Rect(x1, y1, x2 - distance, y2 - distance, colorHigh);
			Rect(x1 + distance, y1 + distance, x2, y2, colorLow);
		}

		void Canvas::Rect3D(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int size)
		{
			x2 -= EXTENT;
			y2 -= EXTENT;

			colorLow = Util::TranslateColor(colorLow);
			colorHigh = Util::TranslateColor(colorHigh);

			UpdateMatrix();
			UpdateVisibleRect();

			for (int i = 0; i < size; ++i)
			{
				ResVB& vb = *m_meshRect3D->GetVB();

				vb.SetPosition(0, static_cast<float>(x1 + i), static_cast<float>(y1 + i), 0.0f);
				vb.SetPosition(1, static_cast<float>(x2 + i), static_cast<float>(y1 - i), 0.0f);
				vb.SetPosition(2, static_cast<float>(x2 + i), static_cast<float>(y2 - i), 0.0f);
				vb.SetPosition(3, static_cast<float>(x2 + i), static_cast<float>(y2 - i), 0.0f); // Repeat.
				vb.SetPosition(4, static_cast<float>(x1 + i), static_cast<float>(y2 - i), 0.0f);
				vb.SetPosition(5, static_cast<float>(x1 + i), static_cast<float>(y1 + i), 0.0f);

				vb.SetTex(0, 0, 0.0f, 0.0f);
				vb.SetTex(1, 0, 1.0f, 0.0f);
				vb.SetTex(2, 0, 1.0f, 1.0f);
				vb.SetTex(3, 0, 1.0f, 1.0f); // Repeat.
				vb.SetTex(4, 0, 0.0f, 1.0f);
				vb.SetTex(5, 0, 0.0f, 0.0f);

				vb.SetColor(0, colorLow.r, colorLow.g, colorLow.b, colorLow.a);
				vb.SetColor(1, colorLow.r, colorLow.g, colorLow.b, colorLow.a);
				vb.SetColor(2, colorLow.r, colorLow.g, colorLow.b, colorLow.a);
				vb.SetColor(3, colorHigh.r, colorHigh.g, colorHigh.b, colorHigh.a);
				vb.SetColor(4, colorHigh.r, colorHigh.g, colorHigh.b, colorHigh.a);
				vb.SetColor(5, colorHigh.r, colorHigh.g, colorHigh.b, colorHigh.a);

				vb.Invalidate();

				if (colorLow.a != 1.0f || colorHigh.a != 1.0f)
				{
					Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
					Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
					Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
				}

				Renderer::I().RenderMesh(*m_meshRect3D);

				if (colorLow.a != 1.0f || colorHigh.a != 1.0f)
				{
					Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
				}
			}
		}

		void Canvas::Arc(int x, int y, float radius, float angle1, float angle2, Color color)
		{
			color = Util::TranslateColor(color);

			UpdateMatrix();
			UpdateVisibleRect();

			ResVB& vb = *m_meshArc->GetVB();

			const size_t vertexCount = ARC_VERTEX_COUNT;

			vb.SetPosition(0, static_cast<float>(x), static_cast<float>(y), 0.0f);

			const float step = (angle2 - angle1) / (vertexCount - 2);

			for (size_t i = 1; i < vertexCount; ++i)
			{
				const float angle = angle1 + step * (i - 1);

				vb.SetPosition(i, x + cos(angle) * radius, y + sin(angle) * radius, 0.0f);
			}

			for (size_t i = 0; i < vertexCount; ++i)
				vb.SetColor(i, color.r, color.g, color.b, color.a);

			vb.Invalidate();

			if (color.a != 1.0f)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			}

			Renderer::I().RenderMesh(*m_meshArc);

			if (color.a != 1.0f)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			}
		}

		void Canvas::Circle(int x, int y, float radius, Color color)
		{
			Arc(x, y, radius, 0.0f, 2.0f * Mx::PI, color);
		}

		// TODO: Move general text rendering to Renderer?
		void Canvas::Text(const Font& font, int x, int y, const std::string& text, Color color, ALIGNMENT alignmentH, ALIGNMENT alignmentV, bool useFontColor)
		{
			if (text.length() == 0)
				return;

			if (font.GetColor().color != COLOR_DEFAULT)
				color = font.GetColor();

			color = Util::TranslateColor(color);

			Mesh mesh;
			mesh.Initialize(PT_TRIANGLE_LIST, true, 6 * text.length(), FVF_XYZ | FVF_COLOR | FVF_TEX1, 0);

			// Fill vertex buffer.
			ResVB& vb = *mesh.GetVB();

			const int height = font.GetFont()->GetHeight();
			const int spacing = font.GetFont()->GetSpacing();

			int position = 0;

			for (size_t i = 0; i < text.length(); ++i)
			{
				const ResFont::Glyph* glyph = font.GetFont()->GetGlyph(text[i]);

				if (glyph != 0)
				{
					const int width = glyph->GetWidth();
					
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

			// Calculate alignment offset.
			const int length = position - spacing;
			int offsetX = 0;
			int offsetY = 0;

			switch (alignmentH)
			{
			case ALIGN_LEFT:
				offsetX = 0;
				break;
			case ALIGN_RIGHT:
				offsetX = -length;
				break;
			case ALIGN_CENTER:
				offsetX = -length  / 2;
				break;
			default:
				// Invalid alignment.
				Assert(0);
				break;
			}

			switch (alignmentV)
			{
			case ALIGN_TOP:
				offsetY = 0;
				break;
			case ALIGN_BOTTOM:
				offsetY = -font.GetFont()->GetHeight();
				break;
			case ALIGN_CENTER:
				offsetY = -font.GetFont()->GetHeight() / 2;
				break;
			default:
				// Invalid alignment.
				Assert(0);
				break;
			}

			m_matrixStack.PushTranslation(static_cast<float>(x), static_cast<float>(y), 0.0f);
			m_matrixStack.PushTranslation(static_cast<float>(offsetX), static_cast<float>(offsetY), 0.0f);
			UpdateMatrix();
			UpdateVisibleRect();

			Renderer::I().GetGraphicsDevice()->SetTex(0, font.GetFont()->GetTexture().get());
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			Renderer::I().GetGraphicsDevice()->SS(0, SS_FILTER, FILTER_POINT);

			Renderer::I().RenderMesh(mesh);

			Renderer::I().GetGraphicsDevice()->SS(0, SS_FILTER, FILTER_INTERPOLATE);
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			Renderer::I().GetGraphicsDevice()->SetTex(0, 0);

			m_matrixStack.Pop();
			m_matrixStack.Pop();
		}

		void Canvas::Text(Graphics::Text& text, int x, int y, ALIGNMENT alignmentH, ALIGNMENT alignmentV)
		{
			if (text.GetFont().GetFont().get() == 0)
				return;

			// Calculate alignment offset.
			const int length = text.GetTextLength();
			const int height = text.GetFont().GetFont()->GetHeight();
			int offsetX = 0;
			int offsetY = 0;

			switch (alignmentH)
			{
			case ALIGN_LEFT:
				offsetX = 0;
				break;
			case ALIGN_RIGHT:
				offsetX = -length;
				break;
			case ALIGN_CENTER:
				offsetX = -length  / 2;
				break;
			default:
				// Invalid alignment.
				Assert(0);
				break;
			}

			switch (alignmentV)
			{
			case ALIGN_TOP:
				offsetY = 0;
				break;
			case ALIGN_BOTTOM:
				offsetY = -height;
				break;
			case ALIGN_CENTER:
				offsetY = -height  / 2;
				break;
			default:
				// Invalid alignment.
				Assert(0);
				break;
			}

			m_matrixStack.PushTranslation(static_cast<float>(x), static_cast<float>(y), 0.0f);
			m_matrixStack.PushTranslation(static_cast<float>(offsetX), static_cast<float>(offsetY), 0.0f);
			UpdateMatrix();
			UpdateVisibleRect();

			Renderer::I().GetGraphicsDevice()->SetTex(0, text.GetFont().GetFont()->GetTexture().get());
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			Renderer::I().GetGraphicsDevice()->SS(0, SS_FILTER, FILTER_POINT);

			text.Render();

			Renderer::I().GetGraphicsDevice()->SS(0, SS_FILTER, FILTER_INTERPOLATE);
			Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			Renderer::I().GetGraphicsDevice()->SetTex(0, 0);

			m_matrixStack.Pop();
			m_matrixStack.Pop();
		}

		void Canvas::Outline(int x, int y, std::vector<Point>& points, Color color, bool closed, PATTERN pattern, float animationSpeed)
		{
			color = Util::TranslateColor(color);

			UpdateMatrix();
			UpdateVisibleRect();

			Mesh mesh;
			mesh.Initialize(PT_LINE_STRIP, true, points.size() + (closed ? 1 : 0), FVF_XYZ | FVF_COLOR | FVF_TEX1, 0);

			ResVB& vb = *mesh.GetVB();

			float offset = m_timer.GetTime() * animationSpeed;
			float totalLength = 0.0f;

			for (size_t i = 0; i < points.size() + (closed ? 1 : 0); ++i)
			{
				const size_t index1 = (i + 0) % points.size();
				const size_t index2 = (i + 1) % points.size();

				const Point& point1 = points[index1];
				const Point& point2 = points[index2];

				vb.SetPosition(i, static_cast<float>(x + point1.x), static_cast<float>(y + point1.y), 0.0f);
				vb.SetColor(i, color.r, color.g, color.b, color.a);
				vb.SetTex(i, 0, offset / 4.0f + totalLength / 4.0f, 0.5f);

				const float length = sqrt(static_cast<float>((point2.x - point1.x) * (point2.x - point1.x) + (point2.y - point1.y) * (point2.y - point1.y)));

				totalLength += length;
			}

			if (pattern != PATTERN_NONE)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);

				switch (pattern)
				{
				case PATTERN_ANTS4:
					Renderer::I().GetGraphicsDevice()->SetTex(0, m_textureAnts4);
					break;
				default:
					Assert(0);
					break;
				}
			}

			Renderer::I().RenderMesh(mesh);

			if (pattern != PATTERN_NONE)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
				Renderer::I().GetGraphicsDevice()->SetTex(0, 0);
			}
		}

		void Canvas::FilledOutline(int x, int y, std::vector<Point>& points, Color color)
		{
			color = Util::TranslateColor(color);

			UpdateMatrix();
			UpdateVisibleRect();

			Mesh mesh;
			mesh.Initialize(PT_TRIANGLE_FAN, true, points.size() + 1, FVF_XYZ | FVF_COLOR, 0);

			ResVB& vb = *mesh.GetVB();

			for (size_t i = 0; i < points.size() + 1; ++i)
			{
				const size_t index = i % points.size();
				const Point& point = points[index];

				vb.SetPosition(i, static_cast<float>(x + point.x), static_cast<float>(y + point.y), 0.0f);
				vb.SetColor(i, color.r, color.g, color.b, color.a);
			}

			Renderer::I().RenderMesh(mesh);
		}

		void Canvas::UpdateMatrix()
		{
			GraphicsDevice* graphicsDevice = Renderer::I().GetGraphicsDevice();

			graphicsDevice->SetMatrix(MAT_WRLD, m_matrixStack.Top());
		}

		void Canvas::UpdateVisibleRect()
		{
			GraphicsDevice* graphicsDevice = Renderer::I().GetGraphicsDevice();

			Gui::Rect visibleRect = m_visibleRectStack.Top();

			graphicsDevice->SetScissorRect(
				visibleRect.min.x,
				visibleRect.min.y,
				visibleRect.max.x,
				visibleRect.max.y);
		}

		Canvas::Canvas()
		{
		}

		Canvas::~Canvas()
		{
		}
#endif
	};
};
