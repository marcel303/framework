#include "gamedefs.h"
#include "gamesim.h"
#include "hud.h"

OPTION_DECLARE(int, PLAYER_STATUS_OFFSET_X_L, 125);
OPTION_DECLARE(int, PLAYER_STATUS_OFFSET_X_R, 125);
OPTION_DECLARE(int, PLAYER_STATUS_OFFSET_Y_T, 150);
OPTION_DECLARE(int, PLAYER_STATUS_OFFSET_Y_B, 150);
OPTION_DEFINE(int, PLAYER_STATUS_OFFSET_X_L, "UI/Player Status HUD/Offset X L");
OPTION_DEFINE(int, PLAYER_STATUS_OFFSET_X_R, "UI/Player Status HUD/Offset X R");
OPTION_DEFINE(int, PLAYER_STATUS_OFFSET_Y_T, "UI/Player Status HUD/Offset Y T");
OPTION_DEFINE(int, PLAYER_STATUS_OFFSET_Y_B, "UI/Player Status HUD/Offset Y B");

static Vec2 getPlayerStatusHudLocation(int playerIndex)
{
	if (playerIndex >= 0 && playerIndex < MAX_PLAYERS)
	{
		const Vec2 location[MAX_PLAYERS] =
		{
			Vec2(         PLAYER_STATUS_OFFSET_X_L,          PLAYER_STATUS_OFFSET_Y_T),
			Vec2(GFX_SX - PLAYER_STATUS_OFFSET_X_R,          PLAYER_STATUS_OFFSET_Y_T),
			Vec2(         PLAYER_STATUS_OFFSET_X_L, GFX_SY - PLAYER_STATUS_OFFSET_Y_B),
			Vec2(GFX_SX - PLAYER_STATUS_OFFSET_X_R, GFX_SY - PLAYER_STATUS_OFFSET_Y_B)
		};

		return location[playerIndex];
	}
	else
	{
		fassert(false);
		return Vec2(0.f, 0.f);
	}
}

static const char * getPlayerStatusHudSpriterName(const GameSim & gameSim, int playerIndex)
{
	const int characterIndex = gameSim.m_players[playerIndex].m_characterIndex;

	fassert(characterIndex >= 0 && characterIndex < MAX_CHARACTERS);
	if (characterIndex >= 0 && characterIndex < MAX_CHARACTERS)
	{
		const char * filenames[MAX_CHARACTERS] =
		{
			"ui/ingame-portraits/TyllyUI/Sprite.scml",
			"ui/ingame-portraits/TyllyUI/Sprite.scml",
			"ui/ingame-portraits/TyllyUI/Sprite.scml",
			"ui/ingame-portraits/TyllyUI/Sprite.scml",
			"ui/ingame-portraits/TyllyUI/Sprite.scml",
			"ui/ingame-portraits/TyllyUI/Sprite.scml",
			"ui/ingame-portraits/TyllyUI/Sprite.scml",
			"ui/ingame-portraits/TyllyUI/Sprite.scml"
		};

		return filenames[characterIndex];
	}
	else
	{
		return "";
	}
}

static const char * getPlayerStatusHudSpriterAnimName(PlayerStatusHud::State state)
{
	switch (state)
	{
	case PlayerStatusHud::kState_Idle:
		return "Idle";
	case PlayerStatusHud::kState_Kill:
		return "Idle_Anim";
	case PlayerStatusHud::kState_Death:
		return "Dead";
		//return "Dead_Transition";
	case PlayerStatusHud::kState_Spawn:
		return "Spawn";
	case PlayerStatusHud::kState_Score:
		return "Idle_Anim";

	default:
		fassert(false);
		return "";
	}
}

void PlayerStatusHud::tick(GameSim & gameSim, int playerIndex, float dt)
{
	stateTime += dt;
}

void PlayerStatusHud::draw(const GameSim & gameSim, int playerIndex) const
{
	fassert(playerIndex >= 0 && playerIndex < MAX_PLAYERS);
	if (playerIndex >= 0 && playerIndex < MAX_PLAYERS)
	{
		Spriter spriter(getPlayerStatusHudSpriterName(gameSim, playerIndex));
		SpriterState spriterState = m_spriterState;

		const Vec2 location = getPlayerStatusHudLocation(playerIndex);
		spriterState.x = location[0];
		spriterState.y = location[1];
		spriterState.startAnim(spriter, getPlayerStatusHudSpriterAnimName(state));
		spriterState.animTime = stateTime;

		setColor(colorWhite);
		spriter.draw(spriterState);
	}
}

void PlayerStatusHud::handleCharacterIndexChange()
{
	state = kState_Idle;
	stateTime = 0.f;
}

void PlayerStatusHud::handleKill()
{
	state = kState_Kill;
	stateTime = 0.f;
}

void PlayerStatusHud::handleDeath()
{
	state = kState_Death;
	stateTime = 0.f;
}

void PlayerStatusHud::handleSpawn()
{
	state = kState_Spawn;
	stateTime = 0.f;
}

void PlayerStatusHud::handleRespawn()
{
	state = kState_Spawn;
	stateTime = 0.f;
}

void PlayerStatusHud::handleNewRound()
{
	state = kState_Spawn;
	stateTime = 0.f;
}

void PlayerStatusHud::handleScore()
{
	state = kState_Score;
	stateTime = 0.f;
}
