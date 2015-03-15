#include "arena.h"
#include "bullet.h"
#include "Calc.h"
#include "client.h"
#include "framework.h"
#include "host.h"
#include "main.h"
#include "player.h"
#include "Timer.h"

#define USE_NEW_COLLISION_CODE 0

/*

todo:

** HIGH PRIORITY **

- add curve editing through options

- melee weapon kinetic energy / character data property

- zweihander sword guy
	- slow attack
	- special on ground:
		- sword up, 2x movement speed, for max 5 seconds
		- on release: stomp attack, sword gets stuck in ground
		- 1 second recovery time
	- special in air:
		- stomp attack, sword gets stuck in ground
		- 1 second recovery time

- shield guy
	- can block in 1 direction
	- has charge that pushes and can flatten players against walls

- axe guy
	- can throw axe away, has to pick it up again

- fix up/down/dash animations
+ analog jump, accel over frames

- prototype pipe bomb
	1 throw/1 explode

** MEDIUM PRIORITY **

+ add HOME/END support to text field

- prototype invisibility ability
	charge period

- prototype rocket punch
	+ idle in air during charge
	+ vulnerable during charge
	+ analog stick = direction of attack
	max charge = faster, further, succeed/or not, pass through all destructibles/not (strength)

	v2:
	+ short recharge
	+ auto discharge
	+ debuff on attack finish

	v3:
	- remove debuff

- add pickup art
- add path for special
- prototype grappling hook

- add callstack gathering
+ add IP's to player names (visible on host)

- check build ID

- better attach to platform logic, so we can have movers closer to each other without the player bugging

** LOW PRIORITY **

- add typing/pause bubble on char
- add key binding

** OTHER **

- prototype 'mine dropper'
	- move without gravity, drop bombs
	- when done, bombs explode one by one

- add Spriter hitbox support

- +1 icon on kill in token hunt game mode

- prototype CHAOS event: maximum pickups

- prototype gravity well level event
- prototype earth quake level event
- prototype wind level event
- prototype time dilation level event (fast and/or slowed down?)
- prototype spike walls level event
- prototype barrel drop level event
- prototype day/night level event

- improve networking reliability layer on resend. trips currently

- gravity well -> make it partially a linear or other kind of curve + more powerful at a distance
- slow down on kill for everyone acceptable?
- add build version check for online
- send client network stats to host so they can be visualized

- add support for CRC compare at any time during game sim tick. log CRC's with function/line number and compare?

- disable changing game options after everyone has readied up

- zoom in on winning player, wait for a while before transitioning to the next round

- need to be able to kick player at char select
- drop rate should scale with number of players
- remove angels in spawn locations -> reduce background noise
- buff star player ?
- score feedback, especially in token hunt mode
- investigate killing the rambo mode, soldat

- better death feedback
- team based game mode
- ammo despawn na x seconds + indicator (?)

- blood particles
- fill the level with lava
- earthquake. players go up on quake

** DONE **

+ torch light flicker speed should be time dilated too
+ add key repeat to text input
+ add global method to get character color
+ add global cache of character properties. reduce need for m_instanceData
+ add task bar blink on game start
+ reset level events on round end
+ slow down not on non-player kill (if there's no attacker like when falling into spikes)
+ add names to players on talk? -> add color to results screen
+ chat log when chat is open
+ increase chat visibility time
+ fix late join
+ fix desync as both players get hit/killed by grenade
+ add user name input
+ fix text chat player index
+ add FixedString class that can be memset(0) and copied over the net. use it for player name, etc
+ clean up flow for adding players
+ test time dilation on kill
+ game speed var
+ player invincibility on spawn (2 seconds?)
+ add game mode selection to char select
+ count down timer game start after char select
+ add time dilation effect on last kill
+ verify bullet pool allocation order -> may be a source of desync issues, due to differences in the order of bullet updates
+ textchat: left/right/insert text/delete key
+ i think less bubble/ice freeze time
+ respawn visual to more quickly locate your respawn location
+ clash sound on attack cancel
+ random sound selection on event to avoid repeat sounds
+ fire up/down
+ fire affects destructible blocks
+ respawn on round begin
+ respawn on level load
+ avoid repeating spawn location
+ play sounds for current client only to avoid volume ramp
+ meelee X, pickup B
+ wrapping support attack
+ add pickup sound
+ add direct connect option
+ spring block tweakable
+ respawn delay
+ character selection
+ force feedback cling animation
+ attack anim up and down
+ analog controls
+ token hunt
+ spawn anim
+ death input
+ hitbox spikes
+ manual spawn (5 seconds?)
+ taunt button
+ shotgun ammo x3
+ add attack vel to player vel
+ attack cancel
+ bullet teleport
+ separate player hitbox for damage
+ grenades!
+ ammo drop gravity
+ input lock dash weg
+ jump velocity reset na 10 pixels of richting change
+ attack cooldown
+ camera shakes
+ real time lighting

*/

// from internal.h
void splitString(const std::string & str, std::vector<std::string> & result);
void splitString(const std::string & str, std::vector<std::string> & result, char c);

OPTION_DECLARE(int, g_playerCharacterIndex, 0);
OPTION_DEFINE(int, g_playerCharacterIndex, "Player/Character Index (On Create)");
OPTION_ALIAS(g_playerCharacterIndex, "character");

OPTION_DECLARE(bool, s_unlimitedAmmo, false);
OPTION_DEFINE(bool, s_unlimitedAmmo, "Player/Unlimited Ammo");

OPTION_DECLARE(bool, s_noSpecial, false);
OPTION_DEFINE(bool, s_noSpecial, "Player/Disable Special Abilities");

OPTION_DECLARE(float, PLAYER_SPRITE_SCALE, 1.f/5.f);
OPTION_DEFINE(float, PLAYER_SPRITE_SCALE, "Experimental/Player Scale");
OPTION_STEP(PLAYER_SPRITE_SCALE, 0, 0, .01f);

OPTION_DECLARE(float, PLAYER_ANIM_MULTIPLIER, 1.f);
OPTION_DEFINE(float, PLAYER_ANIM_MULTIPLIER, "Experimental/Animation Speed Multiplier");
OPTION_STEP(PLAYER_ANIM_MULTIPLIER, 0, 0, .01f);

#define WRAP_AROUND_TOP_AND_BOTTOM 1

#define GAMESIM m_instanceData->m_gameSim

// todo : m_isGrounded should be true when stickied too. review code and make change!

struct PlayerAnimInfo
{
	const char * file;
	const char * name;
	int prio;
} s_animInfos[kPlayerAnim_COUNT] =
{
	{ nullptr,       nullptr,      0 },
	{ "sprite.scml", "Idle" ,      1 },
	{ "sprite.scml", "InAir" ,     1 },
	{ "sprite.scml", "Jump" ,      2 },
	{ "sprite.scml", "WallSlide",  3 },
	{ "sprite.scml", "Walk",       4 },
	{ "sprite.scml", "Attack",     5 },
	{ "sprite.scml", "AttackUp",   5 },
	{ "sprite.scml", "AttackDown", 5 },
	{ "sprite.scml", "Shoot",      5 },
	{ "sprite.scml", "Walk", 5 }, // fixme : charge
	{ "sprite.scml", "Walk", 5 }, // fixme : attack
	//{ "sprite.scml", "RocketPunch_Charge", 5 },
	//{ "sprite.scml", "RocketPunch_Attack", 5 },
	{ "sprite.scml", "AirDash",    5 },
	{ "sprite.scml", "Spawn",      6 },
	{ "sprite.scml", "Die",        7 }
};

//

#include "FileStream.h"
#include "StreamReader.h"

void AnimData::load(const char * filename)
{
	try
	{
		Anim * currentAnim = 0;

		FileStream f;
		f.Open(filename, (OpenMode)(OpenMode_Read|OpenMode_Text));

		StreamReader r(&f, false);

		std::vector<std::string> lines = r.ReadAllLines();
		
		for (size_t l = 0; l < lines.size(); ++l)
		{
			const std::string & line = lines[l];

			// format: <name> <key>:<value> <key:value> <key..
			
			std::vector<std::string> parts;
			splitString(line, parts);
			
			if (parts.size() == 0 || parts[0][0] == '#')
			{
				// empty line or comment
				continue;
			}
			
			if (parts.size() == 1)
			{
				logError("%s: missing parameters: %s (%s)", filename, line.c_str(), parts[0].c_str());
				continue;
			}
			
			const std::string section = parts[0];
			Dictionary args;
			
			for (size_t i = 1; i < parts.size(); ++i)
			{
				const size_t separator = parts[i].find(':');
				
				if (separator == std::string::npos)
				{
					logError("%s: incorrect key:value syntax: %s (%s)", filename, line.c_str(), parts[i].c_str());
					continue;
				}
				
				const std::string key = parts[i].substr(0, separator);
				const std::string value = parts[i].substr(separator + 1, parts[i].size() - separator - 1);
				
				if (key.size() == 0 || value.size() == 0)
				{
					logError("%s: incorrect key:value syntax: %s (%s)", filename, line.c_str(), parts[i].c_str());
					continue;
				}
				
				if (args.contains(key.c_str()))
				{
					logError("%s: duplicate key: %s (%s)", filename, line.c_str(), key.c_str());
					continue;
				}
				
				args.setString(key.c_str(), value.c_str());
			}
			
			// animation name:walk grid_x:0 grid_y:0 frames:12 rate:4 loop:0 pivot_x:2 pivot_y:2
			// trigger frame:3 action:sound sound:test.wav
			
			if (section == "animation")
			{
				currentAnim = 0;
				
				Anim anim;
				anim.name = args.getString("name", "");
				anim.speed = args.getFloat("speed", 1.f);
				if (anim.name.empty())
				{
					logError("%s: name not set: %s", filename, line.c_str());
					continue;
				}
				
				currentAnim = &m_animMap.insert(AnimMap::value_type(anim.name, anim)).first->second;
			}
			else if (section == "trigger")
			{
				if (currentAnim == 0)
				{
					logError("%s: must first define an animation before adding triggers to it! %s", filename, line.c_str());
					continue;
				}
				
				const int time = args.getInt("time", -1);
				
				if (time < 0)
				{
					logWarning("%s: time must be >= 0: %s", filename, line.c_str());
					continue;
				}
				
				//log("added frame trigger. frame=%d, on=%s, action=%s", frame, event.c_str(), action.c_str());
				
				AnimTrigger trigger;
				trigger.action = args.getString("action", "");
				trigger.args = args;
				
				currentAnim->frameTriggers[time].push_back(trigger);
			}
			else
			{
				logError("%s: unknown section: %s (%s)", filename, line.c_str(), section.c_str());
			}
		}
	}
	catch (std::exception & e)
	{
		logError("%s: failed to open file: %s", filename, e.what());
	}
}

//

SoundBag::SoundBag()
	: m_random(false)
{
}

void SoundBag::load(const std::string & files, bool random)
{
	m_files.clear();
	splitString(files, m_files, ',');
	m_random = random;
}

const char * SoundBag::getRandomSound(GameSim & gameSim, int & lastSoundId) const
{
	if (m_files.empty())
		return "";
	else if (m_files.size() == 1)
		return m_files.front().c_str();
	else
	{
		for (;;)
		{
			int index;

			if (m_random)
			{
				if (DEBUG_RANDOM_CALLSITES)
					LOG_DBG("Random called from getRandomSound");
				index = gameSim.Random() % m_files.size();
			}
			else
				index = (index + 1) % m_files.size();

			// add 1 to the index to get the sound ID, so when lastSoundId initially is 0 it means 'pick any sound'
			const int soundId = index + 1;

			if (soundId != lastSoundId)
			{
				lastSoundId = soundId;

				return m_files[index].c_str();
			}
		}
	}

	return "";
}

//

