#include "gamedefs.h"

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

OPTION_DEFINE(int, GAMESTATE_COMPLETE_TIMER, "Menus/Results Screen Time");
OPTION_DEFINE(int, GAMESTATE_COMPLETE_TIME_DILATION_TIMER, "Menus/Results Screen Time Dilation Time");
OPTION_DEFINE(float, GAMESTATE_COMPLETE_TIME_DILATION_BEGIN, "Menus/Results Screen Time Dilation Begin");
OPTION_DEFINE(float, GAMESTATE_COMPLETE_TIME_DILATION_END, "Menus/Results Screen Time Dilation End");

OPTION_DEFINE(float, GAME_SPEED_MULTIPLIER, "App/Game Speed Multiplier");
OPTION_STEP(GAME_SPEED_MULTIPLIER, 0.f, 10.f, .01f);

OPTION_DEFINE(float, PLAYER_RESPAWN_INVINCIBILITY_TIME, "Player/Respawn Invincibility Time");
OPTION_STEP(PLAYER_RESPAWN_INVINCIBILITY_TIME, 0.f, 10.f, .1f);

OPTION_DEFINE(int, PLAYER_DAMAGE_HITBOX_SX, "Player/Hit Boxes/Damage Width");
OPTION_DEFINE(int, PLAYER_DAMAGE_HITBOX_SY, "Player/Hit Boxes/Damage Height");

OPTION_DEFINE(float, PLAYER_SHIELD_IMPACT_MULTIPLIER, "Player/Shield/Impact Multiplier");

OPTION_DEFINE(int, PLAYER_SPEED_MAX, "Player/Max Speed");
OPTION_DEFINE(int, PLAYER_JUMP_SPEED, "Player/Jumping/Speed");
OPTION_DEFINE(int, PLAYER_JUMP_SPEED_FRAMES, "Player/Jumping/Soft Jump Frame Count");
OPTION_DEFINE(int, PLAYER_JUMP_GRACE_PIXELS, "Player/Jumping/Grace Pixels");
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
OPTION_STEP(PLAYER_SWORD_COOLDOWN, 0, 0, 0.1f);
OPTION_STEP(PLAYER_SWORD_PUSH_SPEED, 0, 0, 50);
OPTION_STEP(PLAYER_SWORD_CLING_SPEED, 0, 0, 10);
OPTION_STEP(PLAYER_SWORD_CLING_TIME, 0, 0, 0.05f);

OPTION_DEFINE(float, PLAYER_SCREENSHAKE_STRENGTH_THRESHHOLD, "Player/Effects/Screenshake/Threshhold");

OPTION_DEFINE(float, PLAYER_FIRE_COOLDOWN, "Player/Attacks/Fire/Cooldown Time");
OPTION_STEP(PLAYER_FIRE_COOLDOWN, 0, 0, 0.1f);

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
OPTION_STEP(STEERING_SPEED_ON_GROUND, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_IN_AIR, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_JETPACK, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_DOUBLEMELEE, 0, 0, 10);
OPTION_STEP(STEERING_SPEED_ZWEIHANDER, 0, 0, 10);

OPTION_DEFINE(int, PLAYER_WALLSLIDE_SPEED, "Player/Wall Slide/Speed");
OPTION_STEP(PLAYER_WALLSLIDE_SPEED, 0, 0, 10);

OPTION_DEFINE(float, FRICTION_GROUNDED, "Physics/Friction/On Ground");
OPTION_STEP(FRICTION_GROUNDED, 0, 0, 0.01f);

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
OPTION_STEP(BULLET_GRENADE_NADE_SPEED, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_NADE_BOUNCE_AMOUNT, 0.f, 1.f, 0.01f);
OPTION_STEP(BULLET_GRENADE_NADE_LIFE, 0, 0, 0.1f);
OPTION_STEP(BULLET_GRENADE_NADE_LIFE_AFTER_SETTLE, 0, 0, 0.1f);
OPTION_STEP(BULLET_GRENADE_FRAG_RADIUS_MIN, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_FRAG_RADIUS_MAX, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_FRAG_SPEED_MIN, 0, 0, 10);
OPTION_STEP(BULLET_GRENADE_FRAG_SPEED_MAX, 0, 0, 10);

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

