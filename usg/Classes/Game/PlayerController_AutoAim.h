#pragma once

#include "AngleController.h"
#include "SelectionId.h"
#include "TriggerTimerEx.h"

namespace Game
{
	CD_TYPE PlayerController_SelectAutoAimTarget();

	class AutoAimController
	{
	public:
		AutoAimController();
		void Update(Vec2F position, float dt);

		Vec2F Aim_get() const;
		bool HasTarget_get() const;

	private:
		TriggerTimerW mUpdateTrigger;
		CD_TYPE mTarget;
		AngleController mAngleController;
		Vec2F mDirection;
	};
}
