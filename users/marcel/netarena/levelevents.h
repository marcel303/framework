#pragma once

// level events

// todo : some events can be combined. make a list of events that can be combined!

struct LevelEventTimer
{
	float m_duration;
	float m_time;

	bool tickActive(float dt); // tick timer and return whether its active
	bool tickComplete(float dt); // tick timer and return whether it completed
	float getProgress() const;
	bool isActive() const;

	void operator=(float time);
};

// the earth suddenly starts shaking. players will loose their footing and be kicked into the air
struct LevelEvent_EarthQuake
{
	LevelEventTimer endTimer;
	LevelEventTimer quakeTimer;
};

// a gravity well appears. players get sucked into it and get concentrated into a smaller area
// note : should be possible to escape the well by moving, to avoid getting stuck against walls
//         but it should be hard near the center to escape!
struct LevelEvent_GravityWell
{
	LevelEventTimer endTimer;
	int m_x;
	int m_y;
};

// destructible blocks start exploding! the level will slowly disintegrate!
// note : should only be activated when there's enough blocks in a level. stop at 50% or so of the
//        starting number of blocks
struct LevelEvent_DestroyDestructibleBlocks
{
	int m_remainingBlockCount;
	LevelEventTimer destructionTimer;
};

// time suddenly slows down for a while
// note : need something visual to indicate the effect. maybe a giant hourglas appears on the background
//        layer, floating about
struct LevelEvent_TimeDilation
{
	LevelEventTimer endTimer;
};

// spike walls start closing in from the left, right, or both sides
// note : should be non lethal when they appear. deploy spikes at some point
// note : disable respawning while effect is active to avoid respawning in wall?
struct LevelEvent_SpikeWalls
{
	static const int SPIKEWALLS_TIME_PREVIEW = 2;
	static const int SPIKEWALLS_TIME_CLOSE = 6;
	static const int SPIKEWALLS_TIME_CLOSED = 6;
	static const int SPIKEWALLS_TIME_OPEN = 4;

	enum State
	{
		kState_Idle,
		kState_Warn,
		kState_Close,
		kState_Closed,
		kState_Open,
	};

	CollisionBox getWallCollision(int side, float move) const;
	CollisionBox getWallCollision(int side) const;

	void start(bool left, bool right);
	void tick(GameSim & gameSim, float dt);

	void doCollision(GameSim & gameSim, const CollisionBox & box, int dir);
	void doCollision(GameSim & gameSim);

	void draw() const;

	bool isActive() const { return m_state != kState_Idle; }

	State m_state;
	float m_time;

	bool m_right;
	bool m_left;
};

// the wind suddenly starts blowing. players get accelerated in the left/right direction
// note: need visual. maybe a wind layer with leafs on the foreground layer, maybe rain?
struct LevelEvent_Wind
{
	LevelEventTimer endTimer;
};

// barrels start dropping from the sky! barrels can be hit for powerful attack
// note : should do auto aim to some extent so it's actually possible to hit other players
// note : collision should be disabled on barrels. fake gravity/no gravity/floatiness
struct LevelEvent_BarrelDrop
{
	LevelEventTimer endTimer;
	LevelEventTimer spawnTimer;
};

// the level turns dark for a while. players have limited vision
// note : accompanied by lightning and rain effects?
struct LevelEvent_NightDayCycle
{
	LevelEventTimer endTimer;
};

enum LevelEvent
{
	kLevelEvent_EarthQuake,
	kLevelEvent_GravityWell,
	kLevelEvent_DestroyBlocks,
	kLevelEvent_TimeDilation,
	kLevelEvent_SpikeWalls,
	kLevelEvent_Wind,
	kLevelEvent_BarrelDrop,
	kLevelEvent_NightDayCycle,
	kLevelEvent_COUNT
};
