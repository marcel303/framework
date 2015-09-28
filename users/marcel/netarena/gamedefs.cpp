#include "framework.h"
#include "gamedefs.h"

OPTION_DEFINE(bool, DEMOMODE, "App/Demo Mode");
OPTION_DEFINE(bool, RECORDMODE, "App/Record Mode");
OPTION_ALIAS(DEMOMODE, "demomode");
OPTION_ALIAS(RECORDMODE, "recordmode");

// -- prototypes --
OPTION_DEFINE(bool, PROTO_TIMEDILATION_ON_KILL, "Player/Time Dilation On Kill/Enabled");
OPTION_DEFINE(float, PROTO_TIMEDILATION_ON_KILL_MULTIPLIER1, "Player/Time Dilation On Kill/Multiplier At Start");
OPTION_DEFINE(float, PROTO_TIMEDILATION_ON_KILL_MULTIPLIER2, "Player/Time Dilation On Kill/Multiplier At End");
OPTION_DEFINE(float, PROTO_TIMEDILATION_ON_KILL_DURATION, "Player/Time Dilation On Kill/Duration (Seconds)");
OPTION_STEP(PROTO_TIMEDILATION_ON_KILL_MULTIPLIER1, 0.f, 10.f, .01f);
OPTION_STEP(PROTO_TIMEDILATION_ON_KILL_MULTIPLIER2, 0.f, 10.f, .01f);
OPTION_STEP(PROTO_TIMEDILATION_ON_KILL_DURATION, 0.f, 10.f, .01f);

OPTION_DEFINE(bool, PROTO_ENABLE_LEVEL_EVENTS, "Level Events/Enable Level Events");
OPTION_DEFINE(int, PROTO_LEVEL_EVENT_INTERVAL, "Level Events/Level Event Interval");
OPTION_DEFINE(float, EVENT_GRAVITYWELL_STRENGTH_BEGIN, "Level Events/Gravity Well/Strength (Begin)");
OPTION_DEFINE(float, EVENT_GRAVITYWELL_STRENGTH_END, "Level Events/Gravity Well/Strength (End)");
OPTION_DEFINE(float, EVENT_GRAVITYWELL_DURATION, "Level Events/Gravity Well/Duration");

OPTION_DEFINE(float, EVENT_EARTHQUAKE_DURATION, "Level Events/Earth Quake/Duration");
OPTION_DEFINE(float, EVENT_EARTHQUAKE_INTERVAL, "Level Events/Earth Quake/Interval (Seconds)");
OPTION_DEFINE(float, EVENT_EARTHQUAKE_INTERVAL_RAND, "Level Events/Earth Quake/Interval Variance (Seconds)");
OPTION_DEFINE(float, EVENT_EARTHQUAKE_PLAYER_BOOST, "Level Events/Earth Quake/Player Boost Speed");
OPTION_STEP(EVENT_EARTHQUAKE_INTERVAL, 0, 0, .1f);
OPTION_STEP(EVENT_EARTHQUAKE_INTERVAL_RAND, 0, 0, .1f);
OPTION_STEP(EVENT_EARTHQUAKE_PLAYER_BOOST, 0, 0, 10.f);

OPTION_DEFINE(float, EVENT_TIMEDILATION_DURATION, "Level Events/Time Dilation/Duration (Sec)");
OPTION_DEFINE(float, EVENT_TIMEDILATION_MULTIPLIER_BEGIN, "Level Events/Time Dilation/Multiplier Begin");
OPTION_DEFINE(float, EVENT_TIMEDILATION_MULTIPLIER_END, "Level Events/Time Dilation/Multiplier End");
OPTION_STEP(EVENT_TIMEDILATION_DURATION, 0, 0, .1f);
OPTION_STEP(EVENT_TIMEDILATION_MULTIPLIER_BEGIN, 0, 0, .1f);
OPTION_STEP(EVENT_TIMEDILATION_MULTIPLIER_END, 0, 0, .1f);

OPTION_DEFINE(int, SPIKEWALLS_COVERAGE, "Level Events/Spike Walls/Screen Coverage (Percent)");
OPTION_DEFINE(float, SPIKEWALLS_TIME_PREVIEW, "Level Events/Spike Walls/Warning Time");
OPTION_DEFINE(float, SPIKEWALLS_TIME_CLOSE, "Level Events/Spike Walls/Closing Time");
OPTION_DEFINE(float, SPIKEWALLS_TIME_CLOSED, "Level Events/Spike Walls/Closed Time");
OPTION_DEFINE(float, SPIKEWALLS_TIME_OPEN, "Level Events/Spike Walls/Open Time");
OPTION_DEFINE(bool, SPIKEWALLS_OPEN_ON_LAST_MAN_STANDING, "Level Events/Spike Walls/Open On Last Man Standing");
OPTION_STEP(SPIKEWALLS_TIME_PREVIEW, 0, 0, .1f);
OPTION_STEP(SPIKEWALLS_TIME_CLOSE, 0, 0, .1f);
OPTION_STEP(SPIKEWALLS_TIME_CLOSED, 0, 0, .1f);
OPTION_STEP(SPIKEWALLS_TIME_OPEN, 0, 0, .1f);
// -- prototypes --

OPTION_DEFINE(int, GAMESTATE_LOBBY_STARTZONE_X, "Game State/Lobby/Start Zone X");
OPTION_DEFINE(int, GAMESTATE_LOBBY_STARTZONE_Y, "Game State/Lobby/Start Zone Y");
OPTION_DEFINE(int, GAMESTATE_LOBBY_STARTZONE_SX, "Game State/Lobby/Start Zone SX");
OPTION_DEFINE(int, GAMESTATE_LOBBY_STARTZONE_SY, "Game State/Lobby/Start Zone SY");
OPTION_STEP(GAMESTATE_LOBBY_STARTZONE_X, 0, 0, 10);
OPTION_STEP(GAMESTATE_LOBBY_STARTZONE_Y, 0, 0, 10);
OPTION_STEP(GAMESTATE_LOBBY_STARTZONE_SX, 0, 0, 10);
OPTION_STEP(GAMESTATE_LOBBY_STARTZONE_SY, 0, 0, 10);

OPTION_DEFINE(float, GAMESTATE_ROUNDBEGIN_TRANSITION_TIME, "Game State/Round Begin/Transition Time");
OPTION_DEFINE(float, GAMESTATE_ROUNDBEGIN_SPAWN_DELAY, "Game State/Round Begin/Player Spawn Delay");
OPTION_DEFINE(float, GAMESTATE_ROUNDBEGIN_MESSAGE_DELAY, "Game State/Round Begin/Message Delay");
OPTION_DEFINE(int, GAMESTATE_ROUNDBEGIN_MESSAGE_X, "Game State/Round Begin/Message X");
OPTION_DEFINE(int, GAMESTATE_ROUNDBEGIN_MESSAGE_Y, "Game State/Round Begin/Message Y");

