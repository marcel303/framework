#pragma once

#include <string>
#include "gamesim.h"
#include "gametypes.h"
#include "netobject.h"
#include "Vec2.h"

class Dictionary;
struct Player;
class Sprite;

#if !ENABLE_CLIENT_SIMULATION
class PlayerPos_NS : public NetSerializable
{
	virtual void SerializeStruct();

public:
	PlayerPos_NS(NetSerializableObject * owner)
		: NetSerializable(owner, (1 << REPLICATION_CHANNEL_UNRELIABLE))
	{
		SetChannel(REPLICATION_CHANNEL_UNRELIABLE);
	}
};

class PlayerState_NS : public NetSerializable
{
	virtual void SerializeStruct();

public:
	PlayerState_NS(NetSerializableObject * owner);

	int8_t playerId;
};
#endif

class PlayerAnim_NS : public NetSerializable
{
	virtual void SerializeStruct();

public:
	PlayerAnim_NS(NetSerializableObject * owner)
		: NetSerializable(owner)
	{
	}

	void SetAnim(int anim, bool play, bool restart);
	void SetAttackDirection(int dx, int dy);
	void SetPlay(bool play);
	void ApplyAnim();
};

class SoundBag
{
	std::vector<std::string> m_files;
	bool m_random;
	int m_lastIndex;

public:
	SoundBag();

	void load(const std::string & files, bool random);

	const char * getRandomSound(GameSim & gameSim);
};

struct PlayerInput
{
	PlayerInput()
		: buttons(0)
		, analogX(0)
		, analogY(0)
	{
	}

	bool operator!=(const PlayerInput & other)
	{
		return
			buttons != other.buttons ||
			analogX != other.analogX ||
			analogY != other.analogY;
	}

	uint16_t buttons;
	int8_t analogX;
	int8_t analogY;
};

class PlayerNetObject : public NetObject
{
	friend struct Player;

	friend class PlayerAnim_NS;
	friend class PlayerPos_NS;
	friend class PlayerState_NS;

#if !ENABLE_CLIENT_SIMULATION
	PlayerPos_NS m_pos;
	PlayerState_NS m_state;
#endif
	PlayerAnim_NS m_anim;

	bool m_isAuthorative;

	Dictionary m_props;

	Sprite * m_sprite;
	float m_spriteScale;

	std::map<std::string, SoundBag> m_sounds;
	
	static void handleAnimationAction(const std::string & action, const Dictionary & args);

	// ReplicationObject
	virtual bool RequiresUpdating() const { return true; }

	// NetObject
	virtual NetObjectType getType() const { return kNetObjectType_Player; }

public:
	PlayerNetObject(uint32_t netId = 0, uint16_t owningChannelId = 0, Player * player = 0, GameSim * gameSim = 0);
	~PlayerNetObject();

	int getScore() const;
	int getTotalScore() const;

#if !ENABLE_CLIENT_SIMULATION
	void setPlayerId(int id);
	int getPlayerId() const { return m_state.playerId; }
#else
	int m_playerId;
	void setPlayerId(int id) { m_playerId = id; }
	int getPlayerId() const { return m_playerId; }
#endif

	int getCharacterIndex() const;
	void setCharacterIndex(int index);
	bool hasValidCharacterIndex() const;
	void handleCharacterIndexChange();

	void playSoundBag(const char * name, int volume);

	Player * m_player;
	GameSim * m_gameSim;

	struct InputState
	{
		InputState()
			: m_controllerIndex(-1)
		{
		}

		int m_controllerIndex;

		PlayerInput m_prevState;
		PlayerInput m_currState;

		bool wasDown(int input) { return (m_prevState.buttons & input) != 0; }
		bool isDown(int input) { return (m_currState.buttons & input) != 0; }
		bool wentDown(int input) { return !wasDown(input) && isDown(input); }
		bool wentUp(int input) { return wasDown(input) && !isDown(input); }
		void next() { m_prevState = m_currState; }
	} m_input;
};