void PlayerInstanceData::handleAnimationAction(const std::string & action, const Dictionary & args)
{
	PlayerInstanceData * self = args.getPtrType<PlayerInstanceData>("obj", 0);
	if (self)
	{
		if (g_devMode)
		{
			log("action: %s", action.c_str());
		}

		Player * player = self->m_player;

		if (action == "gravity_enable")
		{
			player->m_animAllowGravity = args.getBool("enable", true);

			if (g_devMode)
				logDebug("-> m_animAllowGravity = %d", player->m_animAllowGravity);
		}
		else if (action == "steering_enable")
		{
			player->m_animAllowSteering = args.getBool("enable", true);
		}
		else if (action == "set_anim_vel")
		{
			player->m_animVel[0] = args.getFloat("x", player->m_animVel[0]);
			player->m_animVel[1] = args.getFloat("y", player->m_animVel[1]);
		}
		else if (action == "set_dash_vel")
		{
			Vec2 dir(player->m_input.m_currState.analogX, player->m_input.m_currState.analogY);
			dir.Normalize();

			player->m_vel += dir * args.getFloat("x", player->m_animVel[0]);
		}
		else if (action == "set_anim_vel_abs")
		{
			player->m_animVelIsAbsolute = args.getBool("abs", player->m_animVelIsAbsolute);
			if (args.getBool("reset", true))
				player->m_vel = Vec2();

			if (g_devMode)
				logDebug("-> vel = (%f, %f), m_animVelIsAbsolute = %d", player->m_vel[0], player->m_vel[1], player->m_animVelIsAbsolute);
		}
		else if (action == "set_attack_vel")
		{
			player->m_attack.attackVel[0] = args.getFloat("x", player->m_attack.attackVel[0]);
			player->m_attack.attackVel[1] = args.getFloat("y", player->m_attack.attackVel[1]);
		}
		else if (action == "commit_attack_vel")
		{
			player->m_vel[0] += player->m_attack.attackVel[0] * player->m_attackDirection[0];
			player->m_vel[1] += player->m_attack.attackVel[1] * player->m_attackDirection[1];
			player->m_attack.attackVel[0] = 0.f;
			player->m_attack.attackVel[1] = 0.f;
		}	
		else if (action == "sound")
		{
			self->m_gameSim->playSound(args.getString("file", "").c_str(), args.getInt("volume", 100));
		}
		else if (action == "char_sound")
		{
			self->m_gameSim->playSound(makeCharacterFilename(player->m_characterIndex, args.getString("file", "").c_str()), args.getInt("volume", 100));
		}
		else if (action == "char_soundbag")
		{
			std::string name = args.getString("name", "");

			self->playSoundBag(name.c_str(), args.getInt("volume", 100));
		}
		else
		{
			logError("unknown action: %s", action.c_str());
		}
	}
}

void Player::testCollision(const CollisionShape & shape, void * arg, CollisionCB cb)
{
	CollisionInfo collision;
	if (getPlayerCollision(collision))
	{
		CollisionShape collisionShape = collision;

		if (collisionShape.intersects(shape))
		{
			cb(shape, arg, 0, 0, this);
		}
	}
}

bool Player::getPlayerCollision(CollisionInfo & collision) const
{
	Assert(m_isUsed);

	if (!m_isAlive)
		return false;

	collision.min = m_pos + m_collision.min;
	collision.max = m_pos + m_collision.max;

	return true;
}

void Player::getDamageHitbox(CollisionInfo & collision) const
{
	Assert(m_isUsed);

	collision.min[0] = m_pos[0] - PLAYER_DAMAGE_HITBOX_SX/2;
	collision.min[1] = m_pos[1] - PLAYER_DAMAGE_HITBOX_SY;
	collision.max[0] = m_pos[0] + PLAYER_DAMAGE_HITBOX_SX/2;
	collision.max[1] = m_pos[1];
}

void Player::getAttackCollision(CollisionInfo & collision) const
{
	Assert(m_isUsed);

	float x1 = m_attack.collision.min[0] * m_facing[0];
	float y1 = m_attack.collision.min[1];
	float x2 = m_attack.collision.max[0] * m_facing[0];
	float y2 = m_attack.collision.max[1];

	if (m_facing[1] < 0)
	{
		y1 = -PLAYER_COLLISION_HITBOX_SY - y1;
		y2 = -PLAYER_COLLISION_HITBOX_SY - y2;
	}

	if (x1 > x2)
		std::swap(x1, x2);
	if (y1 > y2)
		std::swap(y1, y2);

	collision.min[0] = m_pos[0] + x1;
	collision.min[1] = m_pos[1] + y1;
	collision.max[0] = m_pos[0] + x2;
	collision.max[1] = m_pos[1] + y2;
}

float Player::getAttackDamage(Player * other) const
{
	Assert(m_isUsed);

	float result = 0.f;

	if (m_attack.attacking && m_attack.hasCollision)
	{
		CollisionInfo attackCollision;
		CollisionInfo otherCollision;

		getAttackCollision(attackCollision);
		other->getDamageHitbox(otherCollision);

		if (attackCollision.intersects(otherCollision))
		{
			result = 1.f;
		}
	}

	return result;
}

bool Player::isAnimOverrideAllowed(PlayerAnim anim) const
{
	if ((m_ice.timer > 0.f || m_bubble.timer > 0.f || m_special.meleeCounter != 0) && anim != kPlayerAnim_Die)
		return false;

	return !m_isAnimDriven || (s_animInfos[anim].prio > s_animInfos[m_anim].prio);
}

float Player::mirrorX(float x) const
{
	return m_facing[0] > 0 ? x : -x;
}

float Player::mirrorY(float y) const
{
	return m_facing[1] > 0 ? y : PLAYER_COLLISION_HITBOX_SY - y;
}

bool Player::hasValidCharacterIndex() const
{
	return m_characterIndex != (uint8_t)-1;
}

void Player::setDisplayName(const std::string & name)
{
	m_displayName = name.c_str();
}

void Player::setAnim(PlayerAnim anim, bool play, bool restart)
{
	if (anim != m_anim || play != m_animPlay || restart)
	{
		log("setAnim: %d", anim);

		m_anim = anim;
		m_animPlay = play;

		applyAnim();
	}
}

void Player::clearAnimOverrides()
{
	m_animVel = Vec2();
	m_animVelIsAbsolute = false;
	m_animAllowGravity = true;
	m_animAllowSteering = true;
	m_enableInAirAnim = true;
}

void Player::setAttackDirection(int dx, int dy)
{
	m_attackDirection[0] = dx;
	m_attackDirection[1] = dy;

	applyAnim();
}

void Player::applyAnim()
{
	clearAnimOverrides();

	if (m_anim != kPlayerAnim_NULL)
	{
		m_spriterState = SpriterState();
	}

	const CharacterData * characterData = getCharacterData(m_characterIndex);

	if (!m_spriterState.startAnim(*characterData->m_spriter, s_animInfos[m_anim].name))
	{
		const char * filename = makeCharacterFilename(m_characterIndex, "sprite/sprite.scml");
		logError("unable to find animation %s in file %s", s_animInfos[m_anim].name, filename);
		m_spriterState.startAnim(*characterData->m_spriter, s_animInfos[kPlayerAnim_Jump].name);
	}

	if (!m_animPlay)
		m_spriterState.stopAnim(*characterData->m_spriter);
}

//

void Player::playSecondaryEffects(PlayerEvent e)
{
	const CharacterData * characterData = getCharacterData(m_characterIndex);

	switch (e)
	{
	case kPlayerEvent_Spawn:
		break;
	case kPlayerEvent_Respawn:
		m_instanceData->playSoundBag("spawn_sounds", 100);
		break;
	case kPlayerEvent_Die:
		m_instanceData->playSoundBag("die_sounds", 100);
		break;
	case kPlayerEvent_Jump:
		{
			Dictionary args;
			args.setPtr("obj", m_instanceData);
			args.setString("name", "jump_sounds");
			m_instanceData->handleAnimationAction("char_soundbag", args);
			break;
		}
	case kPlayerEvent_WallJump:
		GAMESIM->playSound("player-wall-jump.ogg"); // player performs a walljump
		break;
	case kPlayerEvent_LandOnGround:
		GAMESIM->playSound(makeCharacterFilename(m_characterIndex, "land_on_ground.ogg"), 50); // players lands on solid ground
		break;
	case kPlayerEvent_StickyAttach:
		//GAMESIM->playSound("player-sticky-attach.ogg");
		break;
	case kPlayerEvent_StickyRelease:
		//GAMESIM->playSound("player-sticky-release.ogg");
		break;
	case kPlayerEvent_StickyJump:
		GAMESIM->playSound("player-sticky-jump.ogg"); // player jumps and releases itself from a sticky ceiling
		break;
	case kPlayerEvent_SpringJump:
		GAMESIM->playSound("player-spring-jump.ogg"); // player walks over and activates a jump pad 
		break;
	case kPlayerEvent_SpikeHit:
		//GAMESIM->playSound("player-spike-hit.ogg");
		break;
	case kPlayerEvent_ArenaWrap:
		//GAMESIM->playSound("player-arena-wrap.ogg");
		break;
	case kPlayerEvent_DashAir:
		break;
	case kPlayerEvent_DestructibleDestroy:
		//GAMESIM->playSound("player-arena-wrap.ogg");
		break;

	default:
		Assert(false);
		break;
	}
}

