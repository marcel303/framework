#include "arena.h"
#include "bullet.h"
#include "Calc.h"
#include "client.h"
#include "framework.h"
#include "host.h"
#include "Log.h"
#include "main.h"
#include "player.h"
#include "Timer.h"

#define ENABLE_CHARACTER_OUTLINE 1

#define AUTO_RESPAWN 1

//#pragma optimize("", off)

/*

- indievelopment explosion 101 video
+ add tile transition object type. define rect, specify transition type
+ add transition support to tile sprites
- add support for spriter reskins

+ add teleport object

- add mine radius animation. also makes it more explicit where it's deployed

+ bomb stuck on death kill
+ bomb deploy in air
- player score UI toggle
- add victory animation at end of round 'Victory'
- add emotes

+ fix jump pad anim triggered for all jump pads
+ fix black borders not drawing fullscreen when screen shakes are active
- fix controller assignments and joining/leaving using START when connected through online multiplayer. also, names were messed up
- let +1 icon follow player, improve art/animation

+ add jump pad animation
- and support for particles triggered by spriter animations?
- iterate on bubble gun:
	- add trap animation
	+ make directional
	- add cloud in the background to create a 'field' effect
- reverse player emblem color
- hide player emblem, unless when scoring or when summoned
- add weapon UI prototype. do some drawings first
- improve pipebomb art. add clearly visible flash
- add deploy animation to pipebomb guy. make it mines instead
- add killstreaks to screen. do some drawings first
- 'faster' grapple?
+ add grapple targeting preview
- add down strike animation effect?
# add jumppad object

+ add player character grid UI
- escape on dialog

+ round complete music loops
+ jump pad broken
+ show name on black bar
+ vertical black bars

+ new freeze pickup is huge -> make it smaller

- fix stars getting stuck in geometry
- investigate player collision bug test level
+ improve blinds/slowmo on death. add vertical bar
- prototype invisibility ability
- remove ninja dash from axe character. should be a special ability for a separate character

feedback:

- use lower case on all SCML and PNG filenames referenced in code

- fireball broken / volcano bugged
+ grapple aim
- jetpack op, boost mustn't be spammable
- shield shouldn't be spammable, needs cooldown when destroyed
	cling/cancel melee attack
+ ninja move axe character
- earth quake sound

todo:

- grapple: swing!
- napalm pickup
- freeze gun flow
- animated spring
- fluids
- humor objects
- interactive main menu
- physics particles
- interactive level objects
- arced bullets
- exploding barrels
- flamethrower

- add dialog button spriter object. add active/inactive animations

- jetpack ability V2:
	+ free analog stick control
	# fly/land behavior?
	- how to combine with attacks?
	+ slight bobbing up and down. reset/disable bobbing during fast movement?
	+ affected by gravity or not at all?
	- add fuel system?
	- recharge -> how?
	- debuffs:
		- slower speed when grounded?
		- no up/down/forward attack momentum?
		- less attack range?
	- extra buff ability
		- destroy jetpack with Y, repel players, need full recharge 

- arc bullets
- grapple auto shorten
- grapple small speed boost on attach in swing direction
- diagonal jump pads
- pickup spawn weights per tile
- shield ability: deflect projectiles in aim direction. recharge period. shield gets transparent

** HIGH PRIORITY **

- add animations:
	sword hit non destruct block - sparks at collission point
	sword hit destruct block - pop of block + block particles?
	sword hit character
	gun hit block
	gun hit character
	# dash effect - side
	# dbl jump effect - feet
	sword drag - special hitbox on sword tip, collission with tiles
	gun shoot flash/smoke

- add curve editing through options

- shield guy
	- can block in 1 direction
	- has charge that pushes and can flatten players against walls

** MEDIUM PRIORITY **

- fix bubbled player getting stuck on upward collision

- bubble gun prototype:
	- small charge period on fire (to allow other player to escape)
	- create small bubbles on fire
	- when another player hits bubble, player gets bubbled
	=> transforms bubble gun into a sort of snare trap, instead of offensive
	=> bubbles create temporary shield. they pop after a short while though

- prototype invisibility ability
	charge period

- prototype gravity well teleport

- add path for special

- prototype grappling hook
	+ aim for angle, or auto angle?
	+ jump behavior -> allow double jump?
	+ pull up/down/none?
	+ detach conditions: jump, detach (Y), attack, death, level wrap
	+ swing behavior: separate steering speed
	+ sounds and fx
	- grapple attach animation: will probably need an animation, but shouldn't make it harder to attach..
	- grapple rope draw. textured quad? currently just a line
	=> speed on attach + steering doesn't seem right (too slow). math issue, or add boost for better feeling?

- add callstack gathering

- better attach to platform logic, so we can have movers closer to each other without the player bugging

** LOW PRIORITY **

- add typing/pause bubble on char
- add key binding

** OTHER **

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
- send client network stats to host so they can be visualized

# add support for CRC compare at any time during game sim tick. log CRC's with function/line number and compare?

- zoom in on winning player, wait for a while before transitioning to the next round

- need to be able to kick player at char select
- buff star player ?
- score feedback, especially in token hunt mode
- investigate killing the rambo mode, soldat

- better death feedback
- team based game mode
# ammo despawn na x seconds + indicator (?)

- blood particles
- fill the level with lava

** DONE **

+ spike wall texture
+ lijntjes
# more speed axe throw?
+ bullets faster
+ jump pad jetpack
+ earth quake -> bump items
+ add ninja dash
+ add dialog spriter support
+ add yes/no dialog
+ add dust particles when moving grounded at speed > treshold (> normal walking speed)
+ add explosion effects on bombs 'n stuff
+ fix drawing of aim above/below layers
+ check build ID
	+ add built-in version ID on channel connect
+ add build version check for online
+ disable changing game options after everyone has readied up
+ add title screen
+ add main menu/title screen loop
+ disable auto respawn. player must press X to respawn
+ explosion effects: force affecting players, pickups, etc
+ axe velocity x 50% on deactivate, 50% gravity
+ attack down/passthrough behavior
+ slide block type friction %
+ add reusable aiming code and UI
+ add grapple aim
+ add character select between rounds
+ cancel passthrough/attack down behavior on attack up/double jump/etc (any attack/jump) + proto no auto mode, or duration = attack only

+ prototype rocket punch
	+ idle in air during charge
	+ vulnerable during charge
	+ analog stick = direction of attack
	# max charge = faster, further, succeed/or not, pass through all destructibles/not (strength)
	v2:
	+ short recharge
	+ auto discharge
	+ debuff on attack finish

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

COMMAND_OPTION(s_killPlayers, "Player/Kill Players", []{ g_app->netDebugAction("killPlayers", 0); });

#define WRAP_AROUND_TOP_AND_BOTTOM 1

#define GAMESIM m_instanceData->m_gameSim

#define SHIELD_SPRITER Spriter("objects/shield/sprite.scml")
#define BUBBLE_SPRITER Spriter("objects/bubble/sprite.scml")
#define SHIELDSPECIAL_SPRITER Spriter("objects/shieldspecial/sprite.scml")
#define EMBLEM_SPRITER Spriter("Player_Emblem/Player_Emblem.scml")

// todo : m_isGrounded should be true when stickied too. review code and make change!

struct PlayerAnimInfo
{
	const char * file;
	const char * name;
	int prio;
} s_animInfos[kPlayerAnim_COUNT] =
{
	{ nullptr,       nullptr,              0 },
	{ "sprite.scml", "Idle" ,              1 },
	{ "sprite.scml", "InAir" ,             1 },
	{ "sprite.scml", "Jump" ,              2 },
	{ "sprite.scml", "AirDash",            2 },
	{ "sprite.scml", "WallSlide",          3 },
	{ "sprite.scml", "Walk",               4 },
	{ "sprite.scml", "Attack",             5 },
	{ "sprite.scml", "AttackUp",           5 },
	{ "sprite.scml", "AttackDown",         5 },
	{ "sprite.scml", "Shoot",              5 },
	{ "sprite.scml", "RocketPunch_Charge", 5 },
	{ "sprite.scml", "RocketPunch_Attack", 5 },
	{ "sprite.scml", "Walk"                /*Zweihander_Charge*/,     5 },
	{ "sprite.scml", "AttackDown"          /*Zweihander_Attack*/,     5 },
	{ "sprite.scml", "AttackDown"          /*Zweihander_AttackDown*/, 5 },
	{ "sprite.scml", "Idle"                /*Zweihander_Stunned*/,    5 },
	{ "sprite.scml", "Pipebomb_Deploy",    5 },
	{ "sprite.scml", "AirDash",            5 },
	{ "sprite.scml", "Spawn",              6 },
	{ "sprite.scml", "Die",                7 }
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
			//const Vec2 dir = Vec2(player->m_input.m_currState.analogX, player->m_input.m_currState.analogY).CalcNormalized();
			const Vec2 dir(player->m_facing[0], player->m_facing[1]);
			const Vec2 vel(args.getFloat("x", player->m_animVel[0]), args.getFloat("y", player->m_animVel[1]));

			player->m_vel += dir ^ vel;
		}
		else if (action == "set_jump_vel")
		{
			const Vec2 dir(player->m_facing[0], player->m_facing[1]);
			const Vec2 vel(args.getFloat("x", player->m_animVel[0]), args.getFloat("y", player->m_animVel[1]));

			player->m_vel += dir ^ vel;
		}
		else if (action == "set_anim_vel_abs")
		{
			player->m_animVelIsAbsolute = args.getBool("abs", player->m_animVelIsAbsolute);
			if (args.getBool("reset", true))
				player->m_vel.SetZero();

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

bool Player::getPlayerControl() const
{
	return
		m_isAlive &&
		m_controlDisableTime == 0.f &&
		m_animAllowSteering &&
		m_ice.timer == 0.f &&
		m_bubble.timer == 0.f &&
		m_special.meleeCounter == 0 &&
		m_attack.m_rocketPunch.isActive == false &&
		m_attack.m_axeThrow.isActive == false &&
		GAMESIM->m_gameState != kGameState_RoundBegin &&
		(GAMESIM->m_gameState != kGameState_RoundComplete || GAMESIM->m_roundEnd.m_state == GameSim::RoundEnd::kState_ShowWinner);
}

Vec2 Player::getPlayerCenter() const
{
	const CharacterData * characterData = getCharacterData(m_characterIndex);

	return Vec2(
		m_pos[0],
		m_pos[1] + characterData->m_collisionSy / 2.f);
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

void Player::getDamageHitbox(CollisionShape & shape) const
{
	Assert(m_isUsed);

	shape.set(
		Vec2(m_pos[0] - PLAYER_DAMAGE_HITBOX_SX/2, m_pos[1] - PLAYER_DAMAGE_HITBOX_SY),
		Vec2(m_pos[0] + PLAYER_DAMAGE_HITBOX_SX/2, m_pos[1] - PLAYER_DAMAGE_HITBOX_SY),
		Vec2(m_pos[0] + PLAYER_DAMAGE_HITBOX_SX/2, m_pos[1]),
		Vec2(m_pos[0] - PLAYER_DAMAGE_HITBOX_SX/2, m_pos[1]));
}

bool Player::getAttackCollision(CollisionShape & shape, Vec2Arg shift) const
{
	Assert(m_isUsed);

	const CharacterData * characterData = getCharacterData(m_characterIndex);

	Vec2 points[4];

	if (!characterData->getSpriter()->getHitboxAtTime(m_spriterState.animIndex, "hitbox", m_spriterState.animTime, points))
	{
		return false;
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			points[i][0] *= characterData->m_spriteScale * PLAYER_SPRITE_SCALE * (-m_facing[0]);
			points[i][1] *= characterData->m_spriteScale * PLAYER_SPRITE_SCALE;
			if (m_facing[1] < 0)
				points[i][1] = -characterData->m_collisionSy - points[i][1];
			points[i] += m_pos + shift;
		}

		shape.set(points[0], points[1], points[2], points[3]);

		return true;
	}
}

float Player::getAttackDamage(Player * other) const
{
	Assert(m_isUsed);

	float result = 0.f;

	if (m_attack.attacking && m_attack.hasCollision)
	{
		CollisionShape attackShape;
		if (getAttackCollision(attackShape))
		{
			CollisionShape otherShape;
			other->getDamageHitbox(otherShape);

			if (attackShape.intersects(otherShape))
			{
				result = 1.f;
			}
		}
	}

	return result;
}

bool Player::isAnimOverrideAllowed(PlayerAnim anim) const
{
	Assert(anim < sizeof(s_animInfos) / sizeof(s_animInfos[0]));

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
	const CharacterData * characterData = getCharacterData(m_index);

	return m_facing[1] > 0 ? y : characterData->m_collisionSy - y;
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
	Assert(anim < sizeof(s_animInfos) / sizeof(s_animInfos[0]));
	Assert(isAnimOverrideAllowed(anim));

	if (anim != m_anim || play != m_animPlay || restart)
	{
		//logDebug("setAnim: %d", anim);

		m_anim = anim;
		m_animPlay = play;

		applyAnim();
	}
}

void Player::clearAnimOverrides()
{
	m_animVel.SetZero();
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

	if (!m_spriterState.startAnim(*characterData->getSpriter(), s_animInfos[m_anim].name))
	{
		const char * filename = makeCharacterFilename(m_characterIndex, "sprite/sprite.scml");
		logError("unable to find animation %s in file %s", s_animInfos[m_anim].name, filename);
		m_spriterState.startAnim(*characterData->getSpriter(), s_animInfos[kPlayerAnim_Jump].name);
	}

	if (!m_animPlay)
		m_spriterState.stopAnim(*characterData->getSpriter());
}

//

void Player::tick(float dt)
{
	if (!m_isActive)
		return;

	// -- emote hack --

	m_emoteTime = Calc::Max(0.f, m_emoteTime - dt); // fixme : should use phys time

	int emoteIndex = -1;

	if (m_input.wentUp(INPUT_BUTTON_DPAD_LEFT))
		emoteIndex = 0;
	if (m_input.wentUp(INPUT_BUTTON_DPAD_RIGHT))
		emoteIndex = 1;
	if (m_input.wentUp(INPUT_BUTTON_DPAD_UP))
		emoteIndex = 2;
	if (m_input.wentUp(INPUT_BUTTON_DPAD_DOWN))
		emoteIndex = 3;

	if (emoteIndex != -1)
	{
		m_emoteId = emoteIndex;
		m_emoteTime = EMOTE_DISPLAY_TIME;
	}

	// -- emote hack --

	const CharacterData * characterData = getCharacterData(m_characterIndex);

	if (m_instanceData->m_textChatTicks != 0)
	{
		if (--m_instanceData->m_textChatTicks == 0)
		{
			m_instanceData->m_textChat.clear();
		}
	}

	m_collision.min[0] = -characterData->m_collisionSx / 2.f;
	m_collision.max[0] = +characterData->m_collisionSx / 2.f;
	m_collision.min[1] = -characterData->m_collisionSy / 1.f;
	m_collision.max[1] = 0.f;

	//

	if (m_shield.spriterState.animIsActive)
	{
		m_shield.spriterState.updateAnim(SHIELD_SPRITER, dt);
	}

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
		{
			m_isAnimDriven = false;
			m_bubble.spriterState.startAnim(BUBBLE_SPRITER, "end");
		}
	}

	if (m_bubble.spriterState.animIsActive)
	{
		m_bubble.spriterState.updateAnim(BUBBLE_SPRITER, dt);
	}

	if (m_ice.timer > 0.f ||
		m_bubble.timer > 0.f)
		m_spriterState.animSpeed = 1e-10f;
	else
		m_spriterState.animSpeed = 1.f;

	if (m_pipebombCooldown > 0.f)
	{
		m_pipebombCooldown = Calc::Max(0.f, m_pipebombCooldown - dt);
	}

	m_timeDilationAttack.tick(dt);

	m_multiKillTimer = Calc::Max(0.f, m_multiKillTimer - dt);

	//

	if (m_anim != kPlayerAnim_NULL)
	{
		const CharacterData * characterData = getCharacterData(m_characterIndex);

		const char * name = s_animInfos[m_anim].name;
		const auto & animData = characterData->m_animData.m_animMap[name];
		Assert(animData.speed != 0.f);

		const int oldTime = m_spriterState.animTime * 1000.f;

		const bool isDone = m_spriterState.updateAnim(*characterData->getSpriter(), dt * animData.speed * PLAYER_ANIM_MULTIPLIER);

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
			case kPlayerAnim_DoubleJump:
			case kPlayerAnim_WallSlide:
			case kPlayerAnim_Walk:
				break;

			case kPlayerAnim_Attack:
			case kPlayerAnim_AttackUp:
			case kPlayerAnim_AttackDown:
				m_attack.attacking = false;
				m_enterPassthrough = false;
				break;
			case kPlayerAnim_Fire:
				m_attack.attacking = false;
				break;

			case kPlayerAnim_Zweihander_Charge:
				break;
			case kPlayerAnim_Zweihander_Attack:
				m_attack.m_zweihander.handleAttackAnimComplete(*this);
				break;
			case kPlayerAnim_Zweihander_Stunned:
				break;

			case kPlayerAnim_Pipebomb_Deploy:
				Assert(m_pipebomb.state == PipebombInfo::State_Deploy && m_pipebomb.time > 0.f);
				if (m_pipebomb.state == PipebombInfo::State_Deploy)
					endPipebomb();
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

	if (g_devMode && !g_monkeyMode && !DEMOMODE && m_input.wentDown(INPUT_BUTTON_START))
		respawn(0);

#if AUTO_RESPAWN
	if (!m_isAlive && !m_canRespawn)
	{
		m_canRespawn = true;
		m_canTaunt = true;
		m_respawnTimer = m_isRespawn ? 1.5f : 0.f;
		m_respawnTimerRcp = m_isRespawn ? 1.f / m_respawnTimer : 0.f;
	}
#endif

	if (!m_isAlive && !m_isAnimDriven)
	{
		if (!m_canRespawn)
		{
			m_canRespawn = true;
			m_canTaunt = true;
			m_respawnTimer = m_isRespawn ? 2.f : 0.f;
			m_respawnTimerRcp = m_isRespawn ? 1.f / m_respawnTimer : 0.f;
		}

		if (m_canTaunt && m_input.wentDown(INPUT_BUTTON_Y))
		{
			m_canTaunt = false;
			m_instanceData->playSoundBag("taunt_sounds", 100);
		}

		if (GAMESIM->m_gameState == kGameState_Play &&
			!GAMESIM->m_levelEvents.spikeWalls.isActive() &&
		#if AUTO_RESPAWN
			(m_respawnTimer <= 0.f))
		#else
			(m_input.wentDown(INPUT_BUTTON_X) || (PLAYER_RESPAWN_AUTOMICALLY && m_respawnTimer <= 0.f)))
		#endif
		{
			respawn(0);
		}

		m_respawnTimer = Calc::Max(0.f, m_respawnTimer - dt);
	}

	//if (m_isAlive)
	{
		m_spawnInvincibilityTime = Calc::Max(0.f, m_spawnInvincibilityTime - dt);

		bool playerControl = getPlayerControl();

		if (m_isAlive)
		{
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
					if (!m_shield.hasShield)
					{
						m_shield.hasShield = true;
						m_shield.spriterState.startAnim(SHIELD_SPRITER, "begin");
					}
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

			if (characterData->m_special == kPlayerSpecial_AxeThrow && !m_axe.hasAxe)
			{
				if (m_axe.recoveryTime > 0.f)
				{
					m_axe.recoveryTime -= dt;
					if (m_axe.recoveryTime <= 0.f)
						m_axe.hasAxe = true;
				}

				CollisionInfo playerCollision;
				if (getPlayerCollision(playerCollision))
				{
					if (GAMESIM->grabAxe(playerCollision))
						m_axe.hasAxe = true;
				}

				if (m_axe.hasAxe)
					m_axe.recoveryTime = 0.f;
			}
		}

		float surfaceFriction = 0.f;
		Vec2 animVel(0.f, 0.f);

		animVel[0] += m_animVel[0] * m_facing[0];
		animVel[1] += m_animVel[1] * m_facing[1];

		// attack processing

		if (m_attack.attacking)
		{
			Assert(m_isAlive);

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
						bool cancelled = false;
						bool absorbed = false;

						CollisionShape shape1;
						CollisionShape shape2;

						if (getAttackCollision(shape1) && other.getAttackCollision(shape2))
						{
							if (shape1.intersects(shape2))
								cancelled = true;
						}

						if (cancelled == false)
						{
							const float damage = getAttackDamage(&other);

							if (damage != 0.f)
							{
								Vec2 direction;
								if (m_anim == kPlayerAnim_Attack || m_anim == kPlayerAnim_AttackUp || m_anim == kPlayerAnim_AttackDown)
									direction = Vec2(m_attackDirection[0], m_attackDirection[1]).CalcNormalized();
								else
									direction = Vec2(m_facing[0], 0.f).CalcNormalized();

								if (!other.handleDamage(damage, direction * PLAYER_SWORD_PUSH_SPEED, this))
								{
									absorbed = true;
								}
							}
						}

						if (cancelled || absorbed)
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

								if (j == 1 || cancelled)
								//if (true)
								{
									players[j]->m_vel = reflect * PLAYER_SWORD_CLING_SPEED / characterData->m_weight;
									players[j]->m_controlDisableTime = PLAYER_SWORD_CLING_TIME;
								}

								players[j]->cancelAttack();
							}

							if (cancelled)
							{
								GAMESIM->playSound("melee-cancel.ogg"); // sound when two melee attacks hit at the same time

								Vec2 min1, max1;
								Vec2 min2, max2;
								shape1.getMinMax(min1, max1);
								shape2.getMinMax(min2, max2);
								Vec2 mid = (min1 + max1 + min2 + max2) / 4.f;
								GAMESIM->addAnimationFx("fx/Attack_MeleeCancel.scml", mid[0], mid[1]);
							}
						}
					}
				}

				// see if we've hit a block

				CollisionShape attackCollision;
				if (getAttackCollision(attackCollision))
				{
					const bool hit = GAMESIM->m_arena.handleDamageShape(
						*GAMESIM,
						m_pos[0],
						m_pos[1],
						attackCollision,
						!m_attack.hitDestructible,
						PLAYER_SWORD_SINGLE_BLOCK);

					if (hit && PLAYER_SWORD_SINGLE_BLOCK)
						m_attack.hitDestructible = true;
					if (hit)
						m_vel[1] *= PLAYER_SWORD_BLOCK_DESTROY_SLOWDOWN;
				}

				// see if we hit a fireball

				for (int i = 0; i < MAX_FIREBALLS; ++i)
				{
					FireBall& other = GAMESIM->m_fireballs[i];

					if (other.active)
					{
						CollisionShape shape1;
						CollisionShape shape2;

						if (getAttackCollision(shape1) && other.getCollision(shape2))
						{
							if (shape1.intersects(shape2))
								GAMESIM->m_fireballs[i].active = false;
						}
					}
				}
			}

			// update rocket punch attack

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

			// update axe throw attack

			if (m_attack.m_axeThrow.isActive)
			{
				tickAxeThrow(dt);
			}

			// update zweihander attack

			if (m_attack.m_zweihander.isActive())
			{
				m_attack.m_zweihander.tick(*this, dt);
			}
		}

		// attack cooldown

		if (!m_attack.attacking)
			m_attack.cooldown -= dt;

		// attack triggering

		if (playerControl && !m_attack.attacking && m_attack.cooldown <= 0.f && (GAMESIM->m_gameMode != kGameMode_Lobby))
		{
			if (m_input.wentDown(INPUT_BUTTON_B) && (m_weaponStackSize > 0 || s_unlimitedAmmo) && isAnimOverrideAllowed(kPlayerAnim_Fire))
			{
				m_attack = AttackInfo();

				const float x = m_pos[0] + mirrorX(0.f);
				const float y = m_pos[1] - mirrorY(44.f);

				PlayerAnim anim = kPlayerAnim_NULL;
				BulletType bulletType = kBulletType_COUNT;
				BulletEffect bulletEffect = kBulletEffect_Damage;
				bool hasAttackCollision = false;
				int numBullets = 1;
				bool randomBulletAngle = false;

				PlayerWeapon weaponType = popWeapon();

				if (weaponType == kPlayerWeapon_Fire)
				{
					anim = kPlayerAnim_Fire;
					bulletType = kBulletType_B;
					m_attack.cooldown = PLAYER_FIRE_COOLDOWN;
					
					GAMESIM->playSound("gun-fire.ogg");
				}
				else if (weaponType == kPlayerWeapon_Ice)
				{
					anim = kPlayerAnim_Fire;
					bulletType = kBulletType_B;
					bulletEffect = kBulletEffect_Ice;
					m_attack.cooldown = PLAYER_FIRE_COOLDOWN;

					GAMESIM->playSound("gun-fire-ice.ogg");
				}
				else if (weaponType == kPlayerWeapon_Bubble)
				{
					anim = kPlayerAnim_Fire;
					m_attack.cooldown = PLAYER_FIRE_COOLDOWN;

					const Vec2 playerSpeed = m_lastTotalVel;

					for (int i = 0; i < BULLET_BUBBLE_COUNT; ++i)
					{
						const float angle = GAMESIM->Random() % 256;

						const uint16_t bulletId = GAMESIM->spawnBullet(
							x,
							y,
							angle,
							kBulletType_Bubble,
							kBulletEffect_Bubble,
							m_index);

						if (bulletId != INVALID_BULLET_ID)
						{
							Bullet & b = GAMESIM->m_bulletPool->m_bullets[bulletId];

							const Vec2 dir = (-m_lastTotalVel * BULLET_BUBBLE_PLAYERSPEED_MULTIPLIER + b.m_vel).CalcNormalized();
							const float vel = GAMESIM->RandomFloat(BULLET_BUBBLE_SPEED_MIN, BULLET_BUBBLE_SPEED_MAX);

							b.m_vel = dir * vel;
							b.m_pos += b.m_vel.CalcNormalized() * BULLET_BUBBLE_SPAWN_DISTANCE;
						}
					}

					GAMESIM->addAnimationFx(
						"fx/Attack_FireBullet.scml",
						x,
						y,
						m_facing[0] < 0,
						m_facing[1] < 0);

					GAMESIM->playSound("gun-fire-bubble.ogg");
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
				else
				{
					Assert(false);
				}

				if (anim != kPlayerAnim_NULL)
				{
					setAnim(anim, true, true);
					m_isAnimDriven = true;

					m_attack.attacking = true;
				}

				if (bulletType != kBulletType_COUNT)
				{
					for (int i = 0; i < numBullets; ++i)
					{
						int angle;

						if (randomBulletAngle)
						{
							angle = GAMESIM->Random() % 256;
						}
						else
						{
							// determine attack direction based on player input

							if (m_input.isDown(INPUT_BUTTON_UP))
								angle = 256*1/4;
							else if (m_input.isDown(INPUT_BUTTON_DOWN))
								angle = 256*3/4;
							else if (m_facing[0] < 0)
								angle = 128;
							else
								angle = 0;
						}

						Assert(bulletType != kBulletType_COUNT);
						GAMESIM->spawnBullet(
							x,
							y,
							angle,
							bulletType,
							bulletEffect,
							m_index);
					}

					GAMESIM->addAnimationFx(
						"fx/Attack_FireBullet.scml",
						x,
						y,
						m_facing[0] < 0,
						m_facing[1] < 0);
				}
			}

			if (m_input.wentDown(INPUT_BUTTON_X) && isAnimOverrideAllowed(kPlayerAnim_Attack))
			{
				m_attack = AttackInfo();
				m_attack.attacking = true;

				m_attack.cooldown = characterData->m_meleeCooldown;

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
					if (PLAYER_SWORD_DOWN_PASSTHROUGH)
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
				m_attack.collision.min[1] = -characterData->m_collisionSy/3.f*2;
				m_attack.collision.max[1] = -characterData->m_collisionSy/3.f*2 + m_attackDirection[1] * 50.f; // up or down

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

				endGrapple();
			}

			if (!s_noSpecial && (GAMESIM->m_gameMode != kGameMode_Lobby))
			{
				if (characterData->m_special == kPlayerSpecial_Grapple &&
					m_grapple.state == GrappleInfo::State_Inactive &&
					m_input.wentDown(INPUT_BUTTON_Y) &&
					isAnimOverrideAllowed(kPlayerAnim_Attack))
				{
					beginGrapple();
				}
				else if (characterData->m_special == kPlayerSpecial_Pipebomb &&
					m_isGrounded &&
					m_pipebomb.state == PipebombInfo::State_Inactive &&
					m_input.wentDown(INPUT_BUTTON_Y) &&
					isAnimOverrideAllowed(kPlayerAnim_Attack))
				{
					// detonate?

					bool isDeployed = false;

					for (int i = 0; i < MAX_PIPEBOMBS; ++i)
					{
						if (GAMESIM->m_pipebombs[i].m_isActive && GAMESIM->m_pipebombs[i].m_playerIndex == m_index)
						{
							isDeployed = true;

							if (GAMESIM->m_pipebombs[i].explode())
								m_pipebombCooldown = PIPEBOMB_COOLDOWN;
						}
					}

					if (!isDeployed && m_pipebombCooldown == 0.f)
					{
						beginPipebomb();
					}
				}
				else if (characterData->m_special == kPlayerSpecial_RocketPunch &&
					m_input.wentDown(INPUT_BUTTON_Y) &&
					isAnimOverrideAllowed(kPlayerAnim_Attack))
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
				else if (characterData->m_special == kPlayerSpecial_AxeThrow &&
					m_input.wentDown(INPUT_BUTTON_Y) &&
					isAnimOverrideAllowed(kPlayerAnim_Attack))
				{
					beginAxeThrow();
				}
				else if (characterData->m_special == kPlayerSpecial_DoubleSidedMelee &&
					m_input.wentDown(INPUT_BUTTON_Y) &&
					isAnimOverrideAllowed(kPlayerAnim_Attack))
				{
					m_attack = AttackInfo();
					m_attack.attacking = true;

					m_attack.cooldown = characterData->m_meleeCooldown;

					// start anim

					setAnim(kPlayerAnim_Walk, true, true);

					m_attack.collision.min[0] = 0.f;
					m_attack.collision.max[0] = DOUBLEMELEE_ATTACK_RADIUS;
					m_attack.collision.min[1] = -characterData->m_collisionSy/3.f*2;
					m_attack.collision.max[1] = -characterData->m_collisionSy/3.f*2 + 4.f;

					m_attack.hasCollision = true;

					m_special.meleeCounter = DOUBLEMELEE_SPIN_COUNT;
					m_special.meleeAnimTimer = DOUBLEMELEE_SPIN_TIME;
				}
				else if (characterData->m_special == kPlayerSpecial_Zweihander &&
					m_input.wentDown(INPUT_BUTTON_Y) &&
					isAnimOverrideAllowed(kPlayerAnim_Attack))
				{
					m_attack = AttackInfo();
					m_attack.attacking = true;

					m_attack.m_zweihander.begin(*this);
				}
				else if (characterData->m_special == kPlayerSpecial_Invisibility &&
					m_input.wentDown(INPUT_BUTTON_Y) &&
					isAnimOverrideAllowed(kPlayerAnim_Attack))
				{
					// todo : spawn invisibility curtain

					for (int i = 0; i < INVISIBILITY_PLUME_COUNT; ++i)
					{
						const float angle = Calc::m2PI * i / INVISIBILITY_PLUME_COUNT;
						const float radius = GAMESIM->RandomFloat(INVISIBILITY_PLUME_DISTANCE_MIN, INVISIBILITY_PLUME_DISTANCE_MAX);
						GAMESIM->addAnimationFx(
							"fx/Invisibility_Plume.scml",
							m_pos[0] + std::sin(angle) * radius,
							m_pos[1] + std::cos(angle) * radius);
					}
				}
			}
		}

		playerControl = getPlayerControl();

		// update double melee attack

		if (characterData->m_special == kPlayerSpecial_DoubleSidedMelee && m_special.meleeCounter != 0)
		{
			Assert(m_isAlive);

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

		//

		m_blockMask = ~0;

		const uint32_t currentBlockMask = m_oldBlockMask;

		const bool enterPassThrough = m_enterPassthrough || (m_special.attackDownActive) || (m_attack.m_rocketPunch.isActive)
			|| m_input.isDown(INPUT_BUTTON_DOWN);
		if (m_isInPassthrough || enterPassThrough)
			m_blockMask = ~kBlockMask_Passthrough;

		const uint32_t currentBlockMaskFloor = m_dirBlockMaskDir[1] > 0 ? m_dirBlockMask[1] : 0;
		const uint32_t currentBlockMaskCeil = m_dirBlockMaskDir[1] < 0 ? m_dirBlockMask[1] : 0;

		//if (GAMESIM->m_gameMode == kGameMode_Lobby)
		if (false)
		{
			// none of that special traits stuff onboard the enterprise!
		}
		else if (characterData->hasTrait(kPlayerTrait_AirDash))
		{
			if (playerControl && m_isAirDashCharged && !m_isGrounded && !m_isAttachedToSticky && m_input.wentDown(INPUT_BUTTON_A))
			{
				if (isAnimOverrideAllowed(kPlayerAnim_AirDash))
				{
					if ((getIntersectingBlocksMask(m_pos[0] + m_facing[0], m_pos[1]) & kBlockMask_Solid) == 0)
					{
						m_isAirDashCharged = false;

						setAnim(kPlayerAnim_AirDash, true, true);
						m_enableInAirAnim = false;
					}
				}
			}
		}
		else if (characterData->hasTrait(kPlayerTrait_DoubleJump))
		{
			if (playerControl && m_isAirDashCharged && !m_isGrounded && !m_isAttachedToSticky && !m_isWallSliding && (m_grapple.state == GrappleInfo::State_Inactive) && m_input.wentDown(INPUT_BUTTON_A))
			{
				if (isAnimOverrideAllowed(kPlayerAnim_DoubleJump))
				{
					m_isAirDashCharged = false;

					setAnim(kPlayerAnim_DoubleJump, true, true);
					m_enableInAirAnim = false;

					m_vel[1] = -PLAYER_DOUBLE_JUMP_SPEED;
				}
			}
		}
		else if (characterData->hasTrait(kPlayerTrait_NinjaDash))
		{
			if (playerControl && m_isAirDashCharged && !m_isGrounded && !m_isAttachedToSticky && !m_isWallSliding && (m_grapple.state == GrappleInfo::State_Inactive) && m_input.wentDown(INPUT_BUTTON_A))
			{
				if (isAnimOverrideAllowed(kPlayerAnim_AirDash))
				{
					Vec2 targetPosition;
					if (findNinjaDashTarget(targetPosition))
					{
						m_pos = targetPosition;

						m_isAirDashCharged = false;

						setAnim(kPlayerAnim_AirDash, true, true);
						m_enableInAirAnim = false;
					}
				}
			}
		}

		// death by spike

		if (currentBlockMask & (1 << kBlockType_Spike))
		{
			handleDamage(1.f, Vec2(0.f, 0.f), 0);
		}

		// teleport

		{
			{
				int portalId;

				Portal * portal = GAMESIM->findPortal(
					m_pos[0] + m_collision.min[0],
					m_pos[1] + m_collision.min[1],
					m_pos[0] + m_collision.max[0],
					m_pos[1] + m_collision.max[1],
					true,
					portalId);

				if (!portal || portalId != m_teleport.lastPortalId)
				{
					m_teleport.cooldown = false;
				}
			}

			if (!m_teleport.cooldown)
			{
				int portalId;

				Portal * portal = GAMESIM->findPortal(
					m_pos[0] + m_collision.min[0],
					m_pos[1] + m_collision.min[1],
					m_pos[0] + m_collision.max[0],
					m_pos[1] + m_collision.max[1],
					false,
					portalId);

				if (portal)
				{
					Portal * destination;
					int destinationId;

					if (portal->doTeleport(*GAMESIM, destination, destinationId))
					{
						const Vec2 offset = m_pos - portal->getDestinationPos(Vec2(0.f, 0.f));

						m_pos = destination->getDestinationPos(offset);

						m_teleport.cooldown = true;
						m_teleport.lastPortalId = destinationId;
					}
				}
			}
		}

		const bool allowJumping =
			playerControl;// &&
			//m_attack.m_zweihander.isActive() == false;

		m_controlDisableTime -= dt;
		if (m_controlDisableTime < 0.f)
			m_controlDisableTime = 0.f;

		// sticky ceiling

		if (currentBlockMaskCeil & (1 << kBlockType_Sticky))
		{
			if (m_isAttachedToSticky && allowJumping && m_input.wentDown(INPUT_BUTTON_A))
			{
				m_vel[1] = PLAYER_JUMP_SPEED / 2.f;

				m_isAttachedToSticky = false;

				GAMESIM->playSound("player-sticky-jump.ogg"); // player jumps and releases itself from a sticky ceiling
			}
			else if (m_isAttachedToSticky && allowJumping && m_input.wentDown(INPUT_BUTTON_DOWN))
			{
				m_isAttachedToSticky = false;

				m_vel[1] = 0.f;
			}
			else if (m_vel[1] <= 0.f && characterData->hasTrait(kPlayerTrait_StickyWalk))
			{
				surfaceFriction = FRICTION_GROUNDED;

				if (!m_isAttachedToSticky)
				{
					m_isAttachedToSticky = true;
				}
			}
		}
		else
		{
			m_isAttachedToSticky = false;
		}

		// steering

		float steeringSpeed[2] = { 0.f, 0.f };

		for (int i = 0; i < 2; ++i)
		{
			int numSteeringFrames = 1;

			if (m_jetpack.isActive)
			{
				if (playerControl && isAnimOverrideAllowed(kPlayerAnim_Walk))
				{
					setAnim(kPlayerAnim_Idle, true, false);
				}
			}
			else if (i == 0)
			{
				steeringSpeed[i] += m_input.m_currState.getAnalogDirection()[i];
			
				if ((m_isGrounded || m_isAttachedToSticky) && playerControl)
				{
					if (isAnimOverrideAllowed(kPlayerAnim_Walk))
					{
						if (steeringSpeed[i] != 0.f)
							setAnim(kPlayerAnim_Walk, true, false);
						else
							setAnim(kPlayerAnim_Idle, true, false);
					}
				}
			}

			//

			if (JETPACK_NEW_STEERING && m_jetpack.isActive)
			{
				const Curve * curves[2] = { &jetpackAnalogCurveX, &jetpackAnalogCurveY };
				const Vec2 analog = m_input.getAnalogDirection();
				m_jetpack.dashRemaining = Calc::Max(0.f, m_jetpack.dashRemaining - dt);
				//if (analog.CalcSize() < .1f)
				if (JETPACK_DASH_ON_JUMP && m_input.wentDown(INPUT_BUTTON_A))
					m_jetpack.dashRemaining = JETPACK_DASH_DURATION;
				if (JETPACK_DASH_ON_DIRECTION_CHANGE && analog * m_jetpack.oldAnalog <= JETPACK_DASH_RELOAD_TRESHOLD)
					m_jetpack.dashRemaining = JETPACK_DASH_DURATION;
				m_jetpack.oldAnalog = analog;
				const float mult = (m_jetpack.dashRemaining > 0.f && analog.CalcSize() > .5f)
					? JETPACK_DASH_SPEED_MULTIPLIER
					: 1.f;
				const float value = Calc::Sign(analog[i]) * curves[i]->eval(Calc::Abs(analog[i]));
				//if (value != 0.f)
					m_jetpack.steeringSpeed[i] = value * mult;
				//else
				//	m_jetpack.steeringSpeed[i] *= powf(1.f - FRICTION_JETPACK, dt * 60.f);
				steeringSpeed[i] = m_jetpack.steeringSpeed[i];
			}
			else if (i == 0)
			{
				bool isWalkingOnSolidGround = false;

				if (m_isAttachedToSticky)
					isWalkingOnSolidGround = (currentBlockMaskCeil & kBlockMask_Solid) != 0;
				else
					isWalkingOnSolidGround = (currentBlockMaskFloor & kBlockMask_Solid) != 0;

				if (m_attack.m_zweihander.isActive())
				{
					Assert(m_isAlive);

					if (m_isGrounded)
						steeringSpeed[i] *= STEERING_SPEED_ZWEIHANDER;
					numSteeringFrames = 5;
				}
				else if (isWalkingOnSolidGround)
				{
					steeringSpeed[i] *= STEERING_SPEED_ON_GROUND;
					numSteeringFrames = 5;
				}
				else if (m_jetpack.isActive)
				{
					steeringSpeed[i] *= STEERING_SPEED_JETPACK;
					numSteeringFrames = 5;
				}
				else if (m_special.meleeCounter != 0)
				{
					steeringSpeed[i] *= STEERING_SPEED_DOUBLEMELEE;
					numSteeringFrames = 5;
				}
				else if (m_grapple.state == GrappleInfo::State_Attached)
				{
					const Vec2 v1 = getGrapplePos() - m_grapple.anchorPos;
					if (Calc::Sign(v1[i]) != Calc::Sign(steeringSpeed[i]))
					{
						steeringSpeed[i] *= STEERING_SPEED_GRAPPLE;
						numSteeringFrames = 5;
					}
					else
					{
						steeringSpeed[i] = 0.f;
					}
				}
				else
				{
					steeringSpeed[i] *= STEERING_SPEED_IN_AIR;
					numSteeringFrames = 5;
				}
			}

			bool doSteering =
				m_isAlive &&
				(playerControl || m_special.meleeCounter != 0/* || m_attack.m_axeThrow.isActive*/);

			if (doSteering && steeringSpeed[i] != 0.f)
			{
				float maxSteeringDelta;

				if (steeringSpeed[i] > 0.f)
					maxSteeringDelta = steeringSpeed[i] - m_vel[i];
				if (steeringSpeed[i] < 0.f)
					maxSteeringDelta = steeringSpeed[i] - m_vel[i];

				if (Calc::Sign(steeringSpeed[i]) == Calc::Sign(maxSteeringDelta))
					m_vel[i] += maxSteeringDelta * dt * 60.f / numSteeringFrames;
			}
		}

		// gravity

		float gravity = 0.f;

		const bool wasWallSliding = m_isWallSliding;

		m_isWallSliding = false;
		m_jetpack.isActive = false;

		if (playerControl && characterData->m_special == kPlayerSpecial_Jetpack && (m_input.isDown(INPUT_BUTTON_Y) || JETPACK_NEW_STEERING) && !s_noSpecial)
			m_jetpack.isActive = true;

		if (m_animAllowGravity &&
			//!m_isAttachedToSticky &&
			!m_animVelIsAbsolute &&
			m_bubble.timer == 0.f &&
			(!m_attack.m_rocketPunch.isActive || m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Stunned))
		{
			if (JETPACK_NEW_STEERING && m_jetpack.isActive)
				gravity = 0.f;
			else if (currentBlockMask & (1 << kBlockType_GravityDisable))
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

			if (!JETPACK_NEW_STEERING && m_jetpack.isActive)
				gravity -= JETPACK_ACCEL;
			
			// wall slide

			if (!m_isGrounded &&
				!m_isAttachedToSticky &&
				isAnimOverrideAllowed(kPlayerAnim_WallSlide) &&
				!m_jetpack.isActive &&
				m_special.meleeCounter == 0 &&
				!m_special.attackDownActive &&
				m_attack.m_zweihander.state != AttackInfo::Zweihander::kState_AttackDown &&
				m_vel[0] != 0.f && Calc::Sign(m_facing[0]) == Calc::Sign(m_vel[0]) &&
				//Calc::Sign(m_vel[1]) == Calc::Sign(gravity) &&
				(Calc::Sign(m_vel[1]) == Calc::Sign(gravity) || Calc::Abs(m_vel[1]) <= PLAYER_JUMP_SPEED / 2.f) &&
				m_isHuggingWall)
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

			if (m_attack.m_zweihander.state == AttackInfo::Zweihander::kState_AttackDown)
				m_attack.m_zweihander.end(*this);
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

		if (allowJumping && m_input.wentDown(INPUT_BUTTON_A))
		{
			if ((currentBlockMaskFloor & kBlockMask_Solid) || (m_grapple.state == GrappleInfo::State_Attached))
			{
				m_jump.jumpVelocityLeft = -PLAYER_JUMP_SPEED;
				m_jump.cancelStarted = false;

				Dictionary args;
				args.setPtr("obj", m_instanceData);
				args.setString("name", "jump_sounds");
				m_instanceData->handleAnimationAction("char_soundbag", args);
				if (currentBlockMaskFloor & kBlockMask_Solid)
					GAMESIM->addAnimationFx("fx/Dust_JumpFromGround.scml", m_pos[0], m_pos[1]); // player jumps

				endGrapple();
			}
		}

		// jumping : increment speed over time for soft jump

		if (allowJumping && m_input.isDown(INPUT_BUTTON_A) && m_jump.jumpVelocityLeft != 0.f)
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

		// grapple rope

		tickGrapple(dt);

		if (m_grapple.state == GrappleInfo::State_Attached)
		{
			const float kMaxMove = 5.f;
			const Vec2 p1 = getGrapplePos();
			const Vec2 p2 = m_grapple.anchorPos;
			const Vec2 pd = p2 - p1;
			const Vec2 dn = pd.CalcNormalized();
			const float distance = pd.CalcSize();
			float move = distance - m_grapple.distance;
			if (move > 0.f)
			{
				if (Calc::Abs(move) > kMaxMove)
					move = kMaxMove * Calc::Sign(move);
				m_pos += dn * move;
				const float speed = m_vel.CalcSize();
				const float d = dn * m_vel;
				m_vel -= dn * d;
				//m_vel = m_vel.CalcNormalized() * speed;
			}
		}

		// shield special

		tickShieldSpecial(dt);

		// pipebomb special

		tickPipebomb(dt);

		// update grounded state

	#if 1 // player will think it's grounded if not reset and hitting spring
		if (m_isGrounded && m_vel[1] < 0.f)
			m_isGrounded = false;
	#endif

		const Vec2 oldPos = m_pos;

		// collision

		// colliding with solid object left/right of player

		if (m_isWallSliding && playerControl && m_input.wentDown(INPUT_BUTTON_A))
		{
			// wall jump

			m_vel[0] = -PLAYER_WALLJUMP_RECOIL_SPEED * m_dirBlockMaskDir[0];
			m_vel[1] = -PLAYER_WALLJUMP_SPEED;

			m_controlDisableTime = PLAYER_WALLJUMP_RECOIL_TIME;

			GAMESIM->playSound("player-wall-jump.ogg"); // player performs a walljump
		}

		const bool wasInPassthrough = m_isInPassthrough;

		m_isInPassthrough = false;
		m_isHuggingWall = false;

		Vec2 totalVel = (m_vel * (m_animVelIsAbsolute ? 0.f : 1.f)) + animVel;

		m_lastTotalVel = totalVel;

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

		CollisionShape shape;
		shape.set(
			Vec2(m_collision.min[0], m_collision.min[1]),
			Vec2(m_collision.max[0], m_collision.min[1]),
			Vec2(m_collision.max[0], m_collision.max[1]),
			Vec2(m_collision.min[0], m_collision.max[1]));

		const int kMaxSpringLocations = 32;

		struct CollisionArgs
		{
			Player * self;
			Vec2 totalVel;
			uint32_t * dirBlockMask;
			bool enterPassThrough;
			bool wasInPassthrough;
			float gravity;
			struct
			{
				int x, y;
			} springLocations[kMaxSpringLocations];
			int numSpringLocations;
			int maxSpringLocations;
		};

		CollisionArgs args;

		args.self = this;
		args.totalVel = totalVel;
		args.dirBlockMask = dirBlockMask;
		args.enterPassThrough = enterPassThrough;
		args.wasInPassthrough = wasInPassthrough;
		args.gravity = gravity;
		args.numSpringLocations = 0;
		args.maxSpringLocations = kMaxSpringLocations;

		Vec2 newTotalVel = totalVel;

		updatePhysics(
			*GAMESIM,
			m_pos,
			newTotalVel,
			dt,
			shape,
			&args,
			[](PhysicsUpdateInfo & updateInfo)
			{
				CollisionArgs * args = (CollisionArgs*)updateInfo.arg;
				Player * self = args->self;
				const Vec2 & delta = updateInfo.delta;
				const Vec2 & totalVel = args->totalVel;
				const bool enterPassThrough = args->enterPassThrough;
				const int i = updateInfo.axis;
				const float gravity = args->gravity;

				const CharacterData * characterData = getCharacterData(self->m_characterIndex);

				//

				int result = 0;

				//

				BlockAndDistance * blockAndDistance = updateInfo.blockInfo;

				if (self->m_attack.m_rocketPunch.isActive && self->m_attack.m_rocketPunch.state == AttackInfo::RocketPunch::kState_Attack)
				{
					Assert(self->m_isAlive);

					if (blockAndDistance)
					{
						if (blockAndDistance->block->handleDamage(*self->m_instanceData->m_gameSim, blockAndDistance->x, blockAndDistance->y))
						{
							blockAndDistance = 0;
							result |= kPhysicsUpdateFlag_DontCollide;
						}
						else if ((1 << blockAndDistance->block->type) & kBlockMask_Solid)
							self->endRocketPunch(true);
					}
					if (updateInfo.player && updateInfo.player != self)
						updateInfo.player->handleDamage(1.f, totalVel, self);
				}

				if (self->m_attack.attacking && self->m_attack.hasCollision)
				{
					Assert(self->m_isAlive);

					if (blockAndDistance)
					{
						for (int dx = -1; dx <= +1 && blockAndDistance; ++dx)
						{
							for (int dy = -1; dy <= +1 && blockAndDistance; ++dy)
							{
								CollisionShape attackCollision;
								if (self->getAttackCollision(attackCollision, Vec2(dx * ARENA_SX_PIXELS, dy * ARENA_SY_PIXELS)))
								{
									self->m_attack.hitDestructible |= self->m_instanceData->m_gameSim->m_arena.handleDamageShape(
										*self->m_instanceData->m_gameSim,
										updateInfo.pos[0],
										updateInfo.pos[1],
										attackCollision,
										!self->m_attack.hitDestructible,
										PLAYER_SWORD_SINGLE_BLOCK);

									if (blockAndDistance->block->shape == kBlockShape_Empty)
										blockAndDistance = 0;
								}
							}
						}
					}
				}

				if (blockAndDistance)
				{
					Block * block = blockAndDistance->block;

					if (block)
					{
						const uint32_t mask = (1 << block->type) & ((args->wasInPassthrough || self->m_isInPassthrough || enterPassThrough) ? ~kBlockMask_Passthrough : ~0);

						args->dirBlockMask[i] |= mask;

						if (updateInfo.contactNormal[0] * updateInfo.contactNormal[1] != 0.f)
						{
							args->dirBlockMask[0] |= mask;
							args->dirBlockMask[1] |= mask;
						}

						if ((1 << block->type) & kBlockMask_Passthrough)
						{
							if (i != 1 || delta[1] < 0.f || (args->wasInPassthrough || self->m_isInPassthrough || enterPassThrough))
							{
								result |= kPhysicsUpdateFlag_DontCollide;

								self->m_isInPassthrough = true;
							}
						}

						if (block->type == kBlockType_Spring)
						{
							if (args->numSpringLocations < args->maxSpringLocations)
							{
								args->springLocations[args->numSpringLocations].x = blockAndDistance->x;
								args->springLocations[args->numSpringLocations].y = blockAndDistance->y;
								args->numSpringLocations++;
							}
						}

						if (((1 << block->type) & kBlockMask_Solid) == 0)
							result |= kPhysicsUpdateFlag_DontCollide;
					}

					if (!(result & kPhysicsUpdateFlag_DontCollide))
					{
						// wall slide

						if (delta[0] != 0.f &&
							updateInfo.contactNormal[0] != 0.f &&
							Calc::Sign(delta[0]) == Calc::Sign(updateInfo.contactNormal[0]))
						{
							//self->m_isHuggingWall = delta[0] < 0.f ? -1 : +1;
							self->m_isHuggingWall = true;
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
								3000.f, .3f,
								true);
						}

						if (/*self->m_isAnimDriven && */self->m_anim == kPlayerAnim_AirDash && characterData->hasTrait(kPlayerTrait_AirDash))
						{
							self->m_spriterState.stopAnim(*characterData->getSpriter());
							self->clearAnimOverrides();
						}

						// effects

						if (self->m_ice.timer != 0.f || self->m_bubble.timer != 0.f)
						{
							// ice and bubble effects do their own collision handling (reflect)

							result |= kPhysicsUpdateFlag_DontUpdateVelocity;
						}

						if (i == 1)
						{
							self->m_enterPassthrough = false;
						}

						if (i == 1 && delta[1] < 0.f && gravity >= 0.f)
						{
							self->handleJumpCollision();

							result |= kPhysicsUpdateFlag_DontUpdateVelocity;
						}
					}
				}

				return result;
			});

		if (m_animVelIsAbsolute)
			m_vel.SetZero();
		else
		{
			Vec2 delta = newTotalVel - totalVel;
			m_vel += delta;
		}

		const Vec2 newPos = m_pos;

		// surface type

		const uint32_t blockMask = dirBlockMask[0] | dirBlockMask[1];

		if (JETPACK_NEW_STEERING && m_jetpack.isActive)
			surfaceFriction = 0.f;
		else if ((dirBlockMask[1] & (1 << kBlockType_Slide)) && totalVel[1] >= 0.f)
			surfaceFriction = FRICTION_GROUNDED_SLIDE;
		else if (m_ice.timer != 0.f)
			surfaceFriction = 0.f;
		else if ((Calc::Sign(m_vel[1]) != Calc::Sign(gravity) && m_vel[1] != 0.f) || (dirBlockMask[1] & kBlockMask_Solid) == 0)
			surfaceFriction = 0.f;
		else
			surfaceFriction = FRICTION_GROUNDED;

		for (int i = 0; i < 2; ++i)
		{
			if (m_ice.bounceFrames[i] > 0)
				m_ice.bounceFrames[i]--;
			if (m_bubble.bounceFrames[i] > 0)
				m_bubble.bounceFrames[i]--;

			// spring

			if (i == 1 && (dirBlockMask[i] & (1 << kBlockType_Spring)) && totalVel[i] >= 0.f && !m_jetpack.isActive)
			{
				m_vel[i] = -BLOCKTYPE_SPRING_SPEED;

				GAMESIM->playSound("player-spring-jump.ogg"); // player walks over and activates a jump pad

				for (int s = 0; s < args.numSpringLocations; ++s)
				{
					TileSprite * tileSprite = GAMESIM->findTileSpriteAtBlockXY(args.springLocations[s].x, args.springLocations[s].y);
					if (tileSprite)
						tileSprite->startAnim("Activate");
				}
			}

			// effects

			else if (m_ice.timer != 0.f && (dirBlockMask[i] & kBlockMask_Solid))
			{
				if (m_ice.bounceFrames[i] == 0)
				{
					m_ice.bounceFrames[i] = 2;
					m_vel[i] *= -.5f;
				}
			}
			else if (m_bubble.timer != 0.f && (dirBlockMask[i] & kBlockMask_Solid))
			{
				if (m_bubble.bounceFrames[i] == 0)
				{
					m_bubble.bounceFrames[i] = 2;
					m_vel[i] *= -.75f;
				}
			}

			// wall slide effects

			if (i == 1)
			{
				if (!m_isWallSliding || !wasWallSliding)
				{
					m_wallSlideDistance = 0.f;
				}

				if (m_isWallSliding)
				{
					const float delta = Calc::Abs(newPos[i] - oldPos[i]);

					m_wallSlideDistance += delta;

					while (m_wallSlideDistance >= PLAYER_WALLSLIDE_FX_INTERVAL)
					{
						m_wallSlideDistance -= PLAYER_WALLSLIDE_FX_INTERVAL;

						CollisionInfo playerCollision;
						if (getPlayerCollision(playerCollision))
						{
							GAMESIM->addAnimationFx("fx/Dust_WallSlide.scml",
								m_facing[0] < 0.f ? playerCollision.min[0] : playerCollision.max[0],
								newPos[1],
								m_facing[0] > 0.f);
						}
					}
				}
			}
		}

		m_dirBlockMaskDir[0] = totalVel[0] == 0.f ? 0 : totalVel[0] < 0.f ? -1 : +1;
		m_dirBlockMaskDir[1] = totalVel[1] == 0.f ? 0 : totalVel[1] < 0.f ? -1 : +1;
		m_dirBlockMask[0] = dirBlockMask[0];
		m_dirBlockMask[1] = dirBlockMask[1];
		m_oldBlockMask = blockMask;

		// attempt to stay grounded

		if (m_isGrounded && m_vel[1] >= 0.f)
		{
			const uint32_t groundBlockMask = getIntersectingBlocksMask(m_pos[0], m_pos[1] + 1.f);

			if (groundBlockMask & kBlockMask_Solid)
			{
				//logDebug("keep grounded");

				m_vel[1] = Calc::Max(0.f, m_vel[1]);
				m_dirBlockMaskDir[1] = 1;
				m_dirBlockMask[1] |= kBlockMask_Solid;
			}

			const uint32_t newBlockMask = getIntersectingBlocksMask(m_pos[0], m_pos[1]);

			if ((newBlockMask & kBlockMask_Solid) == 0)
			{
				//logDebug("try keep grounded");

				for (int dy = 1; dy <= 10; ++dy)
				{
					const uint32_t blockMask = getIntersectingBlocksMask(m_pos[0], m_pos[1] + dy);

					if ((blockMask & kBlockMask_Solid) != 0)
					{
						logDebug("keep grounded success");

						m_pos[1] += dy - 1.f;
						m_vel[1] = Calc::Max(0.f, m_vel[1]);
						m_dirBlockMaskDir[1] = 1;
						m_dirBlockMask[1] |= kBlockMask_Solid;
						break;
					}
				}
			}
		}

		// grounded?

		if (m_bubble.timer != 0.f)
			m_isGrounded = false;
		else if (m_dirBlockMaskDir[1] > 0 && (m_dirBlockMask[1] & kBlockMask_Solid) != 0)
		{
			if (!m_isGrounded)
			{
				m_isGrounded = true;

				GAMESIM->playSound(makeCharacterFilename(m_characterIndex, "land_on_ground.ogg"), 50); // players lands on solid ground
				GAMESIM->addAnimationFx("fx/Dust_LandOnGround.scml", m_pos[0], m_pos[1]); // players lands on solid ground
			}
		}
		else
		{
			if (m_isGrounded)
				m_isGrounded = false;
		}

		// ground dash particles

		if (m_isGrounded && m_attack.attacking && m_attack.hasCollision && Calc::Abs(m_vel[0]) > STEERING_SPEED_ON_GROUND * FX_ATTACK_DUST_PLAYER_SPEED_TRESHOLD / 100.f)
		{
			m_groundDashDistance += Calc::Abs(oldPos[0] - m_pos[0]);

			while (m_groundDashDistance >= FX_ATTACK_DUST_INTERVAL)
			{
				m_groundDashDistance -= FX_ATTACK_DUST_INTERVAL;

				GAMESIM->addAnimationFx("fx/Dust_GroundAttack.scml", m_pos[0], m_pos[1]); // attack dust on ground
			}
		}
		else
		{
			m_groundDashDistance = 0.f;
		}

		// breaking

		if (JETPACK_NEW_STEERING && m_jetpack.isActive)
		{
			// fixme : should only decelerate jetpack steering speed, not total velocity?
			m_vel *= powf(1.f - FRICTION_JETPACK, dt * 60.f);
		}
		if (steeringSpeed[0] == 0.f || (std::abs(m_vel[0]) > std::abs(steeringSpeed[0] + m_animVel[0])))
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

		if (m_isAlive && !m_isGrounded && !m_isAttachedToSticky && !m_isWallSliding && !m_isAnimDriven && m_enableInAirAnim)
		{
			if (characterData->getSpriter()->getAnimIndexByName("InAir") != -1)
				setAnim(kPlayerAnim_InAir, true, false);
			else
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

		if (playerControl && steeringSpeed[0] != 0.f)
		{
			const float newFacing = steeringSpeed[0] < 0.f ? -1 : +1;
			if (newFacing != m_facing[0])
			{
				m_facingAnim = 1.f;
				m_facing[0] = newFacing;
			}
		}

		m_facing[1] = m_isAttachedToSticky ? -1 : +1;

		// wrapping

		{
			const Vec2 oldPos = m_pos;

			if (m_pos[0] < 0)
				m_pos[0] = ARENA_SX_PIXELS;
			if (m_pos[0] > ARENA_SX_PIXELS)
				m_pos[0] = 0;

		#if WRAP_AROUND_TOP_AND_BOTTOM
			if (m_pos[1] > ARENA_SY_PIXELS)
				m_pos[1] = 0;

			if (m_pos[1] < 0)
				m_pos[1] = ARENA_SY_PIXELS;
		#endif

			const Vec2 newPos = m_pos;

			if (newPos != oldPos)
			{
				endGrapple();
			}
		}
	}

	//printf("x: %g\n", m_pos[0]);

	m_facingAnim = Calc::Saturate(m_facingAnim - dt / (7.f / 60.f));

	if (m_jetpack.isActive)
	{
		Assert(m_isAlive);

		m_jetpack.fxTime -= dt;
		if (m_jetpack.fxTime <= 0.f)
		{
			m_jetpack.fxTime += JETPACK_FX_INTERVAL;
			GAMESIM->addAnimationFx("fx/Jetpack_Smoke.scml", m_pos[0], m_pos[1], m_facing[0] < 0);
		}
	}

	// death match game mode

	if ((GAMESIM->m_gameMode == kGameMode_DeathMatch) && (GAMESIM->m_gameState == kGameState_Play))
	{
		if (m_isAlive)
		{
			bool isInTheLeaded = true;

			for (int i = 0; i < MAX_PLAYERS; ++i)
				if (i != m_index && GAMESIM->m_players[i].m_isUsed && GAMESIM->m_players[i].m_score >= m_score)
					isInTheLeaded = false;

			if (/*isInTheLeaded || */m_killingSpree >= KILLINGSPREE_START)
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
	if (!m_isActive)
		return;
	if (!m_isAlive && !m_isRespawn)
		return;

	const CharacterData * characterData = getCharacterData(m_index);
	const Color playerColor = getPlayerColor(m_index);

	if (m_grapple.state == GrappleInfo::State_Attached)
	{
		const Vec2 p1 = getGrapplePos();
		const Vec2 p2 = m_grapple.anchorPos;
		setColor(colorWhite);
		drawLine(p1[0], p1[1], p2[0], p2[1]);
		setColor(colorGreen);
		drawRect(p1[0] - 4.f, p1[1] - 4.f, p1[0] + 4.f, p1[1] + 4.f);
		drawRect(p2[0] - 4.f, p2[1] - 4.f, p2[0] + 4.f, p2[1] + 4.f);
	}

#if ENABLE_CHARACTER_OUTLINE
	if (UI_PLAYER_OUTLINE)
	{
		int sx = 256;
		int sy = 256;
		int oy = (sy + characterData->m_collisionSy) / 2;

		static Surface surface1(sx, sy);
		pushSurface(&surface1);
		{
			surface1.clear();
			const bool flipX = m_facing[0] > 0 ? true : false;
			const bool flipY = m_facing[1] < 0 ? true : false;
			drawAt(flipX, flipY, sx/2, oy - (flipY ? characterData->m_collisionSy : 0));
		}
		popSurface();

		static Surface surface2(256, 256);
		pushSurface(&surface2);
		{
			setBlend(BLEND_OPAQUE);
			Shader shader("character-outline");
			setShader(shader);

			shader.setTexture("colormap", 0, surface1.getTexture());
			shader.setImmediate("color", playerColor.r, playerColor.g, playerColor.b, playerColor.a * UI_PLAYER_OUTLINE_ALPHA / 100.f);
			drawRect(0, 0, sx, sy);
			shader.setTexture("colormap", 0, 0);

			clearShader();
			setBlend(BLEND_ALPHA);
		}
		popSurface();

		setColor(colorWhite);
		gxSetTexture(surface2.getTexture());
		gxBegin(GL_QUADS);
		{
			gxTexCoord2f(0.f, 1.f); gxVertex2f(m_pos[0] - sx/2, m_pos[1] - oy);
			gxTexCoord2f(1.f, 1.f); gxVertex2f(m_pos[0] + sx/2, m_pos[1] - oy);
			gxTexCoord2f(1.f, 0.f); gxVertex2f(m_pos[0] + sx/2, m_pos[1] - oy + sy);
			gxTexCoord2f(0.f, 0.f); gxVertex2f(m_pos[0] - sx/2, m_pos[1] - oy + sy);
		}
		gxEnd();
		gxSetTexture(0);
	}
	else
#endif
	{
		const bool flipX = m_facing[0] > 0 ? true : false;
		const bool flipY = m_facing[1] < 0 ? true : false;

		drawAt(flipX, flipY, m_pos[0], m_pos[1] - (flipY ? characterData->m_collisionSy : 0));

		// render additional sprites for wrap around
		drawAt(flipX, flipY, m_pos[0] + ARENA_SX_PIXELS, m_pos[1] - (flipY ? characterData->m_collisionSy : 0));
		drawAt(flipX, flipY, m_pos[0] - ARENA_SX_PIXELS, m_pos[1] - (flipY ? characterData->m_collisionSy : 0));
		drawAt(flipX, flipY, m_pos[0], m_pos[1] - (flipY ? characterData->m_collisionSy : 0) + ARENA_SY_PIXELS);
		drawAt(flipX, flipY, m_pos[0], m_pos[1] - (flipY ? characterData->m_collisionSy : 0) - ARENA_SY_PIXELS);
	}

	// draw invincibility marker

	if (m_spawnInvincibilityTime > 0.f)
	{
		const float t = m_spawnInvincibilityTime / float(PLAYER_RESPAWN_INVINCIBILITY_TIME);
		setColorf(
			playerColor.r,
			playerColor.g,
			playerColor.b,
			t);
		drawRect(
			m_pos[0] + m_collision.min[0], 0,
			m_pos[0] + m_collision.max[0], GFX_SY);
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
	}
}

void Player::drawAt(bool flipX, bool flipY, int x, int y) const
{
	const CharacterData * characterData = getCharacterData(m_characterIndex);
	const Color playerColor = getPlayerColor(m_index);

	if (JETPACK_NEW_STEERING && m_jetpack.isActive)
	{
		// todo : add jetpack bob curve
		y += std::sinf(GAMESIM->m_roundTime * JETPACK_BOB_FREQ * Calc::m2PI) * JETPACK_BOB_AMOUNT;
	}

	// draw backdrop color

	if (UI_PLAYER_BACKDROP_ALPHA != 0)
	{
		Color color = playerColor;
		color.a = UI_PLAYER_BACKDROP_ALPHA / 100.f;
		setColor(color);
		const float px = x + (m_collision.min[0] + m_collision.max[0]) / 2.f;
		const float py = y + (m_collision.min[1] + m_collision.max[1]) / 2.f;
		Sprite("player-light.png").drawEx(px, py, 0.f, 1.5f, 1.5f, false, FILTER_LINEAR);
	}

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

	// draw grapple direction

	if (m_grapple.state == GrappleInfo::State_Aiming)
	{
		m_grapple.aiming.drawBelow(getGrapplePos());
		m_grapple.aiming.drawAbove(getGrapplePos()); // todo : draw above player sprites

		Vec2 anchorPos;
		float length;
		if (findGrappleAnchorPos(anchorPos, length))
		{
			setColor(colorWhite);
			Sprite("coin.png").drawEx(anchorPos[0], anchorPos[1]);
		}
	}
	else if (!GRAPPLE_ANALOG_AIM && GRAPPLE_FIXED_AIM_PREVIEW &&
		characterData->m_special == kPlayerSpecial_Grapple &&
		m_grapple.state == GrappleInfo::State_Inactive
		&& GAMESIM->m_gameState == kGameState_Play)
	{
		Vec2 anchorPos;
		float length;
		if (findGrappleAnchorPos(anchorPos, length))
		{
			setColor(colorWhite);
			Sprite("coin.png").drawEx(anchorPos[0], anchorPos[1]);
		}
	}

	// draw axe throw direction

	if (m_attack.m_axeThrow.isActive)
	{
		m_attack.m_axeThrow.aiming.drawBelow(getAxeThrowPos());
		m_attack.m_axeThrow.aiming.drawAbove(getAxeThrowPos()); // todo : draw above player sprites
	}

	// enable effect when player has the token

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

	// enable ice effect

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
	characterData->getSpriter()->draw(spriterState);

	setColorMode(COLOR_MUL);

	if (m_special.meleeCounter != 0)
	{
		Sprite sprite("doublemelee.png");
		sprite.flipX = m_facing[0] < 0.f;
		setColor(colorWhite);
		sprite.drawEx(x, y + mirrorY(m_attack.collision.min[1]));
	}

	if (m_shieldSpecial.isActive() || m_shieldSpecial.spriterState.animIsActive)
	{
		SpriterState state = m_shieldSpecial.spriterState;
		state.x = x;
		state.y = y + (flipY ? +characterData->m_collisionSy : -characterData->m_collisionSy) / 2.f;
		state.flipX = flipX;
		state.flipY = flipY;
		setColor(colorWhite);
		SHIELDSPECIAL_SPRITER.draw(state);
	}

	if (m_shield.hasShield || m_shield.spriterState.animIsActive)
	{
		SpriterState state = m_shield.spriterState;
		state.x = x;
		state.y = y + (flipY ? +characterData->m_collisionSy : -characterData->m_collisionSy) / 2.f;
		state.flipX = flipX;
		state.flipY = flipY;
		setColor(colorWhite);
		SHIELD_SPRITER.draw(state);
	}

	if (m_bubble.timer > 0.f || m_bubble.spriterState.animIsActive)
	{
		SpriterState state = m_bubble.spriterState;
		state.x = x;
		state.y = y + (flipY ? +characterData->m_collisionSy : -characterData->m_collisionSy) / 2.f;
		state.flipX = flipX;
		state.flipY = flipY;
		setColor(colorWhite);
		BUBBLE_SPRITER.draw(state);
	}

	if (g_devMode && (m_anim == kPlayerAnim_Attack || m_anim == kPlayerAnim_AttackUp || m_anim == kPlayerAnim_AttackDown))
	{
		CollisionShape attackCollision;
		if (getAttackCollision(attackCollision))
		{
			setColor(255, 0, 0);
			attackCollision.debugDraw(false);
		}
	}

	//

	/*
	if (GAMESIM->m_gameMode == kGameMode_TokenHunt && m_tokenHunt.m_hasToken)
	{
		setColorf(1.f, 1.f, 1.f, (std::sin(g_TimerRT.Time_get() * 10.f) * .7f + 1.f) / 2.f);
		drawRect(x - 50, y - 110, x + 50, y - 85);
	}
	*/

	if (!RECORDMODE)
	{
		// draw player emblem
		SpriterState state = m_emblemSpriterState;
		state.x = x;
		state.y = y + UI_PLAYER_EMBLEM_OFFSET_Y;
		state.animIndex = 2;
		setColor(playerColor);
		EMBLEM_SPRITER.draw(state);
		state.animIndex = 0;
		setColor(colorWhite);
		EMBLEM_SPRITER.draw(state);

		// draw score
		setFont("calibri.ttf");
		setColor(colorBlack);
		drawText(x, y + UI_PLAYER_EMBLEM_TEXT_OFFSET_Y, 20, 0.f, +1.f, "%d", m_score);
	}

	// draw player inventory

	for (int i = 0; i < m_weaponStackSize; ++i)
	{
		const int ix = x - 100;
		const int iy = y - 100 + i * 16;
		const PlayerWeapon & weapon = m_weaponStack[i];

		Color color;

		switch (weapon)
		{
		case kPlayerWeapon_Fire:
			color = Color(0.f, 1.f, 0.f);
			break;
		case kPlayerWeapon_Ice:
			color = Color(.5f, .5f, 1.f);
			break;
		case kPlayerWeapon_Bubble:
			color = Color(0.f, 0.f, 1.f);
			break;
		case kPlayerWeapon_Grenade:
			color = Color(1.f, 1.f, 0.f);
			break;
		case kPlayerWeapon_TimeDilation:
			color = Color(1.f, 1.f, 1.f);
			break;
		default:
			Assert(false);
			color = colorWhite;
			break;
		}

		setColor(color);
		drawRect(ix, iy, ix + 12, iy + 12);
	}

#if AUTO_RESPAWN
	if (!m_isAlive && m_respawnTimerRcp != 0.f && (GAMESIM->m_gameState == kGameState_Play) && m_isRespawn)
	{
		int sx = 60;
		int sy = 10;

		setColor(colorBlack);
		drawRect(
			x - sx/2, y - sy/2,
			x + sx/2, y + sy/2);

		setColorf(255, 0, 0);
		drawRect(
			x - sx/2, y - sy/2,
			x - sx/2 + sx * m_respawnTimer * m_respawnTimerRcp, y + sy/2);

		setColor(0, 0, 63);
		drawRectLine(
			x - sx/2, y - sy/2,
			x + sx/2, y + sy/2);
	}
#else
	if (!m_isAlive && m_canRespawn && (GAMESIM->m_gameState == kGameState_Play) && m_isRespawn)
	{
		int sx = 40;
		int sy = 40;

		// we're dead and we're waiting for respawn
		setColor(0, 0, 255, 127);
		drawRect(x - sx / 2, y - sy / 2, x + sx / 2, y + sy / 2);
		setColor(0, 0, 255);
		drawRectLine(x - sx / 2, y - sy / 2, x + sx / 2, y + sy / 2);

		setColor(255, 255, 255);
		setFont("calibri.ttf");
		drawText(x, y, 24, 0, 0, "(X)");
	}
#endif

	// emotes

	if (m_emoteTime > 0.f)
	{
		char filename[64];
		sprintf_s(filename, sizeof(filename), "emotes/%d.scml", m_emoteId + 1);

		Spriter spriter(filename);

		SpriterState state;
		state.startAnim(spriter, 0);
		state.x = m_pos[0];
		state.y = m_pos[1] + EMOTE_DISPLAY_OFFSET_Y;

		setColor(colorWhite);
		spriter.draw(state);
	}
}

void Player::drawLight() const
{
	if (!hasValidCharacterIndex())
		return;
	if (!m_isActive)
		return;
	if (!m_isAlive && !m_isRespawn)
		return;

	const float x = m_pos[0] + (m_collision.min[0] + m_collision.max[0]) / 2.f;
	const float y = m_pos[1] + (m_collision.min[1] + m_collision.max[1]) / 2.f;
	setColor(colorWhite);
	Sprite("player-light.png").drawEx(x, y, 0.f, 3.f, 3.f, false, FILTER_LINEAR);
}

void Player::debugDraw() const
{
	const CharacterData * characterData = getCharacterData(m_characterIndex);

	setColor(0, 31, 63, 63);
	drawRect(
		m_pos[0] + m_collision.min[0],
		m_pos[1] + m_collision.min[1],
		m_pos[0] + m_collision.max[0] + 1,
		m_pos[1] + m_collision.max[1] + 1);

	CollisionShape damageCollision;
	getDamageHitbox(damageCollision);
	setColor(63, 31, 0, 63);
	damageCollision.debugDraw();

	float y = m_pos[1];

	if (m_attack.attacking && m_attack.hasCollision)
	{
		Font font("calibri.ttf");
		setFont(font);

		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "attacking");
		y += 18.f;

		CollisionShape attackCollision;
		if (getAttackCollision(attackCollision))
		{
			setColor(255, 0, 0, 63);
			attackCollision.debugDraw();
		}
	}

	if (m_isGrounded)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "grounded");
		y += 18.f;
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

	if (m_isAttachedToSticky)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "stickied");
		y += 18.f;
	}

	if (characterData->m_special == kPlayerSpecial_AxeThrow && m_axe.hasAxe)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "axe");
		y += 18.f;
	}

	if (m_grapple.state == GrappleInfo::State_Attached)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "%.2f px", getGrappleLength());
		y += 18.f;
	}
	else if (m_grapple.state != GrappleInfo::State_Inactive)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "grappling");
		y += 18.f;
	}

	if (m_pipebomb.state != PipebombInfo::State_Inactive)
	{
		setColor(colorWhite);
		drawText(m_pos[0], y, 14, 0.f, 0.f, "pipebomb");
		y += 18.f;
	}

	setColor(colorWhite);
}

