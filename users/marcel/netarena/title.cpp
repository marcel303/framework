#include "framework.h"
#include "title.h"

void Title::onEnter()
{
	// todo : start audio
}

void Title::onExit()
{
	// todo : stop audio (?)
}

bool Title::tick(float dt)
{
	// todo : update animation

	// todo : check if the animation is done

	if (keyboard.wentDown(SDLK_ESCAPE) ||
		keyboard.wentDown(SDLK_RETURN))
		return true;

	if (gamepad[0].wentDown(GAMEPAD_A) ||
		gamepad[0].wentDown(GAMEPAD_START))
		return true;

	return false;
}

void Title::draw()
{
	// todo : draw animation
}
