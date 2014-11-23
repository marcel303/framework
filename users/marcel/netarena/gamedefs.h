#pragma once

#define BLOCK_SX 128
#define BLOCK_SY 128

#define GFX_SX 1920
#define GFX_SY 1280

#define ARENA_SX (GFX_SX / BLOCK_SX)
#define ARENA_SY (GFX_SY / BLOCK_SY)

#define PLAYER_SX 100
#define PLAYER_SY 100

#define STEERING_SPEED_ON_GROUND 800
#define STEERING_SPEED_IN_AIR (STEERING_SPEED_ON_GROUND * 50 / 100)

#define INPUT_BUTTON_UP    (1 << 0)
#define INPUT_BUTTON_DOWN  (1 << 1)
#define INPUT_BUTTON_LEFT  (1 << 2)
#define INPUT_BUTTON_RIGHT (1 << 3)
#define INPUT_BUTTON_A     (1 << 4)
#define INPUT_BUTTON_B     (1 << 5)
#define INPUT_BUTTON_X     (1 << 6)
#define INPUT_BUTTON_Y     (1 << 7)

#define GRAVITY 6000.f