void Player::tick(float dt)
{
	const CharacterData * characterData = getCharacterData(m_characterIndex);

	if (m_instanceData->m_textChatTicks != 0)
	{
		if (--m_instanceData->m_textChatTicks == 0)
		{
			m_instanceData->m_textChat.clear();
		}
	}

	m_collision.min[0] = -PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_collision.max[0] = +PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_collision.min[1] = -PLAYER_COLLISION_HITBOX_SY / 1.f;
	m_collision.max[1] = 0.f;

	//

	if (m_ice.timer > 0.f)
	{
		m_ice.timer = Calc::Max(0.f, m_ice.timer - dt);
		if (m_ice.timer == 0.f)
			m_isAnimDriven = false;
	}

	if (m_bubble.timer > 0.f)
	{
		m_bubble.timer = Calc::Max(0.f, m_bubble.timer - dt);
		if (m_bubble.timer == 0.f)
			m_isAnimDriven = false;
	}

	if (m_ice.timer > 0.f ||
		m_bubble.timer > 0.f)
		m_spriterState.animSpeed = 1e-10f;
	else
		m_spriterState.animSpeed = 1.f;

	m_timeDilationAttack.tick(dt);

	//

	if (m_anim != kPlayerAnim_NULL)
	{
		const CharacterData * characterData = getCharacterData(m_characterIndex);

		const char * name = s_animInfos[m_anim].name;
		const auto & animData = characterData->m_animData.m_animMap[name];
		Assert(animData.speed != 0.f);

		const int oldTime = m_spriterState.animTime * 1000.f;

		const bool isDone = m_spriterState.updateAnim(*characterData->m_spriter, dt * animData.speed * PLAYER_ANIM_MULTIPLIER);

		const int newTime = m_spriterState.animTime * 1000.f;

		const auto & triggers = animData.frameTriggers;

		// fixme : triggers aren't supported yet for looping animations

		for (int i = oldTime; i < newTime; ++i)
		{
			const auto & triggersItr = triggers.find(i);

			if (triggersItr != triggers.end())
			{
				const auto & triggers = triggersItr->second;

				for (auto & trigger = triggers.cbegin(); trigger != triggers.cend(); ++trigger)
				{
					Dictionary d = trigger->args;
					d.setPtr("obj", m_instanceData);
					m_instanceData->handleAnimationAction(trigger->action, d);
				}
			}
		}

		if (isDone)
		{
			m_isAnimDriven = false;

			clearAnimOverrides();

			switch (m_anim)
			{
			case kPlayerAnim_NULL:
			case kPlayerAnim_Jump:
			case kPlayerAnim_WallSlide:
			case kPlayerAnim_Walk:
				break;

			case kPlayerAnim_Attack:
			case kPlayerAnim_AttackUp:
			case kPlayerAnim_AttackDown:
				m_attack.attacking = false;
				break;
			case kPlayerAnim_Fire:
				m_attack.attacking = false;
				break;
			case kPlayerAnim_AirDash:
				break;
			case kPlayerAnim_Spawn:
				m_isAlive = true;
				break;
			case kPlayerAnim_Die:
				break;

			default:
				Assert(false);
				break;
			}
		}
	}

	//

	if (g_devMode && !g_monkeyMode && m_input.wentDown(INPUT_BUTTON_START))
		respawn();

	if (!m_isAlive && !m_isAnimDriven)
	{
		if (!m_canRespawn)
		{
			m_canRespawn = true;
			m_canTaunt = true;
			m_respawnTimer = m_isRespawn ? 3.f : 0.f;
		}

		if (m_canTaunt && m_input.wentDown(INPUT_BUTTON_Y))
		{
			m_canTaunt = false;
			m_instanceData->playSoundBag("taunt_sounds", 100);
		}

		if (GAMESIM->m_gameState == kGameState_Play &&
			!GAMESIM->m_levelEvents.spikeWalls.isActive() &&
			(m_input.wentDown(INPUT_BUTTON_X) || m_respawnTimer <= 0.f))
		{
			respawn();
		}

		m_respawnTimer -= dt;
	}

	if (m_isAlive)
	{
		if (m_spawnInvincibilityTicks > 0)
			m_spawnInvincibilityTicks--;

		// see if we grabbed any pickup

		Pickup pickup;

		if (GAMESIM->grabPickup(
			m_pos[0] + m_collision.min[0],
			m_pos[1] + m_collision.min[1],
			m_pos[0] + m_collision.max[0],
			m_pos[1] + m_collision.max[1],
			pickup))
		{
			switch (pickup.type)
			{
			case kPickupType_Ammo:
				pushWeapon(kPlayerWeapon_Fire, PICKUP_AMMO_COUNT);
				break;
			case kPickupType_Nade:
				pushWeapon(kPlayerWeapon_Grenade, 1);
				break;
			case kPickupType_Shield:
				m_shield.shield = PICKUP_SHIELD_COUNT;
				break;
			case kPickupType_Ice:
				pushWeapon(kPlayerWeapon_Ice, PICKUP_ICE_COUNT);
				break;
			case kPickupType_Bubble:
				pushWeapon(kPlayerWeapon_Bubble, PICKUP_BUBBLE_COUNT);
				break;
			case kPickupType_TimeDilation:
				pushWeapon(kPlayerWeapon_TimeDilation, 1);
				break;

			default:
				Assert(false);
				break;
			}
		}

		float surfaceFriction = 0.f;
		Vec2 animVel(0.f, 0.f);

		animVel[0] += m_animVel[0] * m_facing[0];
		animVel[1] += m_animVel[1] * m_facing[1];

		// attack
		if (m_attack.attacking)
		{
			if (m_anim == kPlayerAnim_Attack || m_anim == kPlayerAnim_AttackUp || m_anim == kPlayerAnim_AttackDown)
			{
				animVel[0] += m_attack.attackVel[0] * m_attackDirection[0];
				animVel[1] += m_attack.attackVel[1] * m_attackDirection[1];
			}
			else
			{
				animVel[0] += m_attack.attackVel[0] * m_facing[0];
				animVel[1] += m_attack.attackVel[1] * m_facing[1];
			}

			if (m_attack.hasCollision)
			{
				// see if we hit anyone

				for (int i = 0; i < MAX_PLAYERS; ++i)
				{
					Player & other = GAMESIM->m_players[i];

					if (other.m_isUsed && other.m_isAlive && &other != this)
					{
						float damage[2];
						damage[0] = getAttackDamage(&other);
						damage[1] = other.getAttackDamage(this);

						if (damage[0] != 0.f)
						{
							if (damage[1] != 0.f || !other.handleDamage(damage[0], Vec2(m_facing[0] * PLAYER_SWORD_PUSH_SPEED, 0.f), this))
							{
								//log("-> attack cancel");

								Player * players[2] = { this, &other };

								const Vec2 midPoint = (players[0]->m_pos + players[1]->m_pos) / 2.f;

								for (int j = 0; j < 2; ++j)
								{
									const Vec2 delta = midPoint - players[j]->m_pos;
									const Vec2 normal = delta.CalcNormalized();
									const Vec2 attackDirection = Vec2(players[j]->m_attackDirection[0], players[j]->m_attackDirection[1]).CalcNormalized();
									const float dot = attackDirection * normal;
									const Vec2 reflect = attackDirection - normal * dot * 2.f;

									if (damage[j])
									{
										players[j]->m_vel = reflect * PLAYER_SWORD_CLING_SPEED;
										players[j]->m_controlDisableTime = PLAYER_SWORD_CLING_TIME;
									}

									players[j]->cancelAttack();
								}

								GAMESIM->playSound("melee-cancel.ogg"); // sound when two melee attacks hit at the same time
							}
						}
					}
				}

				// see if we've hit a block

				CollisionInfo attackCollision;
				getAttackCollision(attackCollision);

				m_attack.hitDestructible |= GAMESIM->m_arena.handleDamageRect(
					*GAMESIM,
					m_pos[0],
					m_pos[1],
					attackCollision.min[0],
					attackCollision.min[1],
					attackCollision.max[0],
					attackCollision.max[1],
					!m_attack.hitDestructible);
			}

			if (m_attack.m_rocketPunch.isActive)
			{
				if (m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Charge)
				{
					m_attack.m_rocketPunch.chargeTime = Calc::Min(m_attack.m_rocketPunch.chargeTime + dt, ROCKETPUNCH_CHARGE_MAX);

					logDebug("rocket punch: charge: %f", m_attack.m_rocketPunch.chargeTime);

					if (m_input.wentUp(INPUT_BUTTON_Y) || (ROCKETPUNCH_AUTO_DISCHARGE && m_attack.m_rocketPunch.chargeTime == ROCKETPUNCH_CHARGE_MAX))
					{
						if (m_attack.m_rocketPunch.chargeTime < (ROCKETPUNCH_CHARGE_MUST_BE_MAXED ? ROCKETPUNCH_CHARGE_MAX : ROCKETPUNCH_CHARGE_MIN) ||
							m_input.getAnalogDirection().CalcSize() == 0.f)
						{
							logDebug("rocket punch: cancel");

							endRocketPunch(false);
						}
						else
						{
							const float t = (m_attack.m_rocketPunch.chargeTime - ROCKETPUNCH_CHARGE_MIN) / (ROCKETPUNCH_CHARGE_MAX - ROCKETPUNCH_CHARGE_MIN);
							const float speed = ROCKETPUNCH_SPEED_BASED_ON_CHARGE ? Calc::Lerp(ROCKETPUNCH_SPEED_MIN, ROCKETPUNCH_SPEED_MAX, t) : ROCKETPUNCH_SPEED_MAX;
							const float distance = ROCKETPUNCH_DISTANCE_BASED_ON_CHARGE ? Calc::Lerp(ROCKETPUNCH_DISTANCE_MIN, ROCKETPUNCH_DISTANCE_MAX, t) : ROCKETPUNCH_DISTANCE_MAX;

							m_attack.m_rocketPunch.state = AttackInfo::RocketPunch::kState_Attack;
							m_attack.m_rocketPunch.maxDistance = distance;
							m_attack.m_rocketPunch.speed = m_input.getAnalogDirection().CalcNormalized() * speed;

							setAnim(kPlayerAnim_RocketPunch_Attack, true, true);
							m_isAnimDriven = true;
							m_animVelIsAbsolute = true;

							GAMESIM->playSound("rocketpunch-attack.ogg"); // sound that plays when the rocket jump attack enter the attack phase

							logDebug("rocket punch: attack! speed = (%f, %f) max distance = %f", m_attack.m_rocketPunch.speed[0], m_attack.m_rocketPunch.speed[1], m_attack.m_rocketPunch.maxDistance);
						}
					}
				}
				if (m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Attack)
				{
					// move player

					animVel += m_attack.m_rocketPunch.speed;

					m_attack.m_rocketPunch.distance += (animVel * dt).CalcSize();

					logDebug("rocket punch: attack: distance=%f", m_attack.m_rocketPunch.distance);

					if (m_attack.m_rocketPunch.distance >= m_attack.m_rocketPunch.maxDistance)
					{
						endRocketPunch(true);
					}
				}
				if (m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Stunned)
				{
					if (m_attack.m_rocketPunch.stunTime >= ROCKETPUNCH_STUN_TIME)
					{
						endRocketPunch(false);
					}
					else
					{
						m_attack.m_rocketPunch.stunTime += dt;
					}
				}
			}
		}

		if (!m_attack.attacking)
			m_attack.cooldown -= dt;

		if (!m_attack.attacking && m_attack.cooldown <= 0.f)
		{
			if (m_input.wentDown(INPUT_BUTTON_B) && (m_weaponStackSize > 0 || s_unlimitedAmmo) && isAnimOverrideAllowed(kPlayerAnim_Fire))
			{
				m_attack = AttackInfo();

				PlayerAnim anim = kPlayerAnim_NULL;
				BulletType bulletType = kBulletType_COUNT;
				BulletEffect bulletEffect = kBulletEffect_Damage;
				bool hasAttackCollision = false;

				PlayerWeapon weaponType = popWeapon();

				if (weaponType == kPlayerWeapon_Fire)
				{
					anim = kPlayerAnim_Fire;
					bulletType = kBulletType_B;
					m_attack.cooldown = PLAYER_FIRE_COOLDOWN;
				}
				else if (weaponType == kPlayerWeapon_Ice)
				{
					anim = kPlayerAnim_Fire;
					bulletType = kBulletType_B;
					bulletEffect = kBulletEffect_Ice;
					m_attack.cooldown = PLAYER_FIRE_COOLDOWN;
				}
				else if (weaponType == kPlayerWeapon_Bubble)
				{
					anim = kPlayerAnim_Fire;
					bulletType = kBulletType_B;
					bulletEffect = kBulletEffect_Bubble;
					m_attack.cooldown = PLAYER_FIRE_COOLDOWN;
				}
				else if (weaponType == kPlayerWeapon_Grenade)
				{
					bulletType = kBulletType_Grenade;
					GAMESIM->playSound("grenade-throw.ogg"); // player throws a grenade
				}
				else if (weaponType == kPlayerWeapon_TimeDilation)
				{
					// update allowCancelTimeDilationAttack flag on players
					bool wasActive = false;
					for (int i = 0; i < MAX_PLAYERS; ++i)
						if (GAMESIM->m_players[i].m_isUsed && GAMESIM->m_players[i].m_isAlive && GAMESIM->m_players[i].m_timeDilationAttack.isActive())
								wasActive = true;
					if (!wasActive)
					{
						for (int i = 0; i < MAX_PLAYERS; ++i)
							if (GAMESIM->m_players[i].m_isUsed && GAMESIM->m_players[i].m_isAlive && GAMESIM->m_players[i].m_attack.attacking)
								GAMESIM->m_players[i].m_attack.allowCancelTimeDilationAttack = true;
					}

					m_timeDilationAttack.timeRemaining = PLAYER_EFFECT_TIMEDILATION_TIME;
					GAMESIM->playSound("timedilation-activate.ogg"); // sound that occurs when the player activates the time dilation pickup
					GAMESIM->addAnnouncement("'%s' activated time dilation!", m_displayName.c_str());
				}

				if (anim != kPlayerAnim_NULL)
				{
					setAnim(anim, true, true);
					m_isAnimDriven = true;

					m_attack.attacking = true;
				}

				if (bulletType != kBulletType_COUNT)
				{
					// determine attack direction based on player input

					int angle;

					if (m_input.isDown(INPUT_BUTTON_UP))
						angle = 256*1/4;
					else if (m_input.isDown(INPUT_BUTTON_DOWN))
						angle = 256*3/4;
					else if (m_facing[0] < 0)
						angle = 128;
					else
						angle = 0;

					Assert(bulletType != kBulletType_COUNT);
					GAMESIM->spawnBullet(
						m_pos[0] + mirrorX(0.f),
						m_pos[1] - mirrorY(44.f),
						angle,
						bulletType,
						bulletEffect,
						m_index);
				}
			}

			if (m_input.wentDown(INPUT_BUTTON_X) && isAnimOverrideAllowed(kPlayerAnim_Attack))
			{
				m_attack = AttackInfo();
				m_attack.attacking = true;

				m_attack.cooldown = PLAYER_SWORD_COOLDOWN;

				PlayerAnim anim = kPlayerAnim_NULL;

				// determine attack direction based on player input

				if (m_input.isDown(INPUT_BUTTON_UP))
				{
					setAttackDirection(0, -1);
					anim = kPlayerAnim_AttackUp;
				}
				else if (m_input.isDown(INPUT_BUTTON_DOWN))
				{
					setAttackDirection(0, +1);
					anim = kPlayerAnim_AttackDown;
					m_enterPassthrough = true;

					if (characterData->m_special == kPlayerSpecial_DownAttack)
					{
						m_special.attackDownActive = true;
						m_special.attackDownHeight = 0.f;
					}
				}
				else
				{
					setAttackDirection(m_facing[0], 0);
					anim = kPlayerAnim_Attack;
				}

				// start anim

				setAnim(anim, true, true);
				m_isAnimDriven = true;

				// determine attack collision. basically just 3 directions: forward, up and down

				m_attack.collision.min[0] = 0.f;
				m_attack.collision.max[0] = (m_attackDirection[0] == 0) ? 0.f : 50.f; // fordward?
				m_attack.collision.min[1] = -PLAYER_COLLISION_HITBOX_SY/3.f*2;
				m_attack.collision.max[1] = -PLAYER_COLLISION_HITBOX_SY/3.f*2 + m_attackDirection[1] * 50.f; // up or down

				// make sure the attack collision doesn't have a zero sized area

				if (m_attack.collision.min[0] == m_attack.collision.max[0])
				{
					m_attack.collision.min[0] -= 2.f;
					m_attack.collision.max[0] += 2.f;
				}
				if (m_attack.collision.min[1] == m_attack.collision.max[1])
				{
					m_attack.collision.min[1] -= 2.f;
					m_attack.collision.max[1] += 2.f;
				}

				m_attack.hasCollision = true;
			}

			if (m_input.wentDown(INPUT_BUTTON_Y) && !s_noSpecial && isAnimOverrideAllowed(kPlayerAnim_Attack))
			{
				if (characterData->m_special == kPlayerSpecial_RocketPunch)
				{
					if (!ROCKETPUNCH_ONLY_IN_AIR || !m_isGrounded)
					{
						if (!ROCKETPUNCH_MUST_GROUND || m_isAirDashCharged)
						{
							m_isAirDashCharged = false; // fixme : don't reuse air dash..

							beginRocketPunch();
						}
					}
				}
				else if (characterData->m_special == kPlayerSpecial_DoubleSidedMelee)
				{
					m_attack = AttackInfo();
					m_attack.attacking = true;

					m_attack.cooldown = PLAYER_SWORD_COOLDOWN; // todo : melee

					// start anim

					setAnim(kPlayerAnim_Walk, true, true);
					//m_isAnimDriven = true;

					m_attack.collision.min[0] = 0.f;
					m_attack.collision.max[0] = DOUBLEMELEE_ATTACK_RADIUS;
					m_attack.collision.min[1] = -PLAYER_COLLISION_HITBOX_SY/3.f*2;
					m_attack.collision.max[1] = -PLAYER_COLLISION_HITBOX_SY/3.f*2 + 4.f;

					m_attack.hasCollision = true;

					m_special.meleeCounter = DOUBLEMELEE_SPIN_COUNT;
					m_special.meleeAnimTimer = DOUBLEMELEE_SPIN_TIME;
				}
			}
		}

		// update double melee attack

		if (characterData->m_special == kPlayerSpecial_DoubleSidedMelee && m_special.meleeCounter != 0)
		{
			m_special.meleeAnimTimer -= dt;

			if (m_special.meleeAnimTimer <= 0.f)
			{
				if (--m_special.meleeCounter == 0)
				{
					m_special.meleeAnimTimer = 0.f;
					m_attack.attacking = false;
					m_isAnimDriven = false;
				}
				else
				{
					// reverse attack and animation direction

					m_facing[0] *= -1.f;
					m_special.meleeAnimTimer = DOUBLEMELEE_SPIN_TIME;
				}
			}
		}

		m_blockMask = ~0;

	#if USE_NEW_COLLISION_CODE
		const uint32_t currentBlockMask = m_oldBlockMask;
	#else
		const uint32_t currentBlockMask = getIntersectingBlocksMask(m_pos[0], m_pos[1]);
		m_isInPassthrough = (currentBlockMask & kBlockMask_Passthrough) != 0;
	#endif

		const bool enterPassThrough = m_enterPassthrough || (m_special.attackDownActive) || (m_attack.m_rocketPunch.isActive);
		if (m_isInPassthrough || enterPassThrough)
			m_blockMask = ~kBlockMask_Passthrough;

	#if USE_NEW_COLLISION_CODE
		const uint32_t currentBlockMaskFloor = m_dirBlockMaskDir[1] > 0 ? m_dirBlockMask[1] : 0;
		const uint32_t currentBlockMaskCeil = m_dirBlockMaskDir[1] < 0 ? m_dirBlockMask[1] : 0;
	#else
		const uint32_t currentBlockMaskFloor = getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f);
		const uint32_t currentBlockMaskCeil = getIntersectingBlocksMask(m_pos[0], m_pos[1] - 1.f);
	#endif

		if (m_isAirDashCharged && !m_isGrounded && !m_isAttachedToSticky && m_input.wentDown(INPUT_BUTTON_A))
		{
			if (isAnimOverrideAllowed(kPlayerAnim_AirDash) && false)
			{
				if ((getIntersectingBlocksMask(m_pos[0] + m_facing[0], m_pos[1]) & kBlockMask_Solid) == 0)
				{
					m_isAirDashCharged = false;

					setAnim(kPlayerAnim_AirDash, true, true);
					m_enableInAirAnim = false;
				}
			}
		}

		// death by spike

		if (currentBlockMask & (1 << kBlockType_Spike))
		{
			if (isAnimOverrideAllowed(kPlayerAnim_Die))
			{
				handleDamage(1.f, Vec2(0.f, 0.f), 0);

				playSecondaryEffects(kPlayerEvent_SpikeHit);
			}
		}

		// teleport

		{
			int px = int(m_pos[0] + (m_collision.min[0] + m_collision.max[0]) / 2) / BLOCK_SX;
			int py = int(m_pos[1] + (m_collision.min[1] + m_collision.max[1]) / 2) / BLOCK_SY;

			if (px != m_teleport.x || py != m_teleport.y)
			{
				m_teleport.cooldown = false;
			}

			if (!m_teleport.cooldown && px >= 0 && px < ARENA_SX && py >= 0 && py < ARENA_SY)
			{
				const Block & block = GAMESIM->m_arena.getBlock(px, py);

				if (block.type == kBlockType_Teleport)
				{
					// find a teleport destination

					int destinationX;
					int destinationY;

					if (GAMESIM->m_arena.getTeleportDestination(*GAMESIM, px, py, destinationX, destinationY))
					{
						m_pos[0] = destinationX * BLOCK_SX;
						m_pos[1] = destinationY * BLOCK_SY;

						m_pos[0] += BLOCK_SX / 2;
						m_pos[1] += BLOCK_SY - 1;

						m_teleport.cooldown = true;
						m_teleport.x = destinationX;
						m_teleport.y = destinationY;
					}
				}
			}
		}

		const bool playerControl =
			m_controlDisableTime == 0.f &&
			m_animAllowSteering &&
			m_ice.timer == 0.f &&
			m_bubble.timer == 0.f &&
			m_special.meleeCounter == 0 &&
			m_attack.m_rocketPunch.isActive == false;

		m_controlDisableTime -= dt;
		if (m_controlDisableTime < 0.f)
			m_controlDisableTime = 0.f;

		// sticky ceiling

		if (currentBlockMaskCeil & (1 << kBlockType_Sticky))
		{
			if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
			{
				m_vel[1] = PLAYER_JUMP_SPEED / 2.f;

				m_isAttachedToSticky = false;

				playSecondaryEffects(kPlayerEvent_StickyJump);
			}
			else if (playerControl && m_input.wentDown(INPUT_BUTTON_DOWN))
			{
				m_isAttachedToSticky = false;

				playSecondaryEffects(kPlayerEvent_StickyRelease);

				m_vel[1] = 0.f;
			}
			else if (m_vel[1] <= 0.f)
			{
				surfaceFriction = FRICTION_GROUNDED;

				if (!m_isAttachedToSticky)
				{
					m_isAttachedToSticky = true;

					playSecondaryEffects(kPlayerEvent_StickyAttach);
				}
			}
		}
		else
		{
			m_isAttachedToSticky = false;
		}

		// steering

		float steeringSpeed = 0.f;

		if (true)
		{
			int numSteeringFrame = 1;

			steeringSpeed += m_input.m_currState.analogX / 100.f;
			
			if ((m_isGrounded || m_isAttachedToSticky) && playerControl)
			{
				if (isAnimOverrideAllowed(kPlayerAnim_Walk))
				{
					if (steeringSpeed != 0.f)
						setAnim(kPlayerAnim_Walk, true, false);
					else
						setAnim(kPlayerAnim_Idle, true, false);
				}
			}

			//

			bool isWalkingOnSolidGround = false;

			if (m_isAttachedToSticky)
				isWalkingOnSolidGround = (currentBlockMaskCeil & kBlockMask_Solid) != 0;
			else
				isWalkingOnSolidGround = (currentBlockMaskFloor & kBlockMask_Solid) != 0;

			if (isWalkingOnSolidGround)
			{
				steeringSpeed *= STEERING_SPEED_ON_GROUND;
				numSteeringFrame = 5;
			}
			else if (m_isUsingJetpack)
			{
				steeringSpeed *= STEERING_SPEED_JETPACK;
				numSteeringFrame = 5;
			}
			else if (m_special.meleeCounter != 0)
			{
				steeringSpeed *= STEERING_SPEED_DOUBLEMELEE;
				numSteeringFrame = 5;
			}
			else
			{
				steeringSpeed *= STEERING_SPEED_IN_AIR;
				numSteeringFrame = 5;
			}

			bool doSteering =
				playerControl ||
				m_special.meleeCounter != 0;

			if (doSteering && steeringSpeed != 0.f)
			{
				float maxSteeringDelta;

				if (steeringSpeed > 0)
					maxSteeringDelta = steeringSpeed - m_vel[0];
				if (steeringSpeed < 0)
					maxSteeringDelta = steeringSpeed - m_vel[0];

				if (Calc::Sign(steeringSpeed) == Calc::Sign(maxSteeringDelta))
					m_vel[0] += maxSteeringDelta * dt * 60.f / numSteeringFrame;
			}
		}

		// gravity

		float gravity = 0.f;

		m_isWallSliding = false;
		m_isUsingJetpack = false;

		if (m_animAllowGravity &&
			//!m_isAttachedToSticky &&
			!m_animVelIsAbsolute &&
			m_bubble.timer == 0.f &&
			(!m_attack.m_rocketPunch.isActive || m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Stunned))
		{
			if (currentBlockMask & (1 << kBlockType_GravityDisable))
				gravity = 0.f;
			else if (currentBlockMask & (1 << kBlockType_GravityReverse))
				gravity = GRAVITY * BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER;
			else if (currentBlockMask & (1 << kBlockType_GravityStrong))
				gravity = GRAVITY * BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER;
			else if (currentBlockMask & ((1 << kBlockType_GravityLeft) | (1 << kBlockType_GravityRight)))
				gravity = 0.f;
			else
				gravity = m_isAttachedToSticky ? -GRAVITY : +GRAVITY;

			bool canWallSlide = true;

			if (characterData->m_special == kPlayerSpecial_Jetpack && m_input.isDown(INPUT_BUTTON_Y) && !s_noSpecial)
			{
				gravity -= JETPACK_ACCEL;
				m_isUsingJetpack = true;
			}
			
			// wall slide

			if (!m_isGrounded &&
				!m_isAttachedToSticky &&
				isAnimOverrideAllowed(kPlayerAnim_WallSlide) &&
				!m_isUsingJetpack &&
				m_special.meleeCounter == 0 &&
				!m_special.attackDownActive &&
				m_vel[0] != 0.f && Calc::Sign(m_facing[0]) == Calc::Sign(m_vel[0]) &&
				//Calc::Sign(m_vel[1]) == Calc::Sign(gravity) &&
				(Calc::Sign(m_vel[1]) == Calc::Sign(gravity) || Calc::Abs(m_vel[1]) <= PLAYER_JUMP_SPEED / 2.f) &&
			#if USE_NEW_COLLISION_CODE
				m_isHuggingWall != 0)
			#else
				(getIntersectingBlocksMask(m_pos[0] + m_facing[0], m_pos[1]) & kBlockMask_Solid) != 0)
			#endif
			{
				m_isWallSliding = true;

				setAnim(kPlayerAnim_WallSlide, true, false);

				if (m_vel[1] > PLAYER_WALLSLIDE_SPEED && Calc::Sign(m_vel[1]) == Calc::Sign(gravity))
					m_vel[1] = PLAYER_WALLSLIDE_SPEED;
			}
		}

		if (m_special.meleeCounter != 0 && Calc::Sign(m_vel[1]) == Calc::Sign(gravity))
		{
			gravity *= DOUBLEMELEE_GRAVITY_MULTIPLIER;
		}

		m_vel[1] += gravity * dt;

		if (gravity < GRAVITY)
		{
			m_special.attackDownActive = false;
		}

		if (currentBlockMask & (1 << kBlockType_GravityLeft))
			m_vel[0] -= GRAVITY * dt;
		if (currentBlockMask & (1 << kBlockType_GravityRight))
			m_vel[0] += GRAVITY * dt;

		// converyor belt

		if (currentBlockMaskFloor & (1 << kBlockType_ConveyorBeltLeft))
			animVel[0] += -BLOCKTYPE_CONVEYOR_SPEED;
		if (currentBlockMaskFloor & (1 << kBlockType_ConveyorBeltRight))
			animVel[0] += +BLOCKTYPE_CONVEYOR_SPEED;

		// jumping

		if (playerControl && m_input.wentDown(INPUT_BUTTON_A))
		{
			if (currentBlockMaskFloor & kBlockMask_Solid)
			{
				m_jump.jumpVelocityLeft = -PLAYER_JUMP_SPEED;
				m_jump.cancelStarted = false;

				playSecondaryEffects(kPlayerEvent_Jump);
			}
		}

		// jumping : increment speed over time for soft jump

		if (m_input.isDown(INPUT_BUTTON_A) && m_jump.jumpVelocityLeft != 0.f)
		{
			const float velPerSecond = PLAYER_JUMP_SPEED * TICKS_PER_SECOND / (float)PLAYER_JUMP_SPEED_FRAMES;
			float delta = velPerSecond * Calc::Sign(m_jump.jumpVelocityLeft) * dt;
			if (Calc::Abs(delta) > Calc::Abs(m_jump.jumpVelocityLeft))
				delta = m_jump.jumpVelocityLeft;
			m_vel[1] += delta;
			m_jump.jumpVelocityLeft -= delta;
		}
		else
		{
			m_jump.jumpVelocityLeft = 0.f;
		}

		// update grounded state

		//if (m_isGrounded && m_vel[1] < 0.f)
		//	m_isGrounded = false;

		// collision

