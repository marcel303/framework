#ifndef CONTROLLEREXAMPLE_H
#define CONTROLLEREXAMPLE_H
#pragma once

#include "Controller.h"

#define CONTROLLER_EXAMPLE 1

// Example controller.
class ControllerExample : public Controller
{
public:
	enum ACTION
	{
		ACTION_MOVE_FORWARD,
		ACTION_MOVE_BACK,
		ACTION_STRAFE_LEFT,
		ACTION_STRAFE_RIGHT,
		ACTION_JUMP,
		ACTION_ROTATE_H,
		ACTION_ROTATE_V,
		ACTION_FIRE,
		ACTION_ZOOM
	};

	ControllerExample(Client* client);
};

#endif
