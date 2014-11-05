#ifndef SYSTEMDEFAULT_H
#define SYSTEMDEFAULT_H
#pragma once

#include "System.h"

class SystemDefault : public System
{
public:
	virtual void RegisterFileSystems();
	virtual void RegisterResourceLoaders();
	virtual GraphicsDevice* CreateGraphicsDevice();
	virtual SoundDevice* CreateSoundDevice();
};

#endif
