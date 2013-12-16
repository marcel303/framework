#include "libgui_precompiled.h"

#if 0

#include "GuiGraphicsCanvas.h"
#include "GuiGraphicsCursor.h"

namespace Gui
{
	namespace Graphics
	{
		Cursor::Cursor()
		{
			SetTrailFadeTime(0.0f);
			m_timer.SetTickFrequency(20.0);
		}

		Cursor::Cursor(const Image& image)
		{
			SetCursorImage(image);
			SetTrailImage(image);
			SetTrailFadeTime(0.0f);
			m_timer.SetTickFrequency(20.0);
		}

		Cursor::~Cursor()
		{
		}

		const Image& Cursor::GetCursorImage() const
		{
			return m_cursorImage;
		}

		const Image& Cursor::GetTrailImage() const
		{
			return m_trailImage;
		}

		float Cursor::GetTrailFadeTime() const
		{
			return m_trailFadeTime;
		}

		void Cursor::SetCursorImage(const Image& image)
		{
			m_cursorImage = image;
		}

		void Cursor::SetCursorCenter(Point center)
		{
			m_cursorCenter = center;
		}

		void Cursor::SetTrailImage(const Image& image)
		{
			m_trailImage = image;
		}

		void Cursor::SetTrailCenter(Point center)
		{
			m_trailCenter = center;
		}

		void Cursor::SetTrailFadeTime(float time)
		{
			m_trailFadeTime = time;
		}

		void Cursor::OnMove(int x, int y, MouseState* mouseState)
		{
			if (m_trailFadeTime == 0.0f)
				return;

			if (m_timer.ReadTickEvent() == false)
				return;

			m_timer.ClearTickEvents();

			TrailNode& node = AllocateTrailNode();

			node.x = static_cast<float>(x);
			node.y = static_cast<float>(y);
		}

		void Cursor::OnButton(MouseButton button, ButtonState state, MouseState* mouseState)
		{
			if (m_trailFadeTime == 0.0f)
				return;

			TrailNode& node = AllocateTrailNode();

			node.x = static_cast<float>(mouseState->x);
			node.y = static_cast<float>(mouseState->y);
		}

		void Cursor::Update(float deltaTime)
		{
			for (size_t i = 0; i < m_trail.size(); ++i)
			{
				m_trail[i].x += deltaTime * 100.0f;
				m_trail[i].y += deltaTime * 200.0f;
			}
		}

		void Cursor::Render(int x, int y)
		{
			UpdateTrail();

			for (size_t i = 0; i < m_trail.size(); ++i)
				Canvas::I().DrawImage(
					static_cast<int>(m_trail[i].x - m_trailCenter.x),
					static_cast<int>(m_trail[i].y - m_trailCenter.y), m_trailImage);

			Canvas::I().DrawImage(x - m_cursorCenter.x, y - m_cursorCenter.y, m_cursorImage);
		}

		Cursor::TrailNode& Cursor::AllocateTrailNode()
		{
			TrailNode node;

			node.time = m_timer.GetTime();
			node.index = node.time;

			m_trail.push_back(node);

			return m_trail[m_trail.size() - 1];
		}

		const std::vector<Cursor::TrailNode>& Cursor::GetTrailNodes()
		{
			return m_trail;
		}

		void Cursor::UpdateTrail()
		{
			// Remove aged nodes.

			for (std::vector<TrailNode>::iterator i = m_trail.begin(); i != m_trail.end();)
			{
				if (i->time + m_trailFadeTime < m_timer.GetTime())
					i = m_trail.erase(i);
				else
					++i;
			}

			// Sort by index.

			std::sort(m_trail.begin(), m_trail.end());
		}
	};
};

#endif
