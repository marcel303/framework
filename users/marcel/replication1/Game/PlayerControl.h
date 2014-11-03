#ifndef PLAYERCONTROL_H
#define PLAYERCONTROL_H
#pragma once

#include "Entity.h"

// PlayerControl.
// --
// Controls camera position and rotation for an FPS player.

class PlayerControl
{
public:
	PlayerControl();

	void RegisterParameters(Entity* entity);

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

private:
	void Apply();
};

#endif