OPTION_DEFINE(float, GAMESTATE_ROUNDCOMPLETE_SHOWWINNER_TIME, "Game State/Round Complete/Time Dilation Time");
OPTION_DEFINE(float, GAMESTATE_ROUNDCOMPLETE_SHOWWINNER_TIMEDILATION_BEGIN, "Game State/Round Complete/Time Dilation Begin");
OPTION_DEFINE(float, GAMESTATE_ROUNDCOMPLETE_SHOWWINNER_TIMEDILATION_END, "Game State/Round Complete/Time Dilation End");
OPTION_DEFINE(float, GAMESTATE_ROUNDCOMPLETE_SHOWRESULTS_TIME, "Game State/Round Complete/Show Results Time");
OPTION_DEFINE(float, GAMESTATE_ROUNDCOMPLETE_TRANSITION_TIME, "Game State/Round Complete/Transition Time");

OPTION_DEFINE(float, GAME_SPEED_MULTIPLIER, "App/Game Speed Multiplier");
OPTION_STEP(GAME_SPEED_MULTIPLIER, 0.f, 10.f, .01f);

OPTION_DEFINE(int, NUM_LOCAL_PLAYERS_TO_ADD, "App/Num Local Players");
OPTION_ALIAS(NUM_LOCAL_PLAYERS_TO_ADD, "numlocal");
OPTION_DEFINE(int, PLAYER_INACTIVITY_TIME, "Player/Inactivity Detection/Time (Sec)");
OPTION_DEFINE(bool, PLAYER_INACTIVITY_KICK, "Player/Inactivity Detection/Kick Player When Inactive");
OPTION_DEFINE(int, MIN_PLAYER_COUNT, "Player/Inactivity Detection/Minimum Player Count");

OPTION_DEFINE(int, MAX_CONSECUTIVE_ROUND_COUNT, "Game State/Play/Max Consecutive Round Count (DEMOMODE)");

OPTION_DEFINE(bool, PLAYER_RESPAWN_AUTOMICALLY, "Player/Respawn Automatically");
OPTION_DEFINE(float, PLAYER_RESPAWN_INVINCIBILITY_TIME, "Player/Respawn Invincibility Time");
OPTION_STEP(PLAYER_RESPAWN_INVINCIBILITY_TIME, 0.f, 10.f, .1f);

OPTION_DEFINE(float, PLAYER_DEATH_VIBRATION_DURATION, "Player/Death/Vibration Duration");
OPTION_DEFINE(float, PLAYER_DEATH_VIBRATION_STRENGTH, "Player/Death/Vibration Strength");
OPTION_DEFINE(float, PLAYER_DEATH_ZOOM_FACTOR, "Player/Death/Zoom Factor");
OPTION_STEP(PLAYER_DEATH_VIBRATION_DURATION, 0.f, 1.f, .1f);
OPTION_STEP(PLAYER_DEATH_VIBRATION_STRENGTH, 0.f, 1.f, .1f);

OPTION_DEFINE(int, PLAYER_DAMAGE_HITBOX_SX, "Player/Hit Boxes/Damage Width");
OPTION_DEFINE(int, PLAYER_DAMAGE_HITBOX_SY, "Player/Hit Boxes/Damage Height");

OPTION_DEFINE(float, PLAYER_SHIELD_IMPACT_MULTIPLIER, "Player/Shield/Impact Multiplier");

OPTION_DEFINE(float, PLAYER_FACING_ANIM_FRAMES, "Player/Animation/Paper Mario Frames");

OPTION_DEFINE(int, PLAYER_SPEED_MAX, "Player/Max Speed");
OPTION_DEFINE(int, PLAYER_JUMP_SPEED, "Player/Jumping/Speed");
OPTION_DEFINE(int, PLAYER_JUMP_SPEED_FRAMES, "Player/Jumping/Soft Jump Frame Count");
OPTION_DEFINE(int, PLAYER_JUMP_GRACE_PIXELS, "Player/Jumping/Grace Pixels");
OPTION_DEFINE(int, PLAYER_DOUBLE_JUMP_SPEED, "Player/Jumping/Double Jump Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_SPEED, "Player/Wall Jump/Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_RECOIL_SPEED, "Player/Wall Jump/Recoil Speed");
OPTION_DEFINE(float, PLAYER_WALLJUMP_RECOIL_TIME, "Player/Wall Jump/Control Time");
OPTION_STEP(PLAYER_SPEED_MAX, 0, 0, 10);
OPTION_STEP(PLAYER_JUMP_SPEED, 0, 0, 10);
OPTION_STEP(PLAYER_WALLJUMP_SPEED, 0, 0, 10);
OPTION_STEP(PLAYER_WALLJUMP_RECOIL_SPEED, 0, 0, 10);
OPTION_STEP(PLAYER_WALLJUMP_RECOIL_TIME, 0, 0, 0.05f);

OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_X1, "Player/Attacks/Sword/Collision X1");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_Y1, "Player/Attacks/Sword/Collision Y1");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_X2, "Player/Attacks/Sword/Collision X2");
OPTION_DEFINE(int, PLAYER_SWORD_COLLISION_Y2, "Player/Attacks/Sword/Collision Y2");
OPTION_DEFINE(float, PLAYER_SWORD_COOLDOWN, "Player/Attacks/Sword/Cooldown Time");
OPTION_DEFINE(int, PLAYER_SWORD_PUSH_SPEED, "Player/Attacks/Sword/Push Speed");
OPTION_DEFINE(int, PLAYER_SWORD_CLING_SPEED, "Player/Attacks/Sword/Cancel Recoil Speed");
OPTION_DEFINE(float, PLAYER_SWORD_CLING_TIME, "Player/Attacks/Sword/Cancel Recoil Control Time");
OPTION_DEFINE(bool, PLAYER_SWORD_SINGLE_BLOCK, "Player/Attacks/Sword/Damage Single Block Only");
OPTION_DEFINE(float, PLAYER_SWORD_BLOCK_DESTROY_SLOWDOWN, "Player/Attacks/Sword/Slowdown On Block Destroy");
OPTION_DEFINE(bool, PLAYER_SWORD_DOWN_PASSTHROUGH, "Player/Attacks/Sword/Passthrough Mode On Down Attack");
OPTION_STEP(PLAYER_SWORD_COOLDOWN, 0, 0, 0.1f);
OPTION_STEP(PLAYER_SWORD_PUSH_SPEED, 0, 0, 50);
OPTION_STEP(PLAYER_SWORD_CLING_SPEED, 0, 0, 10);
OPTION_STEP(PLAYER_SWORD_CLING_TIME, 0, 0, 0.05f);
OPTION_STEP(PLAYER_SWORD_BLOCK_DESTROY_SLOWDOWN, 0, 0, 0.01f);

OPTION_DEFINE(float, MULTIKILL_TIMER, "Player/MultiKill/MultiKill Timer");
OPTION_DEFINE(int, KILLINGSPREE_START, "Player/KillingSpree/Spree Start Kill Count");
OPTION_DEFINE(int, KILLINGSPREE_UNSTOPPABLE, "Player/KillingSpree/Unstoppable Kill Count");

