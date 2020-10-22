#pragma once

#include "framework-vr-hands.h" // for VrSide
#include "Mat4x4.h"

enum VrButton
{
	VrButton_Trigger,
	VrButton_GripTrigger,
	VrButton_A,
	VrButton_B,
	VrButton_COUNT
};

class VrPointerBase
{
protected:
	bool m_isDown[VrButton_COUNT] = { };
	bool m_hasChanged[VrButton_COUNT] = { };
	
	Mat4x4 transform = Mat4x4(true);
	
public:
	bool isPrimary = false; // read only. true when this pointer is determined to be the 'primary' controller (should be used as pointer for the virtual desktop, etc)
	bool hasTransform = false; // read only. true if the pointer is currently tracked and getTransform() may be called
	bool wantsToVibrate = false; // when set to true, the pointer will perform haptic vribration (if supported)

public:
	Mat4x4 getTransform(Vec3Arg vrOrigin) const;
	
public:
	bool wentDown(const VrButton index) const
	{
		return m_hasChanged[index] && m_isDown[index];
	}
	
	bool wentUp(const VrButton index) const
	{
		return m_hasChanged[index] && !m_isDown[index];
	}
	
	bool isDown(const VrButton index) const
	{
		return m_isDown[index];
	}
	
	bool isUp(const VrButton index) const
	{
		return !m_isDown[index];
	}
};

#if FRAMEWORK_USE_OVR_MOBILE

#include <VrApi_Input.h>

class VrPointer : public VrPointerBase
{
private:
	friend class FrameworkOvr;
	
	VrSide side = VrSide_Undefined;
	
	ovrDeviceID DeviceID = -1; // cached by updateInputState. used by updateHaptics()

public:
	void init(VrSide side);
	void shut();
	
	void updateInputState();
	void updateHaptics();
};

#else

class VrPointer : public VrPointerBase
{
public:
};

#endif
