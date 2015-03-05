#pragma once

#include <string>
#include "gamesim.h"
#include "gametypes.h"
#include "Vec2.h"

class Dictionary;
struct Player;
class Sprite;
class Spriter;

class AnimData
{
public:
	class AnimTrigger
	{
	public:
		std::string action;
		Dictionary args;
	};

	typedef std::vector<AnimTrigger> AnimTriggers;

	class Anim
	{
	public:
		std::string name;
		float speed;
		
		std::map<int, AnimTriggers> frameTriggers;
		
		Anim()
			: speed(1.f)
		{
		}
	};
	
	typedef std::map<std::string, Anim> AnimMap;

	mutable AnimMap m_animMap;

	void load(const char * filename);
};

class SoundBag
{
	std::vector<std::string> m_files;
	bool m_random;

public:
	SoundBag();

	void load(const std::string & files, bool random);

	const char * getRandomSound(GameSim & gameSim, int & lastIndex) const;
};

class CharacterData
{
public:
	Dictionary m_props;
	AnimData m_animData;
#if USE_SPRITER_ANIMS
	Spriter * m_spriter;
#endif
	float m_spriteScale;
	PlayerSpecial m_special;
	std::map<std::string, SoundBag> m_sounds;

	//

	CharacterData(int characterIndex);
	~CharacterData();

	void load(int characterIndex);
};

class PlayerInstanceData
{
	friend struct Player;

#if !USE_SPRITER_ANIMS
	Sprite * m_sprite;
#endif
	
	static void handleAnimationAction(const std::string & action, const Dictionary & args);

public:
	PlayerInstanceData(Player * player, GameSim * gameSim);
	~PlayerInstanceData();

	void setCharacterIndex(int index);
	void handleCharacterIndexChange();

	void playSoundBag(const char * name, int volume);

	void addTextChat(const std::string & line);

	Player * m_player;
	GameSim * m_gameSim; // todo : remove?

	std::map<std::string, int> m_lastSoundIds;

	std::string m_textChat;
	int m_textChatTicks;

	struct InputState
	{
		InputState()
			: m_controllerIndex(-1)
		{
		}

		int m_controllerIndex;
		PlayerInput m_lastRecv;
		PlayerInput m_lastSent;
	} m_input;
};

void initCharacterData();
void shutCharacterData();
const CharacterData * getCharacterData(int characterIndex);
char * makeCharacterFilename(int characterIndex, const char * filename);
Color getCharacterColor(int characterIndex);
Color getPlayerColor(int playerIndex);