OPTION_DEFINE(float, PLAYER_SCREENSHAKE_STRENGTH_THRESHHOLD, "Player/Effects/Screenshake/Threshhold");

OPTION_DEFINE(float, PLAYER_WEAPON_GUN_COOLDOWN, "Player/Attacks/Gun/Cooldown Time");
OPTION_DEFINE(float, PLAYER_WEAPON_GUN_KNOCKBACK, "Player/Attacks/Gun/Knockback Speed");
OPTION_DEFINE(float, PLAYER_WEAPON_ICE_COOLDOWN, "Player/Attacks/Ice/Cooldown Time");
OPTION_DEFINE(float, PLAYER_WEAPON_ICE_KNOCKBACK, "Player/Attacks/Ice/Knockback Speed");
OPTION_DEFINE(float, PLAYER_WEAPON_BUBBLE_COOLDOWN, "Player/Attacks/Bubble/Knockback Speed");
OPTION_DEFINE(float, PLAYER_WEAPON_GRENADE_COOLDOWN, "Player/Attacks/Grenade/Cooldown Time");
OPTION_DEFINE(float, PLAYER_WEAPON_GRENADE_KNOCKBACK, "Player/Attacks/Grenade/Knockback Speed");
OPTION_STEP(PLAYER_WEAPON_GUN_COOLDOWN, 0, 0, 0.1f);
OPTION_STEP(PLAYER_WEAPON_ICE_COOLDOWN, 0, 0, 0.1f);
OPTION_STEP(PLAYER_WEAPON_BUBBLE_COOLDOWN, 0, 0, 0.1f);
OPTION_STEP(PLAYER_WEAPON_GRENADE_COOLDOWN, 0, 0, 0.1f);

OPTION_DEFINE(float, PLAYER_EFFECT_ICE_TIME, "Player/Effects/Ice/Duration");
OPTION_DEFINE(float, PLAYER_EFFECT_ICE_IMPACT_MULTIPLIER, "Player/Effects/Ice/Impact Multiplier");
OPTION_DEFINE(float, PLAYER_EFFECT_BUBBLE_TIME, "Player/Effects/Bubble/Duration");
OPTION_DEFINE(float, PLAYER_EFFECT_BUBBLE_SPEED, "Player/Effects/Bubble/Speed");
OPTION_DEFINE(float, PLAYER_EFFECT_TIMEDILATION_TIME, "Player/Effects/Time Dilation/Time");
OPTION_DEFINE(float, PLAYER_EFFECT_TIMEDILATION_MULTIPLIER, "Player/Effects/Time Dilation/Speed Multiplier");
OPTION_DEFINE(bool, PLAYER_EFFECT_TIMEDILATION_ON_OTHERS, "Player/Effects/Time Dilation/Apply On Others");
OPTION_STEP(PLAYER_EFFECT_ICE_TIME, 0, 0, 0.1f);
OPTION_STEP(PLAYER_EFFECT_ICE_IMPACT_MULTIPLIER, 0, 0, 0.01f);
OPTION_STEP(PLAYER_EFFECT_BUBBLE_TIME, 0, 0, 0.1f);
OPTION_STEP(PLAYER_EFFECT_BUBBLE_SPEED, 0, 0, 10.f);
OPTION_STEP(PLAYER_EFFECT_TIMEDILATION_MULTIPLIER, 0, 0, .05f);

OPTION_DEFINE(int, STEERING_SPEED_ON_GROUND, "Player/Steering/Speed On Ground");
OPTION_DEFINE(int, STEERING_SPEED_IN_AIR, "Player/Steering/Speed In Air");
OPTION_DEFINE(int, STEERING_SPEED_JETPACK, "Player/Steering/Speed When Using Jetpack");
OPTION_DEFINE(int, STEERING_SPEED_DOUBLEMELEE, "Player/Steering/Speed During Double Melee");
OPTION_DEFINE(int, STEERING_SPEED_ZWEIHANDER, "Player/Steering/Speed During Zweihander Attack");
OPTION_DEFINE(int, STEERING_SPEED_GRAPPLE, "Player/Steering/Speed During Grapple");
OPTION_STEP(STEERING_SPEED_ON_GROUND, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_IN_AIR, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_JETPACK, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_DOUBLEMELEE, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_ZWEIHANDER, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_GRAPPLE, 0, 0, 10);

OPTION_DEFINE(int, PLAYER_WALLSLIDE_SPEED, "Player/Wall Slide/Speed");
OPTION_DEFINE(int, PLAYER_WALLSLIDE_FX_INTERVAL, "Player/Wall Slide/FX Interval (px)");
OPTION_STEP(PLAYER_WALLSLIDE_SPEED, 0, 0, 10);

OPTION_DEFINE(int, PLAYER_SKIN_OVERRIDE, "Player/Skin/Override Skin (Index)");

OPTION_DEFINE(float, BACKGROUND_ZOOM_MULTIPLIER, "Background/Zoom Multiplier");
OPTION_DEFINE(float, BACKGROUND_SCREENSHAKE_MULTIPLIER, "Background/Screen Shake Multiplier");
OPTION_STEP(BACKGROUND_ZOOM_MULTIPLIER, 0, 0, 0.05f);
OPTION_STEP(BACKGROUND_SCREENSHAKE_MULTIPLIER, 0, 0, 0.05f);

OPTION_DEFINE(float, FRICTION_GROUNDED, "Physics/Friction/On Ground");
OPTION_DEFINE(float, FRICTION_GROUNDED_SLIDE, "Physics/Friction/On Slippery Ground");
OPTION_DEFINE(float, FRICTION_JETPACK, "Physics/Friction/When Using (New) Jetpack");
OPTION_STEP(FRICTION_GROUNDED, 0, 0, 0.01f);
OPTION_STEP(FRICTION_GROUNDED_SLIDE, 0, 0, 0.01f);
OPTION_STEP(FRICTION_JETPACK, 0, 0, 0.01f);

OPTION_DEFINE(float, GRAVITY, "Physics/Gravity");
OPTION_STEP(GRAVITY, 0, 10000, 50);

// block types

OPTION_DEFINE(int, BLOCKTYPE_CONVEYOR_SPEED, "Blocks/Conveyor Speed");
OPTION_STEP(BLOCKTYPE_CONVEYOR_SPEED, 0, 10000, 10);

OPTION_DEFINE(float, BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, "Blocks/Gravity Reverse");
OPTION_STEP(BLOCKTYPE_GRAVITY_REVERSE_MULTIPLIER, -10.f, 0.f, 0.05f);

OPTION_DEFINE(float, BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, "Blocks/Gravity Strong");
OPTION_STEP(BLOCKTYPE_GRAVITY_STRONG_MULTIPLIER, 0.f, 10.f, 0.05f);

OPTION_DEFINE(int, BLOCKTYPE_SPRING_SPEED, "Blocks/Spring Speed");
OPTION_STEP(BLOCKTYPE_SPRING_SPEED, 0, 0, 10);

