#pragma once

#include <stack>
#include "Debugging.h"
#include "GuiRect.h"

namespace Gui
{
	namespace Graphics
	{
		// TODO: Rename. Area stack?

		class RectStack
		{
		public:
			RectStack()
			{
				Rect rect;
				rect.min.x = -1000000;
				rect.min.y = -1000000;
				rect.max.x = +1000000;
				rect.max.y = +1000000;
				m_stack.push(rect);
			}
			~RectStack()
			{
				// Size must be 1 upon exit.
				Assert(m_stack.size() == 1);
			}

			void PushClip(Rect rect)
			{
				Rect clippedRect;

				rect.Clip(Top(), clippedRect);

				m_stack.push(clippedRect);
			}

			void Pop()
			{
				// Size may never become less than 1.
				Assert(m_stack.size() >= 2);

				m_stack.pop();
			}

			Rect Top()
			{
				// Check if there is a top.
				Assert(m_stack.size() >= 1);

				return m_stack.top();
			}

		private:
			std::stack<Rect> m_stack;
		};
	}
}
