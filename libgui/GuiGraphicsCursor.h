#pragma once

#if 0

#include <boost/shared_ptr.hpp>
#include <vector>
#include "GuiGraphicsImage.h"
#include "GuiMouseState.h"
#include "GuiPoint.h"
#include "GuiTypes.h"
#include "Timer.h"

namespace Gui
{
	namespace Graphics
	{
		class Cursor
		{
		public:
			Cursor();
			Cursor(const Image& image);
			virtual ~Cursor();

			const Image& GetCursorImage() const;
			const Image& GetTrailImage() const;
			float GetTrailFadeTime() const;

			void SetCursorImage(const Image& image);
			void SetCursorCenter(Point center);
			void SetTrailImage(const Image& image);
			void SetTrailCenter(Point center);
			void SetTrailFadeTime(float time);

			virtual void OnMove(int x, int y, MouseState* mouseState);
			virtual void OnButton(MouseButton button, ButtonState state, MouseState* mouseState);
			virtual void Update(float deltaTime);
			virtual void Render(int x, int y);

		protected:
			class TrailNode
			{
			public:
				float time;
				float index;

				float x;
				float y;

				ButtonState left;
				ButtonState right;
				ButtonState middle;

				void* userData;

				bool operator<(const TrailNode& other) const
				{
					return index < other.index;
				}
			};

			TrailNode& AllocateTrailNode();
			const std::vector<TrailNode>& GetTrailNodes();
			void UpdateTrail();

		private:
			Image m_cursorImage;
			Point m_cursorCenter;

			Image m_trailImage;
			Point m_trailCenter;

			std::vector<TrailNode> m_trail;
			float m_trailFadeTime;
			Timer m_timer;
		};

		typedef boost::shared_ptr<Cursor> ShCursor;
	}
};

#endif