OPTION_DEFINE(int, PICKUP_AMMO_COUNT, "Pickups/Ammo Count");
OPTION_DEFINE(int, PICKUP_AMMO_WEIGHT, "Pickups/Spawn Weights/Ammo");
OPTION_DEFINE(int, PICKUP_NADE_WEIGHT, "Pickups/Spawn Weights/Nade");
OPTION_DEFINE(int, PICKUP_SHIELD_COUNT, "Pickups/Shield Count");
OPTION_DEFINE(int, PICKUP_SHIELD_WEIGHT, "Pickups/Spawn Weights/Shield");
OPTION_DEFINE(int, PICKUP_ICE_COUNT, "Pickups/Ice Count");
OPTION_DEFINE(int, PICKUP_ICE_WEIGHT, "Pickups/Spawn Weights/Ice");
OPTION_DEFINE(int, PICKUP_BUBBLE_COUNT, "Pickups/Bubble Count");
OPTION_DEFINE(int, PICKUP_BUBBLE_WEIGHT, "Pickups/Spawn Weights/Bubble");
OPTION_DEFINE(int, PICKUP_TIMEDILATION_WEIGHT, "Pickups/Spawn Weights/Time Dilation");

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

OPTION_DEFINE(float, JETPACK_ACCEL, "Special Abilities/Jetpack/Acceleration");

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

OPTION_DEFINE(int, UI_CHARSELECT_BASE_X, "UI/Character Select/Base X");
OPTION_DEFINE(int, UI_CHARSELECT_BASE_Y, "UI/Character Select/Base Y");
OPTION_DEFINE(int, UI_CHARSELECT_SIZE_X, "UI/Character Select/Size X");
OPTION_DEFINE(int, UI_CHARSELECT_SIZE_Y, "UI/Character Select/Size Y");
OPTION_DEFINE(int, UI_CHARSELECT_STEP_X, "UI/Character Select/Step X");
OPTION_DEFINE(int, UI_CHARSELECT_PREV_X, "UI/Character Select/Prev X");
OPTION_DEFINE(int, UI_CHARSELECT_PREV_Y, "UI/Character Select/Prev Y");
OPTION_DEFINE(int, UI_CHARSELECT_NEXT_X, "UI/Character Select/Next X");
OPTION_DEFINE(int, UI_CHARSELECT_NEXT_Y, "UI/Character Select/Next Y");
OPTION_DEFINE(int, UI_CHARSELECT_READY_X, "UI/Character Select/Ready X");
OPTION_DEFINE(int, UI_CHARSELECT_READY_Y, "UI/Character Select/Ready Y");
OPTION_DEFINE(int, UI_CHARSELECT_PORTRAIT_X, "UI/Character Select/Portrait X");
OPTION_DEFINE(int, UI_CHARSELECT_PORTRAIT_Y, "UI/Character Select/Portrait Y");

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

OPTION_DEFINE(int, INGAME_TEXTCHAT_FONT_SIZE, "UI/In-Game Text Chat/Text Size");
OPTION_DEFINE(int, INGAME_TEXTCHAT_PADDING_X, "UI/In-Game Text Chat/Text Padding X");
OPTION_DEFINE(int, INGAME_TEXTCHAT_PADDING_Y, "UI/In-Game Text Chat/Text Padding Y");
OPTION_DEFINE(int, INGAME_TEXTCHAT_SIZE_Y, "UI/In-Game Text Chat/Box Height");
OPTION_DEFINE(int, INGAME_TEXTCHAT_OFFSET_Y, "UI/In-Game Text Chat/Box Offset Y");
OPTION_DEFINE(float, INGAME_TEXTCHAT_DURATION, "UI/In-Game Text Chat/Time Visible");
OPTION_STEP(INGAME_TEXTCHAT_DURATION, 0.f, 0.f, .1f);

// fixme : move to gametypes.cpp ?

#include "gametypes.h"
const char * g_gameModeNames[kGameMode_COUNT] =
{
	"Death Match",
	"Token Hunt",
	"Coin Collector"
};