OPTION_DEFINE(float, BLOCKTYPE_REGEN_TIME, "Blocks/Regen Time");
OPTION_STEP(BLOCKTYPE_REGEN_TIME, 0, 0, 0.1f);

// bullets

OPTION_DEFINE(int,  BULLET_TYPE0_MAX_WRAP_COUNT, "Bullets/Type 0/Max Wrap Count");
OPTION_DEFINE(int,  BULLET_TYPE0_MAX_REFLECT_COUNT, "Bullets/Type 0/Max Reflect Count");
OPTION_DEFINE(int,  BULLET_TYPE0_MAX_DISTANCE_TRAVELLED, "Bullets/Type 0/Max Distance Travelled");
OPTION_DEFINE(int,  BULLET_TYPE0_SPEED, "Bullets/Type 0/Speed");
OPTION_STEP(BULLET_TYPE0_MAX_DISTANCE_TRAVELLED, 0.f, 10000.f, 100.f);
OPTION_STEP(BULLET_TYPE0_SPEED, 0.f, 10000.f, 10.f);

OPTION_DEFINE(int, BULLET_GRENADE_NADE_SPEED, "Bullets/Grenade/Nade Speed");
OPTION_DEFINE(int, BULLET_GRENADE_NADE_BOUNCE_COUNT, "Bullets/Grenade/Nade Bounce Count");
OPTION_DEFINE(float, BULLET_GRENADE_NADE_BOUNCE_AMOUNT, "Bullets/Grenade/Nade Bounce Amount");
OPTION_DEFINE(float, BULLET_GRENADE_NADE_LIFE, "Bullets/Grenade/Nade Life");
OPTION_DEFINE(float, BULLET_GRENADE_NADE_LIFE_AFTER_SETTLE, "Bullets/Grenade/Nade Life After Settling");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_COUNT, "Bullets/Grenade/Frag Count");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_RADIUS_MIN, "Bullets/Grenade/Frag Radius Min");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_RADIUS_MAX, "Bullets/Grenade/Frag Radius Max");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_SPEED_MIN, "Bullets/Grenade/Frag Speed Min");
OPTION_DEFINE(int, BULLET_GRENADE_FRAG_SPEED_MAX, "Bullets/Grenade/Frag Speed Max");
OPTION_DEFINE(float, BULLET_GRENADE_BLAST_RADIUS, "Bullets/Grenade/Blast Radius");
OPTION_DEFINE(float, BULLET_GRENADE_BLAST_STRENGTH_NEAR, "Bullets/Grenade/Blast Strength (Near)");
OPTION_DEFINE(float, BULLET_GRENADE_BLAST_STRENGTH_FAR, "Bullets/Grenade/Blast Strength (Far)");
OPTION_STEP(BULLET_GRENADE_NADE_SPEED, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_NADE_BOUNCE_AMOUNT, 0.f, 1.f, 0.01f);
OPTION_STEP(BULLET_GRENADE_NADE_LIFE, 0, 0, 0.1f);
OPTION_STEP(BULLET_GRENADE_NADE_LIFE_AFTER_SETTLE, 0, 0, 0.1f);
OPTION_STEP(BULLET_GRENADE_FRAG_RADIUS_MIN, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_FRAG_RADIUS_MAX, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_FRAG_SPEED_MIN, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_FRAG_SPEED_MAX, 0, 0, 10);

// todo : add bubble size variation
OPTION_DEFINE(int, BULLET_BUBBLE_COUNT, "Bullets/Bubble/Spawn Count");
OPTION_DEFINE(int, BULLET_BUBBLE_SPAWN_DISTANCE, "Bullets/Bubble/Spawn Distance");
OPTION_DEFINE(int, BULLET_BUBBLE_RADIUS_MIN, "Bullets/Bubble/Radius Min");
OPTION_DEFINE(int, BULLET_BUBBLE_RADIUS_MAX, "Bullets/Bubble/Radius Max");
OPTION_DEFINE(int, BULLET_BUBBLE_SPEED_MIN, "Bullets/Bubble/Speed Min");
OPTION_DEFINE(int, BULLET_BUBBLE_SPEED_MAX, "Bullets/Bubble/Speed Max");
OPTION_DEFINE(float, BULLET_BUBBLE_PLAYERSPEED_MULTIPLIER, "Bullets/Bubble/Player Speed Multiplier");
OPTION_STEP(BULLET_BUBBLE_RADIUS_MIN, 0, 0, 5);
OPTION_STEP(BULLET_BUBBLE_RADIUS_MAX, 0, 0, 5);
OPTION_STEP(BULLET_BUBBLE_SPEED_MIN, 0, 0, 5);
OPTION_STEP(BULLET_BUBBLE_SPEED_MAX, 0, 0, 5);
OPTION_STEP(BULLET_BUBBLE_PLAYERSPEED_MULTIPLIER, 0, 0, .02f);

OPTION_DEFINE(int, BULLET_BLOOD_COUNT, "Bullets/Blood/Count");
OPTION_DEFINE(float, BULLET_BLOOD_LIFE, "Bullets/Blood/Life");
OPTION_DEFINE(int, BULLET_BLOOD_SPEED_MIN, "Bullets/Blood/Speed Min");
OPTION_DEFINE(int, BULLET_BLOOD_SPEED_MAX, "Bullets/Blood/Speed Max");
OPTION_STEP(BULLET_BLOOD_LIFE, 0, 0, 0.1f);
OPTION_STEP(BULLET_BLOOD_SPEED_MIN, 0, 0, 5);
OPTION_STEP(BULLET_BLOOD_SPEED_MAX, 0, 0, 5);

OPTION_DEFINE(int, PICKUP_INTERVAL, "Pickups/Spawn Interval (Sec)");
OPTION_DEFINE(int, PICKUP_INTERVAL_VARIANCE, "Pickups/Spawn Interval Random (Sec)");
OPTION_DEFINE(int, MAX_PICKUP_COUNT, "Pickups/Maximum Pickup Count");
OPTION_DEFINE(float, PICKUP_RATE_MULTIPLIER_1, "Pickups/Scaling/1 Players");
OPTION_DEFINE(float, PICKUP_RATE_MULTIPLIER_2, "Pickups/Scaling/2 Players");
OPTION_DEFINE(float, PICKUP_RATE_MULTIPLIER_3, "Pickups/Scaling/3 Players");
OPTION_DEFINE(float, PICKUP_RATE_MULTIPLIER_4, "Pickups/Scaling/4 Players");
OPTION_STEP(PICKUP_RATE_MULTIPLIER_1, 0, 0, 0.1f);
OPTION_STEP(PICKUP_RATE_MULTIPLIER_2, 0, 0, 0.1f);
OPTION_STEP(PICKUP_RATE_MULTIPLIER_3, 0, 0, 0.1f);
OPTION_STEP(PICKUP_RATE_MULTIPLIER_4, 0, 0, 0.1f);

