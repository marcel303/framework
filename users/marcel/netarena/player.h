#pragma once

#include <string>
#include "gamesim.h"
#include "gametypes.h"
#include "Vec2.h"

class Dictionary;
struct Player;
class Sprite;
class Spriter;

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

class PlayerInstanceData
{
	friend struct Player;

	friend class PlayerAnim_NS;
	
	Dictionary m_props;

#if USE_SPRITER_ANIMS
	Spriter * m_spriter;
#else
	Sprite * m_sprite;
#endif
	float m_spriteScale;

	std::map<std::string, SoundBag> m_sounds;
	
	static void handleAnimationAction(const std::string & action, const Dictionary & args);

public:
	PlayerInstanceData(Player * player, GameSim * gameSim);
	~PlayerInstanceData();

	void setCharacterIndex(int index);
	void handleCharacterIndexChange();

	void playSoundBag(const char * name, int volume);

	void addTextChat(const std::string & line);

	Player * m_player;
	GameSim * m_gameSim;

	std::string m_textChat;
	int m_textChatTicks;

	struct InputState
	{
		InputState()
			: m_controllerIndex(-1)
			, m_actions(0)
		{
		}

		int m_controllerIndex;

		PlayerInput m_prevState; // todo : move prevState and currState to Player class?
		PlayerInput m_currState;
		PlayerInput m_lastSent;

		uint32_t m_actions;

		bool wasDown(int input) { return (m_prevState.buttons & input) != 0; }
		bool isDown(int input) { return (m_currState.buttons & input) != 0; }
		bool wentDown(int input) { return !wasDown(input) && isDown(input); }
		bool wentUp(int input) { return wasDown(input) && !isDown(input); }
		void next() { m_prevState = m_currState; m_actions = 0; }
	} m_input;
};

Color getCharacterColor(int characterIndex);
Color getPlayerColor(int playerIndex);
