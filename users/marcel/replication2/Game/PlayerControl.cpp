#include <limits>
#include "Calc.h"
#include "Debug.h"
#include "Entity.h"
#include "Player.h"
#include "PlayerControl.h"

#define ACCEL 100.0f

PlayerControl::PlayerControl(Player * player)
	: NetSerializable(player)
{
	// Controls.
	m_controls.m_moveForward = false;
	m_controls.m_moveBack    = false;
	m_controls.m_strafeLeft  = false;
	m_controls.m_strafeRight = false;
	m_controls.m_jump        = false;
	m_controls.m_fire        = false;

	m_controls.m_rotationImpulseH = 0.0f;
	m_controls.m_rotationImpulseV = 0.0f;

	// State.
	m_controls.m_fAccel = 0.0f;
	m_controls.m_vAccel = 0.0f;
	m_controls.m_sAccel = 0.0f;

	m_rotationX = 0.0f;
	m_rotationY = 0.0f;
}

void PlayerControl::Animate(Phy::Object* phyObject, float dt)
{
	Assert(phyObject);

	Vec3 forward = GetOrientation();
	Vec3 up = Vec3(0.0f, 1.0f, 0.0f);
	Vec3 strafe = (up % forward).CalcNormalized();
	
	Vec3 delta(0.f, 0.f, 0.f);
	delta += forward * m_controls.m_fAccel;
	delta += strafe  * m_controls.m_sAccel;
	delta += up      * m_controls.m_vAccel;

#if 0
	// direct manipulation of velocity requires change in PhyScene. it needs to clip *after* integration
	phyObject->m_velocity[0] = delta[0];
	phyObject->m_velocity[2] = delta[2];
#else
	phyObject->m_force += delta;
	phyObject->m_velocityDamp = Vec3(0.1f, 0.1f, 0.1f);
#endif

	// clip speed
	const float maxSpeed = 20.f;
	const Vec3 velocityXZ = Vec3(phyObject->m_velocity[0], 0.f, phyObject->m_velocity[2]);
	const float speed = velocityXZ.CalcSize();
	if (speed > maxSpeed)
	{
		phyObject->m_velocity[0] *= maxSpeed / speed;
		phyObject->m_velocity[2] *= maxSpeed / speed;
	}

	if (m_controls.m_rotationImpulseV)
	{
		m_rotationX += m_controls.m_rotationImpulseV; m_controls.m_rotationImpulseV = 0.0f;
	}
	if (m_controls.m_rotationImpulseH)
	{
		m_rotationY += m_controls.m_rotationImpulseH; m_controls.m_rotationImpulseH = 0.0f;
	}

	m_rotationX = Calc::Clamp(m_rotationX, -Calc::mPI / 2.1f, +Calc::mPI / 2.1f);
}

Mat4x4 PlayerControl::GetOrientationMat() const
{
	Mat4x4 mat;
	Mat4x4 matX;
	Mat4x4 matY;

	matX.MakeRotationX(+m_rotationX);
	matY.MakeRotationY(-m_rotationY);

	mat = matY * matX;

	return mat;
}

Vec3 PlayerControl::GetOrientation() const
{
	Mat4x4 mat = GetOrientationMat();

	Vec3 orientation = mat.Mul(Vec4(0.0f, 0.0f, 1.0f, 0.0f)).XYZ();

	return orientation;
}

Mat4x4 PlayerControl::GetTransform(const Vec3& position) const
{
	Mat4x4 matPos;
	matPos.MakeTranslation(position);

	Mat4x4 matRot = GetOrientationMat();

	return matPos * matRot;
}

void PlayerControl::MoveForward(bool enabled)
{
	m_controls.m_moveForward = enabled;
}

void PlayerControl::MoveBack(bool enabled)
{
	m_controls.m_moveBack = enabled;
}

void PlayerControl::StrafeLeft(bool enabled)
{
	m_controls.m_strafeLeft = enabled;
}

void PlayerControl::StrafeRight(bool enabled)
{
	m_controls.m_strafeRight = enabled;
}

void PlayerControl::RotateH(float angle)
{
	m_controls.m_rotationImpulseH += angle;
}

void PlayerControl::RotateV(float angle)
{
	m_controls.m_rotationImpulseV += angle;
}

void PlayerControl::Apply()
{
	SetDirty();

	m_controls.m_fAccel = 0.0f;

	if (m_controls.m_moveForward)
		m_controls.m_fAccel += ACCEL;
	if (m_controls.m_moveBack)
		m_controls.m_fAccel -= ACCEL;

	m_controls.m_sAccel = 0.0f;

	if (m_controls.m_strafeLeft)
		m_controls.m_sAccel -= ACCEL;
	if (m_controls.m_strafeRight)
		m_controls.m_sAccel += ACCEL;
}
