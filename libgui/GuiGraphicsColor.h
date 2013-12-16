#pragma once

#include "GuiTypes.h"

namespace Gui
{
	namespace Graphics
	{
		class Color
		{
		public:
			inline Color()
			{
				r = g = b = a = 0.0f;
				color = COLOR_USER;
			}
			inline Color(float r, float g, float b)
			{
				this->r = r;
				this->g = g;
				this->b = b;
				this->a = 1.0f;
				color = COLOR_USER;
			}
			inline Color(float r, float g, float b, float a)
			{
				this->r = r;
				this->g = g;
				this->b = b;
				this->a = a;
				color = COLOR_USER;
			}
			inline Color(int color)
			{
				this->color = color;
			}

			inline float& operator[](int index)
			{
				return rgba[index];
			}

			union
			{
				struct
				{
					float r;
					float g;
					float b;
					float a;
				};
				float rgba[4];
			};

			int color; // Color enum. See STDCOLOR enum in GuiTypes.h.
		};
	};
};