OPTION_DEFINE(int, PICKUP_GUN_COUNT, "Pickups/Gun Count");
OPTION_DEFINE(int, PICKUP_GUN_WEIGHT, "Pickups/Spawn Weights/Gun");
OPTION_DEFINE(int, PICKUP_NADE_WEIGHT, "Pickups/Spawn Weights/Nade");
OPTION_DEFINE(int, PICKUP_SHIELD_WEIGHT, "Pickups/Spawn Weights/Shield");
OPTION_DEFINE(int, PICKUP_ICE_COUNT, "Pickups/Ice Count");
OPTION_DEFINE(int, PICKUP_ICE_WEIGHT, "Pickups/Spawn Weights/Ice");
OPTION_DEFINE(int, PICKUP_BUBBLE_COUNT, "Pickups/Bubble Count");
OPTION_DEFINE(int, PICKUP_BUBBLE_WEIGHT, "Pickups/Spawn Weights/Bubble");
OPTION_DEFINE(int, PICKUP_TIMEDILATION_WEIGHT, "Pickups/Spawn Weights/Time Dilation");

// fireballs

OPTION_DEFINE(float, FIREBALL_SPEED, "Level Events/FireBalls/Fireball Speed");

// zoom effect

OPTION_DEFINE(int, ZOOM_PLAYER, "Zoom Effect/Override Player");
OPTION_DEFINE(float, ZOOM_PLAYER_MAX_DISTANCE, "Zoom Effect/Max Player Distance From Center");
OPTION_DEFINE(float, ZOOM_FACTOR, "Zoom Effect/Override Zoom Factor");
OPTION_DEFINE(float, ZOOM_FACTOR_MIN, "Zoom Effect/Zoom Factor Min");
OPTION_DEFINE(float, ZOOM_FACTOR_MAX, "Zoom Effect/Zoom Factor Max");
OPTION_DEFINE(float, ZOOM_CONVERGE_SPEED, "Zoom Effect/Converge Speed");
OPTION_STEP(ZOOM_FACTOR, 0.f, 10.f, .1f);
OPTION_STEP(ZOOM_FACTOR_MIN, 0.f, 10.f, .1f);
OPTION_STEP(ZOOM_FACTOR_MAX, 0.f, 10.f, .1f);
OPTION_STEP(ZOOM_CONVERGE_SPEED, 0.f, 1.f, .01f);

// death match

OPTION_DEFINE(int, DEATHMATCH_SCORE_LIMIT, "Game Modes/Death Match/Max Player Score");

// token hunt

OPTION_DEFINE(int, TOKENHUNT_SCORE_LIMIT, "Game Modes/Token Hunt/Max Player Score");

OPTION_DEFINE(int, TOKEN_FLEE_SPEED, "Game Objects/Token/Flee Speed");
OPTION_DEFINE(float, TOKEN_DROP_TIME, "Game Objects/Token/No Pickup Time (Sec)");
OPTION_DEFINE(float, TOKEN_DROP_SPEED_MULTIPLIER, "Game Objects/Token/Player Drop Speed Multiplier");
OPTION_DEFINE(float, TOKEN_BOUNCINESS, "Game Objects/Token/Bounciness");
OPTION_DEFINE(int, TOKEN_BOUNCE_SOUND_TRESHOLD, "Game Objects/Token/Bounce Sound Speed Treshold");
OPTION_STEP(TOKEN_FLEE_SPEED, 0, 0, 10);
OPTION_STEP(TOKEN_DROP_TIME, 0, 0, 0.05f);
OPTION_STEP(TOKEN_DROP_SPEED_MULTIPLIER, 0, 0, 0.05f);
OPTION_STEP(TOKEN_BOUNCINESS, 0, 0, 0.05f);
OPTION_STEP(TOKEN_BOUNCE_SOUND_TRESHOLD, 0, 0, 10);

// coin collector

OPTION_DEFINE(int, COINCOLLECTOR_SCORE_LIMIT, "Game Modes/Coin Collector/Max Player Score");
OPTION_DEFINE(int, COINCOLLECTOR_INITIAL_COIN_DROP, "Game Modes/Coin Collector/Initial Coin Drop");
OPTION_DEFINE(bool, COINCOLLECTOR_PLAYER_CAN_BE_KILLED, "Game Modes/Coin Collector/Player Can Be Killed");
OPTION_DEFINE(int, COINCOLLECTOR_COIN_DROP_PERCENTAGE, "Game Modes/Coin Collector/Player Coin Drop Percentage");
OPTION_DEFINE(int, COINCOLLECTOR_COIN_LIMIT, "Game Modes/Coin Collector/Coin Limit");

OPTION_DEFINE(int, COIN_SPAWN_INTERVAL, "Game Objects/Coin/Spawn Interval (Sec)");
OPTION_DEFINE(int, COIN_SPAWN_INTERVAL_VARIANCE, "Game Objects/Coin/Spawn Interval Random (Sec)");
OPTION_DEFINE(int, COIN_FLEE_SPEED, "Game Objects/Coin/Flee Speed");
OPTION_DEFINE(int, COIN_DROP_SPEED, "Game Objects/Coin/Drop Speed");
OPTION_DEFINE(float, COIN_DROP_TIME, "Game Objects/Coin/No Pickup Time (Sec)");
OPTION_DEFINE(float, COIN_DROP_SPEED_MULTIPLIER, "Game Objects/Coin/Player Drop Speed Multiplier");
OPTION_DEFINE(float, COIN_BOUNCINESS, "Game Objects/Coin/Bounciness");
OPTION_DEFINE(int, COIN_BOUNCE_SOUND_TRESHOLD, "Game Objects/Coin/Bounce Sound Speed Treshold");
OPTION_STEP(COIN_DROP_TIME, 0, 0, 0.05f);
OPTION_STEP(COIN_DROP_SPEED_MULTIPLIER, 0, 0, 0.05f);
OPTION_STEP(COIN_BOUNCINESS, 0, 0, 0.05f);
OPTION_STEP(COIN_BOUNCE_SOUND_TRESHOLD, 0, 0, 10);

OPTION_DEFINE(float, TORCH_FLICKER_STRENGTH, "Game Objects/Torch/Flicker Strength");
OPTION_DEFINE(int, TORCH_FLICKER_FREQ_A, "Game Objects/Torch/Flicker Frequency A");
OPTION_DEFINE(int, TORCH_FLICKER_FREQ_B, "Game Objects/Torch/Flicker Frequency B");
OPTION_DEFINE(int, TORCH_FLICKER_FREQ_C, "Game Objects/Torch/Flicker Frequency C");
OPTION_DEFINE(int, TORCH_FLICKER_Y_OFFSET, "Game Objects/Torch/Flicker Y Offset");
OPTION_STEP(TORCH_FLICKER_STRENGTH, 0, 0, .02f);

OPTION_DEFINE(int, LIGHTING_DEBUG_MODE, "Graphics/Lighting/Debug Mode");

