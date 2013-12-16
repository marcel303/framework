#pragma once

#include "GuiTypes.h"

namespace Gui
{
	class MouseState
	{
	public:
		void Initialize(
			ButtonState left,
			ButtonState right,
			ButtonState middle,
			int x,
			int y,
			int z)
		{
			this->left = left;
			this->right = right;
			this->middle = middle;
			this->x = x;
			this->y = y;
			this->z = z;
		}

		int x;
		int y;
		int z;
		int deltaX;
		int deltaY;
		int deltaZ;
		union
		{
			struct
			{
				ButtonState left;
				ButtonState right;
				ButtonState middle;
			};
			ButtonState buttons[3];
		};
	};
};
