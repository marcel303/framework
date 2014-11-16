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
	const static int ACCEL = 100;

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

		float m_rotationImpulseH;
		float m_rotationImpulseV;

	} m_controls;

	struct
	{
		float m_rotationX;
		float m_rotationY;

		// Accelerations.
		float m_fAccel; // Forward speed.
		float m_sAccel; // Strafe speed.
		float m_vAccel; // Vertical speed.
	} m_currentValues, m_serializedValues;

	virtual void SerializeStruct()
	{
		bool rotationChanged =
			m_currentValues.m_rotationX != m_serializedValues.m_rotationX ||
			m_currentValues.m_rotationY != m_serializedValues.m_rotationY;
		Serialize(rotationChanged);
		if (rotationChanged)
		{
			while (m_currentValues.m_rotationY < 0.f)
				m_currentValues.m_rotationY += Calc::m2PI;
			while (m_currentValues.m_rotationY >= Calc::m2PI)
				m_currentValues.m_rotationY -= Calc::m2PI;

			SerializeFloatRange(m_currentValues.m_rotationX, -Calc::mPI2, +Calc::mPI2, 16);
			SerializeFloatRange(m_currentValues.m_rotationY,         0.f,  Calc::m2PI, 16);
		}

		bool accelChanged =
			m_currentValues.m_fAccel != m_serializedValues.m_fAccel ||
			m_currentValues.m_sAccel != m_serializedValues.m_sAccel ||
			m_currentValues.m_vAccel != m_serializedValues.m_vAccel;
		Serialize(accelChanged);
		if (accelChanged)
		{
			float * vp[3] =
			{
				&m_currentValues.m_fAccel,
				&m_currentValues.m_sAccel,
				&m_currentValues.m_vAccel
			};

			for (int i = 0; i < 3; ++i)
			{
				float & v = *vp[i];
				bool isZero = (v == 0.f);
				Serialize(isZero);
				if (!isZero)
					SerializeFloatRange(v, -ACCEL, +ACCEL, 1);
				else
					v = 0.f;
			}
		}

		m_serializedValues = m_currentValues;
	}

//private:
	void Apply();
};

#endif