OPTION_DEFINE(bool, JETPACK_NEW_STEERING, "Special Abilities/Jetpack/New Steering Behavior");
OPTION_DEFINE(float, JETPACK_NEW_STEERING_CURVE_MIN, "Special Abilities/Jetpack/New Steering Speed Min");
OPTION_DEFINE(float, JETPACK_NEW_STEERING_CURVE_MAX, "Special Abilities/Jetpack/New Steering Speed Max");
OPTION_DEFINE(float, JETPACK_ACCEL, "Special Abilities/Jetpack/Acceleration");
OPTION_DEFINE(float, JETPACK_FX_INTERVAL, "Special Abilities/Jetpack/Smoke Fx Interval (Sec)");
OPTION_DEFINE(float, JETPACK_BOB_AMOUNT, "Special Abilities/Jetpack/Bob Amount (px)");
OPTION_DEFINE(float, JETPACK_BOB_FREQ, "Special Abilities/Jetpack/Bob Frequency (Hz)");
OPTION_DEFINE(bool, JETPACK_DASH_ON_JUMP, "Special Abilities/Jetpack/Dash On X Button Press");
OPTION_DEFINE(bool, JETPACK_DASH_ON_DIRECTION_CHANGE, "Special Abilities/Jetpack/Dash On Direction Change");
OPTION_DEFINE(float, JETPACK_DASH_SPEED_MULTIPLIER, "Special Abilities/Jetpack/Dash Speed Multiplier");
OPTION_DEFINE(float, JETPACK_DASH_DURATION, "Special Abilities/Jetpack/Dash Duration");
OPTION_DEFINE(float, JETPACK_DASH_RELOAD_TRESHOLD, "Special Abilities/Jetpack/Dash Reload Input Treshold");
OPTION_STEP(JETPACK_NEW_STEERING_CURVE_MIN, 0, 0, 10.f);
OPTION_STEP(JETPACK_NEW_STEERING_CURVE_MAX, 0, 0, 10.f);
OPTION_STEP(JETPACK_DASH_SPEED_MULTIPLIER, 0, 0, .1f);
OPTION_STEP(JETPACK_DASH_DURATION, 0, 0, .1f);
OPTION_STEP(JETPACK_DASH_RELOAD_TRESHOLD, 0, 0, .1f);

OPTION_DEFINE(int, DOUBLEMELEE_ATTACK_RADIUS, "Special Abilities/Double Melee/Attack Radius");
OPTION_DEFINE(int, DOUBLEMELEE_SPIN_COUNT, "Special Abilities/Double Melee/Spin Count");
OPTION_DEFINE(float, DOUBLEMELEE_SPIN_TIME, "Special Abilities/Double Melee/Spin Time");
OPTION_DEFINE(float, DOUBLEMELEE_GRAVITY_MULTIPLIER, "Special Abilities/Double Melee/Gravity Multiplier");
OPTION_STEP(DOUBLEMELEE_ATTACK_RADIUS, 0, 0, 10);
OPTION_STEP(DOUBLEMELEE_SPIN_TIME, 0, 0, .01f);
OPTION_STEP(DOUBLEMELEE_GRAVITY_MULTIPLIER, 0, 0, .02f);

OPTION_DEFINE(int, STOMP_EFFECT_MAX_SIZE, "Special Abilities/Stomp Attack/Max Effect Size");
OPTION_DEFINE(int, STOMP_EFFECT_MIN_HEIGHT, "Special Abilities/Stomp Attack/Effect Min Height");
OPTION_DEFINE(int, STOMP_EFFECT_MAX_HEIGHT, "Special Abilities/Stomp Attack/Effect Max Height");
OPTION_DEFINE(float, STOMP_EFFECT_PROPAGATION_INTERVAL, "Special Abilities/Stomp Attack/Propagation Interval (Sec)");
OPTION_DEFINE(float, STOMP_EFFECT_DAMAGE_DURATION, "Special Abilities/Stomp Attack/Damage Duration (Sec)");
OPTION_DEFINE(int, STOMP_EFFECT_DAMAGE_HITBOX_SX, "Special Abilities/Stomp Attack/Damage Hitbox Width");
OPTION_DEFINE(int, STOMP_EFFECT_DAMAGE_HITBOX_SY, "Special Abilities/Stomp Attack/Damage Hitbox Height");
OPTION_STEP(STOMP_EFFECT_PROPAGATION_INTERVAL, 0, 0, .01f);
OPTION_STEP(STOMP_EFFECT_DAMAGE_DURATION, 0, 0, .01f);

OPTION_DEFINE(float, ROCKETPUNCH_CHARGE_MIN, "Special Abilities/Rocket Punch/Charge Min (Sec)");
OPTION_DEFINE(float, ROCKETPUNCH_CHARGE_MAX, "Special Abilities/Rocket Punch/Charge Max (Sec)");
OPTION_DEFINE(bool, ROCKETPUNCH_CHARGE_MUST_BE_MAXED, "Special Abilities/Rocket Punch/Charge Must Be Maxed");
OPTION_DEFINE(bool, ROCKETPUNCH_AUTO_DISCHARGE, "Special Abilities/Rocket Punch/Auto Discharge");
OPTION_DEFINE(int, ROCKETPUNCH_SPEED_MIN, "Special Abilities/Rocket Punch/Speed Min");
OPTION_DEFINE(int, ROCKETPUNCH_SPEED_MAX, "Special Abilities/Rocket Punch/Speed Max");
OPTION_DEFINE(bool, ROCKETPUNCH_SPEED_BASED_ON_CHARGE, "Special Abilities/Rocket Punch/Speed Based On Charge");
OPTION_DEFINE(int, ROCKETPUNCH_DISTANCE_MIN, "Special Abilities/Rocket Punch/Distance Min");
OPTION_DEFINE(int, ROCKETPUNCH_DISTANCE_MAX, "Special Abilities/Rocket Punch/Distance Max");
OPTION_DEFINE(bool, ROCKETPUNCH_DISTANCE_BASED_ON_CHARGE, "Special Abilities/Rocket Punch/Distance Based On Charge");
OPTION_DEFINE(bool, ROCKETPUNCH_MUST_GROUND, "Special Abilities/Rocket Punch/Must Hit Ground First");
OPTION_DEFINE(bool, ROCKETPUNCH_ONLY_IN_AIR, "Special Abilities/Rocket Punch/Only In Air");
OPTION_DEFINE(float, ROCKETPUNCH_STUN_TIME, "Special Abilities/Rocket Punch/Stun Time (Sec)");
OPTION_STEP(ROCKETPUNCH_CHARGE_MIN, 0, 0, .1f);
OPTION_STEP(ROCKETPUNCH_CHARGE_MAX, 0, 0, .1f);
OPTION_STEP(ROCKETPUNCH_SPEED_MIN, 0, 0, 100);
OPTION_STEP(ROCKETPUNCH_SPEED_MAX, 0, 0, 100);
OPTION_STEP(ROCKETPUNCH_DISTANCE_MIN, 0, 0, 100);
OPTION_STEP(ROCKETPUNCH_DISTANCE_MAX, 0, 0, 100);
OPTION_STEP(ROCKETPUNCH_STUN_TIME, 0, 0, .1f);

