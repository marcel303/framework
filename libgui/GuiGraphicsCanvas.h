#pragma once

#include <string>
#include <vector>
#include "GuiGraphicsColor.h"
#include "GuiGraphicsFont.h"
#include "GuiGraphicsImage.h"
#include "GuiGraphicsShape.h"
#include "GuiGraphicsText.h"
#include "GuiPoint.h"
#include "GuiRectStack.h"
#include "GuiTypes.h"
#include "MatrixStack.h"
//#include "Mesh.h"
#include "Timer.h"

#include "Debugging.h" // FIXME
#include "GuiRect.h"

namespace Gui
{
	namespace Graphics
	{
		// TODO: Rename stuff in cavas.. VisibleArea/Rect ?

		class ICanvas
		{
		public:
			enum PATTERN
			{
				PATTERN_NONE,
				PATTERN_ANTS4
			};

			//static ICanvas* Current();
			//static void SetCurrent(ICanvas* canvas);

			virtual void MakeCurrent();
			virtual void UndoMakeCurrent();

			virtual MatrixStack& GetMatrixStack() = 0;
			virtual RectStack& GetVisibleRectStack() = 0;

			virtual void DrawImage(int x, int y, const Image& image) = 0;
			virtual void DrawImageScale(int x, int y, float scale, const Image& image) = 0;
			virtual void DrawImageStretch(int x, int y, int width, int height, const Image& image) = 0;
			virtual void Rect(int x1, int y1, int x2, int y2, Color color) = 0;
			virtual void FilledRect(int x1, int y1, int x2, int y2, Color color) = 0;
			virtual void BeveledRect(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int distance = 1.0f) = 0;
			virtual void Rect3D(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int size = 1) = 0;
			virtual void Arc(int x, int y, float radius, float angle1, float angle2, Color color) = 0;
			virtual void Circle(int x, int y, float radius, Color color) = 0;
			virtual void Text(const Font& font, int x, int y, const std::string& text, Color color, Alignment alignmentH = Alignment_Left, Alignment alignmentV = Alignment_Top, bool useFontColor = false) = 0;
			//void Text(Graphics::Text& text, int x, int y, Alignment alignmentH = Alignment_Left, Alignment alignmentV = Alignment_Top) = 0;
			virtual void Outline(int x, int y, std::vector<Point>& points, Color color, bool closed = true, PATTERN pattern = PATTERN_NONE, float animationSpeed = 0.0f) = 0;
			virtual void FilledOutline(int x, int y, std::vector<Point>& points, Color color) = 0; // NOTE: Only works on convex shapes.

		private:
			static ICanvas* mCurrent;
		};

#if 0
		class Canvas_R2 : public ICanvas
		{
		public:
			static Canvas& I();

			void Initialize();
			void Shutdown();

			virtual void MakeCurrent();
			virtual void UndoMakeCurrent();

			MatrixStack& GetMatrixStack();
			RectStack& GetVisibleRectStack();

			//void DrawShape(const Shape& shape);

			virtual void DrawImage(int x, int y, const Image& image);
			virtual void DrawImageScale(int x, int y, float scale, const Image& image);
			virtual void DrawImageStretch(int x, int y, int width, int height, const Image& image);
			virtual void Rect(int x1, int y1, int x2, int y2, Color color);
			virtual void FilledRect(int x1, int y1, int x2, int y2, Color color);
			virtual void BeveledRect(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int distance = 1.0f);
			virtual void Rect3D(int x1, int y1, int x2, int y2, Color colorLow, Color colorHigh, int size = 1);
			virtual void Arc(int x, int y, float radius, float angle1, float angle2, Color color);
			virtual void Circle(int x, int y, float radius, Color color);
			virtual void Text(const IFont& font, int x, int y, const std::string& text, Color color, Alignment alignmentH = Alignment_Left, Alignment alignmentV = Alignment_Top, bool useFontColor = false);
			virtual void Text(Graphics::Text& text, int x, int y, Alignment alignmentH = Alignment_Left, Alignment alignmentV = Alignment_Top);
			virtual void Outline(int x, int y, std::vector<Point>& points, Color color, bool closed = true, PATTERN pattern = PATTERN_NONE, float animationSpeed = 0.0f);
			virtual void FilledOutline(int x, int y, std::vector<Point>& points, Color color); // NOTE: Only works on convex shapes.

		private:
			Canvas();
			~Canvas();

			void UpdateMatrix();
			void UpdateVisibleRect();

			MatrixStack m_matrixStack;
			RectStack m_visibleRectStack;

#if 0
			// Rendering helpers.
			Mesh* m_meshRect;
			Mesh* m_meshRectFill;
			Mesh* m_meshRect3D;
			Mesh* m_meshArc;
#endif

			Shape* m_imageRect;

			const static int ARC_VERTEX_COUNT = 25;

			Timer m_timer;

			//ResTex* m_textureAnts4;

			Mat4x4 m_oldProjMatrix;
			Mat4x4 m_oldViewMatrix;
			Mat4x4 m_oldWrldMatrix;
		};
#endif
	};
};
