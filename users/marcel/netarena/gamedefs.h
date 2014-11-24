#pragma once

#include "Options.h"

#define BLOCK_SX 128
#define BLOCK_SY 128

#define GFX_SX 1920
#define GFX_SY 1280

#define ARENA_SX (GFX_SX / BLOCK_SX)
#define ARENA_SY (GFX_SY / BLOCK_SY)

OPTION_DECLARE(int, PLAYER_COLLISION_SX, 100);
OPTION_DECLARE(int, PLAYER_COLLISION_SY, 100);

OPTION_DECLARE(int, PLAYER_JUMP_SPEED, 2000);
OPTION_DECLARE(int, PLAYER_WALLJUMP_SPEED, 1500);
OPTION_DECLARE(int, PLAYER_WALLJUMP_RECOIL_SPEED, 1500);

OPTION_DECLARE(int, STEERING_SPEED_ON_GROUND, 800);
OPTION_DECLARE(int, STEERING_SPEED_IN_AIR, 400);

OPTION_DECLARE(float, FRICTION_GROUNDED, 0.5f);

OPTION_DECLARE(float, GRAVITY, 6000.f);

#define INPUT_BUTTON_UP    (1 << 0)
#define INPUT_BUTTON_DOWN  (1 << 1)
#define INPUT_BUTTON_LEFT  (1 << 2)
#define INPUT_BUTTON_RIGHT (1 << 3)
#define INPUT_BUTTON_A     (1 << 4)
#define INPUT_BUTTON_B     (1 << 5)
#define INPUT_BUTTON_X     (1 << 6)
#define INPUT_BUTTON_Y     (1 << 7)
