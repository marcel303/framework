#pragma once

namespace Gui
{
	namespace Graphics
	{
		class Shape
		{
		};
	}
}

#if 0

#include "GraphicsTypes.h"
#include "GuiGraphicsColor.h"
#include "GuiPoint.h"
#include "Mesh.h"
#include "Timer.h"

namespace Gui
{
	namespace Graphics
	{
		class Shape
		{
		public:
			enum PATTERN
			{
				PATTERN_NONE,
				PATTERN_ANTS4
			};

			Shape();
			~Shape();

			void MakeRect(int x1, int y1, int x2, int y2, Color color);
			void MakeFilledRect(int x1, int y1, int x2, int y2, Color color);
			void MakeBeveledRect(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int distance = 1.0f);
			void MakeRect3D(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int size = 1);
			void MakeArc(int x, int y, float radius, float angle1, float angle2, Color color);
			void MakeCircle(int x, int y, float radius, Color color);
			void MakeOutline(int x, int y, std::vector<Point>& points, Color color, bool closed = true, PATTERN pattern = PATTERN_NONE, float animationSpeed = 0.0f);
			void MakeFilledOutline(int x, int y, std::vector<Point>& points, Color color); // NOTE: Only works on convex shapes.

			void Render() const;

		private:
			void Create(PRIMITIVE_TYPE primitiveType, size_t vertexCount, size_t fvf, bool blendEnabled);
			void Destroy();

			Mesh* m_mesh;
			PRIMITIVE_TYPE m_primitiveType;
			size_t m_fvf;
			bool m_blendEnabled;

			const static int ARC_VERTEX_COUNT = 25;

			Timer m_timer;

			ResTex* m_textureAnts4;
		};
	};
};

#endif
