#include "Client.h"
#include "ControllerExample.h"

ControllerExample::ControllerExample(Client* client)
	: Controller(CONTROLLER_EXAMPLE, client)
{
	BindKey(IK_UP,       ACTION_MOVE_FORWARD);
	BindKey(IK_DOWN,     ACTION_MOVE_BACK   );
	BindKey(IK_LEFT,     ACTION_STRAFE_LEFT );
	BindKey(IK_RIGHT,    ACTION_STRAFE_RIGHT);
	BindKey(IK_a,        ACTION_JUMP        );
	BindKey(IK_CONTROLR, ACTION_JUMP        );
	BindKey(IK_SPACE,    ACTION_JUMP        );
	BindKey(IK_DELETE,   ACTION_JUMP        );
	BindKey(IK_z,        ACTION_ZOOM        );

	BindMouseAxis(INPUT_AXIS_X, ACTION_ROTATE_H);
	BindMouseAxis(INPUT_AXIS_Y, ACTION_ROTATE_V);

	BindMouseButton(INPUT_BUTTON1, ACTION_FIRE);
	BindMouseButton(INPUT_BUTTON2, ACTION_JUMP);
	BindMouseButton(INPUT_BUTTON3, ACTION_ZOOM);
}