uint32_t Player::getIntersectingBlocksMaskInternal(int x, int y, bool doWrap) const
{
	const int x1 = (x + (int)m_collision.min[0] + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int x2 = (x + (int)m_collision.max[0] + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int x3 = (x + (int)(m_collision.min[0] + m_collision.max[0]) / 2 + ARENA_SX_PIXELS) % ARENA_SX_PIXELS;
	const int y1 = (y + (int)m_collision.min[1] + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y2 = (y + (int)m_collision.max[1] + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;
	const int y3 = (y + (int)(m_collision.min[1] + m_collision.max[1]) / 2 + ARENA_SY_PIXELS) % ARENA_SY_PIXELS;

	uint32_t result = 0;
	
	const Arena & arena = GAMESIM->m_arena;

	result |= arena.getIntersectingBlocksMask(x1, y1);
	result |= arena.getIntersectingBlocksMask(x2, y1);
	result |= arena.getIntersectingBlocksMask(x1, y2);
	result |= arena.getIntersectingBlocksMask(x2, y2);

	result |= arena.getIntersectingBlocksMask(x3, y1);
	result |= arena.getIntersectingBlocksMask(x3, y2);

	result |= arena.getIntersectingBlocksMask(x1, y3);
	result |= arena.getIntersectingBlocksMask(x2, y3);

#if 1 // mover collision
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
	m_isActive = true;

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

void Player::handleLeave()
{
	despawn(false);

	const CharacterData * characterData = getCharacterData(m_characterIndex);
	GameSim & gameSim = *GAMESIM;

	for (int i = 0; i < MAX_BULLETS; ++i)
	{
		if (gameSim.m_bulletPool->m_bullets[i].isAlive && gameSim.m_bulletPool->m_bullets[i].ownerPlayerId == m_index)
			gameSim.m_bulletPool->m_bullets[i].ownerPlayerId = -1;
	}

	for (int i = 0; i < MAX_AXES; ++i)
	{
		if (gameSim.m_axes[i].m_isActive && gameSim.m_axes[i].m_playerIndex == m_index)
			gameSim.m_axes[i].m_playerIndex = -1;
	}
}

bool Player::respawn(Vec2 * pos)
{
	if (!hasValidCharacterIndex())
		return false;

	bool hasSpawnPoint = false;
	int x, y;

	if (pos)
	{
		hasSpawnPoint = true;
		x = (*pos)[0];
		y = (*pos)[1];
	}
	else
	{
		hasSpawnPoint = GAMESIM->m_arena.getRandomSpawnPoint(
			*GAMESIM, x, y,
			m_lastSpawnIndex,
			this);
	}

	Assert(hasSpawnPoint);
	if (hasSpawnPoint)
	{
		m_isActive = true;

		m_spawnInvincibilityTime = PLAYER_RESPAWN_INVINCIBILITY_TIME;

		m_pos[0] = (float)x;
		m_pos[1] = (float)y;

		m_vel[0] = 0.f;
		m_vel[1] = 0.f;

		m_lastTotalVel[0] = 0.f;
		m_lastTotalVel[1] = 0.f;

		m_isAlive = true;

		m_controlDisableTime = 0.f;

		m_emblemSpriterState = SpriterState();
		m_emblemSpriterState.startAnim(EMBLEM_SPRITER, 1);
		m_multiKillTimer = 0.f;
		m_multiKillCount = 0;
		m_killingSpree = 0;

		m_weaponStackSize = 0;

		m_attack = AttackInfo();
		m_timeDilationAttack = TimeDilationAttack();
		m_teleport = TeleportInfo();
		m_jump = JumpInfo();
		m_shield = ShieldInfo();
		m_ice = IceInfo();
		m_bubble = BubbleInfo();
		m_grapple = GrappleInfo();
		m_pipebomb = PipebombInfo();
		m_axe = AxeInfo();
		m_shieldSpecial = ShieldSpecial();

		m_blockMask = 0;

		m_dirBlockMask[0] = 0;
		m_dirBlockMask[1] = 0;
		m_dirBlockMaskDir[0] = 0;
		m_dirBlockMaskDir[1] = 0;
		m_oldBlockMask = 0;

		m_isGrounded = false;
		m_isAttachedToSticky = false;
		m_isAnimDriven = false;
		m_enableInAirAnim = true;
		m_animVelIsAbsolute = false;
		m_isAirDashCharged = false;
		m_isInPassthrough = false;
		m_isHuggingWall = false;
		m_isWallSliding = false;
		m_enterPassthrough = false;

		m_pipebombCooldown = 0.f;

		m_axe.hasAxe = true;
		m_axe.recoveryTime = 0.f;

		//

		setAnim(kPlayerAnim_Spawn, true, true);
		m_isAnimDriven = true;

		if (m_isRespawn)
		{
			m_instanceData->playSoundBag("spawn_sounds", 100);
		}
		else
		{
			m_isRespawn = true;
		}
	}

	return true;
}

void Player::despawn(bool willRespawn)
{
	// reset some stuff now

	m_isActive = willRespawn;

	m_canRespawn = false;
	m_isRespawn = willRespawn;

	m_isAlive = false;

	m_special = SpecialInfo();

	m_attack = AttackInfo();

	m_ice = IceInfo();
	m_bubble = BubbleInfo();

	m_jetpack = JetpackInfo();

	endGrapple();

	// make sure pipe bombs are cleaned up

	for (int i = 0; i < MAX_PIPEBOMBS; ++i)
	{
		if (GAMESIM->m_pipebombs[i].m_isActive && GAMESIM->m_pipebombs[i].m_playerIndex == m_index)
			GAMESIM->m_pipebombs[i] = PipeBomb();
	}
}

bool Player::isSpawned() const
{
	return m_isActive && m_isAlive;
}

void Player::cancelAttack()
{
	if (m_attack.attacking)
	{
		m_attack = AttackInfo();

		setAnim(kPlayerAnim_Walk, false, true);
	}
}

void Player::handleImpact(Vec2Arg velocity)
{
	Vec2 velDelta = velocity - m_vel;

	for (int i = 0; i < 2; ++i)
	{
		if (Calc::Sign(velDelta[i]) == Calc::Sign(velocity[i]))
			m_vel[i] += velDelta[i] / getCharacterData(m_characterIndex)->m_weight;
	}

	endGrapple();
}

bool Player::shieldAbsorb(float amount)
{
	if (m_shieldSpecial.isActive())
	{
		return true;
	}
	if (m_shield.hasShield)
	{
		m_shield.hasShield = false;
		m_shield.spriterState.startAnim(SHIELD_SPRITER, "end");
		GAMESIM->playSound("objects/shield/pop.ogg");
		GAMESIM->addAnimationFx("fx/Shield_Pop.scml", m_pos[0], m_pos[1]); // player shield is popped
		return true;
	}
	else
	{
		return false;
	}
}

bool Player::handleDamage(float amount, Vec2Arg velocity, Player * attacker, bool isNeutralDamage)
{
	if (m_isAlive && isAnimOverrideAllowed(kPlayerAnim_Die))
	{
		handleImpact(velocity);

		if (m_spawnInvincibilityTime > 0.f ||
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
				// play death animation

				setAnim(kPlayerAnim_Die, true, true);
				m_isAnimDriven = true;
				m_enableInAirAnim = false;

				// item drop

				if (m_weaponStackSize > 0)
				{
					const int blockX = m_pos[0] / BLOCK_SX;
					const int blockY = (m_pos[1] - 1.f) / BLOCK_SY - 1;

					if (GAMESIM->m_arena.isValidPickupLocation(blockX, blockY, false))
					{
						Pickup * pickup = GAMESIM->allocPickup();
						if (pickup)
						{
							const int index = GAMESIM->Random() % m_weaponStackSize;
							const PlayerWeapon weapon = m_weaponStack[index];

							PickupType pickupType;

							switch (weapon)
							{
							case kPlayerWeapon_Fire:
								pickupType = kPickupType_Ammo;
								break;
							case kPlayerWeapon_Ice:
								pickupType = kPickupType_Ice;
								break;
							case kPlayerWeapon_Bubble:
								pickupType = kPickupType_Bubble;
								break;
							case kPlayerWeapon_Grenade:
								pickupType = kPickupType_Nade;
								break;
							case kPlayerWeapon_TimeDilation:
								pickupType = kPickupType_TimeDilation;
								break;
							default:
								AssertMsg(false, "missing translation for player weapon %d to pickup type", weapon);
								break;
							}

							GAMESIM->spawnPickup(*pickup, pickupType, blockX, blockY);

							pickup->m_vel = velocity * 2.f / 3.f;
						}
					}
				}

				// fixme.. mid pos
				const CharacterData * characterData = getCharacterData(m_index);
				ParticleSpawnInfo spawnInfo(m_pos[0], m_pos[1] + mirrorY(-characterData->m_collisionSy/2.f), kBulletType_ParticleA, 200, 50, 350, 140);
				spawnInfo.color = 0xff0000ff;

				GAMESIM->spawnParticles(spawnInfo);

				if (PROTO_TIMEDILATION_ON_KILL && attacker)
				{
					GAMESIM->addTimeDilationEffect(
						PROTO_TIMEDILATION_ON_KILL_MULTIPLIER1,
						PROTO_TIMEDILATION_ON_KILL_MULTIPLIER2,
						PROTO_TIMEDILATION_ON_KILL_DURATION);
				}

				if (attacker)
				{
					const Vec2 mid = m_pos + (m_collision.min + m_collision.max) / 2.f;
					const bool vertical = Calc::Abs(velocity[1]) > Calc::Abs(velocity[0]);
					GAMESIM->addBlindsEffect(m_index, mid[0], mid[1], 100, vertical, .5f, attacker->m_displayName.c_str());
				}

				if (m_instanceData->m_input.m_controllerIndex != -1)
				{
					if (m_instanceData->m_input.m_controllerIndex >= 0 && m_instanceData->m_input.m_controllerIndex < MAX_GAMEPAD)
					{
						gamepad[m_instanceData->m_input.m_controllerIndex].vibrate(PLAYER_DEATH_VIBRATION_DURATION, PLAYER_DEATH_VIBRATION_STRENGTH);
					}
				}

				despawn(true);
			}

			if (attacker && attacker != this)
			{
				bool hasScored = false;

				switch (GAMESIM->m_gameMode)
				{
				case kGameMode_DeathMatch:
					attacker->awardScore(1);
					hasScored = true;
					break;

				case kGameMode_TokenHunt:
					// if the attacker has the token, or we're the token bearer
					if (attacker->m_tokenHunt.m_hasToken || m_tokenHunt.m_hasToken)
					{
						attacker->awardScore(1);
						hasScored = true;
					}
					break;
				}

				attacker->handleKill(hasScored);
			}
			else
			{
				switch (GAMESIM->m_gameMode)
				{
				case kGameMode_CoinCollector:
					break;

				default:
					if (!isNeutralDamage)
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
			m_bubble.spriterState.startAnim(BUBBLE_SPRITER, "begin");
		}
	}

	return m_isAlive;
}

void Player::awardScore(int score)
{
	m_score += score;
}

void Player::handleKill(bool hasScored)
{
	if (hasScored)
	{
		// multi kill

		if (m_multiKillTimer > 0.f)
			m_multiKillCount++;
		else
			m_multiKillCount = 1;

		m_multiKillTimer = MULTIKILL_TIMER;

		// killing spree

		m_killingSpree++;

		// sound feedback

		if (m_multiKillCount >= 2)
		{
			logDebug("multikill: %d", m_multiKillCount);

			char name[64];
			sprintf_s(name, sizeof(name), "multikill-%d.ogg", Calc::Min(5, m_multiKillCount));
			GAMESIM->playSound(name);
		}
		else if (m_killingSpree == KILLINGSPREE_START)
			GAMESIM->playSound("killingspree-start.ogg");
		else if (m_killingSpree == KILLINGSPREE_UNSTOPPABLE)
			GAMESIM->playSound("killingspree-unstoppable.ogg");

		// score UI

		{
			char name[64];
			sprintf_s(name, sizeof(name), "ui/killcounter/%d.scml", Calc::Min(5, m_multiKillCount));
			GAMESIM->addAnimationFx(name, m_pos[0] + UI_KILLCOUNTER_OFFSET_X, m_pos[1] + UI_KILLCOUNTER_OFFSET_Y);
		}
	}
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
		for (int i = m_weaponStackSize - 1; i >= 0; --i)
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

		m_vel.SetZero();
	}
}

void Player::beginAxeThrow()
{
	if (m_axe.hasAxe)
	{
		m_attack = AttackInfo();
		m_attack.attacking = true;

		m_attack.m_axeThrow.isActive = true;
		m_attack.m_axeThrow.aiming.begin(AXE_ANALOG_TRESHOLD);
	}
}

void Player::endAxeThrow()
{
	m_attack = AttackInfo();

	// throw axe

	const Vec2 pos = getAxeThrowPos();
	const Vec2 dir = m_input.getAnalogDirection().CalcNormalized();

	GAMESIM->spawnAxe(pos, dir * AXE_THROW_SPEED, m_index);

	m_axe = AxeInfo();
}

void Player::tickAxeThrow(float dt)
{
	Assert(m_attack.m_axeThrow.isActive);

	// update direction vector

	m_attack.m_axeThrow.aiming.tick(*GAMESIM, m_input, dt);

	// throw it?

	if (m_input.wentUp(INPUT_BUTTON_Y))
	{
		endAxeThrow();
	}
}

Vec2 Player::getAxeThrowPos() const
{
	return m_pos + Vec2(0.f, -30.f);
}

void Player::beginShieldSpecial()
{
	Assert(m_shieldSpecial.state == ShieldSpecial::State_Inactive);

	m_shieldSpecial.state = ShieldSpecial::State_Active;
	m_shieldSpecial.spriterState.startAnim(SHIELDSPECIAL_SPRITER, "begin");

	GAMESIM->playSound("objects/shieldspecial/activate.ogg");
}

void Player::endShieldSpecial()
{
	Assert(m_shieldSpecial.state != ShieldSpecial::State_Inactive);

	m_shieldSpecial.state = ShieldSpecial::State_Inactive;
	m_shieldSpecial.cooldown = SHIELDSPECIAL_COOLDOWN;
	m_shieldSpecial.spriterState.startAnim(SHIELDSPECIAL_SPRITER, "end");

	GAMESIM->playSound("objects/shieldspecial/deactivate.ogg");
}

void Player::tickShieldSpecial(float dt)
{
	const CharacterData * characterData = getCharacterData(m_characterIndex);

	if (m_shieldSpecial.spriterState.animIsActive)
		m_shieldSpecial.spriterState.updateAnim(SHIELDSPECIAL_SPRITER, dt);

	switch (m_shieldSpecial.state)
	{
	case ShieldSpecial::State_Inactive:
		if (m_isAlive && characterData->m_special == kPlayerSpecial_Shield)
		{
			m_shieldSpecial.cooldown = Calc::Max(0.f, m_shieldSpecial.cooldown - dt);
			m_shieldSpecial.charge = Calc::Min(SHIELDSPECIAL_CHARGE_MAX, m_shieldSpecial.charge + SHIELDSPECIAL_CHARGE_SPEED * dt);

			if (m_input.wentDown(INPUT_BUTTON_Y) &&
				isAnimOverrideAllowed(kPlayerAnim_Attack) &&
				m_shieldSpecial.cooldown == 0.f)
			{
				beginShieldSpecial();
			}
		}
		break;

	case ShieldSpecial::State_Active:
		m_shieldSpecial.charge = Calc::Max(0.f, m_shieldSpecial.charge - dt);
		if (m_shieldSpecial.charge == 0.f || m_input.wentUp(INPUT_BUTTON_Y))
			endShieldSpecial();
		break;
	}
}

bool Player::shieldSpecialReflect(Vec2Arg pos, Vec2 & dir) const
{
	if (m_shieldSpecial.isActive())
	{
		const Vec2 delta = getPlayerCenter() - pos;

		if (delta * dir < 0.f)
			return false;
		if (delta.CalcSize() < 4.f)
			return false;
		if (delta.CalcSize() > SHIELDSPECIAL_RADIUS)
			return false;

		const Vec2 n = delta.CalcNormalized();
		const float d = n * dir;
		dir = dir - n * d * 2.f;

		GAMESIM->playSound("objects/shieldspecial/reflect.ogg");

		return true;
	}

	return false;
}

void Player::beginGrapple()
{
	Assert(m_grapple.state == GrappleInfo::State_Inactive);

	m_grapple = GrappleInfo();

	m_grapple.state = GrappleInfo::State_Aiming;

	m_grapple.aiming.begin(AXE_ANALOG_TRESHOLD); // fixme : grapple aiming treshold

	// todo : change animation to grappling anim (reuse attack anim for now)
}

bool Player::findGrappleAnchorPos(Vec2 & anchorPos, float & length) const
{
	if (GRAPPLE_ANALOG_AIM && !m_grapple.aiming.aimIsValid)
		return false;

	// find anchor point

	const float kGrappleAngle = Calc::DegToRad(GRAPPLE_FIXED_AIM_ANGLE * m_facing[0]);
	const float dx = GRAPPLE_ANALOG_AIM ? m_grapple.aiming.aim[0] : +std::sin(kGrappleAngle);
	const float dy = GRAPPLE_ANALOG_AIM ? m_grapple.aiming.aim[1] : -std::cos(kGrappleAngle);

	const Arena & arena = GAMESIM->m_arena;

	const Vec2 grapplePos = getGrapplePos();

	const int grappleBlockMask =
		kBlockMask_Solid;

	for (float x = grapplePos[0], y = grapplePos[1]; x >= 0 && x < ARENA_SX_PIXELS && y >= 0 && y < ARENA_SY_PIXELS; x += dx, y += dy)
	{
		if (arena.getIntersectingBlocksMask(x, y) & grappleBlockMask)
		{
			anchorPos.Set(x, y);
			length = (grapplePos - anchorPos).CalcSize();

			if (length >= GRAPPLE_LENGTH_MIN && length <= GRAPPLE_LENGTH_MAX)
			{
				return true;
			}
		}
	}

	return false;
}

void Player::endGrapple()
{
	switch (m_grapple.state)
	{
	case GrappleInfo::State_Inactive:
		break;
	case GrappleInfo::State_Aiming:
		m_grapple.state = GrappleInfo::State_Inactive;
		break;
	case GrappleInfo::State_Attaching:
		m_grapple.state = GrappleInfo::State_Inactive;
		break;
	case GrappleInfo::State_Attached:
		GAMESIM->playSound("grapple-detach.ogg"); // sound played when grapple is detached
		m_grapple.state = GrappleInfo::State_Detaching;
		break;
	case GrappleInfo::State_Detaching:
		break;
	}
}

void Player::tickGrapple(float dt)
{
	switch (m_grapple.state)
	{
	case GrappleInfo::State_Inactive:
		break;

	case GrappleInfo::State_Aiming:
		m_grapple.aiming.tick(*GAMESIM, m_input, dt);

		if (m_input.wentUp(INPUT_BUTTON_Y) || !GRAPPLE_ANALOG_AIM)
		{
			if (findGrappleAnchorPos(m_grapple.anchorPos, m_grapple.distance))
			{
				m_grapple.state = GrappleInfo::State_Attaching;

				GAMESIM->playSound("grapple-attach.ogg"); // sound played when grapple is attached
			}
			else
			{
				m_grapple.state = GrappleInfo::State_Detaching;

				GAMESIM->playSound("grapple-attach-fail.ogg"); // sound played when grapple attach fails
			}
		}
		break;

	case GrappleInfo::State_Attaching:
		m_grapple.state = GrappleInfo::State_Attached;
		break;

	case GrappleInfo::State_Attached:
		{
			// check if player is grounded. if grounded, release grapple

			const bool isAttached = (GAMESIM->m_arena.getIntersectingBlocksMask(m_grapple.anchorPos[0], m_grapple.anchorPos[1]) & kBlockMask_Solid) != 0;

			if (m_input.isDown(INPUT_BUTTON_DOWN))
			{
				//const bool isMaxLength = getGrappleLength() + 4.f >= m_grapple.distance;
				//if (m_vel[1] >= 0.f && isMaxLength && m_vel[1] < GRAPPLE_PULL_DOWN_SPEED)
				//	m_vel[1] = GRAPPLE_PULL_DOWN_SPEED;

				m_grapple.distance = Calc::Min((float)GRAPPLE_LENGTH_MAX, m_grapple.distance + dt * GRAPPLE_PULL_DOWN_SPEED);
			}
			if (m_input.isDown(INPUT_BUTTON_UP))
				m_grapple.distance = Calc::Max((float)GRAPPLE_LENGTH_MIN, m_grapple.distance - dt * GRAPPLE_PULL_UP_SPEED);

			if (m_input.wentDown(INPUT_BUTTON_Y) || !isAttached)
				endGrapple();
		}
		break;

	case GrappleInfo::State_Detaching:
		m_grapple = GrappleInfo();
		m_isAirDashCharged = true;
		break;
	}
}

Vec2 Player::getGrapplePos() const
{
	return m_pos + (m_collision.min + m_collision.max) / 2.f;
}

float Player::getGrappleLength() const
{
	Assert(m_grapple.state == GrappleInfo::State_Attached);
	const Vec2 p1 = getGrapplePos();
	const Vec2 p2 = m_grapple.anchorPos;
	return (p2 - p1).CalcSize();
}

bool Player::findNinjaDashTarget(Vec2 & destination)
{
	bool result = false;

	const Vec2 oldPos = m_pos;

	for (int d = NINJADASH_DISTANCE_MAX; d >= NINJADASH_DISTANCE_MIN; --d)
	{
		// check if this location doesn't intersect with anything that may block

		m_pos[0] = oldPos[0] + d * m_facing[0];

		CollisionInfo playerCollision;
		if (getPlayerCollision(playerCollision))
		{
			bool collision = false;

			GAMESIM->testCollision(playerCollision, &collision, [](const CollisionShape & shape, void * arg, PhysicsActor * actor, BlockAndDistance * blockAndDistance, Player * player)
			{
				if (blockAndDistance && ((1 << blockAndDistance->block->type) & kBlockMask_Solid))
				{
					bool * collision = (bool*)arg;

					*collision = true;
				}
			});

			if (!collision)
			{
				result = true;
				destination = m_pos;
				break;
			}
		}
	}

	m_pos = oldPos;

	return result;
}

void Player::beginPipebomb()
{
	Assert(m_pipebomb.state == PipebombInfo::State_Inactive);

	if (isAnimOverrideAllowed(kPlayerAnim_Pipebomb_Deploy))
	{
		m_pipebomb = PipebombInfo();

		m_pipebomb.state = PipebombInfo::State_Deploy;
		m_pipebomb.time = PIPEBOMB_DEPLOY_TIME;

		setAnim(kPlayerAnim_Pipebomb_Deploy, true, true);
		m_isAnimDriven = true;
		m_animVelIsAbsolute = true;
		m_animAllowSteering = false;
	}
}

void Player::endPipebomb()
{
	// throw a new one
	const Vec2 pos = m_pos + Vec2(0.f, m_collision.min[1] + m_collision.max[1]);
	GAMESIM->spawnPipeBomb(pos, m_vel * PIPEBOMB_PLAYER_SPEED_MULTIPLIER + Vec2(m_facing[0] * PIPEBOMB_THROW_SPEED, 0.f), m_index);

	Assert(m_pipebomb.state != PipebombInfo::State_Inactive);
	m_pipebomb = PipebombInfo();
}

void Player::tickPipebomb(float dt)
{
	switch (m_pipebomb.state)
	{
	case PipebombInfo::State_Inactive:
		break;
	case PipebombInfo::State_Deploy:
		break;

	default:
		Assert(false);
		break;
	}
}

//

Player::AttackInfo::Zweihander::Zweihander()
{
	memset(this, 0, sizeof(Zweihander));
}

bool Player::AttackInfo::Zweihander::isActive() const
{
	return state != kState_Idle;
}

void Player::AttackInfo::Zweihander::begin(Player & player)
{
	Assert(state == kState_Idle);

	if (player.isAnimOverrideAllowed(kPlayerAnim_Zweihander_Charge))
	{
		state = kState_Charge;
		timer = ZWEIHANDER_CHARGE_TIME;
	}
	else
	{
		end(player);
	}
}

void Player::AttackInfo::Zweihander::end(Player & player)
{
	Assert(state != kState_Idle);
	memset(this, 0, sizeof(Zweihander));
	player.m_attack = AttackInfo();
	player.m_isAnimDriven = false;
	player.clearAnimOverrides();
}

void Player::AttackInfo::Zweihander::handleAttackAnimComplete(Player & player)
{
	Assert(state == kState_Attack || state == kState_AttackDown);

	if (player.m_anim == kPlayerAnim_Zweihander_AttackDown)
	{
		player.clearAnimOverrides();
		player.setAnim(kPlayerAnim_Idle, true, true);
	}

	if (player.isAnimOverrideAllowed(kPlayerAnim_Zweihander_Stunned))
	{
		state = kState_Stunned;
		timer = ZWEIHANDER_STUN_TIME;
		player.setAnim(kPlayerAnim_Zweihander_Stunned, true, true);
		player.m_isAnimDriven = true;
		player.m_animVelIsAbsolute = true;
		player.m_animAllowSteering = false;

		player.m_instanceData->m_gameSim->addFloorEffect(
			player.m_index, player.m_pos[0], player.m_pos[1],
			ZWEIHANDER_STOMP_EFFECT_SIZE, (ZWEIHANDER_STOMP_EFFECT_SIZE + 1) * 2 / 3);
	}
	else
	{
		end(player);
	}
}

void Player::AttackInfo::Zweihander::tick(Player & player, float dt)
{
	switch (state)
	{
	case kState_Idle:
		break;
				
	case kState_Charge:
		timer -= dt;
		if (timer < 0.f || player.m_input.wentUp(INPUT_BUTTON_Y))
		{
			if (player.isAnimOverrideAllowed(kPlayerAnim_Zweihander_Attack))
			{
				if (player.m_isGrounded)
				{
					state = kState_Attack;
					timer = 0.f;
					player.setAnim(kPlayerAnim_Zweihander_Attack, true, true);
					player.m_isAnimDriven = true;
					player.m_animVelIsAbsolute = true;
				}
				else
				{
					state = kState_AttackDown;
					timer = 0.f;
					player.setAnim(kPlayerAnim_Zweihander_AttackDown, true, true);
					player.m_isAnimDriven = true;
					player.m_enterPassthrough = true;
				}
			}
			else
			{
				end(player);
			}
		}
		break;

	case kState_Stunned:
		timer -= dt;
		if (timer < 0.f)
		{
			end(player);
		}
		break;
	}
}

//

CharacterData::CharacterData(int characterIndex)
	: m_collisionSx(0)
	, m_collisionSy(0)
	, m_spriter(0)
	, m_spriteScale(1.f)
	, m_weight(1.f)
	, m_meleeCooldown(0.f)
	, m_special(kPlayerSpecial_None)
	, m_traits(0)
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
	m_characterIndex = characterIndex;

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

	m_collisionSx = m_props.getInt("collision_sx", 10);
	m_collisionSy = m_props.getInt("collision_sy", 10);

	m_animData = AnimData();
	m_animData.load(makeCharacterFilename(characterIndex, "animdata.txt"));

	delete m_spriter;
	m_spriter = 0;

	if (!g_devMode)
		getSpriter();

	m_spriteScale = m_props.getFloat("sprite_scale", 1.f);
	m_meleeCooldown = m_props.getFloat("melee_cooldown", PLAYER_SWORD_COOLDOWN);
	m_weight = m_props.getFloat("weight", 1.f);

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
	if (specialStr == "zweihander")
		special = kPlayerSpecial_Zweihander;
	if (specialStr == "pipebomb")
		special = kPlayerSpecial_Pipebomb;
	if (specialStr == "axe")
		special = kPlayerSpecial_AxeThrow;
	if (specialStr == "grapple")
		special = kPlayerSpecial_Grapple;

	m_special = special;

	//

	m_traits = 0;

	const std::string traitsStr = m_props.getString("traits", "");

	if (traitsStr.find(" ") != std::string::npos)
		m_traits |= kPlayerTrait_StickyWalk;
	if (traitsStr.find("double_jump") != std::string::npos)
		m_traits |= kPlayerTrait_DoubleJump;
	if (traitsStr.find("air_dash") != std::string::npos)
		m_traits |= kPlayerTrait_AirDash;
	if (traitsStr.find("ninja_dash") != std::string::npos)
		m_traits |= kPlayerTrait_NinjaDash;
}

bool CharacterData::hasTrait(PlayerTrait trait) const
{
	return (m_traits & trait) != 0;
}

Spriter * CharacterData::getSpriter() const
{
	if (!m_spriter)
	{
		const char * filename = makeCharacterFilename(m_characterIndex, "sprite/sprite.scml");
		m_spriter = new Spriter(filename);
	}

	return m_spriter;
}

//

PlayerInstanceData::PlayerInstanceData(Player * player, GameSim * gameSim)
	: m_player(player)
	, m_gameSim(gameSim)
	, m_textChatTicks(0)
{
	m_player->m_instanceData = this;
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
	const CharacterData * characterData = getCharacterData(m_player->m_index);

	//m_player->m_spriterState.animIndex = // todo : recalc anim index?

	m_player->m_collision.min[0] = -characterData->m_collisionSx / 2.f;
	m_player->m_collision.max[0] = +characterData->m_collisionSx / 2.f;
	m_player->m_collision.min[1] = -characterData->m_collisionSy / 1.f;
	m_player->m_collision.max[1] = 0.f;
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
	if (characterIndex < 4)
		return getPlayerColor(characterIndex);
	else
		return getPlayerColor(characterIndex - 4).interp(colorBlack, .5f);
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

	Assert(playerIndex >= 0 && playerIndex < MAX_PLAYERS);
	if (playerIndex >= 0 && playerIndex < MAX_PLAYERS)
		return colors[playerIndex];
	else
		return colors[MAX_PLAYERS];
}

#undef GAMESIM

//#pragma optimize("", on)
