#include "Calc.h"
#include "gamedefs.h"
#include "gamesim.h"
#include "quicklook.h"

QuickLook::QuickLook()
	: m_isActive(false)
	, m_animProgress(0.f)
{
}

QuickLook::~QuickLook()
{
}

void QuickLook::open(bool animated)
{
	m_isActive = true;

	if (!animated)
		m_animProgress = 1.f;
}

void QuickLook::close(bool animated)
{
	m_isActive = false;

	if (!animated)
		m_animProgress = 0.f;
}

bool QuickLook::isActive() const
{
	return m_isActive;
}

void QuickLook::tick(float dt)
{
	if (m_isActive)
		m_animProgress = Calc::Min(1.f, m_animProgress + dt / UI_QUICKLOOK_OPEN_TIME);
	else
		m_animProgress = Calc::Max(0.f, m_animProgress - dt / UI_QUICKLOOK_CLOSE_TIME);
}

void QuickLook::drawWorld(GameSim & gameSim)
{
	if (m_isActive)
	{
		// todo : for each player : draw score box
	}
}

void QuickLook::drawHud(GameSim & gameSim)
{
	const int py = 300;
	const int sx = 400;
	const int sy = 500;

	if (m_animProgress != 0.f)
	{
		// todo : draw panel with scores in them

		gxMatrixMode(GL_MODELVIEW);
		gxPushMatrix();
		{
			gxTranslatef(-sx * (1.f - m_animProgress), py, 0.f);

			setColor(0, 0, 0, 191);
			drawRect(0, 0, sx, sy);

			setColor(colorWhite);
		}
		gxPopMatrix();
	}
}