#if USE_NEW_COLLISION_CODE

		// colliding with solid object left/right of player

		if (m_isHuggingWall && playerControl && m_input.wentDown(INPUT_BUTTON_A))
		{
			// wall jump

			m_vel[0] = -PLAYER_WALLJUMP_RECOIL_SPEED * m_dirBlockMaskDir[0];
			m_vel[1] = -PLAYER_WALLJUMP_SPEED;

			m_controlDisableTime = PLAYER_WALLJUMP_RECOIL_TIME;

			playSecondaryEffects(kPlayerEvent_WallJump);
		}

		const bool wasInPassthrough = m_isInPassthrough;

		m_isInPassthrough = false;
		m_isHuggingWall = false;

		Vec2 totalVel = (m_vel * (m_animVelIsAbsolute ? 0.f : 1.f)) + animVel;

		// input step:
		// - update velocities

		// simulation step:
		// - physics scene updates
		// - player receives callbacks
		// - keeps track of stuff. gets to decide velocity update scheme
		//   - block mask!

		// player process step:
		// - react to physics update. evaluate the new block mask, etc
		// - check for attacks, wall jump, etc

		uint32_t dirBlockMask[2] = { 0, 0 };

		// todo : update this from player input update

		for (int i = 0; i < 2; ++i)
		{
			Vec2 delta;
			delta[i] = totalVel[i] * dt;

			if (delta[i] == 0.f)
				continue;

			Vec2 newPos = m_pos + delta;

			const Vec2 min = newPos + m_collision.min;
			const Vec2 max = newPos + m_collision.max;

			// todo : phys object shape

			struct ContactInfo
			{
				Vec2 n;
				float d;

				bool operator==(const ContactInfo & other) const
				{
					return
						n == other.n &&
						d == other.d;
				}
			};

			struct CollisionArgs
			{
				CollisionArgs()
				{
					updateVelocity = false;
				}

				Player * self;
				Vec2 delta;
				Vec2 totalVel;
				Vec2 newPos;
				int axis;
				uint32_t * dirBlockMask;
				bool enterPassThrough;
				bool wasInPassthrough;
				float gravity;

				// these are inputs for the physics system..
				std::vector<ContactInfo> contacts;
				bool updateVelocity;
			};

			CollisionArgs args;

			args.self = this;
			args.axis = i;
			args.delta = delta;
			args.totalVel = totalVel;
			args.newPos = newPos;
			args.dirBlockMask = dirBlockMask;
			args.enterPassThrough = enterPassThrough;
			args.wasInPassthrough = wasInPassthrough;
			args.gravity = gravity;

			CollisionShape playerShape;
			playerShape.set(
				Vec2(min[0], min[1]),
				Vec2(max[0], min[1]),
				Vec2(max[0], max[1]),
				Vec2(min[0], max[1]));

			GAMESIM->testCollision(
				playerShape,
				&args,
				[](const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * blockAndDistance, Player * player)
				{
					CollisionArgs * args = (CollisionArgs*)arg;
					Player * self = args->self;
					Vec2 & delta = args->delta;
					const Vec2 & totalVel = args->totalVel;
					Vec2 & newPos = args->newPos;
					const bool enterPassThrough = args->enterPassThrough;
					const int i = args->axis;
					const float gravity = args->gravity;

					const CharacterData * characterData = getCharacterData(self->m_characterIndex);

					//

					// todo : move to below checkCollision test
					if (self->m_attack.m_rocketPunch.isActive && self->m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Attack)
					{
						if (blockAndDistance)
						{
							if (blockAndDistance->block->handleDamage(*self->m_instanceData->m_gameSim, blockAndDistance->x, blockAndDistance->y))
								blockAndDistance = 0;
							else if ((1 << blockAndDistance->block->type) & kBlockMask_Solid)
								self->endRocketPunch(true);
						}
						if (player && player != self)
							player->handleDamage(1.f, totalVel, self);
					}

					if (blockAndDistance)
					{
						Block * block = blockAndDistance->block;

						CollisionShape blockShape = Arena::getBlockCollision(block->shape);
						blockShape.translate(blockAndDistance->x * BLOCK_SX, blockAndDistance->y * BLOCK_SY);

						float contactDistance;
						Vec2 contactNormal;

						if (shape.checkCollision(blockShape, delta, contactDistance, contactNormal))
						{
							// todo : from here is player callback code!

							bool collide = true;

							if (block)
							{
								const uint32_t mask = (1 << block->type) & ((args->wasInPassthrough || self->m_isInPassthrough || enterPassThrough) ? ~kBlockMask_Passthrough : ~0);

								args->dirBlockMask[i] |= mask;

								if (contactNormal[0] * contactNormal[1] != 0.f)
								{
									args->dirBlockMask[0] |= mask;
									args->dirBlockMask[1] |= mask;
								}

								if ((1 << block->type) & kBlockMask_Passthrough)
								{
									if (i != 1 || delta[1] < 0.f || (args->wasInPassthrough || self->m_isInPassthrough || enterPassThrough))
									{
										collide = false;

										self->m_isInPassthrough = true;
									}
								}

								if (((1 << block->type) & kBlockMask_Solid) == 0) // todo : should pass all types to player. let player filter on solid yes/no
									collide = false;
							}
							
							if (collide)
							{
								// todo : let phys update know if collision was handled
								// todo : do offset after evaluating all collisions, to avoid missing intersecting blocks

								ContactInfo contact;
								contact.n = contactNormal;
								contact.d = contactDistance;
								args->contacts.push_back(contact);

								// wall slide

								if (delta[0] != 0.f)
								{
									self->m_isHuggingWall = delta[0] < 0.f ? -1 : +1;
								}

								// screen shake

								const float sign = Calc::Sign(delta[i]);
								float strength = (Calc::Abs(totalVel[i]) - PLAYER_JUMP_SPEED) / 25.f;

								if (strength > PLAYER_SCREENSHAKE_STRENGTH_THRESHHOLD)
								{
									strength = sign * strength / 4.f;
									self->GAMESIM->addScreenShake(
										i == 0 ? strength : 0.f,
										i == 1 ? strength : 0.f,
										3000.f, .3f);
								}

								if (self->m_isAnimDriven && self->m_anim == kPlayerAnim_AirDash)
								{
									self->m_spriterState.stopAnim(*characterData->m_spriter);
								}

								//

								bool updateVelocity = true;

								// effects

								// todo : if ice or bubble : do not allow vel change. instead, change vel after all collision code is done

								if (self->m_ice.timer != 0.f || self->m_bubble.timer != 0.f)
									updateVelocity = false;

								if (i == 1)
								{
									self->m_enterPassthrough = false;
								}

								if (i == 1 && delta[1] < 0.f && gravity >= 0.f)
								{
									self->handleJumpCollision();

									updateVelocity = false;
								}

								args->updateVelocity = updateVelocity;
							}
						}
					}
				});

			m_pos = newPos;

			auto u = std::unique(args.contacts.begin(), args.contacts.end());
			args.contacts.resize(std::distance(args.contacts.begin(), u));

			for (auto contact = args.contacts.begin(); contact != args.contacts.end(); ++contact)
			{
				Vec2 offset = contact->n * contact->d;

				m_pos += offset;

				if (args.updateVelocity)
				{
					// todo : let phys update know whether to update velocity

					const float d = m_vel * contact->n;

					if (d > 0.f)
					{
						m_vel -= contact->n * d;

						//logDebug("vel = %f, %f", m_vel[0], m_vel[1]);
					}
				}
			}
		}

		// attempt to stay grounded

		if (m_isGrounded)
			m_vel[1] = 0.f;

	#if 0
		if (m_isGrounded && !(dirBlockMask[1] & kBlockMask_Solid))
		{
			const Vec2 min = m_pos + m_collision.min;
			const Vec2 max = m_pos + m_collision.max + Vec2(0.f, 4.f);

			CollisionShape playerShape;
			playerShape.set(
				Vec2(min[0], min[1]),
				Vec2(max[0], min[1]),
				Vec2(max[0], max[1]),
				Vec2(min[0], max[1]));

			GAMESIM->testCollision(
				playerShape,
				this,
				[](const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * blockAndDistance, Player * player)
				{
					Player * self = (Player*)arg;

					if (blockAndDistance)
					{
						Block * block = blockAndDistance->block;

						if ((1 << block->type) & kBlockMask_Solid)
						{
							CollisionShape blockShape = Arena::getBlockCollision(block->shape);
							blockShape.translate(blockAndDistance->x * BLOCK_SX, blockAndDistance->y * BLOCK_SY);

							float contactDistance;
							Vec2 contactNormal;

							if (shape.checkCollision(blockShape, Vec2(0.f, 1.f), contactDistance, contactNormal))
							{
								if (contactNormal[1] * contactDistance < 0.f)
								{
									self->m_pos[1] += 4.f + contactNormal[1] * contactDistance;
									log("stay grounded!");
								}
							}
						}
					}
				});
		}
	#endif

		// surface type

		const uint32_t blockMask = dirBlockMask[0] | dirBlockMask[1];

		if ((dirBlockMask[1] & (1 << kBlockType_Slide)) && totalVel[1] >= 0.f)
			surfaceFriction = 0.f;
		else if (m_ice.timer != 0.f)
			surfaceFriction = 0.f;
		else if ((Calc::Sign(m_vel[1]) != Calc::Sign(gravity) && m_vel[1] != 0.f) || (dirBlockMask[1] & kBlockMask_Solid) == 0)
			surfaceFriction = 0.f;
		else
			surfaceFriction = FRICTION_GROUNDED;

		// spring

		if ((dirBlockMask[1] & (1 << kBlockType_Spring)) && totalVel[1] >= 0.f)
		{
			m_vel[1] = -BLOCKTYPE_SPRING_SPEED;

			playSecondaryEffects(kPlayerEvent_SpringJump);
		}

		// effects

		for (int i = 0; i < 2; ++i)
		{
			if (m_ice.timer != 0.f && (dirBlockMask[i] & kBlockMask_Solid))
			{
				m_vel[i] *= -.5f;
			}
			else if (m_bubble.timer != 0.f && (dirBlockMask[i] & kBlockMask_Solid))
			{
				m_vel[i] *= -.75f;
			}
		}

		m_dirBlockMaskDir[0] = totalVel[0] == 0.f ? 0 : totalVel[0] < 0.f ? -1 : +1;
		m_dirBlockMaskDir[1] = totalVel[1] == 0.f ? 0 : totalVel[1] < 0.f ? -1 : +1;
		m_dirBlockMask[0] = dirBlockMask[0];
		m_dirBlockMask[1] = dirBlockMask[1];
		m_oldBlockMask = blockMask;
