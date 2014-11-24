#include "gamedefs.h"

OPTION_DEFINE(int, PLAYER_COLLISION_SX, "Player/Collision/Width");
OPTION_DEFINE(int, PLAYER_COLLISION_SY, "Player/Collision/Height");

OPTION_DEFINE(int, PLAYER_JUMP_SPEED, "Player/Jumping/Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_SPEED, "Player/Wall Jump/Speed");
OPTION_DEFINE(int, PLAYER_WALLJUMP_RECOIL_SPEED, "Player/Wall Jump/Recoil Speed");

OPTION_DEFINE(int, STEERING_SPEED_ON_GROUND, "Player/Steering/Speed On Ground");
OPTION_DEFINE(int, STEERING_SPEED_IN_AIR, "Player/Steering/Speed In Air");

OPTION_DEFINE(float, FRICTION_GROUNDED, "Physics/Friction/On Ground");

OPTION_DEFINE(float, GRAVITY, "Physics/Gravity");
