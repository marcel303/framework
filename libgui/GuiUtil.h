#pragma once

#include <string>
#include "Exception.h"
#include "GuiGraphicsColor.h"
#include "GuiGraphicsFont.h"
#include "GuiPoint.h"
#include "GuiRect.h"
#include "GuiTypes.h"

namespace Gui
{
	namespace Util
	{
		inline void GetBoxInBoxAlignmentPivot(
			const Rect& outsideRect, 
			const Rect& insideRect,
			Alignment alignmentH, 
			Alignment alignmentV,
			Point& out_pivot)
		{
			const int w1 = outsideRect.max.x - outsideRect.min.x;
			const int h1 = outsideRect.max.y - outsideRect.min.y;
			const int w2 = insideRect.max.x - insideRect.min.x;
			const int h2 = insideRect.max.y - insideRect.min.y;

			switch (alignmentH)
			{
			case Alignment_Left:
				out_pivot.x = outsideRect.min.x;
				break;
			case Alignment_Right:
				out_pivot.x = outsideRect.max.x - w2;
				break;
			case Alignment_Center:
				out_pivot.x = outsideRect.min.x + (w1 - w2) / 2;
				break;
			default:
				// Invalid alignment.
				throw ExceptionNA();
			}

			switch (alignmentV)
			{
			case Alignment_Top:
				out_pivot.y = outsideRect.min.y;
				break;
			case Alignment_Bottom:
				out_pivot.y = outsideRect.max.y - h2;
				break;
			case Alignment_Center:
				out_pivot.y = outsideRect.min.y + (h1 - h2) / 2;
				break;
			default:
				// Invalid alignment.
				throw ExceptionNA();
			}
		}

		inline void GetTextAlignmentPivot(
			const Graphics::Font& font,
			int textLength,
			const Point& boxSize,
			Alignment alignmentH,
			Alignment alignmentV,
			Point& out_pivot)
		{
			Rect outside;
			Rect inside;
			Point pivot;

			outside.min.x = 0;
			outside.min.y = 0;
			outside.max.x = boxSize.x;
			outside.max.y = boxSize.y;

			inside.min.x = 0;
			inside.min.y = 0;
			inside.max.x = textLength;
			inside.max.y = font.GetHeight();

			GetBoxInBoxAlignmentPivot(outside, inside, alignmentH, alignmentV, out_pivot);
		}

		inline void GetTextAlignmentPivot(
			const Graphics::Font& font,
			const std::string& text,
			const Point& boxSize,
			Alignment alignmentH,
			Alignment alignmentV,
			Point& out_pivot)
		{
			GetTextAlignmentPivot(
				font,
				font.GetWidth(text.c_str()),
				boxSize,
				alignmentH,
				alignmentV,
				out_pivot);
		}

		inline Graphics::Color GetColor(STDCOLOR color)
		{
			Graphics::Color result;
			switch (color)
			{
			case COLOR_AQUA:      result = Graphics::Color(0.0f, 1.0f, 1.0f); break;
			case COLOR_BLACK:     result = Graphics::Color(0.0f, 0.0f, 0.0f); break;
			case COLOR_BLUE:      result = Graphics::Color(0.0f, 0.0f, 1.0f); break;
			case COLOR_DARKGRAY:  result = Graphics::Color(0.2f, 0.2f, 0.2f); break;
			case COLOR_FUCHSIA:   result = Graphics::Color(1.0f, 0.0f, 1.0f); break;
			case COLOR_GRAY:      result = Graphics::Color(0.5f, 0.5f, 0.5f); break;
			case COLOR_GREEN:     result = Graphics::Color(0.0f, 0.5f, 0.0f); break;
			case COLOR_LIME:      result = Graphics::Color(0.0f, 1.0f, 0.0f); break;
			case COLOR_LIGHTGRAY: result = Graphics::Color(0.8f, 0.8f, 0.8f); break;
			case COLOR_MAROON:    result = Graphics::Color(0.5f, 0.0f, 0.0f); break;
			case COLOR_NAVY:      result = Graphics::Color(0.0f, 0.0f, 0.5f); break;
			case COLOR_OLIVE:     result = Graphics::Color(0.5f, 0.5f, 0.0f); break;
			case COLOR_PURPLE:    result = Graphics::Color(0.5f, 0.0f, 0.5f); break;
			case COLOR_RED:       result = Graphics::Color(1.0f, 0.0f, 0.0f); break;
			case COLOR_SILVER:    result = Graphics::Color(0.9f, 0.9f, 0.9f); break;
			case COLOR_TEAL:      result = Graphics::Color(0.0f, 0.5f, 0.5f); break;
			case COLOR_WHITE:     result = Graphics::Color(1.0f, 1.0f, 1.0f); break;
			case COLOR_YELLOW:    result = Graphics::Color(1.0f, 1.0f, 0.0f); break;
			default:              result = Graphics::Color(0.0f, 0.0f, 0.0f); break;
			}
			
			return result;
		}

		inline Graphics::Color TranslateColor(Graphics::Color& color)
		{
			if (color.color == COLOR_USER)
				return color;
			else
				return GetColor(static_cast<STDCOLOR>(color.color));
		}
	};
};