OPTION_DEFINE(float, ZWEIHANDER_CHARGE_TIME, "Special Abilities/Zweihander/Charge Time (Sec)");
OPTION_DEFINE(float, ZWEIHANDER_STUN_TIME, "Special Abilities/Zweihander/Stun Time (Sec)");
OPTION_DEFINE(int, ZWEIHANDER_STOMP_EFFECT_SIZE, "Special Abilities/Zweihander/Stomp Effect Size");

OPTION_DEFINE(int, PIPEBOMB_COLLISION_SX, "Special Abilities/Pipe Bomb/Collision SX");
OPTION_DEFINE(int, PIPEBOMB_COLLISION_SY, "Special Abilities/Pipe Bomb/Collision SY");
OPTION_DEFINE(float, PIPEBOMB_PLAYER_SPEED_MULTIPLIER, "Special Abilities/Pipe Bomb/Player Speed Multiplier");
OPTION_DEFINE(float, PIPEBOMB_THROW_SPEED, "Special Abilities/Pipe Bomb/Throw Speed");
OPTION_DEFINE(float, PIPEBOMB_ACTIVATION_TIME, "Special Abilities/Pipe Bomb/Activation Time (Sec)");
OPTION_DEFINE(float, PIPEBOMB_DEPLOY_TIME, "Special Abilities/Pipe Bomb/Deploy Time (Sec)");
OPTION_DEFINE(float, PIPEBOMB_COOLDOWN, "Special Abilities/Pipe Bomb/Cooldown Time (Sec)");
OPTION_DEFINE(float, PIPEBOMB_BLAST_RADIUS, "Special Abilities/Pipe Bomb/Blast Radius");
OPTION_DEFINE(float, PIPEBOMB_BLAST_STRENGTH_NEAR, "Special Abilities/Pipe Bomb/Blast Strength (Near)");
OPTION_DEFINE(float, PIPEBOMB_BLAST_STRENGTH_FAR, "Special Abilities/Pipe Bomb/Blast Strength (Far)");
OPTION_STEP(PIPEBOMB_THROW_SPEED, 0, 0, 10);
OPTION_STEP(PIPEBOMB_DEPLOY_TIME, 0, 0, .1f);
OPTION_STEP(PIPEBOMB_ACTIVATION_TIME, 0, 0, .1f);
OPTION_STEP(PIPEBOMB_COOLDOWN, 0, 0, .1f);

OPTION_DEFINE(int, AXE_COLLISION_SX, "Special Abilities/Axe/Collision SX");
OPTION_DEFINE(int, AXE_COLLISION_SY, "Special Abilities/Axe/Collision SY");
OPTION_DEFINE(float, AXE_ANALOG_TRESHOLD, "Special Abilities/Axe/Analog Treshold");
OPTION_DEFINE(float, AXE_GRAVITY_MULTIPLIER_ACTIVE, "Special Abilities/Axe/Gravity Multiplier When Active");
OPTION_DEFINE(float, AXE_GRAVITY_MULTIPLIER_INACTIVE, "Special Abilities/Axe/Gravity Multiplier When Dead");
OPTION_DEFINE(float, AXE_THROW_TIME, "Special Abilities/Axe/Throw Time (Sec)");
OPTION_DEFINE(float, AXE_THROW_SPEED, "Special Abilities/Axe/Throw Speed");
OPTION_DEFINE(float, AXE_ROTATION_SPEED, "Special Abilities/Axe/Rotation Speed (Degrees Per Sec)");
OPTION_DEFINE(float, AXE_SPEED_MULTIPLIER_ON_DIE, "Special Abilities/Axe/Speed Multiplier On Die");
OPTION_DEFINE(float, AXE_FADE_TIME, "Special Abilities/Axe/Fade Time (Sec)");
OPTION_STEP(AXE_ANALOG_TRESHOLD, 0, 0, .02f);
OPTION_STEP(AXE_GRAVITY_MULTIPLIER_ACTIVE, 0, 0, .05f);
OPTION_STEP(AXE_GRAVITY_MULTIPLIER_INACTIVE, 0, 0, .05f);
OPTION_STEP(AXE_THROW_TIME, 0, 0, .1f);
OPTION_STEP(AXE_THROW_SPEED, 0, 0, 10);
OPTION_STEP(AXE_SPEED_MULTIPLIER_ON_DIE, 0, 0, .05f);

OPTION_DEFINE(bool, GRAPPLE_ANALOG_AIM, "Special Abilities/Grapple Rope/Analog Aim");
OPTION_DEFINE(float, GRAPPLE_FIXED_AIM_ANGLE, "Special Abilities/Grapple Rope/Fixed Aim Angle");
OPTION_DEFINE(bool, GRAPPLE_FIXED_AIM_PREVIEW, "Special Abilities/Grapple Rope/Fixed Aim Preview");
OPTION_DEFINE(float, GRAPPLE_LENGTH_MIN, "Special Abilities/Grapple Rope/Min Length");
OPTION_DEFINE(float, GRAPPLE_LENGTH_MAX, "Special Abilities/Grapple Rope/Max Length");
OPTION_DEFINE(float, GRAPPLE_PULL_UP_SPEED, "Special Abilities/Grapple Rope/Pull Up Speed");
OPTION_DEFINE(float, GRAPPLE_PULL_DOWN_SPEED, "Special Abilities/Grapple Rope/Pull Down Speed");

OPTION_DEFINE(float, SHIELDSPECIAL_CHARGE_MAX, "Special Abilities/Shield/Charge Max (Sec)");
OPTION_DEFINE(float, SHIELDSPECIAL_COOLDOWN, "Special Abilities/Shield/Cooldown (Sec)");
OPTION_DEFINE(float, SHIELDSPECIAL_RADIUS, "Special Abilities/Shield/Radius");
OPTION_DEFINE(float, SHIELDSPECIAL_CHARGE_SPEED, "Special Abilities/Shield/Charge Speed (Charge/Sec)");

OPTION_DEFINE(int, NINJADASH_DISTANCE_MAX, "Special Abilities/Ninja Dash/Max Distance");
OPTION_DEFINE(int, NINJADASH_DISTANCE_MIN, "Special Abilities/Ninja Dash/Min Distance");
OPTION_STEP(NINJADASH_DISTANCE_MAX, 0, 0, 10);
OPTION_STEP(NINJADASH_DISTANCE_MIN, 0, 0, 10);

OPTION_DEFINE(int, INVISIBILITY_PLUME_COUNT, "Special Abilities/Invisibility/Plume Count");
OPTION_DEFINE(float, INVISIBILITY_PLUME_DISTANCE_MIN, "Special Abilities/Invisibility/Plume Distance Min");
OPTION_DEFINE(float, INVISIBILITY_PLUME_DISTANCE_MAX, "Special Abilities/Invisibility/Plume Distance Max");