#else
		const Vec2 totalVel = (m_vel * (m_animVelIsAbsolute ? 0.f : 1.f)) + animVel;

		for (int i = 0; i < 2; ++i)
		{
			float totalDelta = totalVel[i] * dt;

			const float deltaSign = totalDelta < 0.f ? -1.f : +1.f;

			if (i == 1)
			{
				// update passthrough mode
				m_blockMask = ~0;
				if ((getIntersectingBlocksMask(m_pos[0], m_pos[1]) & kBlockMask_Passthrough) || enterPassThrough)
					m_blockMask = ~kBlockMask_Passthrough;
			}

			while (totalDelta != 0.f)
			{
				const float delta = (std::abs(totalDelta) < 1.f) ? totalDelta : deltaSign;

				Vec2 newPos = m_pos;

				newPos[i] += delta;

				// fixme : horrible code..
				if (m_attack.m_rocketPunch.isActive && m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Attack)
				{
					const CollisionShape shape = m_collision.getTranslated(newPos);
					void * args[2] = { this, (void*)&totalVel };
					GAMESIM->testCollision(
						shape,
						args,
						[](const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * block, Player * player)
						{
							void ** args = (void**)arg;
							Player * self = (Player*)args[0];
							Vec2 * vel = (Vec2*)args[1];
							if (block)
								block->block->handleDamage(*self->m_instanceData->m_gameSim, block->x, block->y);
							if (player && player != self)
								player->handleDamage(1.f, *vel, self);
						});
				}

				uint32_t newBlockMask = getIntersectingBlocksMask(newPos[0], newPos[1]);

				// ignore passthough blocks if we're moving horizontally or upwards
				// todo : update block mask each iteration. reset m_blockMask first
				//if (i != 1 || delta <= 0.f)
				//if ((!m_isGrounded || isInPassthough) && (i != 0 || delta <= 0.f))
				if ((!m_isGrounded || m_isInPassthrough) && (i != 1 || delta <= 0.f))
				//if ((i == 0 && isInPassthough) || (i == 1 && delta <= 0.f))
					newBlockMask &= ~kBlockMask_Passthrough;

				if (m_attack.m_rocketPunch.isActive && (newBlockMask & kBlockMask_Solid))
				{
					if (m_attack.m_rocketPunch.state != AttackInfo::RocketPunch::kState_Charge)
						endRocketPunch(true);
				}

				// make sure we stay grounded, within reason. allows the player to walk up/down slopes
				if (i == 0 && m_isGrounded)
				{
					if (newBlockMask & kBlockMask_Solid)
					{
						for (int dy = 1; dy <= 2; ++dy)
						{
							const uint32_t blockMask = getIntersectingBlocksMask(newPos[0], newPos[1] - dy);

							if ((blockMask & kBlockMask_Solid) == 0)
							{
								m_pos[1] -= dy;
								newPos[1] -= dy;
								newBlockMask = blockMask;
								break;
							}
						}
					}
					else
					{
						for (int dy = 1; dy <= 1; ++dy)
						{
							const uint32_t blockMask = getIntersectingBlocksMask(newPos[0], newPos[1] + 1.f);

							if ((blockMask & kBlockMask_Solid) == 0)
							{
								m_pos[1] += 1.f;
								newPos[1] += 1.f;
								newBlockMask = blockMask;
								break;
							}
						}
					}
				}

				if ((newBlockMask & kBlockMask_Solid) && m_ice.timer != 0.f)
				{
					m_vel[i] *= -.5f;

					totalDelta = 0.f;
				}
				else if ((newBlockMask & kBlockMask_Solid) && m_bubble.timer != 0.f)
				{
					m_vel[i] *= -.75f;

					totalDelta = 0.f;
				}
				else if (newBlockMask & kBlockMask_Solid)
				{
					if (i == 0)
					{
						const float sign = Calc::Sign(totalDelta);
						float strength = (Calc::Abs(totalDelta) / dt - PLAYER_JUMP_SPEED) / 25.f;

						if (strength > PLAYER_SCREENSHAKE_STRENGTH_THRESHHOLD)
						{
							strength = sign * strength / 4.f;
							GAMESIM->addScreenShake(strength, 0.f, 3000.f, .3f);
						}

						// colliding with solid object left/right of player

						if (!m_isGrounded && playerControl && m_input.wentDown(INPUT_BUTTON_A))
						{
							// wall jump

							m_vel[0] = -PLAYER_WALLJUMP_RECOIL_SPEED * deltaSign;
							m_vel[1] = -PLAYER_WALLJUMP_SPEED;

							m_controlDisableTime = PLAYER_WALLJUMP_RECOIL_TIME;

							playSecondaryEffects(kPlayerEvent_WallJump);
						}
						else
						{
							m_vel[0] = 0.f;

							if (m_isAnimDriven && m_anim == kPlayerAnim_AirDash)
							{
								m_spriterState.stopAnim(*characterData->m_spriter);
							}
						}
					}

					if (i == 1)
					{
						m_enterPassthrough = false;

						// grounded

						if (newBlockMask & (1 << kBlockType_Slide))
							surfaceFriction = 0.f;
						else if (m_ice.timer != 0.f)
							surfaceFriction = 0.f;
						else if (Calc::Sign(m_vel[1]) != Calc::Sign(gravity))
							surfaceFriction = 0.f;
						else
							surfaceFriction = FRICTION_GROUNDED;

						if (delta >= 0.f)
						{
							if (newBlockMask & (1 << kBlockType_Spring))
							{
								// spring

								m_vel[i] = -BLOCKTYPE_SPRING_SPEED;

								playSecondaryEffects(kPlayerEvent_SpringJump);
							}
							else
							{
								const float sign = Calc::Sign(totalDelta);
								float strength = (Calc::Abs(totalDelta) / dt - PLAYER_JUMP_SPEED) / 25.f;

								if (strength > PLAYER_SCREENSHAKE_STRENGTH_THRESHHOLD)
								{
									strength = sign * strength / 4.f;
									GAMESIM->addScreenShake(0.f, strength, 3000.f, .3f);
								}

								// todo : use separate PlayerAnim for special attack

								if (characterData->m_special == kPlayerSpecial_DownAttack && m_special.attackDownActive && !s_noSpecial)
								{
									int size = (int(m_special.attackDownHeight) - STOMP_EFFECT_MIN_HEIGHT) * STOMP_EFFECT_MAX_SIZE / (STOMP_EFFECT_MAX_HEIGHT - STOMP_EFFECT_MIN_HEIGHT);
									if (size > STOMP_EFFECT_MAX_SIZE)
										size = STOMP_EFFECT_MAX_SIZE;
									if (size >= 1)
										GAMESIM->addFloorEffect(m_index, m_pos[0], m_pos[1], size, (size + 1) * 2 / 3);
								}

								m_special.attackDownActive = false;
								m_special.attackDownHeight = 0.f;

								m_vel[i] = 0.f;
							}
						}
						else
						{
							handleJumpCollision();
						}
					}

					totalDelta = 0.f;
				}
				else
				{
					m_pos[i] = newPos[i];

					totalDelta -= delta;
				}

				if (m_special.attackDownActive && i == 1 && delta > 0.f)
					m_special.attackDownHeight += delta;
			}
		}
	#endif

		// grounded?

		if (m_bubble.timer != 0.f)
			m_isGrounded = false;
	#if USE_NEW_COLLISION_CODE
		else if (m_dirBlockMaskDir[1] > 0 && (dirBlockMask[1] & kBlockMask_Solid) != 0)
	#else
		else if (m_vel[1] >= 0.f && (getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f) & kBlockMask_Solid) != 0)
	#endif
		{
			if (!m_isGrounded)
			{
				m_isGrounded = true;

				playSecondaryEffects(kPlayerEvent_LandOnGround);
			}
		}
		else
		{
			m_isGrounded = false;
		}

		// breaking

		if (steeringSpeed == 0.f || (std::abs(m_vel[0]) > std::abs(steeringSpeed)))
		{
			m_vel[0] *= powf(1.f - surfaceFriction, dt * 60.f);
		}

		if (m_attack.m_rocketPunch.isActive)
		{
			m_vel *= powf(1.f - .05f, dt * 60.f);
		}

		// speed clamp

		if (std::abs(m_vel[1]) > PLAYER_SPEED_MAX)
		{
			m_vel[1] = PLAYER_SPEED_MAX * Calc::Sign(m_vel[1]);
		}

		// air dash

		if (m_isGrounded || m_isAttachedToSticky || m_isWallSliding)
		{
			m_isAirDashCharged = true;
		}

		// animation

		if (!m_isGrounded && !m_isAttachedToSticky && !m_isWallSliding && !m_isAnimDriven && m_enableInAirAnim)
		{
			setAnim(kPlayerAnim_Jump, true, false);
		}

	#if !WRAP_AROUND_TOP_AND_BOTTOM
		// death by fall

		if (m_pos[1] > ARENA_SY_PIXELS)
		{
			if (isAnimOverrideAllowed(kPlayerAnim_Die))
			{
				setAnim(kPlayerAnim_Die, true, true);
				m_isAnimDriven = true;

				awardScore(-1);
			}
		}
	#endif

		// facing

		if (playerControl && steeringSpeed != 0.f)
		{
			const float newFacing = steeringSpeed < 0.f ? -1 : +1;
			if (newFacing != m_facing[0])
			{
				m_facingAnim = 1.f;
				m_facing[0] = newFacing;
			}
		}
		m_facing[1] = m_isAttachedToSticky ? -1 : +1;

		// wrapping

		if (m_pos[0] < 0)
		{
			m_pos[0] = ARENA_SX_PIXELS;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}
		if (m_pos[0] > ARENA_SX_PIXELS)
		{
			m_pos[0] = 0;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}

	#if WRAP_AROUND_TOP_AND_BOTTOM
		if (m_pos[1] > ARENA_SY_PIXELS)
		{
			m_pos[1] = 0;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}

		if (m_pos[1] < 0)
		{
			m_pos[1] = ARENA_SY_PIXELS;
			playSecondaryEffects(kPlayerEvent_ArenaWrap);
		}
	#endif
	}

	//printf("x: %g\n", m_pos[0]);

	m_facingAnim = Calc::Saturate(m_facingAnim - dt / (7.f / 60.f));

	if (m_isUsingJetpack)
	{
		ParticleSpawnInfo spawnInfo(
			m_pos[0], m_pos[1],
			kBulletType_ParticleA, 2,
			50.f, 100.f, 50.f);
		spawnInfo.color = 0xffffff80;
		GAMESIM->spawnParticles(spawnInfo);
	}

	// token hunt game mode

	if ((GAMESIM->m_gameMode == kGameMode_TokenHunt) && (GAMESIM->m_gameState == kGameState_Play))
	{
		if (m_isAlive)
		{
			CollisionInfo playerCollision;
			if (getPlayerCollision(playerCollision))
			{
				if (GAMESIM->pickupToken(playerCollision))
				{
					m_tokenHunt.m_hasToken = true;
				}
			}

			if (m_tokenHunt.m_hasToken && (GAMESIM->GetTick() % 4) == 0)
			{
				ParticleSpawnInfo spawnInfo(
					m_pos[0], m_pos[1],
					kBulletType_ParticleA, 2,
					50.f, 100.f, 50.f);
				spawnInfo.color = 0xffffff80;
				GAMESIM->spawnParticles(spawnInfo);
			}
		}
	}

	// coin collector game mode

	if ((GAMESIM->m_gameMode == kGameMode_CoinCollector) && (GAMESIM->m_gameState == kGameState_Play))
	{
		if (m_isAlive)
		{
			CollisionInfo playerCollision;
			if (getPlayerCollision(playerCollision))
			{
				if (GAMESIM->pickupCoin(playerCollision))
				{
					m_score++;
				}
			}
		}
	}
}

