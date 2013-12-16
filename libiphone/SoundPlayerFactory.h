#pragma once

#include "libiphone_fwd.h"

class SoundPlayerFactory
{
public:
	static ISoundPlayer* Create();
};
