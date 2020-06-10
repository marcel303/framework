#pragma once

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
	
public:
	Mat4x4 transform = Mat4x4(true);
	bool hasTransform = false;
	bool wantsToVibrate = false;

public:
	virtual void init(VrSide side) = 0;
	virtual void shut() = 0;
	
	virtual void updateInputState() = 0;
	virtual void updateHaptics() = 0;
	
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
	VrSide side = VrSide_Undefined;
	
	ovrDeviceID DeviceID = -1; // cached by updateInputState. todo : remove (?)

public:
	virtual void init(VrSide side) override final;
	virtual void shut() override final;
	
	virtual void updateInputState() override final;
	virtual void updateHaptics() override final;
};

#else

class VrPointer : public VrPointerBase
{
public:
	virtual void init(VrSide side) override final { }
	virtual void shut() override final { }
	
	virtual void updateInputState() override final { }
	virtual void updateHaptics() override final { }
};

#endif