void Player::draw() const
{
	if (!hasValidCharacterIndex())
		return;

	const bool flipX = m_facing[0] > 0 ? true : false;
	const bool flipY = m_facing[1] < 0 ? true : false;

	drawAt(flipX, flipY, m_pos[0], m_pos[1] - (flipY ? PLAYER_COLLISION_HITBOX_SY : 0));

	// render additional sprites for wrap around
	drawAt(flipX, flipY, m_pos[0] + ARENA_SX_PIXELS, m_pos[1] - (flipY ? PLAYER_COLLISION_HITBOX_SY : 0));
	drawAt(flipX, flipY, m_pos[0] - ARENA_SX_PIXELS, m_pos[1] - (flipY ? PLAYER_COLLISION_HITBOX_SY : 0));
	drawAt(flipX, flipY, m_pos[0], m_pos[1] - (flipY ? PLAYER_COLLISION_HITBOX_SY : 0) + ARENA_SY_PIXELS);
	drawAt(flipX, flipY, m_pos[0], m_pos[1] - (flipY ? PLAYER_COLLISION_HITBOX_SY : 0) - ARENA_SY_PIXELS);

	// draw player color

	const Color color = getPlayerColor(m_index);
	setColorf(
		color.r,
		color.g,
		color.b,
		.5f);
	drawRect(m_pos[0] - 50, m_pos[1] - 110, m_pos[0] + 50, m_pos[1] - 85);

	// draw invincibility marker

	if (m_spawnInvincibilityTicks > 0)
	{
		const float t = m_spawnInvincibilityTicks / float(TICKS_PER_SECOND * PLAYER_RESPAWN_INVINCIBILITY_TIME);
		setColorf(
			color.r,
			color.g,
			color.b,
			t);
		drawRect(
			m_pos[0] + m_collision.min[0], 0,
			m_pos[0] + m_collision.max[0], GFX_SY);
	}
	
	/*
	if (GAMESIM->m_gameMode == kGameMode_TokenHunt && m_tokenHunt.m_hasToken)
	{
		setColorf(1.f, 1.f, 1.f, (std::sin(g_TimerRT.Time_get() * 10.f) * .7f + 1.f) / 2.f);
		drawRect(m_pos[0] - 50, m_pos[1] - 110, m_pos[0] + 50, m_pos[1] - 85);
	}
	*/

	// draw score
	setFont("calibri.ttf");
	setColor(255, 255, 255);
	drawText(m_pos[0], m_pos[1] - 110, 20, 0.f, +1.f, "%d", m_score);

	if (!m_isAlive && m_canRespawn && (GAMESIM->m_gameState == kGameState_Play) && m_isRespawn)
	{
		int sx = 40;
		int sy = 40;

		// we're dead and we're waiting for respawn
		setColor(0, 0, 255, 127);
		drawRect(m_pos[0] - sx / 2, m_pos[1] - sy / 2, m_pos[0] + sx / 2, m_pos[1] + sy / 2);
		setColor(0, 0, 255);
		drawRectLine(m_pos[0] - sx / 2, m_pos[1] - sy / 2, m_pos[0] + sx / 2, m_pos[1] + sy / 2);

		setColor(255, 255, 255);
		setFont("calibri.ttf");
		drawText(m_pos[0], m_pos[1], 24, 0, 0, "(X)");
	}

	// draw text chat

	if (!m_instanceData->m_textChat.empty())
	{
		float sx, sy;
		measureText(INGAME_TEXTCHAT_FONT_SIZE, sx, sy, "%s", m_instanceData->m_textChat.c_str());

		const int sizeX = INGAME_TEXTCHAT_PADDING_X * 2 + sx;
		const int sizeY = INGAME_TEXTCHAT_SIZE_Y;
		const int offsetX = m_pos[0] - sizeX / 2;
		const int offsetY = m_pos[1] - 150;

		setColor(255, 255, 255, 227);
		drawRect(offsetX, offsetY, offsetX + sizeX, offsetY + sizeY);
		setColor(0, 0, 255, 127);
		drawRectLine(offsetX, offsetY, offsetX + sizeX, offsetY + sizeY);

		setFont("calibri.ttf");
		setColor(0, 0, 255);
		drawText(offsetX + sizeX/2, offsetY + INGAME_TEXTCHAT_PADDING_Y, INGAME_TEXTCHAT_FONT_SIZE, 0.f, +1.f, "%s", m_instanceData->m_textChat.c_str());

		setColor(colorWhite);
	}
}

