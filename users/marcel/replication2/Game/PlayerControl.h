#ifndef PLAYERCONTROL_H
#define PLAYERCONTROL_H
#pragma once

#include "Entity.h"

class Player;

// PlayerControl.
// --
// Controls camera position and rotation for an FPS player.

class PlayerControl : public NetSerializable
{
public:
	PlayerControl(Player * owner);

	void Animate(Phy::Object* phyObject, float dt);

	Mat4x4 GetOrientationMat() const;
	Vec3 GetOrientation() const;
	Mat4x4 GetTransform(const Vec3& position) const;

	void MoveForward(bool enabled);
	void MoveBack(bool enabled);
	void StrafeLeft(bool enabled);
	void StrafeRight(bool enabled);
	void RotateH(float angle);
	void RotateV(float angle);

	float m_rotationX;
	float m_rotationY;

	// Controls.
	struct
	{
		// Button states.
		bool m_moveForward;
		bool m_moveBack;
		bool m_strafeLeft;
		bool m_strafeRight;

		bool m_jump;
		bool m_fire;

		// Accelerations.
		float m_fAccel; // Forward speed.
		float m_sAccel; // Strafe speed.
		float m_vAccel; // Vertical speed.

		float m_rotationAngleH; // For calculating delta only.
		float m_rotationAngleV;
		float m_rotationImpulseH;
		float m_rotationImpulseV;
	} m_controls;

	virtual void SerializeStruct()
	{
		Serialize(m_controls.m_fAccel); // todo : 8.8-ish compression
		Serialize(m_controls.m_sAccel);
		Serialize(m_controls.m_vAccel);
		Serialize(m_rotationX); // todo : u16-ish compression
		Serialize(m_rotationY);
		Serialize(m_controls.m_fire); // todo : on change only
	}

private:
	void Apply();
};

#endif
