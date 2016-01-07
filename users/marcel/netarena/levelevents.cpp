#include "Calc.h"
#include "framework.h"
#include "gamesim.h"
#include "levelevents.h"

//

bool LevelEventTimer::tickActive(float dt)
{
	Assert(m_time >= 0.f && m_time <= m_duration);
	if (m_time < m_duration)
	{
		m_time += dt;
		if (m_time >= m_duration)
			m_time = m_duration;
		return true;
	}
	return false;
}

bool LevelEventTimer::tickComplete(float dt)
{
	Assert(m_time >= 0.f && m_time <= m_duration);
	if (m_time < m_duration)
	{
		m_time += dt;
		if (m_time >= m_duration)
		{
			m_time = m_duration;
			return true;
		}
	}
	return false;
}

float LevelEventTimer::getProgress() const
{
	Assert(m_time >= 0.f && m_time <= m_duration);
	if (m_duration == 0.f)
		return 0.f;
	else
		return m_time / m_duration;
}

bool LevelEventTimer::isActive() const
{
	Assert(m_time >= 0.f && m_time <= m_duration);
	return m_time < m_duration;
}

void LevelEventTimer::operator=(float time)
{
	Assert(time >= 0.f);
	m_duration = time;
	m_time = 0.f;
}

//

void LevelEvent_EarthQuake::start(GameSim & gameSim)
{
	endTimer = EVENT_EARTHQUAKE_DURATION;

	nextQuake(gameSim);
}

void LevelEvent_EarthQuake::nextQuake(GameSim & gameSim)
{
	quakeTimer = EVENT_EARTHQUAKE_INTERVAL + gameSim.RandomFloat(0.f, EVENT_EARTHQUAKE_INTERVAL_RAND);
}

void LevelEvent_EarthQuake::tick(GameSim & gameSim, float dt)
{
	if (endTimer.tickActive(dt))
	{
		if (quakeTimer.tickComplete(dt))
		{
			nextQuake(gameSim);

			// trigger quake

			gameSim.addScreenShake(0.f, 25.f, 1000.f, .3f, true);

			gameSim.doQuake(EVENT_EARTHQUAKE_PLAYER_BOOST);

			gameSim.playSound("events/quake/trigger.ogg");
		}
	}
}

//

CollisionBox LevelEvent_SpikeWalls::getWallCollision(const GameSim & gameSim, int side, float move) const
{
	const float sx = gameSim.m_arena.m_sxPixels * SPIKEWALLS_COVERAGE / 100 / 2;

	CollisionBox box;
	box.min = Vec2(0.f, 0.f);
	box.max = Vec2(sx, gameSim.m_arena.m_sxPixels);

	const float offset =
		side == 0
		?                        -sx + move * sx
		: gameSim.m_arena.m_sxPixels - move * sx;

	const Vec2 offsetVec(offset, 0.f);
	box.min += offsetVec;
	box.max += offsetVec;

	return box;
}

CollisionBox LevelEvent_SpikeWalls::getWallCollision(const GameSim & gameSim, int side) const
{
	float move = 0.f;

	switch (m_state)
	{
	case kState_Warn:
		move = .05f;
		break;

	case kState_Close:
		move = m_time / SPIKEWALLS_TIME_CLOSE;
		break;

	case kState_Closed:
		move = 1.f;
		break;

	case kState_Open:
		move = 1.f -  m_time / SPIKEWALLS_TIME_OPEN;
		break;
	}

	return getWallCollision(gameSim, side, move);
}

void LevelEvent_SpikeWalls::start(bool left, bool right)
{
	m_left = left;
	m_right = right;
	m_state = kState_Warn;
}

void LevelEvent_SpikeWalls::tick(GameSim & gameSim, float dt)
{
	switch (m_state)
	{
	case kState_Idle:
		break;

	case kState_Warn:
		m_time += dt;
		if (m_time >= SPIKEWALLS_TIME_PREVIEW)
		{
			m_state = kState_Close;
			m_time = 0.f;
		}
		break;

	case kState_Close:
		doCollision(gameSim);
		m_time += dt;
		if (m_time >= SPIKEWALLS_TIME_CLOSE)
		{
			m_state = kState_Closed;
			m_time = 0.f;
		}
		break;

	case kState_Closed:
		doCollision(gameSim);
		m_time += dt;
		if (m_time >= SPIKEWALLS_TIME_CLOSED)
		{
			m_state = kState_Open;
			m_time = 0.f;
		}
		break;

	case kState_Open:
		doCollision(gameSim);
		m_time += dt;
		if (m_time >= SPIKEWALLS_TIME_OPEN)
		{
			memset(this, 0, sizeof(*this));
		}
		break;
	}

	if (m_state != kState_Idle)
	{
		int numPlayersAlive = 0;
		for (int i = 0; i < MAX_PLAYERS; ++i)
			if (gameSim.m_players[i].m_isUsed && gameSim.m_players[i].m_isAlive)
				numPlayersAlive++;

		if (numPlayersAlive <= 1 && SPIKEWALLS_OPEN_ON_LAST_MAN_STANDING)
		{
			if (m_state == kState_Closed)
			{
				m_state = kState_Open;
				m_time = 0.f;
			}
		}
	}
}

void LevelEvent_SpikeWalls::doCollision(GameSim & gameSim, const CollisionBox & box, int dir)
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		Player & player = gameSim.m_players[i];

		if (player.m_isUsed && player.m_isAlive)
		{
			CollisionInfo playerCollision;
			if (player.getPlayerCollision(playerCollision))
			{
				if (playerCollision.intersects(box))
				{
					player.handleDamage(1.f, Vec2(dir * PLAYER_SWORD_PUSH_SPEED /* todo : speed */, 0.f), 0);
				}
			}
		}
	}
}

void LevelEvent_SpikeWalls::doCollision(GameSim & gameSim)
{
	if (m_left)
		doCollision(gameSim, getWallCollision(gameSim, 0), +1);

	if (m_right)
		doCollision(gameSim, getWallCollision(gameSim, 1), -1);
}

void LevelEvent_SpikeWalls::draw(const GameSim & gameSim) const
{
	const Color colorWall(127, 0, 0, 127);

	for (int i = 0; i < 2; ++i)
	{
		if (i == 0 && !m_left)
			continue;
		if (i == 1 && !m_right)
			continue;

		const CollisionBox box = getWallCollision(gameSim, i);

		switch (m_state)
		{
		case kState_Warn:
			{
				const float t = m_time / SPIKEWALLS_TIME_PREVIEW;
				setColor(colorRed);
				drawRect(box.min[0], box.min[1], box.max[0], box.max[1]);
			}
			break;

		case kState_Close:
			{
				const float t = m_time / SPIKEWALLS_TIME_CLOSE;
				setColor(colorWall);
				drawRect(box.min[0], box.min[1], box.max[0], box.max[1]);
			}
			break;

		case kState_Closed:
			{
				const float t = m_time / SPIKEWALLS_TIME_CLOSED;
				setColor(colorWall);
				drawRect(box.min[0], box.min[1], box.max[0], box.max[1]);
			}
			break;

		case kState_Open:
			{
				const float t = m_time / SPIKEWALLS_TIME_OPEN;
				setColor(colorWall);
				drawRect(box.min[0], box.min[1], box.max[0], box.max[1]);
			}
			break;
		}
	}

	setColor(colorWhite);
}