void Player::drawAt(bool flipX, bool flipY, int x, int y) const
{
	const CharacterData * characterData = getCharacterData(m_characterIndex);

	// draw rocket punch charge and direction

	if (m_attack.m_rocketPunch.isActive && m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Charge)
	{
		const float t = (m_attack.m_rocketPunch.chargeTime - ROCKETPUNCH_CHARGE_MIN) / (ROCKETPUNCH_CHARGE_MAX - ROCKETPUNCH_CHARGE_MIN);
		Color c1 = colorRed;
		Color c2 = colorYellow;
		Color c = t == 1.f ? colorWhite : c1.interp(c2, t);
		setColor(c);
		drawRect(
			x - 50,
			y - 100,
			x + 50,
			y + 50);

		const float px = x;
		const float py = y - 30.f;
		const Vec2 dir = m_input.getAnalogDirection().CalcNormalized();
		const Vec2 off1 = dir * 50.f;
		const Vec2 off2 = dir * 100.f;

		setColor(colorGreen);
		drawLine(
			px + off1[0],
			py + off1[1],
			px + off2[0],
			py + off2[1]);
		drawRect(
			px + off2[0] - 5.f,
			py + off2[1] - 5.f,
			px + off2[0] + 5.f,
			py + off2[1] + 5.f);
	}

	if (GAMESIM->m_gameMode == kGameMode_TokenHunt)
	{
		setColorMode(COLOR_ADD);
		if (m_tokenHunt.m_hasToken)
			setColorf(1.f, 1.f, 1.f, 1.f, (std::sin(g_TimerRT.Time_get() * 10.f) * .7f + 1.f) / 4.f);
		else
			setColorf(0.f, 0.f, 0.f);
	}
	else
	{
		setColor(colorWhite);
	}

	if (m_ice.timer > 0.f)
		setColor(63, 127, 255);

	const float scale = characterData->m_spriteScale * PLAYER_SPRITE_SCALE;
	const float animScale = (1.f - m_facingAnim * 2.f);
	const float animScale2 = (animScale < 0.f ? (animScale - .5f) : (animScale + .5f)) / 1.5f;

	SpriterState spriterState = m_spriterState;
	spriterState.x = x;
	spriterState.y = y;
	spriterState.scaleX = scale * animScale2;
	spriterState.scaleY = scale;
	spriterState.flipX = flipX;
	spriterState.flipY = flipY;
	characterData->m_spriter->draw(spriterState);

	setColorMode(COLOR_MUL);
	setColor(255, 255, 255);

	if (m_special.meleeCounter != 0)
	{
		Sprite sprite("doublemelee.png");
		sprite.flipX = m_facing[0] < 0.f;
		sprite.drawEx(x, y + mirrorY(m_attack.collision.min[1]));
	}

	if (m_shield.shield)
	{
		const float px = x;
		const float py = y + (flipY ? +PLAYER_COLLISION_HITBOX_SY : -PLAYER_COLLISION_HITBOX_SY) / 2.f;
		Sprite("shield-bubble.png").drawEx(px, py);
	}

	if (m_bubble.timer > 0.f)
	{
		const float px = x;
		const float py = y + (flipY ? +PLAYER_COLLISION_HITBOX_SY : -PLAYER_COLLISION_HITBOX_SY) / 2.f;
		Sprite("bubble-bubble.png").drawEx(px, py);
	}

	if (m_anim == kPlayerAnim_Attack || m_anim == kPlayerAnim_AttackUp || m_anim == kPlayerAnim_AttackDown)
	{
		CollisionInfo collisionInfo;
		getAttackCollision(collisionInfo);

		setColor(255, 0, 0);
		//drawRect(collisionInfo.x1, collisionInfo.y1, collisionInfo.x2, collisionInfo.y2);
		//drawLine(x, y, x + m_anim.m_attackDx * 50, y + m_anim.m_attackDy * 50);
	}
}

void Player::drawLight() const
{
	const float x = m_pos[0] + (m_collision.min[0] + m_collision.max[0]) / 2.f;
	const float y = m_pos[1] + (m_collision.min[1] + m_collision.max[1]) / 2.f;
	Sprite("player-light.png").drawEx(x, y, 0.f, 3.f, 3.f, false, FILTER_LINEAR);
}

void Player::debugDraw() const
{
	setColor(0, 31, 63, 63);
	drawRect(
		m_pos[0] + m_collision.min[0],
		m_pos[1] + m_collision.min[1],
		m_pos[0] + m_collision.max[0] + 1,
		m_pos[1] + m_collision.max[1] + 1);

	CollisionInfo damageCollision;
	getDamageHitbox(damageCollision);
	setColor(63, 31, 0, 63);
	drawRect(
		damageCollision.min[0],
		damageCollision.min[1],
		damageCollision.max[0] + 1,
		damageCollision.max[1] + 1);

	float y = m_pos[1];

	if (m_attack.attacking && m_attack.hasCollision)
	{
		CollisionInfo attackCollision;
		getAttackCollision(attackCollision);

		Font font("calibri.ttf");
		setFont(font);

		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "attacking");
		y += 18.f;

		setColor(255, 0, 0, 63);
		drawRect(
			attackCollision.min[0],
			attackCollision.min[1],
			attackCollision.max[0],
			attackCollision.max[1]);
	}

	if (m_isHuggingWall)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "wallhug");
		y += 18.f;
	}

	if (m_isWallSliding)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "wallslide");
		y += 18.f;
	}

	if (m_isGrounded)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "grounded");
		y += 18.f;
	}

	if (m_isAttachedToSticky)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "stickied");
		y += 18.f;
	}

	setColor(colorWhite);
}

