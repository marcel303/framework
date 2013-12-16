#pragma once

#include "SpriteGfx.h"

namespace Calc
{
	void Initialize_Color();
	
	SpriteColor Color_FromHue(float hue);
	SpriteColor Color_FromHue_NoLUT(float hue);
}
