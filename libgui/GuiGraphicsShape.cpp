#include "libgui_precompiled.h"

#if 0

#include "GuiGraphicsShape.h"
#include "GuiUtil.h"
#include "MxMath.h"
#include "Renderer.h"

#define EXTENT 1

// NOTE: Apply extent only for line based shapes.
//       Filled shapes have 1 pixel 'less' already due to rasterization rules of Direct3D/OpenGL/etc.

namespace Gui
{
	namespace Graphics
	{
		Shape::Shape()
		{
			m_mesh = 0;
		}

		Shape::~Shape()
		{
			if (m_mesh)
				Destroy();
		}

		void Shape::MakeRect(int x1, int y1, int x2, int y2, Color color)
		{
			x2 -= EXTENT;
			y2 -= EXTENT;
			color = Util::TranslateColor(color);

			Create(PT_LINE_STRIP, 5, FVF_XYZ | FVF_COLOR | FVF_TEX1, color.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

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
		}

		void Shape::MakeFilledRect(int x1, int y1, int x2, int y2, Color color)
		{
			color = Util::TranslateColor(color);

			Create(PT_TRIANGLE_FAN, 4, FVF_XYZ | FVF_COLOR | FVF_TEX1, color.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

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
		}

		void Shape::MakeBeveledRect(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int distance)
		{
			x2 -= EXTENT;
			y2 -= EXTENT;
			colorLow = Util::TranslateColor(colorLow);
			colorHigh = Util::TranslateColor(colorHigh);

			if (x1 + distance > x2 - distance + 1 || y1 + distance > y2 - distance + 1)
				return;

			Create(PT_LINE_LIST, 16, FVF_XYZ | FVF_COLOR | FVF_TEX1, colorLow.a != 1.0f || colorHigh.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

			// First rect.
			vb.SetPosition(0, static_cast<float>(x1), static_cast<float>(y1), 0.0f);
			vb.SetPosition(1, static_cast<float>(x2 - distance), static_cast<float>(y1), 0.0f);

			vb.SetPosition(2, static_cast<float>(x2 - distance), static_cast<float>(y1), 0.0f);
			vb.SetPosition(3, static_cast<float>(x2 - distance), static_cast<float>(y2 - distance), 0.0f);

			vb.SetPosition(4, static_cast<float>(x2 - distance), static_cast<float>(y2 - distance), 0.0f);
			vb.SetPosition(5, static_cast<float>(x1), static_cast<float>(y2 - distance), 0.0f);

			vb.SetPosition(6, static_cast<float>(x1), static_cast<float>(y2 - distance), 0.0f);
			vb.SetPosition(7, static_cast<float>(x1), static_cast<float>(y1), 0.0f);

			vb.SetTex(0, 0, 0.0f, 0.0f);
			vb.SetTex(1, 0, 1.0f, 0.0f);

			vb.SetTex(2, 0, 1.0f, 0.0f);
			vb.SetTex(3, 0, 1.0f, 1.0f);

			vb.SetTex(4, 0, 1.0f, 1.0f);
			vb.SetTex(5, 0, 0.0f, 1.0f);

			vb.SetTex(6, 0, 0.0f, 1.0f);
			vb.SetTex(7, 0, 0.0f, 0.0f);

			for (int i = 0; i < 8; ++i)
				vb.SetColor(i, colorHigh.r, colorHigh.g, colorHigh.b, colorHigh.a);

			// Second rect.
			vb.SetPosition(8 + 0, static_cast<float>(x1 + distance), static_cast<float>(y1 + distance), 0.0f);
			vb.SetPosition(8 + 1, static_cast<float>(x2), static_cast<float>(y1+ distance), 0.0f);

			vb.SetPosition(8 + 2, static_cast<float>(x2), static_cast<float>(y1+ distance), 0.0f);
			vb.SetPosition(8 + 3, static_cast<float>(x2), static_cast<float>(y2), 0.0f);

			vb.SetPosition(8 + 4, static_cast<float>(x2), static_cast<float>(y2), 0.0f);
			vb.SetPosition(8 + 5, static_cast<float>(x1 + distance), static_cast<float>(y2), 0.0f);

			vb.SetPosition(8 + 6, static_cast<float>(x1 + distance), static_cast<float>(y2), 0.0f);
			vb.SetPosition(8 + 7, static_cast<float>(x1 + distance), static_cast<float>(y1 + distance), 0.0f);

			vb.SetTex(8 + 0, 0, 0.0f, 0.0f);
			vb.SetTex(8 + 1, 0, 1.0f, 0.0f);

			vb.SetTex(8 + 2, 0, 1.0f, 0.0f);
			vb.SetTex(8 + 3, 0, 1.0f, 1.0f);

			vb.SetTex(8 + 4, 0, 1.0f, 1.0f);
			vb.SetTex(8 + 5, 0, 0.0f, 1.0f);

			vb.SetTex(8 + 6, 0, 0.0f, 1.0f);
			vb.SetTex(8 + 7, 0, 0.0f, 0.0f);

			for (int i = 0; i < 8; ++i)
				vb.SetColor(8 + i, colorLow.r, colorLow.g, colorLow.b, colorLow.a);
		}

		void Shape::MakeRect3D(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int size)
		{
			x2 -= EXTENT;
			y2 -= EXTENT;
			colorLow = Util::TranslateColor(colorLow);
			colorHigh = Util::TranslateColor(colorHigh);

			Create(PT_LINE_STRIP, 6 * size, FVF_XYZ | FVF_COLOR | FVF_TEX1, colorLow.a != 1.0f || colorHigh.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

			for (int i = 0; i < size; ++i)
			{
				vb.SetPosition(i * 6 + 0, static_cast<float>(x1 + i), static_cast<float>(y1 + i), 0.0f);
				vb.SetPosition(i * 6 + 1, static_cast<float>(x2 + i), static_cast<float>(y1 - i), 0.0f);
				vb.SetPosition(i * 6 + 2, static_cast<float>(x2 + i), static_cast<float>(y2 - i), 0.0f);
				vb.SetPosition(i * 6 + 3, static_cast<float>(x2 + i), static_cast<float>(y2 - i), 0.0f); // Repeat.
				vb.SetPosition(i * 6 + 4, static_cast<float>(x1 + i), static_cast<float>(y2 - i), 0.0f);
				vb.SetPosition(i * 6 + 5, static_cast<float>(x1 + i), static_cast<float>(y1 + i), 0.0f);

				vb.SetTex(i * 6 + 0, 0, 0.0f, 0.0f);
				vb.SetTex(i * 6 + 1, 0, 1.0f, 0.0f);
				vb.SetTex(i * 6 + 2, 0, 1.0f, 1.0f);
				vb.SetTex(i * 6 + 3, 0, 1.0f, 1.0f); // Repeat.
				vb.SetTex(i * 6 + 4, 0, 0.0f, 1.0f);
				vb.SetTex(i * 6 + 5, 0, 0.0f, 0.0f);

				vb.SetColor(i * 6 + 0, colorLow.r, colorLow.g, colorLow.b, colorLow.a);
				vb.SetColor(i * 6 + 1, colorLow.r, colorLow.g, colorLow.b, colorLow.a);
				vb.SetColor(i * 6 + 2, colorLow.r, colorLow.g, colorLow.b, colorLow.a);
				vb.SetColor(i * 6 + 3, colorHigh.r, colorHigh.g, colorHigh.b, colorHigh.a);
				vb.SetColor(i * 6 + 4, colorHigh.r, colorHigh.g, colorHigh.b, colorHigh.a);
				vb.SetColor(i * 6 + 5, colorHigh.r, colorHigh.g, colorHigh.b, colorHigh.a);
			}
		}

		void Shape::MakeArc(int x, int y, float radius, float angle1, float angle2, Color color)
		{
			color = Util::TranslateColor(color);

			Create(PT_TRIANGLE_FAN, ARC_VERTEX_COUNT, FVF_XYZ | FVF_COLOR | FVF_TEX1, color.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

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
		}

		void Shape::MakeCircle(int x, int y, float radius, Color color)
		{
			MakeArc(x, y, radius, 0.0f, 2.0f * Mx::PI, color);
		}

		void Shape::MakeOutline(int x, int y, std::vector<Point>& points, Color color, bool closed, PATTERN pattern, float animationSpeed)
		{
			color = Util::TranslateColor(color);

			Create(PT_LINE_STRIP, points.size() + (closed ? 1 : 0), FVF_XYZ | FVF_COLOR | FVF_TEX1, color.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

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

			Renderer::I().RenderMesh(*m_mesh);

			if (pattern != PATTERN_NONE)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
				Renderer::I().GetGraphicsDevice()->SetTex(0, 0);
			}
		}

		void Shape::MakeFilledOutline(int x, int y, std::vector<Point>& points, Color color)
		{
			color = Util::TranslateColor(color);

			Create(PT_TRIANGLE_FAN, points.size() + 1, FVF_XYZ | FVF_COLOR, color.a != 1.0f);
			ResVB& vb = *m_mesh->GetVB();

			for (size_t i = 0; i < points.size() + 1; ++i)
			{
				const size_t index = i % points.size();
				const Point& point = points[index];

				vb.SetPosition(i, static_cast<float>(x + point.x), static_cast<float>(y + point.y), 0.0f);
				vb.SetColor(i, color.r, color.g, color.b, color.a);
			}
		}

		void Shape::Render() const
		{
			if (m_mesh == 0)
				return;

			if (m_blendEnabled)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 1);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_SRC, BLEND_SRC);
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND_DST, BLEND_INV_SRC);
			}

			Renderer::I().RenderMesh(*m_mesh);

			if (m_blendEnabled)
			{
				Renderer::I().GetGraphicsDevice()->RS(RS_BLEND, 0);
			}
		}

		void Shape::Create(PRIMITIVE_TYPE primitiveType, size_t vertexCount, size_t fvf, bool blendEnabled)
		{
			if (m_mesh)
				Destroy();

			m_primitiveType = primitiveType;
			m_fvf = fvf;
			m_blendEnabled = blendEnabled;

			m_mesh = new Mesh;
			m_mesh->Initialize(primitiveType, false, vertexCount, m_fvf, 0);
		}

		void Shape::Destroy()
		{
			SAFE_FREE(m_mesh);
		}
	};
};

#endif