uint32_t Player::getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const
{
	const int x1 = (x + (int)m_collision.min[0] + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int x2 = (x + (int)m_collision.max[0] + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int y1 = (y + (int)m_collision.min[1] + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y2 = (y + (int)m_collision.max[1] + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y3 = (y + (int)(m_collision.min[1] + m_collision.max[1]) / 2 + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;

	uint32_t result = 0;
	
	const Arena & arena = GAMESIM->m_arena;

	result |= arena.getIntersectingBlocksMask(x1, y1);
	result |= arena.getIntersectingBlocksMask(x2, y1);
	result |= arena.getIntersectingBlocksMask(x2, y2);
	result |= arena.getIntersectingBlocksMask(x1, y2);

	result |= arena.getIntersectingBlocksMask(x1, y3);
	result |= arena.getIntersectingBlocksMask(x2, y3);

#if 1
	CollisionInfo collisionInfo = m_collision;
	collisionInfo.min[0] += x;
	collisionInfo.min[1] += y;
	collisionInfo.max[0] += x;
	collisionInfo.max[1] += y;

	for (int i = 0; i < MAX_MOVERS; ++i)
	{
		const Mover & mover = GAMESIM->m_movers[i];

		if (mover.m_isActive && mover.intersects(collisionInfo))
			//result |= (1 << kBlockType_Indestructible);
			result |= (1 << kBlockType_Passthrough);
	}
#endif

	return result;
}

uint32_t Player::getIntersectingBlocksMask(int x, int y) const
{
	return (getIntersectingBlocksMaskInternal(x, y, true) & m_blockMask);
}

void Player::handleNewGame()
{
	m_score = 0;
	m_totalScore = 0;
}

void Player::handleNewRound()
{
	if (m_score > 0)
		m_totalScore += m_score;
	m_score = 0;

	m_lastSpawnIndex = -1;
	m_isRespawn = false;

	m_weaponStackSize = 0;

	m_ice = IceInfo();
	m_bubble = BubbleInfo();

	m_tokenHunt = TokenHunt();
}

void Player::respawn()
{
	if (!hasValidCharacterIndex())
		return;

	int x, y;

	if (GAMESIM->m_arena.getRandomSpawnPoint(*GAMESIM, x, y, m_lastSpawnIndex, this))
	{
		m_spawnInvincibilityTicks = TICKS_PER_SECOND * PLAYER_RESPAWN_INVINCIBILITY_TIME;

		m_pos[0] = (float)x;
		m_pos[1] = (float)y;

		m_vel[0] = 0.f;
		m_vel[1] = 0.f;

		m_isAlive = true;

		m_controlDisableTime = 0.f;

		m_weaponStackSize = 0;

		m_attack = AttackInfo();

		m_timeDilationAttack = TimeDilationAttack();

		m_blockMask = 0;

		m_dirBlockMask[0] = 0;
		m_dirBlockMask[1] = 0;
		m_dirBlockMaskDir[0] = 0;
		m_dirBlockMaskDir[1] = 0;
		m_oldBlockMask = 0;

		m_isGrounded = false;
		m_isAttachedToSticky = false;
		m_isAnimDriven = false;
		m_animVelIsAbsolute = false;
		m_isAirDashCharged = false;
		m_isInPassthrough = false;
		m_isHuggingWall = false;
		m_isWallSliding = false;
		m_enterPassthrough = false;

		//

		setAnim(kPlayerAnim_Spawn, true, true);
		m_isAnimDriven = true;

		if (m_isRespawn)
			playSecondaryEffects(kPlayerEvent_Respawn);
		else
		{
			playSecondaryEffects(kPlayerEvent_Spawn);
			m_isRespawn = true;
		}
	}
}

void Player::cancelAttack()
{
	if (m_attack.attacking)
	{
		m_attack.attacking = false;

		setAnim(kPlayerAnim_Walk, false, true);
	}
}

void Player::handleImpact(Vec2Arg velocity)
{
	Vec2 velDelta = velocity - m_vel;

	for (int i = 0; i < 2; ++i)
	{
		if (Calc::Sign(velDelta[i]) == Calc::Sign(velocity[i]))
			m_vel[i] += velDelta[i];
	}
}

bool Player::shieldAbsorb(float amount)
{
	if (m_shield.shield > 0)
	{
		m_shield.shield--;
		return true;
	}
	else
	{
		return false;
	}
}

bool Player::handleDamage(float amount, Vec2Arg velocity, Player * attacker)
{
	if (m_isAlive && isAnimOverrideAllowed(kPlayerAnim_Die))
	{
		handleImpact(velocity);

		if (m_spawnInvincibilityTicks > 0 ||
			shieldAbsorb(amount) ||
			GAMESIM->m_gameState != kGameState_Play)
		{
			return false;
		}
		else
		{
			bool canBeKilled = true;

			if (GAMESIM->m_gameMode == kGameMode_CoinCollector)
				canBeKilled = COINCOLLECTOR_PLAYER_CAN_BE_KILLED || (attacker == 0) || (attacker == this);

			if (canBeKilled)
			{
				setAnim(kPlayerAnim_Die, true, true);
				m_isAnimDriven = true;

				m_canRespawn = false;

				m_isAlive = false;

				m_ice = IceInfo();
				m_bubble = BubbleInfo();

				// fixme.. mid pos
				ParticleSpawnInfo spawnInfo(m_pos[0], m_pos[1] + mirrorY(-PLAYER_COLLISION_HITBOX_SY/2.f), kBulletType_ParticleA, 20, 50, 350, 40);
				spawnInfo.color = 0xff0000ff;

				GAMESIM->spawnParticles(spawnInfo);

				if (PROTO_TIMEDILATION_ON_KILL && attacker)
				{
					GAMESIM->addTimeDilationEffect(
						PROTO_TIMEDILATION_ON_KILL_MULTIPLIER1,
						PROTO_TIMEDILATION_ON_KILL_MULTIPLIER2,
						PROTO_TIMEDILATION_ON_KILL_DURATION);
				}
			}

			if (attacker && attacker != this)
			{
				bool canScore = true;

				switch (GAMESIM->m_gameMode)
				{
				case kGameMode_DeathMatch:
					attacker->awardScore(1);
					break;

				case kGameMode_TokenHunt:
					// if the attacker has the token, or we're the token bearer
					if (attacker->m_tokenHunt.m_hasToken || m_tokenHunt.m_hasToken)
						attacker->awardScore(1);
					break;
				}
			}
			else
			{
				switch (GAMESIM->m_gameMode)
				{
				case kGameMode_CoinCollector:
					break;

				default:
					awardScore(-1);
					break;
				}
			}

			// token hunt

			if (GAMESIM->m_gameMode == kGameMode_TokenHunt)
			{
				if (m_tokenHunt.m_hasToken)
				{
					m_tokenHunt.m_hasToken = false;

					Token & token = GAMESIM->m_tokenHunt.m_token;
					token.setup(
						int(m_pos[0] / BLOCK_SX),
						int(m_pos[1] / BLOCK_SY));
					token.m_vel.Set(velocity[0] * TOKEN_DROP_SPEED_MULTIPLIER, -800.f);
					token.m_isDropped = true;
					token.m_dropTimer = TOKEN_DROP_TIME;
					GAMESIM->playSound("token-bounce.ogg"); // sound when the token is dropped
				}
			}

			// coin collector

			if (GAMESIM->m_gameMode == kGameMode_CoinCollector)
			{
				if (m_score >= 1)
				{
					const int numCoins = std::max(1, m_score * COINCOLLECTOR_COIN_DROP_PERCENTAGE / 100);

					dropCoins(numCoins);
				}
			}

			return true;
		}
	}
	else
	{
		return false;
	}
}

bool Player::handleIce(Vec2Arg velocity, Player * attacker)
{
	cancelAttack();

	if (m_isAlive && isAnimOverrideAllowed(kPlayerAnim_Die))
	{
		if (shieldAbsorb(1.f))
		{
			handleImpact(velocity * PLAYER_SHIELD_IMPACT_MULTIPLIER);
		}
		else
		{
			handleImpact(velocity * PLAYER_EFFECT_ICE_IMPACT_MULTIPLIER);

			setAnim(kPlayerAnim_Walk, true, true);
			m_isAnimDriven = true;
			m_enableInAirAnim = false;

			m_ice.timer = PLAYER_EFFECT_ICE_TIME;
		}
	}

	return m_isAlive;
}

bool Player::handleBubble(Vec2Arg velocity, Player * attacker)
{
	cancelAttack();

	if (m_isAlive && isAnimOverrideAllowed(kPlayerAnim_Die))
	{
		if (shieldAbsorb(1.f))
		{
			handleImpact(velocity * PLAYER_SHIELD_IMPACT_MULTIPLIER);
		}
		else
		{
			Vec2 direction = velocity.CalcNormalized();
			direction += Vec2(0.f, -1.5f);
			direction.Normalize();
			m_vel = direction * PLAYER_EFFECT_BUBBLE_SPEED;

			setAnim(kPlayerAnim_Walk, true, true);
			m_isAnimDriven = true;
			m_enableInAirAnim = false;

			m_bubble.timer = PLAYER_EFFECT_BUBBLE_TIME;
		}
	}

	return m_isAlive;
}

void Player::awardScore(int score)
{
	m_score += score;
}

void Player::dropCoins(int numCoins)
{
	Assert(numCoins <= m_score);

	m_score -= numCoins;

	for (int i = 0; i < numCoins; ++i)
	{
		Coin * coin = GAMESIM->allocCoin();

		if (coin)
		{
			const int blockX = (int(m_pos[0] / BLOCK_SX) + ARENA_SX) % ARENA_SX;
			const int blockY = (int(m_pos[1] / BLOCK_SY) + ARENA_SY) % ARENA_SY;

			coin->setup(blockX, blockY);
			coin->m_dropTimer = COIN_DROP_TIME;

			coin->m_vel.Set(GAMESIM->RandomFloat(-COIN_DROP_SPEED, +COIN_DROP_SPEED), -COIN_DROP_SPEED);

			GAMESIM->playSound("coin-bounce.ogg"); // sound when a coin hits the ground and bounces
		}
	}
}

void Player::pushWeapon(PlayerWeapon weapon, int ammo)
{
	// fixme ?
	ammo = 1;

	for (int a = 0; a < ammo; ++a)
	{
		for (int i = 0; i < m_weaponStackSize; ++i)
			if (i + 1 < MAX_WEAPON_STACK_SIZE)
				m_weaponStack[i + 1] = m_weaponStack[i];
		m_weaponStack[0] = weapon;
		m_weaponStackSize = std::min(m_weaponStackSize + 1, MAX_WEAPON_STACK_SIZE);
	}
}

PlayerWeapon Player::popWeapon()
{
	PlayerWeapon result = kPlayerWeapon_None;
	if (m_weaponStackSize > 0)
	{
		result = m_weaponStack[0];
		for (int i = 0; i < m_weaponStackSize - 1; ++i)
			m_weaponStack[i] = m_weaponStack[i + 1];
		m_weaponStackSize--;
	}
	return result;
}

void Player::handleJumpCollision()
{
	if (!m_jump.cancelStarted)
	{
		m_jump.cancelStarted = true;
		m_jump.cancelled = false;
		m_jump.cancelX = m_pos[0];
		m_jump.cancelFacing = m_facing[0];
	}
	else
	{
		m_jump.cancelled =
			m_jump.cancelled ||
			std::abs(m_jump.cancelX - m_pos[0]) > PLAYER_JUMP_GRACE_PIXELS ||
			m_facing[0] != m_jump.cancelFacing;

		if (m_jump.cancelled)
		{
			m_vel[1] = 0.f;
		}
	}
}

void Player::beginRocketPunch()
{
	m_attack = AttackInfo();
	m_attack.attacking = true;

	m_attack.m_rocketPunch.isActive = true;

	// start anim

	setAnim(kPlayerAnim_RocketPunch_Charge, true, true);
	m_isAnimDriven = true;

	GAMESIM->playSound("rocketpunch-charge.ogg"); // charge at the start of the rocket punch attack
}

void Player::endRocketPunch(bool stunned)
{
	if (stunned)
	{
		Assert(m_attack.m_rocketPunch.isActive);
		Assert(m_attack.m_rocketPunch.stunTime == 0.f);

		// todo : change animation

		m_attack.m_rocketPunch.state = AttackInfo::RocketPunch::kState_Stunned;

		m_animVelIsAbsolute = false;
	}
	else
	{
		logDebug("rocket punch: attack: done!");

		m_attack = AttackInfo();
		clearAnimOverrides();

		setAnim(kPlayerAnim_Walk, false, true);

		m_vel = Vec2();
	}
}

//

CharacterData::CharacterData(int characterIndex)
	: m_spriter(0)
	, m_spriteScale(1.f)
	, m_special(kPlayerSpecial_None)
{
	load(characterIndex);
}

CharacterData::~CharacterData()
{
	delete m_spriter;
	m_spriter = 0;
}

void CharacterData::load(int characterIndex)
{
	// reload character properties

	m_props.load(makeCharacterFilename(characterIndex, "props.txt"));

	const char * soundBags[] =
	{
		"spawn_sounds",
		"die_sounds",
		"jump_sounds",
		"attack_sounds",
		"taunt_sounds"
	};

	for (size_t i = 0; i < sizeof(soundBags) / sizeof(soundBags[0]); ++i)
	{
		const char * name = soundBags[i];

		m_sounds[name].load(m_props.getString(name, ""), true);
	}

	m_animData = AnimData();
	m_animData.load(makeCharacterFilename(characterIndex, "animdata.txt"));

	delete m_spriter;
	m_spriter = 0;

	const char * filename = makeCharacterFilename(characterIndex, "sprite/sprite.scml");
	m_spriter = new Spriter(filename);

	m_spriteScale = m_props.getFloat("sprite_scale", 1.f);

	// special

	PlayerSpecial special = kPlayerSpecial_None;

	const std::string specialStr = m_props.getString("special", "");

	if (specialStr == "rocket_punch")
		special = kPlayerSpecial_RocketPunch;
	if (specialStr == "double_melee")
		special = kPlayerSpecial_DoubleSidedMelee;
	if (specialStr == "down_attack")
		special = kPlayerSpecial_DownAttack;
	if (specialStr == "shield")
		special = kPlayerSpecial_Shield;
	if (specialStr == "invisibility")
		special = kPlayerSpecial_Invisibility;
	if (specialStr == "jetpack")
		special = kPlayerSpecial_Jetpack;

	m_special = special;
}

//

PlayerInstanceData::PlayerInstanceData(Player * player, GameSim * gameSim)
	: m_player(player)
	, m_gameSim(gameSim)
	, m_textChatTicks(0)
{
	m_player->m_instanceData = this;

	//

	m_player->m_collision.min[0] = -PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_player->m_collision.max[0] = +PLAYER_COLLISION_HITBOX_SX / 2.f;
	m_player->m_collision.min[1] = -PLAYER_COLLISION_HITBOX_SY / 1.f;
	m_player->m_collision.max[1] = 0.f;
}

PlayerInstanceData::~PlayerInstanceData()
{
}

void PlayerInstanceData::setCharacterIndex(int index)
{
	m_player->m_characterIndex = index;

	handleCharacterIndexChange();
}

void PlayerInstanceData::handleCharacterIndexChange()
{
	//m_player->m_spriterState.animIndex = // todo : recalc anim index?
}

void PlayerInstanceData::playSoundBag(const char * name, int volume)
{
	const CharacterData * characterData = getCharacterData(m_player->m_characterIndex);

	auto sb = characterData->m_sounds.find(name);
	Assert(sb != characterData->m_sounds.end());
	if (sb != characterData->m_sounds.end())
	{
		int & lastSoundId = m_lastSoundIds[name];
		m_gameSim->playSound(makeCharacterFilename(m_player->m_characterIndex, sb->second.getRandomSound(*m_gameSim, lastSoundId)), volume);
	}
}

void PlayerInstanceData::addTextChat(const std::string & line)
{
	m_textChat = line;
	m_textChatTicks = TICKS_PER_SECOND * INGAME_TEXTCHAT_DURATION;
}

//

static CharacterData * s_characterData[MAX_CHARACTERS] = { };

void initCharacterData()
{
	Assert(!s_characterData[0]);
	for (int i = 0; i < MAX_CHARACTERS; ++i)
		s_characterData[i] = new CharacterData(i);
}

void shutCharacterData()
{
	for (int i = 0; i < MAX_CHARACTERS; ++i)
	{
		delete s_characterData[i];
		s_characterData[i] = 0;
	}
}

const CharacterData * getCharacterData(int characterIndex)
{
	Assert(characterIndex >= 0 && characterIndex < MAX_CHARACTERS);
	return s_characterData[characterIndex];
}

char * makeCharacterFilename(int characterIndex, const char * filename)
{
	static char temp[64];
	sprintf_s(temp, sizeof(temp), "char%d/%s", characterIndex, filename);
	return temp;
}

Color getCharacterColor(int characterIndex)
{
	return getPlayerColor(characterIndex);
}

Color getPlayerColor(int playerIndex)
{
	// todo : get character color from character props

	static const Color colors[MAX_PLAYERS + 1] =
	{
		Color(255, 0,   0,   255),
		Color(255, 255, 0,   255),
		Color(0,   0,   255, 255),
		Color(0,   255, 0,   255),
		Color(50,  50,  50,  255)
	};

	if (playerIndex >= 0 && playerIndex < MAX_PLAYERS)
		return colors[playerIndex];
	else
		return colors[MAX_PLAYERS];
}

#undef GAMESIM