OPTION_DEFINE(int, DECAL_COUNT, "Graphics/Decals/Num Decals");
OPTION_DEFINE(float, DECAL_SIZE_MIN, "Graphics/Decals/Random Size Min");
OPTION_DEFINE(float, DECAL_SIZE_MAX, "Graphics/Decals/Random Size Max");
OPTION_DEFINE(bool, DECAL_ENABLED, "Graphics/Decals/Enabled");
OPTION_DEFINE(int, DECAL_COLOR, "Graphics/Decals/Color Override");
OPTION_STEP(DECAL_SIZE_MIN, 0, 0, .05f);
OPTION_STEP(DECAL_SIZE_MAX, 0, 0, .05f);

OPTION_DEFINE(float, FX_ATTACK_DUST_INTERVAL, "FX/Attack Dust/Interval (px)");
OPTION_DEFINE(int, FX_ATTACK_DUST_PLAYER_SPEED_TRESHOLD, "FX/Attack Dust/Player Speed Treshold (%%)");

OPTION_DEFINE(int, MAINMENU_BUTTON_TEXT_X, "UI/Menu Button Text X");
OPTION_DEFINE(int, MAINMENU_BUTTON_TEXT_Y, "UI/Menu Button Text Y");
OPTION_DEFINE(int, MAINMENU_BUTTON_TEXT_SIZE, "UI/Menu Button Text Size");

OPTION_DEFINE(int, CUSTOMIZEMENU_PORTRAIT_BASE_X, "UI/Customize Screen/Portrait Base X");
OPTION_DEFINE(int, CUSTOMIZEMENU_PORTRAIT_BASE_Y, "UI/Customize Screen/Portrait Base Y");
OPTION_DEFINE(int, CUSTOMIZEMENU_PORTRAIT_SPACING_X, "UI/Customize Screen/Portrait Spacing X");
OPTION_DEFINE(int, CUSTOMIZEMENU_PORTRAIT_SPACING_Y, "UI/Customize Screen/Portrait Spacing Y");

OPTION_DEFINE(bool, UI_DEBUG_VISIBLE, "UI/Debug UI/Visible");

OPTION_DEFINE(bool, UI_LOBBY_LEAVEJOIN_ENABLE, "UI/Lobby/LeaveJoin Enable");
OPTION_DEFINE(bool, UI_LOBBY_GAMEMODE_SELECT_ENABLE, "UI/Lobby/Game Mode Select Enable");

OPTION_DEFINE(int, UI_BUTTONLEGEND_X, "UI/Button Legend/X");
OPTION_DEFINE(int, UI_BUTTONLEGEND_Y, "UI/Button Legend/Y");
OPTION_DEFINE(int, UI_BUTTONLEGEND_FONT_SIZE, "UI/Button Legend/Font Size");
OPTION_DEFINE(int, UI_BUTTONLEGEND_ICON_SPACING, "UI/Button Legend/Icon Spacing");
OPTION_DEFINE(int, UI_BUTTONLEGEND_TEXT_SPACING, "UI/Button Legend/Text Spacing");

OPTION_DEFINE(int, UI_TEXTCHAT_MAX_TEXT_SIZE, "UI/Text Chat/Max Text Size");
OPTION_DEFINE(int, UI_TEXTCHAT_X, "UI/Text Chat/X");
OPTION_DEFINE(int, UI_TEXTCHAT_Y, "UI/Text Chat/Y");
OPTION_DEFINE(int, UI_TEXTCHAT_SX, "UI/Text Chat/Size X");
OPTION_DEFINE(int, UI_TEXTCHAT_SY, "UI/Text Chat/Size Y");
OPTION_DEFINE(int, UI_TEXTCHAT_PADDING_X, "UI/Text Chat/Text Padding X");
OPTION_DEFINE(int, UI_TEXTCHAT_PADDING_Y, "UI/Text Chat/Text Padding Y");
OPTION_DEFINE(int, UI_TEXTCHAT_FONT_SIZE, "UI/Text Chat/Text Size");
OPTION_DEFINE(int, UI_TEXTCHAT_CARET_SX, "UI/Text Chat/Caret Width");
OPTION_DEFINE(int, UI_TEXTCHAT_CARET_PADDING_X, "UI/Text Chat/Caret Padding (Left Side)");
OPTION_DEFINE(int, UI_TEXTCHAT_CARET_PADDING_Y, "UI/Text Chat/Caret Padding (Top, Bottom)");

OPTION_DEFINE(float, UI_QUICKLOOK_OPEN_TIME, "UI/Quick Look/Open Time");
OPTION_DEFINE(float, UI_QUICKLOOK_CLOSE_TIME, "UI/Quick Look/Close Time");

OPTION_DEFINE(int, INGAME_TEXTCHAT_FONT_SIZE, "UI/In-Game Text Chat/Text Size");
OPTION_DEFINE(int, INGAME_TEXTCHAT_PADDING_X, "UI/In-Game Text Chat/Text Padding X");
OPTION_DEFINE(int, INGAME_TEXTCHAT_PADDING_Y, "UI/In-Game Text Chat/Text Padding Y");
OPTION_DEFINE(int, INGAME_TEXTCHAT_SIZE_Y, "UI/In-Game Text Chat/Box Height");
OPTION_DEFINE(int, INGAME_TEXTCHAT_OFFSET_Y, "UI/In-Game Text Chat/Box Offset Y");
OPTION_DEFINE(float, INGAME_TEXTCHAT_DURATION, "UI/In-Game Text Chat/Time Visible");
OPTION_STEP(INGAME_TEXTCHAT_DURATION, 0.f, 0.f, .1f);

OPTION_DEFINE(bool, UI_PLAYER_OUTLINE, "UI/Player/Outline Enabled");
OPTION_DEFINE(int, UI_PLAYER_OUTLINE_ALPHA, "UI/Player/Outline Alpha (%%)");
OPTION_DEFINE(int, UI_PLAYER_BACKDROP_ALPHA, "UI/Player/Background Alpha (%%)");
OPTION_DEFINE(int, UI_PLAYER_EMBLEM_OFFSET_Y, "UI/Player/Emblem Y Offset");
OPTION_DEFINE(int, UI_PLAYER_EMBLEM_TEXT_OFFSET_Y, "UI/Player/Emblem Text Y Offset");

OPTION_DEFINE(int, UI_KILLCOUNTER_OFFSET_X, "UI/Player/Kill Counter X Offset");
OPTION_DEFINE(int, UI_KILLCOUNTER_OFFSET_Y, "UI/Player/Kill Counter Y Offset");

OPTION_DEFINE(int, EMOTE_DISPLAY_OFFSET_Y, "UI/Emotes/Display Y Offset");
OPTION_DEFINE(float, EMOTE_DISPLAY_TIME, "UI/Emotes/Display Time");
OPTION_STEP(EMOTE_DISPLAY_TIME, 0.f, 0.f, .1f);

OPTION_DEFINE(bool, VOLCANO_LOOP, "Level Events/Volcano/Loop");
OPTION_DEFINE(int, VOLCANO_LOOP_TIME, "Level Events/Volcano/LoopTime");

void setMainFont()
{
	setFont("font-main.ttf");
}

void setDebugFont()
{
	setFont("calibri.ttf");
}
